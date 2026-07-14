#pragma once

#include <string>
#include <vector>
#include <chrono>

std::string OpenFileDialog(const std::vector<std::string>& filters = {"*"}, std::string delimiter=":");
std::string SaveFileDialog(const std::vector<std::string>& filters = {"*"}, std::string delimiter=":");
std::string OpenDirectoryDialog();
void OpenPath(const std::string& path);
long long GetLastWriteTimeInt(const std::string &path);
std::string GetLastWriteTime(const std::string &path);
bool WaitForWritableFile(const std::string &path,
                         std::chrono::milliseconds retryInterval = std::chrono::milliseconds(100),
                         std::chrono::seconds timeout = std::chrono::seconds(1));

