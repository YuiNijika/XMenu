#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <windows.h>

namespace {
    bool lastMapMouseDown = false;

    ImVec2 MapToScreen(const CVector& pos, const ImVec2& origin, const ImVec2& size) {
        const float width = MenuState::TeleportMapWidth != 0.0f ? MenuState::TeleportMapWidth : 6000.0f;
        const float height = MenuState::TeleportMapHeight != 0.0f ? MenuState::TeleportMapHeight : 6000.0f;
        return ImVec2(origin.x + ((pos.x + width * 0.5f) / width) * size.x, origin.y + ((height * 0.5f - pos.y) / height) * size.y);
    }

    CVector ScreenToMap(const ImVec2& point, const ImVec2& origin, const ImVec2& size) {
        const float width = MenuState::TeleportMapWidth != 0.0f ? MenuState::TeleportMapWidth : 6000.0f;
        const float height = MenuState::TeleportMapHeight != 0.0f ? MenuState::TeleportMapHeight : 6000.0f;
        const float x = ((point.x - origin.x) / size.x) * width - width * 0.5f;
        const float y = height * 0.5f - ((point.y - origin.y) / size.y) * height;
        return CVector(x, y, 0.0f);
    }
}

namespace Controllers::Teleport {
    void Process() {
        if (!MenuState::QuickTeleport) {
            MenuState::QuickTeleportMapActive = false;
        }
    }

    void DrawQuickMap() {
        if (!MenuState::QuickTeleport || !MenuState::QuickTeleportMapActive) {
            lastMapMouseDown = false;
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(420.0f, 420.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(I18n::T("quickMap.title"), &MenuState::QuickTeleportMapActive, ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextUnformatted(I18n::T("quickMap.hint"));
            const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            const float available = ImGui::GetContentRegionAvail().x;
            const ImVec2 canvasSize(available > 260.0f ? available : 260.0f, available > 260.0f ? available : 260.0f);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(20, 25, 32, 220));
            drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(160, 180, 210, 255));

            const ImVec2 center(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
            drawList->AddLine(ImVec2(center.x, canvasPos.y), ImVec2(center.x, canvasPos.y + canvasSize.y), IM_COL32(80, 90, 105, 180));
            drawList->AddLine(ImVec2(canvasPos.x, center.y), ImVec2(canvasPos.x + canvasSize.x, center.y), IM_COL32(80, 90, 105, 180));

            const CVector playerPos = GetCurrentPosition();
            const ImVec2 playerPoint = MapToScreen(playerPos, canvasPos, canvasSize);
            drawList->AddCircleFilled(playerPoint, 5.0f, IM_COL32(80, 220, 120, 255));

            ImGui::InvisibleButton("##QuickMapCanvas", canvasSize);
            const bool hovered = ImGui::IsItemHovered();
            const bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (hovered && mouseDown && !lastMapMouseDown) {
                const CVector target = ScreenToMap(ImGui::GetIO().MousePos, canvasPos, canvasSize);
                MapPosition(target.x, target.y, MenuState::SpawnUnderwater);
                MenuState::QuickTeleportMapActive = false;
            }
            lastMapMouseDown = mouseDown;
        }
        ImGui::End();
    }
}