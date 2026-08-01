#pragma once

#if defined(GTA3)
#include <XBase/BulletAssist.h>
#endif

namespace Controllers::BulletAssist {
#if defined(GTA3)
    inline void Process() {
        XBase::BulletAssist::Process();
    }

    inline void Draw() {
        XBase::BulletAssist::Draw();
    }
#else
    void Init();
    void Process();
    void Draw();
#endif
}