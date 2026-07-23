#pragma once

#include <string>

std::wstring WaitForPipeMessage(const std::string& appname);
void SendPipeMessage(const std::wstring &msg, const std::string &appname);
bool AlreadyRunning(const std::string &appname);
