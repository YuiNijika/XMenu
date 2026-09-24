#pragma once

namespace Menu {
    enum class Page {
        Player, Vehicle, Weapon, World, Scene, Visual, Teleport, Web, Settings, About, Ped
    };

    void Draw();
    void Process();

    void PushPage(Page page);
    void PopPage();

    // 界面外观或交互模式改了之后调用，让列表界面在下一帧复位选中项
    void NotifySurfaceChanged();
}