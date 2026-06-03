#include "randomcheats_sa.h"
#include "ui/MenuState.h"
#include "CTimer.h"
#include "plugin.h"
#include "CSprite2d.h"
#include "CRect.h"
#include "CRGBA.h"
#include "imgui/imgui.h"
#include "utils/JsonLoader.h"
#include "resources/ResourceData.h"

RandomCheatsMgr& RandomCheats = RandomCheatsMgr::Get();

void RandomCheatsMgr::Process()
{ 
    if (!MenuState::RandomCheatsEnabled) {
        return;
    }

    unsigned int timer = CTimer::m_snTimeInMilliseconds;
    static unsigned int m_nTimer = 0;

    if ((timer - m_nTimer) > (static_cast<unsigned int>(MenuState::RandomCheatsInterval) * 1000))
    {
        int id = plugin::RandomNumberInRange(0, 91);

        if (m_EnabledCheats[id][1] == "true")
        {
            plugin::Call<0x438370>(id); // cheatEnableLegimate(int CheatID)
            plugin::Call<0x69F0B0>((char*)m_EnabledCheats[id][0].c_str(), 2000, 0, false); // CMessages::AddMessage
            m_nTimer = timer;
        }
    }

    if (MenuState::RandomCheatsProgressBar)
    {
        // Next cheat timer bar
        unsigned int screenWidth = RsGlobal.maximumWidth;
        unsigned int screenHeight = RsGlobal.maximumHeight;
        unsigned int totalTime = MenuState::RandomCheatsInterval;
        float progress = (totalTime - (timer - m_nTimer) / 1000.0f) / totalTime;

        float fWidth = static_cast<float>(screenWidth);
        float fHeight = static_cast<float>(screenHeight);
        CRect sizeBox = CRect(0.0f, 0.0f, fWidth, fHeight / 50.0f);
        CRect sizeProgress = CRect(0.0f, 0.0f, fWidth * progress, fHeight / 50.0f);
        CRGBA colorBG = CRGBA(24, 99, 44, 255);
        CRGBA colorProgress = CRGBA(33, 145, 63, 255);

        CSprite2d::DrawRect(sizeBox, colorBG);
        CSprite2d::DrawRect(sizeProgress, colorProgress);
    }
}

void RandomCheatsMgr::DrawList()
{
    for (int i = 0; i < 92; i++)
    {
        bool selected = (m_EnabledCheats[i][1] == "true");
        if (ImGui::MenuItem(m_EnabledCheats[i][0].c_str(), nullptr, selected))
        {
            m_EnabledCheats[i][1] = selected ? "false" : "true";
        }
    }
}

RandomCheatsMgr::RandomCheatsMgr()
{
    // Need to load cheat list from json, currently mocked or placeholder
    for (int i = 0; i < 92; i++) {
        m_EnabledCheats[i][0] = "Cheat " + std::to_string(i);
        m_EnabledCheats[i][1] = "true";
    }
}