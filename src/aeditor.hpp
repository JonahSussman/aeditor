#pragma once

#include <string>
#include <map>

#include "imgui.h"
#include "imgui-SFML.h"

#include "util.hpp"

#define i_static inline static
#define s_inline static inline

#define IM_COL24(R, G, B) IM_COL32(R, G, B, 0xff)

namespace ae {

// struct Color {
s_inline uint32_t bg0   = IM_COL24(0x28, 0x28, 0x28);
s_inline uint32_t bg0_s = IM_COL24(0x32, 0x30, 0x2f);
s_inline uint32_t bg1   = IM_COL24(0x3c, 0x38, 0x36);
s_inline uint32_t bg2   = IM_COL24(0x50, 0x49, 0x45);
s_inline uint32_t bg3   = IM_COL24(0x66, 0x5c, 0x54);
s_inline uint32_t fg0   = IM_COL24(0xfb, 0xf1, 0xc7);

s_inline uint32_t aqua_dark  = IM_COL24(0x42, 0x7b, 0x58);
s_inline uint32_t aqua       = IM_COL24(0x68, 0x9d, 0x6a);
s_inline uint32_t aqua_light = IM_COL24(0x83, 0xc0, 0x7c);

s_inline uint32_t red        = IM_COL24(0xfb, 0x49, 0x34);
s_inline uint32_t red_light  = IM_COL24(0xfb, 0x49, 0x34);
// };

inline void ig_style() {
  ImGuiIO* io = &ImGui::GetIO();
  // io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io->Fonts->Clear();
  io->Fonts->AddFontFromFileTTF("iosevka.ttf", 18.f);
  io->Fonts->AddFontFromFileTTF("iosevka.ttf", 42.f); // was 36
  ImGui::SFML::UpdateFontTexture();

  ImGuiStyle* style = &ImGui::GetStyle();
  style->WindowRounding    = 0.f;
  style->ScrollbarRounding = 0.f;
  style->TabRounding       = 0.f;

  ImVec4* colors = style->Colors;
  colors[ImGuiCol_Text]                   = ImVec4(0.98f, 0.95f, 0.78f, 1.00f);
  colors[ImGuiCol_TextDisabled]           = ImVec4(0.66f, 0.60f, 0.52f, 1.00f);
  colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
  colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.19f, 0.18f, 1.00f);
  colors[ImGuiCol_FrameBg]                = ImVec4(0.31f, 0.29f, 0.27f, 1.00f);
  colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.22f, 0.21f, 1.00f);
  colors[ImGuiCol_FrameBgActive]          = ImVec4(0.40f, 0.36f, 0.33f, 1.00f);
  colors[ImGuiCol_TitleBgActive]          = ImVec4(0.26f, 0.48f, 0.35f, 1.00f);
  colors[ImGuiCol_MenuBarBg]              = ImVec4(0.24f, 0.22f, 0.21f, 1.00f);
  colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.11f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.29f, 0.27f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.36f, 0.33f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.44f, 0.39f, 1.00f);
  colors[ImGuiCol_CheckMark]              = ImVec4(0.98f, 0.95f, 0.78f, 1.00f);
  colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.62f, 0.42f, 1.00f);
  colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.51f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_Button]                 = ImVec4(0.41f, 0.62f, 0.42f, 1.00f);
  colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.48f, 0.35f, 1.00f);
  colors[ImGuiCol_ButtonActive]           = ImVec4(0.51f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.48f, 0.35f, 1.00f);
  colors[ImGuiCol_HeaderHovered]          = ImVec4(0.41f, 0.62f, 0.42f, 1.00f);
  colors[ImGuiCol_HeaderActive]           = ImVec4(0.56f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.36f, 0.33f, 1.00f);
  colors[ImGuiCol_ResizeGrip]             = ImVec4(0.92f, 0.86f, 0.70f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.92f, 0.86f, 0.70f, 0.67f);
  colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.56f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_Tab]                    = ImVec4(0.26f, 0.48f, 0.35f, 1.00f);
  colors[ImGuiCol_TabHovered]             = ImVec4(0.56f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_TabActive]              = ImVec4(0.41f, 0.62f, 0.42f, 1.00f);
  colors[ImGuiCol_TabUnfocused]           = ImVec4(0.11f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
  colors[ImGuiCol_PlotLines]              = ImVec4(0.92f, 0.86f, 0.70f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.98f, 0.29f, 0.20f, 1.00f);
  colors[ImGuiCol_PlotHistogram]          = ImVec4(0.84f, 0.60f, 0.13f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.98f, 0.74f, 0.18f, 1.00f);
  colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.48f, 0.35f, 1.00f);
  colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_NavHighlight]           = ImVec4(0.56f, 0.75f, 0.49f, 1.00f);
  colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.92f, 0.86f, 0.70f, 1.00f);
  colors[ImGuiCol_SeparatorActive]        = ImVec4(0.98f, 0.95f, 0.78f, 1.00f);
}

}