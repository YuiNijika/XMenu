#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "integration/XBaseBridge.h"
#include <XBase/Teleport.h>
#include <XBase/UI.h>

namespace {
    bool lastMapMouseDown = false;

    XBase::Teleport::MapBounds GetMapArea() {
        XBase::Teleport::MapBounds area;
        if (MenuState::TeleportMapWidth > 0.0f && MenuState::TeleportMapHeight > 0.0f) {
            area.minX = -MenuState::TeleportMapWidth * 0.5f;
            area.maxX = MenuState::TeleportMapWidth * 0.5f;
            area.minY = -MenuState::TeleportMapHeight * 0.5f;
            area.maxY = MenuState::TeleportMapHeight * 0.5f;
            return area;
        }
        if (XBase::Teleport::TryGetMapBounds(area)) {
            return area;
        }
        area = {-3000.0f, -3000.0f, 3000.0f, 3000.0f};
        return area;
    }

    XBase::Vec2 MapToScreen(const XBase::Vec3& pos, XBase::Vec2 origin, XBase::Vec2 size, const XBase::Teleport::MapBounds& area) {
        const float width = area.maxX - area.minX;
        const float height = area.maxY - area.minY;
        if (width <= 0.0f || height <= 0.0f) return origin;
        return {origin.x + ((pos.x - area.minX) / width) * size.x, origin.y + ((area.maxY - pos.y) / height) * size.y};
    }

    XBase::Vec3 ScreenToMap(XBase::Vec2 point, XBase::Vec2 origin, XBase::Vec2 size, const XBase::Teleport::MapBounds& area) {
        const float width = area.maxX - area.minX;
        const float height = area.maxY - area.minY;
        const float x = area.minX + ((point.x - origin.x) / size.x) * width;
        const float y = area.maxY - ((point.y - origin.y) / size.y) * height;
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

            const XBase::Teleport::MapBounds area = GetMapArea();
            XBase::Vec3 playerPos;
            if (XBase::Teleport::TryGetCurrentPosition(playerPos)) {
                const XBase::Vec2 playerPoint = MapToScreen(playerPos, canvasPos, canvasSize, area);
                XBase::UI::Canvas::CircleFilled(playerPoint, 5.0f, {80, 220, 120, 255});
            }

            XBase::UI::InvisibleButton("##QuickMapCanvas", canvasSize);
            const bool hovered = XBase::UI::IsLastItemHovered();
            const bool mouseDown = XBase::UI::IsMouseDown(XBase::UI::MouseButton::Left);
            if (hovered && mouseDown && !lastMapMouseDown) {
                const XBase::Vec3 target = ScreenToMap(XBase::UI::GetMousePosition(), canvasPos, canvasSize, area);
                XBase::Teleport::MapPosition(target.x, target.y, MenuState::SpawnUnderwater);
                MenuState::QuickTeleportMapActive = false;
            }
            lastMapMouseDown = mouseDown;
        }, &MenuState::QuickTeleportMapActive, XBase::UI::Flag(XBase::UI::WindowFlag::NoCollapse));
    }
}