#pragma once

// VC / SA 行人/车辆包围盒与局部碰撞线框、骨骼、子弹追踪/穿墙
// III 不实现 Init/Process/Draw 为空桩
namespace Controllers::BulletAssist {
    void Init();
    void Process(); // 每帧更新锁定目标 不改相机
    void Draw();
}