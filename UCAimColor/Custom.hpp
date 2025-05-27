#pragma once

#define  IMGUI_DEFINE_MATH_OPERATORS
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"

#include <string>
#include <vector>
#include <functional>
#include "xor.hpp"
#include "icon.hpp"
inline ImFont* icons_font = nullptr;
using namespace std;

#define to_vec4( r, g, b, a ) ImColor( r / 255.f, g / 255.f, b / 255.f, a )

class c_custom {

public:
    float m_anim = 0.f;

    int m_tab = 0;
    vector < const char* > tabs = { ("Assist"), ("RCS"), ("Advanced"), ("Configs"), ("User")}, tabs_icons = {(ICON_FA_CROSSHAIRS),(ICON_FA_RETWEET), (ICON_FA_COG), (ICON_FA_SAVE), (ICON_FA_USER)};

    int m_user_subtab = 0;
    vector < const char* > user_subtabs = { ("User") };

    int m_rcs_subtab = 0;
    vector < const char* > rcs_subtabs = { ("Recoil Control System") };

    int m_rage_subtab = 0;
    vector < const char* > rage_subtabs = { ("Aimbot"), ("Magnet"), ("Triggerbot"), ("Flicker Aim"), "Silent Aim"};

    int m_visuals_subtab = 0;
    vector < const char* > visuals_subtabs = { ("Color Configuration") };


    int m_config_subtab = 0;
    vector < const char* > config_subtabs = { ("Config") };

    float col_buf[4] = { 1.f, 1.f, 1.f, 1.f };

    void render_arrows_for_horizontal_bar(ImVec2 pos, float alpha, float width, float height);

    bool expand_button(const char* label, bool selected, float rounding = 0.f, ImDrawFlags flags = NULL);
    bool tab(const char* icon, const char* label, bool selected);
    bool sub_tab(const char* label, bool selected);

    void tab_area(const char* str_id, ImVec2 size, std::function < void() > content);

    void begin_child(const char* name, ImVec2 size);
    void end_child();

    int accent_color[4] = { 255, 255, 255, 255 };
    ImColor get_accent_color(float a = 1.f) {

        return to_vec4(accent_color[0], accent_color[1], accent_color[2], a);
    }

};

inline c_custom custom;
