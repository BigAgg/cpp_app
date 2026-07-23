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
  HWND hwnd = FindWindowA(nullptr, appname.c_str());
  if (hwnd) {
    if (IsIconic(hwnd))
      ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
  }
  return hwnd;
}
