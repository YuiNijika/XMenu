#include "particle_sa.h"
#include "CPlayerPed.h"
#include "extensions/ScriptCommands.h"
#include "plugin.h"

ParticleMgr& Particle = ParticleMgr::Get();

void ParticleMgr::Play(const std::string& particle)
{
    CPlayerPed* pPlayer = FindPlayerPed();
    if (pPlayer)
    {
        CVector pos = pPlayer->GetPosition();
        int handle;
        plugin::Command<plugin::Commands::CREATE_FX_SYSTEM>(particle.c_str(), pos.x, pos.y, pos.z, 1, &handle);
        plugin::Command<plugin::Commands::PLAY_FX_SYSTEM>(handle);
        m_nList.push_back(handle);
    }
}

void ParticleMgr::RemoveAll()
{
    for (int& p : m_nList)
    {
        plugin::Command<plugin::Commands::KILL_FX_SYSTEM>(p);
    }
    m_nList.clear();
}

void ParticleMgr::RemoveLatest()
{
    if (m_nList.empty()) return;
    plugin::Command<plugin::Commands::KILL_FX_SYSTEM>(m_nList.back()); // stop if anything is running
    m_nList.pop_back();
}