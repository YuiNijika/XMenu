#pragma once
#include "imgui/imgui.h"

namespace UI {
    ImVec2 CalcSize(short count = 1, bool spacing = true);
    bool Button(const char* label, short columns = 1);
    void SameLineEvery(int index, int columns);
    void SpacingSeparator();
    void TextCentered(const char* text);
    bool BeginTabBar(const char* id);
}