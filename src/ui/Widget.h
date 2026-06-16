#pragma once
#include "imgui/imgui.h"

namespace UI {
    // 列表导航状态 (List Navigation State)
    extern int listSelectedIndex;
    extern int listTotalItems;
    
    void ResetListIndex();
    void SetListTotalItems();
    void UpdateListNavigation();

    ImVec2 CalcSize(short count = 1, bool spacing = true);
    void SameLineEvery(int index, int columns);
    void SpacingSeparator();
    void TextCentered(const char* text);
    
    // --- 统一双模 UI 接口 (Unified Dual-Mode UI API) ---
    // 自动兼容 ImGui 传统面板模式 和 列表模式

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

    // 布局控制 (在 Native 模式下会自动忽略)
    void SameLine();
    void Columns(int count, const char* id = nullptr, bool border = false);
    void NextColumn();
    void BeginDisabled(bool disabled = true);
    void EndDisabled();
    void PushItemWidth(float item_width);
    void PopItemWidth();
}