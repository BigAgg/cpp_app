#include "app/updater.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <utils/stringconverter.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "utils/logging.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class UpdateInfo {
public:
  static UpdateInfo& Get () {
    static UpdateInfo instance;
    return instance;
  }

  bool initialized = false;
  bool updateavail = false;
  bool inprogress = false;
  bool silentupdate = false;
  std::string versionavail;
  std::string versioncurrent;
  std::string path;
  std::string installerpath;
  std::string updateinfo;
  std::string updaterinfopath;
  std::string versioninfo;
  std::string versioninfopath;
};

struct ReleaseInfo {
  std::string version;
  std::string downloadUrl;
  std::string body;
};

static void TriggerUpdate (const std::string name) {
  auto &ui = UpdateInfo::Get();
	fs::copy(ui.installerpath, ".\\installer.exe", fs::copy_options::overwrite_existing);
	system("start installer.exe");
	ui.inprogress = true;
}

void updater::TriggerSilentUpdate (const std::string& exename) {
  LOG_INFO("Triggering Silent Update!");
  auto &ui = UpdateInfo::Get();
  const std::string outpath = fs::current_path().string() + "\\" + exename;
  const std::string cmd = "start updater.exe --infile \"" + ui.installerpath + "\" --outfile \"" + outpath + "\" --start";
  system(cmd.c_str());
}

bool updater::UpdateAvailable () {
  const auto &ui = UpdateInfo::Get();
  if (!ui.initialized)
    return false;
  return ui.updateavail;
}

bool updater::SilentUpdate () {
  auto &ui = UpdateInfo::Get();
  return ui.silentupdate;
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
  LOG_INFO("Initializing updater:\n\t\t%s\n\t\t%s", updaterPath.c_str(), installername.c_str());
  if (!fs::exists (updaterPath)) {
    LOG_ERROR("Could not initialize updater: \"%s\" does not exist!", updaterPath.c_str());
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
    LOG_ERROR("Could not initialize updater: \"%s\" does not exist!", ui.versioninfopath.c_str());
  }
  if (!fs::exists (ui.updaterinfopath)) {
    populate = false;
    LOG_ERROR("Could not initialize updater: \"%s\" does not exist!", ui.updaterinfopath.c_str());
  }
  if (!fs::exists (ui.installerpath)) {
    populate = false;
    LOG_ERROR("Could not initialize updater: \"%s\" does not exist!", ui.installerpath.c_str());
  }
  if (!populate) {
    return;
  }
  
  std::ifstream file(ui.versioninfopath, std::ios::binary);
  if (!file) {
    LOG_ERROR("Could not initialize updater: \"%s\" could not be loaded!", ui.versioninfopath.c_str());
    return;
  }
  ui.versioninfo = std::string(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  file.close();
  file.open(ui.updaterinfopath, std::ios::binary);
  if (!file) {
    LOG_ERROR("Could not initialize updater: \"%s\" could not be loaded!", ui.updaterinfopath.c_str());
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
  LOG_INFO("Updater initialized!");
}

namespace {
  size_t WriteCallback (
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }
}

static std::string HttpGet (const std::string& url) {
  CURL *curl = curl_easy_init();

  if (!curl)
    return {};

  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "App-Updater");

  CURLcode result = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK) {
    LOG_WARNING("Unable to retrieve result");
    return {};
  }

  return response;
}

static ReleaseInfo GetLatestRelease (const std::string& repo, const std::string& filename) {
  LOG_INFO("Searching for Update...");
  std::string apiUrl = repo;
  if (!apiUrl.contains ("api.github.com")) {
    if (apiUrl.contains ("github.com")) {
      replace_all(apiUrl, "github.com", "api.github.com");
    } else {
      LOG_ERROR("Invalid github repo: %s", repo.c_str());
      return ReleaseInfo();
    }
  }
  if (!apiUrl.starts_with("https://"))
    apiUrl = "https://" + apiUrl;

  auto body = HttpGet(apiUrl);

  if (body.empty ()) {
    LOG_WARNING("No response received");
    return {};
  }

  auto j = json::parse(body, nullptr, false);
  if (j.is_discarded ()) {
    LOG_ERROR("Invalid JSON received");
    return {};
  }
  ReleaseInfo info;

  if (!j.contains ("tag_name")) {
    LOG_WARNING("Unexpected GitHub response:\n%s", j.dump(4).c_str());
    return ReleaseInfo();
  }
  info.version = j.value("tag_name", "");
  
  info.body = j.value("body", "");
  
  if (!j.contains ("assets")) {
    LOG_WARNING("Unexpected GitHub response:\n%s", j.dump(4).c_str());
    return ReleaseInfo();
  }
  for (const auto& asset : j["assets"]) {
    if (asset["name"] == filename) {
      info.downloadUrl = asset["browser_download_url"];
      break;
    }
  }
  
  return info;
}

static bool DownloadUpdate (const std::string& outputFile, const ReleaseInfo& info) {
  if (info.downloadUrl.empty())
    return false;

  CURL *curl = curl_easy_init();

  if (!curl)
    return false;

  FILE *fp = fopen(outputFile.c_str(), "wb");

  if (!fp) {
    curl_easy_cleanup(curl);
    return false;
  }

  LOG_INFO("Downloading from URL: %s", info.downloadUrl.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, info.downloadUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "App-Updater");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode result = curl_easy_perform(curl);

  fclose(fp);
  curl_easy_cleanup(curl);
  return result == CURLE_OK;
}

void updater::InitGit(const std::string &repo, const std::string& filename, const std::string &currentVersion) {
  const auto &info = GetLatestRelease(repo, filename);
  LOG_INFO("Update information: %s, %s\n%s", info.version, info.downloadUrl, info.body);
  const bool updateavail = (info.version != currentVersion && !info.version.empty());
  if (updateavail) {
    if (!DownloadUpdate (filename, info)) {
      LOG_WARNING("Unable to download update");
      return;
    }
  }
  auto &ui = UpdateInfo::Get();
  ui.initialized = true;
  ui.versionavail = info.version;
  ui.updateinfo = info.body;
  ui.versioncurrent = currentVersion;
  ui.updateavail = updateavail;
  ui.installerpath = fs::current_path().string() + "\\" + filename;
  if (ui.versionavail.contains ("-")) {
    const auto &[version, trigger] = split_at(ui.versionavail, "-");
    if (trigger == "silent")
      ui.silentupdate = true;
  }
}
