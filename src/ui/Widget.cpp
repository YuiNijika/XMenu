#include "Widget.h"

#include "MenuState.h"

#include <XBase/UI.h>

namespace {

bool UsesMenuSurface() {
    return MenuState::UseNativeMenu;
}

} // namespace

namespace UI {

void SameLineEvery(int index, int columns) {
    if (!UsesMenuSurface() && columns > 1 && (index + 1) % columns != 0) {
        XBase::UI::SameLine();
    }
}

void SpacingSeparator() {
    if (UsesMenuSurface()) return;
    XBase::UI::Spacing();
    XBase::UI::Separator();
    XBase::UI::Spacing();
}

void TextCentered(const char* text) {
    if (!UsesMenuSurface()) XBase::UI::CenterText(text);
}

bool CollapsingHeader(const char* label, bool& isOpen) {
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceCollapsingSection(label, isOpen);
    return XBase::UI::CollapsingSection(label, isOpen);
}

bool Checkbox(const char* label, bool* value) {
    if (!value) return false;
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceCheckbox(label, *value);
    return XBase::UI::Checkbox(label, *value);
}

bool Button(const char* label, short columns) {
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceButton(label);
    return XBase::UI::Button(label, XBase::UI::GridItemSize(columns));
}

bool SliderFloat(const char* label, float* value, float minValue, float maxValue, const char* format) {
    if (!value) return false;
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceSlider(label, *value, minValue, maxValue, format);
    return XBase::UI::Slider(label, *value, minValue, maxValue, format);
}

bool SliderInt(const char* label, int* value, int minValue, int maxValue) {
    if (!value) return false;
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceSlider(label, *value, minValue, maxValue);
    return XBase::UI::Slider(label, *value, minValue, maxValue);
}

bool InputFloat(const char* label, float* value, float step, float fastStep, const char* format) {
    if (!value) return false;
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceInput(label, *value, step, fastStep, format);
    return XBase::UI::Input(label, *value, step, fastStep, format);
}

bool InputInt(const char* label, int* value, int step, int fastStep) {
    if (!value) return false;
    if (UsesMenuSurface()) return XBase::UI::MenuSurfaceInput(label, *value, step, fastStep);
    return XBase::UI::Input(label, *value, step, fastStep);
}

void SameLine() {
    if (!UsesMenuSurface()) XBase::UI::SameLine();
}

void Columns(int count, const char* id, bool border) {
    if (!UsesMenuSurface()) XBase::UI::Columns(count, id, border);
}

void NextColumn() {
    if (!UsesMenuSurface()) XBase::UI::NextColumn();
}

void PushItemWidth(float width) {
    if (!UsesMenuSurface()) XBase::UI::PushItemWidth(width);
}

void PopItemWidth() {
    if (!UsesMenuSurface()) XBase::UI::PopItemWidth();
}

} // namespace UI