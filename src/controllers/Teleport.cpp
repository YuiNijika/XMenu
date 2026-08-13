#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "integration/XBaseBridge.h"
#include <XBase/Teleport.h>
#include <XBase/UI.h>

namespace {
    bool lastMapMouseDown = false;

    XBase::Vec2 MapToScreen(const XBase::Vec3& pos, XBase::Vec2 origin, XBase::Vec2 size) {
        const float width = MenuState::TeleportMapWidth != 0.0f ? MenuState::TeleportMapWidth : 6000.0f;
        const float height = MenuState::TeleportMapHeight != 0.0f ? MenuState::TeleportMapHeight : 6000.0f;
        return {origin.x + ((pos.x + width * 0.5f) / width) * size.x, origin.y + ((height * 0.5f - pos.y) / height) * size.y};
    }

    XBase::Vec3 ScreenToMap(XBase::Vec2 point, XBase::Vec2 origin, XBase::Vec2 size) {
        const float width = MenuState::TeleportMapWidth != 0.0f ? MenuState::TeleportMapWidth : 6000.0f;
        const float height = MenuState::TeleportMapHeight != 0.0f ? MenuState::TeleportMapHeight : 6000.0f;
        const float x = ((point.x - origin.x) / size.x) * width - width * 0.5f;
        const float y = height * 0.5f - ((point.y - origin.y) / size.y) * height;
        return {x, y, 0.0f};
    }
}

namespace Controllers::Teleport {
    void ProcessHost() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::TeleportBasic)) {
            MenuState::QuickTeleport = false;
            MenuState::QuickTeleportMapActive = false;
            MenuState::TeleportMarker = false;
            MenuState::TeleportForwardHold = false;
            return;
        }
        if (!MenuState::QuickTeleport) {
            MenuState::QuickTeleportMapActive = false;
        }
    }

    void DrawQuickMap() {
        if (!MenuState::QuickTeleport || !MenuState::QuickTeleportMapActive) {
            lastMapMouseDown = false;
            return;
        }

        XBase::UI::SetNextWindowSize({420.0f, 420.0f}, true);
        XBase::UI::Window("QuickTeleportMap", I18n::T("quickMap.title"), [&] {
            XBase::UI::Text(I18n::T("quickMap.hint"));
            const XBase::Vec2 canvasPos = XBase::UI::GetCursorScreenPosition();
            const float available = XBase::UI::GetContentAvailable().x;
            const XBase::Vec2 canvasSize{available > 260.0f ? available : 260.0f, available > 260.0f ? available : 260.0f};
            XBase::UI::Canvas::RectFilled(canvasPos, {canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y}, {20, 25, 32, 220});
            XBase::UI::Canvas::Rect(canvasPos, {canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y}, {160, 180, 210, 255});

            const XBase::Vec2 center{canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f};
            XBase::UI::Canvas::Line({center.x, canvasPos.y}, {center.x, canvasPos.y + canvasSize.y}, {80, 90, 105, 180});
            XBase::UI::Canvas::Line({canvasPos.x, center.y}, {canvasPos.x + canvasSize.x, center.y}, {80, 90, 105, 180});

            XBase::Vec3 playerPos;
            if (XBase::Teleport::TryGetCurrentPosition(playerPos)) {
                const XBase::Vec2 playerPoint = MapToScreen(playerPos, canvasPos, canvasSize);
                XBase::UI::Canvas::CircleFilled(playerPoint, 5.0f, {80, 220, 120, 255});
            }

            XBase::UI::InvisibleButton("##QuickMapCanvas", canvasSize);
            const bool hovered = XBase::UI::IsLastItemHovered();
            const bool mouseDown = XBase::UI::IsMouseDown(XBase::UI::MouseButton::Left);
            if (hovered && mouseDown && !lastMapMouseDown) {
                const XBase::Vec3 target = ScreenToMap(XBase::UI::GetMousePosition(), canvasPos, canvasSize);
                XBase::Teleport::MapPosition(target.x, target.y, MenuState::SpawnUnderwater);
                MenuState::QuickTeleportMapActive = false;
            }
            lastMapMouseDown = mouseDown;
        }, &MenuState::QuickTeleportMapActive, XBase::UI::Flag(XBase::UI::WindowFlag::NoCollapse));
    }
}