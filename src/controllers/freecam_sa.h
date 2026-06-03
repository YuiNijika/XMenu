#pragma once

class CPed;

/*
    Freeroam Camera Mode for SA
    Similar to how airbreaks work
    But more flexible
*/
class FreecamMgr
{
private:
    CPed* m_pPed = nullptr;         // pointer to the dummy ped
    int m_nPed = -1;      // handle to the dummy ped
    bool m_bHudState = false;     // backup of the prev game hud state
    bool m_bRadarState = false;   // backup of the prev game radar state

    FreecamMgr();
    FreecamMgr(const FreecamMgr&) = delete;

public:
    static FreecamMgr& Get() {
        static FreecamMgr instance;
        return instance;
    }

    void Process();
    void Enable();
    void Disable();
};

extern FreecamMgr& Freecam;