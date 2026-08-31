#include "utils/pipehandler.h"
#include <string>
#include <Windows.h>

std::wstring WaitForPipeMessage(const std::string &appname) {
  const std::wstring PIPE_NAME = L"\\\\.\\pipe\\" + std::wstring(appname.begin(), appname.end());

  HANDLE hPipe = CreateNamedPipeW(
      PIPE_NAME.c_str(),
      PIPE_ACCESS_INBOUND,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
      1,
      1024,
      1024,
      0,
      nullptr);

  if (hPipe == INVALID_HANDLE_VALUE)
    return L"";

  BOOL connected =
      ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

  if (!connected) {
    CloseHandle(hPipe);
    return L"";
  }

  wchar_t buffer[1024] = {};
  DWORD bytesRead = 0;

  if (!ReadFile(
          hPipe,
          buffer,
          sizeof(buffer) - sizeof(wchar_t),
          &bytesRead,
          nullptr)) {
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return L"";
  }

  buffer[bytesRead / sizeof(wchar_t)] = L'\0';

  DisconnectNamedPipe(hPipe);
  CloseHandle(hPipe);

  return buffer;
}

void SendPipeMessage(const std::wstring &msg, const std::string &appname) {
  const std::wstring PIPE_NAME = L"\\\\.\\pipe\\" + std::wstring(appname.begin(), appname.end());

  HANDLE hPipe = CreateFileW(
      PIPE_NAME.c_str(),
      GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      0,
      nullptr);

  if (hPipe == INVALID_HANDLE_VALUE)
    return;

  DWORD bytesWritten;
  WriteFile(
      hPipe,
      msg.c_str(),
      static_cast<DWORD>((msg.size() + 1) * sizeof(wchar_t)),
      &bytesWritten,
      nullptr);

  CloseHandle(hPipe);
}

bool AlreadyRunning(const std::string &appname) {
  // Use a global mutex for single instance detection
  // This is more reliable than window search and won't conflict with IDE windows
  std::wstring mutexName = L"Global\\ServiceTracker_" + std::wstring(appname.begin(), appname.end());

  HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());

  if (hMutex == nullptr)
    return true; // Error creating mutex means something is wrong

  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(hMutex);
    // Try to find and activate existing window
    HWND hwnd = FindWindowA(nullptr, appname.c_str());
    if (hwnd) {
      if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);
      SetForegroundWindow(hwnd);
      BringWindowToTop(hwnd);
    }
    return true;
  }

  // We own the mutex now - don't close it, let it live for the app's lifetime
  return false;
}
