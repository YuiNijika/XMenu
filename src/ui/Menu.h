#pragma once

namespace Menu {
    enum class Page {
        Player, Vehicle, Weapon, World, Scene, Visual, Teleport, Settings, About, Ped
    };

    void Draw();
    void Process();

    void PushPage(Page page);
    void PopPage();
}