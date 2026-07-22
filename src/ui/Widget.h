#pragma once
#include "imgui/imgui.h"
#include <string>

namespace UI {
    // 列表导航状态（供 Menu 页脚等读取）
    extern int listSelectedIndex;
    extern int listTotalItems;

    void ResetListIndex();
    void SetListTotalItems();
    void UpdateListNavigation();

    // 列表外壳（GTA5 风格）
    void BeginListShell(const char* title, const char* subtitle, bool showBackHint);
    void EndListShell();
    // 当前选中项是否支持左右调值（页脚提示用）
    bool ListSelectionCanAdjust();

    ImVec2 CalcSize(short count = 1, bool spacing = true);
    void SameLineEvery(int index, int columns);
    void SpacingSeparator();
    void TextCentered(const char* text);
    // 页面级作用域
    void BeginPage(const char* id, const char* title, const char* subtitle = "OPTIONS");
    void EndPage();

    // 选项卡 / 子菜单
    bool BeginTab(const char* id, const char* label);
    void EndTab();
    bool BeginTabBar(const char* id);
    void EndTabBar();

    // 基础组件
    bool CollapsingHeader(const char* label, bool& isOpen);
    bool Checkbox(const char* label, bool* v);
    bool Button(const char* label, short columns = 1);
    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f");
    bool SliderInt(const char* label, int* v, int v_min, int v_max);
    bool InputFloat(const char* label, float* v, float step = 1.0f, float step_fast = 10.0f, const char* format = "%.1f");
    bool InputInt(const char* label, int* v, int step = 1, int step_fast = 10);

    // 布局控制
    void SameLine();
    void Columns(int count, const char* id = nullptr, bool border = false);
    void NextColumn();
    void BeginDisabled(bool disabled = true);
    void EndDisabled();
    void PushItemWidth(float item_width);
    void PopItemWidth();
}