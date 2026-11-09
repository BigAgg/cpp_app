#pragma once

#include "utils/filedialog.h"
#include "utils/stringconverter.h"
#include <cstdarg>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace ImGui {
inline void HelpMarker(const char *fmt, ...) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::BeginTooltip();
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::EndTooltip();
  }
}

inline void HelpMarkerColored(bool hightlight, const char *fmt, ...) {
  ImVec4 color = hightlight
                     ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) // red
                     : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  ImGui::TextUnformatted("(?)");
  ImGui::PopStyleColor();

  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::BeginTooltip();
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::EndTooltip();
  }
}

inline bool IsCurrentWindowHovered() {
  // Get the current child window
  ImGuiWindow *child = ImGui::GetCurrentWindow();
  // Child window rectangle in absolute coordinates:
  ImVec2 min = child->Pos;
  ImVec2 max = ImVec2(child->Pos.x + child->Size.x,
                      child->Pos.y + child->Size.y);
  return ImGui::IsMouseHoveringRect(min, max);
}

inline void Filewalker(std::string &path, std::vector<std::string> &files) {
  namespace fs = std::filesystem;
  if (path.empty())
    return;
  static std::string search;
  fs::path p(path);
  if (ImGui::ArrowButton("Back", ImGuiDir_Left) && p.has_parent_path()) {
    path = p.parent_path().string();
    p = fs::path(path);
    files.clear();
    for (const auto &entry : fs::directory_iterator(p)) {
      files.push_back(entry.path().string());
    }
  }
  ImGui::SameLine();
  ImGui::Text("%s", p.filename().string().c_str());
  ImGui::InputTextWithHint("##search", "suchen...", &search);
  if (ImGui::BeginListBox("##files")) {
    const std::string lowersearch = to_lower(search);
    for (const auto &file : files) {
      const std::string lowerfile = to_lower(file);
      if (!lowerfile.contains(lowersearch))
        continue;
      fs::path filepath(file);
      if (ImGui::Selectable(filepath.filename().string().c_str())) {
        if (fs::is_directory(filepath)) {
          path = file;
          files.clear();
          for (const auto &entry : fs::directory_iterator(filepath)) {
            files.push_back(entry.path().string());
          }
          break;
        } else
          OpenPath(file);
      }
      ImGui::SetItemTooltip("%s", file.c_str());
    }
    ImGui::EndListBox();
  }
  if (ImGui::Button("Ordner öffnen"))
    OpenPath(path);
}

inline bool InputTextSearch(
    const char *name,
    const char *hint,
    std::string *str,
    const std::vector<std::string> &elements) {
  struct SearchState {
    int selectedIndex = -1;
    bool popupOpen = false;
  };

  static std::unordered_map<ImGuiID, SearchState> states;

  ImGuiID id = ImGui::GetID(name);
  auto &state = states[id];

  std::string original = *str;

  ImGui::InputTextWithHint(name, hint, str);

  const bool inputActive = ImGui::IsItemActive();
  const bool inputFocused = ImGui::IsItemFocused();

  const float width = ImGui::GetItemRectSize().x;

  std::vector<std::string> filtered;

  if (!str->empty()) {
    std::string search = to_lower(*str);

    for (const auto &item : elements) {
      if (to_lower(item).contains(search))
        filtered.push_back(item);
    }
  }

  if (inputActive || inputFocused)
    state.popupOpen = true;

  if (filtered.empty()) {
    state.popupOpen = false;
    state.selectedIndex = -1;
  }

  bool popupHovered = false;
  bool selectionMade = false;

  if (state.popupOpen && !filtered.empty()) {
    const float itemHeight =
        ImGui::GetTextLineHeightWithSpacing();

    const float padding =
        ImGui::GetStyle().WindowPadding.y * 2.0f;

    const float maxVisibleItems = 8.0f;

    const float height =
        std::min(
            (float)filtered.size(),
            maxVisibleItems) *
            itemHeight +
        padding;

    std::string childId =
        std::string("##autocomplete_") + name;

    if (ImGui::BeginChild(
            childId.c_str(),
            ImVec2(width, height),
            true)) {
      popupHovered = ImGui::IsWindowHovered(
          ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

      for (int i = 0; i < (int)filtered.size(); ++i) {
        bool selected =
            state.selectedIndex == i;

        if (selected)
          ImGui::SetScrollHereY();

        if (ImGui::Selectable(
                filtered[i].c_str(),
                selected)) {
          *str = filtered[i];
          state.selectedIndex = -1;
          selectionMade = true;
        }
      }
    }

    ImGui::EndChild();

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      state.selectedIndex = std::min(
          state.selectedIndex + 1,
          (int)filtered.size() - 1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      state.selectedIndex = std::max(
          state.selectedIndex - 1,
          0);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter) &&
        state.selectedIndex >= 0 &&
        state.selectedIndex < (int)filtered.size()) {
      *str = filtered[state.selectedIndex];
      selectionMade = true;
      state.selectedIndex = -1;
    }
  }

  if (selectionMade) {
    state.popupOpen = false;
    ImGui::SetKeyboardFocusHere(-1);
    return true;
  }

  if (!inputActive &&
      !inputFocused &&
      !popupHovered &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    state.popupOpen = false;
    state.selectedIndex = -1;
  }

  return original != *str;
}

#ifndef IMGUI_DATEPICKER_YEAR_MIN
#define IMGUI_DATEPICKER_YEAR_MIN 1900
#endif // !IMGUI_DATEPICKER_YEAR_MIN

#ifndef IMGUI_DATEPICKER_YEAR_MAX
#define IMGUI_DATEPICKER_YEAR_MAX 3000
#endif // !IMGUI_DATEPICKER_YEAR_MAX

/// Date field with a calendar popup.
///
/// `date` is read and written in `format` (strftime/get_time syntax). The default
/// is the German notation; pass "%Y-%m-%d" for the ISO form an API speaks.
///
/// Sizes follow the current style: pass `width = 0.0f` to have the field measured
/// against the formatted date rather than a fixed pixel count, which is what keeps
/// it in proportion when the theme or the font changes. `itemSpacing = 0.0f` puts
/// the field directly after the label.
IMGUI_API bool DatePickerEx(const std::string &label, std::string &date, ImFont *altFont, bool clampToBorder = false, float itemSpacing = 0.0f, float width = 0.0f, const char *format = "%>.%m.%Y");

IMGUI_API bool DatePicker(const std::string &label, std::string &date, bool clampToBorder = false, float itemSpacing = 0.0f, float width = 0.0f, const char *format = "%d.%m.%Y");
} // namespace ImGui
