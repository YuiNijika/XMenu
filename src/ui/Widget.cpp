#include "Widget.h"

namespace {
    float ContentWidth() {
        return ImGui::GetContentRegionAvail().x;
    }

    float RowWidth() {
        return ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::GetCursorStartPos().x;
    }
}

namespace UI {
    ImVec2 CalcSize(short count, bool spacing) {
        if (count <= 1) {
            return ImVec2(ContentWidth(), ImGui::GetFrameHeight() * 1.3f);
        }

        const float rowWidth = RowWidth();
        const float spacingWidth = spacing ? ImGui::GetStyle().ItemSpacing.x * (count - 1) : 0.0f;
        const float width = (rowWidth - spacingWidth) / count;

        return ImVec2(width, ImGui::GetFrameHeight() * 1.3f);
    }

    bool Button(const char* label, short columns) {
        return ImGui::Button(label, CalcSize(columns));
    }

    void SameLineEvery(int index, int columns) {
        if (columns > 1 && (index + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }

    void SpacingSeparator() {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void TextCentered(const char* text) {
        const ImVec2 size = ImGui::CalcTextSize(text);
        ImGui::NewLine();
        ImGui::SameLine((ContentWidth() - size.x) / 2.0f);
        ImGui::TextUnformatted(text);
    }

    bool BeginTabBar(const char* id) {
        return ImGui::BeginTabBar(id, ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll);
    }
}