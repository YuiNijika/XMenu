#include "Widget.h"

namespace {
    float ContentWidth() {
        return ImGui::GetContentRegionAvail().x;
    }
}

namespace UI {
    ImVec2 CalcSize(short count, bool spacing) {
        const float contentWidth = ContentWidth();
        if (count <= 1) {
            return ImVec2(contentWidth, ImGui::GetFrameHeight() * 1.3f);
        }

        float factor = ImGui::GetStyle().ItemSpacing.x / 2.0f;
        if (count == 3) {
            factor = ImGui::GetStyle().ItemSpacing.x / 1.403f;
        }

        const float width = spacing
            ? contentWidth / count - factor
            : contentWidth / count;

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