#pragma once

#include <string>
#include <vector>

std::string OpenFileDialog(const std::vector<std::string>& filters = {"*"}, std::string delimiter=":");
std::string SaveFileDialog(const std::vector<std::string>& filters = {"*"}, std::string delimiter=":");
std::string OpenDirectoryDialog();
void OpenPath(const std::string& path);
long long GetLastWriteTimeInt(const std::string &path);
std::string GetLastWriteTime(const std::string &path);

