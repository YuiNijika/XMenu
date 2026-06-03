#pragma once
#include <string>
#include <vector>

/*
    Particle Player Class for SA
    Spawns particles in the world
*/
class ParticleMgr
{
private:
    std::vector<int> m_nList;

    ParticleMgr(){};
    ParticleMgr(const ParticleMgr&) = delete;

public:
    static ParticleMgr& Get() {
        static ParticleMgr instance;
        return instance;
    }

    // Plays a particle
    void Play(const std::string& particle);

    // Removes all spawned particles
    void RemoveAll();

    // Removes the most recent spawned particle
    void RemoveLatest();
};

extern ParticleMgr& Particle;