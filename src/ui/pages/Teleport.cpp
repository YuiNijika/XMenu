#include "Teleport.h"
#include "controllers/Teleport.h"
#include "imgui/imgui.h"

namespace Pages::Teleport {
    void Draw() {
        if (ImGui::Button((const char*)u8"向前挪 5 米")) {
            Controllers::Teleport::Forward(5.0f);
        }

        ImGui::Separator();
        ImGui::Text((const char*)u8"常去地点");
        Controllers::Teleport::DrawLocations();
    }
}