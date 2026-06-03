#pragma once
#include <string>

/*
    RandomCheats for SA
    Activates/Disactivates cheats randomly
*/
class RandomCheatsMgr
{
private:
    std::string m_EnabledCheats[92][2];

    RandomCheatsMgr();
    RandomCheatsMgr(const RandomCheatsMgr&) = delete;

public:
    static RandomCheatsMgr& Get() {
        static RandomCheatsMgr instance;
        return instance;
    }

    void Process();
    void DrawList();
};

extern RandomCheatsMgr& RandomCheats;