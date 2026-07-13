#pragma once

#include <string>

namespace updater {
bool UpdateAvailable();
bool UpdateInProgress();
void UpdateWindow();
void Init(const std::string& updaterPath, const std::string& installername, const std::string& currentVersion);
}
