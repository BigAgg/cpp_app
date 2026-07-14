#include "app/updater.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <utils/stringconverter.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace fs = std::filesystem;

class UpdateInfo {
public:
  static UpdateInfo& Get () {
    static UpdateInfo instance;
    return instance;
  }

  bool initialized = false;
  bool updateavail = false;
  bool inprogress = false;
  std::string versionavail;
  std::string versioncurrent;
  std::string path;
  std::string installerpath;
  std::string updateinfo;
  std::string updaterinfopath;
  std::string versioninfo;
  std::string versioninfopath;
};

bool updater::UpdateAvailable () {
  const auto &ui = UpdateInfo::Get();
  if (!ui.initialized)
    return false;
  return ui.updateavail;
}

bool updater::UpdateInProgress () {
  const auto &ui = UpdateInfo::Get();
  return ui.inprogress;
}

void updater::UpdateWindow () {
  auto &ui = UpdateInfo::Get();
  ImGui::Text("Neues Update verfügbar: %s", ui.versionavail.c_str());
  ImGui::Text("Aktuelle Version: %s", ui.versioncurrent.c_str());
  if (ImGui::Button ("Update")) {
    fs::copy(ui.installerpath, ".\\installer.exe", fs::copy_options::overwrite_existing);
    system("start installer.exe");
    ui.inprogress = true;
  }
  ImGui::SeparatorText("Änderungen");
  ImGui::InputTextMultiline("##Changes", &ui.updateinfo, ImVec2(750, 500), ImGuiInputTextFlags_ReadOnly);
}

void updater::Init (const std::string& updaterPath, const std::string& installername, const std::string& currentVersion) {
  if (!fs::exists (updaterPath)) {
    std::fprintf(stderr, "[updater::Init] Updater path does not exist \"%s\"n", updaterPath.c_str());
    return;
  }
  auto &ui = UpdateInfo::Get();
  ui.versioninfopath = updaterPath + "/" + "VERSION";
  ui.updaterinfopath = updaterPath + "/" + "CHANGES";
  ui.installerpath = updaterPath + "/" + installername;
  ui.path = updaterPath;
  ui.initialized = true;
  bool populate = true;
  if (!fs::exists (ui.versioninfopath)) {
    populate = false;
    std::fprintf(stderr, "[updater::Init] Versioninfo does not exist \"%s\"n", ui.versioninfopath.c_str());
  }
  if (!fs::exists (ui.updaterinfopath)) {
    populate = false;
    std::fprintf(stderr, "[updater::Init] Updateinfo does not exist \"%s\"n", ui.updaterinfopath.c_str());
  }
  if (!fs::exists (ui.installerpath)) {
    populate = false;
    std::fprintf(stderr, "[updater::Init] Installer does not exist \"%s\"n", ui.installerpath.c_str());
  }
  if (!populate) {
    return;
  }
  
  std::ifstream file(ui.versioninfopath, std::ios::binary);
  if (!file) {
    std::fprintf(stderr, "[updater::Init] Unable to load versioninfo \"%s\"n", ui.versioninfopath.c_str());
    return;
  }
  ui.versioninfo = std::string(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  file.close();
  file.open(ui.updaterinfopath, std::ios::binary);
  if (!file) {
    std::fprintf(stderr, "[updater::Init] Unable to load updaterinfo \"%s\"n", ui.updaterinfopath.c_str());
    return;
  }
  ui.updateinfo = std::string(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  file.close();
  ui.versioncurrent = currentVersion;
  std::string version = ui.versioncurrent;
  unsigned int version_major = std::stoi(split_at(version, ".").first);
  unsigned int version_minor = std::stoi(split_at(split_at(version, ".").second, ".").first);
  unsigned int version_alpha = std::stoi(split_at(split_at(version, ".").second, ".").second);
  std::string version_file = ui.versioninfo;
  ui.versionavail = version_file;
  unsigned int version_avail_major = std::stoi(split_at(version_file, ".").first);
  unsigned int version_avail_minor = std::stoi(split_at(split_at(version_file, ".").second, ".").first);
  unsigned int version_avail_alpha = std::stoi(split_at(split_at(version_file, ".").second, ".").second);
  ui.updateavail = false;
  // Check for update
  if (version_major < version_avail_major)
    ui.updateavail = true;
  if (version_minor < version_avail_minor && version_major == version_avail_major)
    ui.updateavail = true;
  if (version_alpha < version_avail_alpha && version_major == version_avail_major && version_minor == version_avail_minor)
    ui.updateavail = true;
}
