#pragma once

#include <string>

namespace updater {
bool UpdateAvailable();
bool SilentUpdate();
bool UpdateInProgress();
void TriggerSilentUpdate(const std::string &exename);
void UpdateWindow();
void Init(const std::string& updaterPath, const std::string& installername, const std::string& currentVersion);
void InitGit(const std::string &repo, const std::string& filename, const std::string &currentVersion);
}
