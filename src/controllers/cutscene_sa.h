#pragma once
#include <string>

class CVehicle;

/*
    Cutscene Player Class for SA
    Plays mission cutscenes
*/
class CutsceneMgr
{
private:
    bool m_bRunning = false;          // is cutscene currently running

    // backup data
    int m_nInterior = 0;          // interior player was in 
    CVehicle *m_pLastVeh = nullptr;     // vehicle player was in
    int m_nVehSeat = -1;           // seat id of player vehicle

    CutsceneMgr();
    CutsceneMgr(const CutsceneMgr&) = delete;

public:
    static CutsceneMgr& Get() {
        static CutsceneMgr instance;
        return instance;
    }

    // Plays a cutscene 
    void Play(const std::string& cutsceneId, const std::string& interior);

    // Stops a running cutscene
    void Stop();

    bool IsRunning() const { return m_bRunning; }
};

extern CutsceneMgr& Cutscene;