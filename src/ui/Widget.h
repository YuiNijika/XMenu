#pragma once

namespace UI {

void SameLineEvery(int index, int columns);
void SpacingSeparator();
void TextCentered(const char* text);

bool CollapsingHeader(const char* label, bool& isOpen);
bool Checkbox(const char* label, bool* value);
bool Button(const char* label, short columns = 1);
bool SliderFloat(const char* label, float* value, float minValue, float maxValue, const char* format = "%.1f");
bool SliderInt(const char* label, int* value, int minValue, int maxValue);
bool InputFloat(const char* label, float* value, float step = 1.0f, float fastStep = 10.0f, const char* format = "%.1f");
bool InputInt(const char* label, int* value, int step = 1, int fastStep = 10);

void SameLine();
void Columns(int count, const char* id = nullptr, bool border = false);
void NextColumn();
void PushItemWidth(float width);
void PopItemWidth();

} // namespace UI