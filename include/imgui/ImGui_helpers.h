#pragma once

#include "utils/filedialog.h"
#include "utils/stringconverter.h"
#include <algorithm>
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

/// Text input with an autocomplete overlay listing the `elements` that contain the typed text.
///
/// The suggestion list is a separate floating window, so it takes no layout space and is drawn
/// on top of whatever follows. It never takes keyboard focus while you type: the input stays active,
/// and Up/Down/Enter go through the input. If you click the list (an item or the scrollbar), focus
/// goes back to the input afterwards. Returns true when `str` changed, either from typing or from
/// picking a suggestion. `picked` (optional) is set to true only when a suggestion was taken (click or Enter).
inline bool InputTextSearch(
    const char *name,
    const char *hint,
    std::string *str,
    const std::vector<std::string> &elements,
    bool *picked = nullptr) {
  struct SearchState {
    int selectedIndex = -1;
    int itemCount = 0;
    bool open = false;
    bool scrollToSelected = false;
    bool suppressReopen = false; // after a pick, keep the list closed until the text is edited again
  };

  static std::unordered_map<ImGuiID, SearchState> states;

  ImGuiContext &g = *GImGui;
  const ImGuiID id = ImGui::GetID(name);
  SearchState &state = states[id];

  char popupName[64];
  ImFormatString(popupName, IM_ARRAYSIZE(popupName), "##InputTextSearch_%08X", id);
  ImGuiWindow *popupWindow = ImGui::FindWindowByName(popupName);

  // Up/Down handled through the history callback so the input owns these keys and nav does not move.
  auto historyCallback = [](ImGuiInputTextCallbackData *data) -> int {
    auto *s = static_cast<SearchState *>(data->UserData);
    if (!s->open || s->itemCount <= 0)
      return 0;
    if (data->EventKey == ImGuiKey_DownArrow)
      s->selectedIndex = std::min(s->selectedIndex + 1, s->itemCount - 1);
    else if (data->EventKey == ImGuiKey_UpArrow)
      s->selectedIndex = std::max(s->selectedIndex - 1, 0);
    s->scrollToSelected = true;
    return 0;
  };

  const std::string original = *str;
  ImGui::InputTextWithHint(name, hint, str, ImGuiInputTextFlags_CallbackHistory, historyCallback, &state);

  const ImGuiLastItemData inputItem = g.LastItemData;
  const bool inputActive = ImGui::IsItemActive();
  const bool inputActivated = ImGui::IsItemActivated();
  const bool inputDeactivated = ImGui::IsItemDeactivated();
  const bool edited = original != *str;
  const ImVec2 inputMin = ImGui::GetItemRectMin();
  const ImVec2 inputMax = ImGui::GetItemRectMax();

  std::vector<const std::string *> filtered;
  if (!str->empty()) {
    const std::string search = to_lower(*str);
    for (const auto &item : elements)
      if (to_lower(item).contains(search))
        filtered.push_back(&item);
  }

  // The user is working with the overlay. Hover counts too, because clicking the overlay deactivates the
  // input in the same frame (InputText drops focus on any outside click) before the overlay item can take the click.
  const bool popupInteracting =
      popupWindow && popupWindow->WasActive &&
      (g.ActiveIdWindow == popupWindow || g.NavWindow == popupWindow ||
       (g.HoveredWindow && g.HoveredWindow->RootWindow == popupWindow));

  if (edited) {
    state.open = inputActive;
    state.selectedIndex = -1;
    state.suppressReopen = false;
  } else if (inputActivated && !str->empty() && !state.suppressReopen) {
    state.open = true;
  }
  if (!inputActive && !popupInteracting)
    state.open = false;

  bool selectionMade = false;
  bool refocusInput = false;

  // Enter deactivates a single-line input. If a suggestion is highlighted, take it.
  if (inputDeactivated && state.selectedIndex >= 0 && state.selectedIndex < (int)filtered.size() &&
      (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
    *str = *filtered[state.selectedIndex];
    selectionMade = true;
  }

  if (filtered.empty() || selectionMade)
    state.open = false;
  state.itemCount = (int)filtered.size();
  if (state.selectedIndex >= state.itemCount)
    state.selectedIndex = state.itemCount - 1;

  if (state.open) {
    const ImGuiStyle &style = ImGui::GetStyle();
    const float visibleItems = std::min((float)filtered.size(), 8.0f);
    const float height = visibleItems * ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y + style.WindowPadding.y * 2.0f;
    const float width = inputMax.x - inputMin.x;

    // Show the list below the input. If there is no room left in the viewport, show it above.
    ImGuiViewport *viewport = ImGui::GetWindowViewport();
    ImVec2 pos(inputMin.x, inputMax.y);
    if (pos.y + height > viewport->WorkPos.y + viewport->WorkSize.y && inputMin.y - height >= viewport->WorkPos.y)
      pos.y = inputMin.y - height;

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, std::max(style.PopupBorderSize, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, style.Colors[ImGuiCol_PopupBg]);
    if (ImGui::Begin(popupName, nullptr, flags)) {
      // Re-raise every frame: focusing the parent window (e.g. clicking the input) puts the parent in front
      // of this window, and nothing else would put this window back on top.
      ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

      for (int i = 0; i < (int)filtered.size(); ++i) {
        const bool selected = state.selectedIndex == i;
        ImGui::PushID(i);
        if (ImGui::Selectable(filtered[i]->c_str(), selected)) {
          *str = *filtered[i];
          selectionMade = true;
        }
        ImGui::PopID();
        if (selected && state.scrollToSelected)
          ImGui::SetScrollHereY();
      }
      state.scrollToSelected = false;
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // A click on the overlay (scrollbar, empty space) gave it focus. Give focus back to the input once the click is over.
    if (!selectionMade && !inputActive && popupInteracting && g.ActiveId == 0)
      refocusInput = true;
  }

  if (selectionMade) {
    state.open = false;
    state.selectedIndex = -1;
    state.suppressReopen = true;
    refocusInput = true;
  }

  // Make the input the last item again, so the caller's IsItemActive()/IsItemDeactivated()/... refer to the input.
  g.LastItemData = inputItem;
  if (refocusInput)
    ImGui::SetKeyboardFocusHere(-1);
  if (picked)
    *picked = selectionMade;

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
