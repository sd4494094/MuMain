
#include "stdafx.h"
#include "I18N/All.h"

#include "NewBloodCastleSystem.h"
#include "UI/Dialogs/CustomMessageBox.h"
#include "UI/Core/WindowSystem.h"
#include "Audio/DSPlaySound.h"
#include "CSChaosCastle.h"
#include "World/MapInfra/MapManager.h"
#include "Render/Text/CUIRenderText.h"

using namespace SEASON3B;
using namespace mu::ui::window;

CNewBloodCastleSystem::CNewBloodCastleSystem()
{
}

CNewBloodCastleSystem::~CNewBloodCastleSystem()
{
}

void CNewBloodCastleSystem::SetMatchResult(const int iNumDevilRank, const int iMyRank, const MatchResult* pMatchResult, const int Success)
{
    if (iNumDevilRank != 255)
    {
        return;
    }

    m_iNumResult = Success;
    memcpy(m_MatchResult, pMatchResult, sizeof(MatchResult));
    mu::ui::window::CreateMessageBox(MSGBOX_LAYOUT_CLASS(mu::ui::window::CBloodCastleResultMsgBoxLayout));
}

void CNewBloodCastleSystem::SetMatchGameCommand(const LPPRECEIVE_MATCH_GAME_STATE data)
{
    switch (data->m_byPlayState)
    {
    case 0:
        SetAllAction(PLAYER_RUSH1);
        PlayBuffer(SOUND_BLOODCASTLE, NULL, true);

    case 1:
        UpdateMatchState(*data);
        break;

    case 2:
        m_Objective = {};
        clearMatchInfo();
        StopBuffer(SOUND_BLOODCASTLE, true);

        break;

    case 3:
        SetActionObject(gMapManager.WorldActive, 36, 20, 1.f);
        break;
    case 4:
        UpdateMatchState(*data);
        break;
    }
}

void CNewBloodCastleSystem::UpdateMatchState(const PRECEIVE_MATCH_GAME_STATE& data)
{
    constexpr int kLegacyMatchDuration = 15 * 60;
    SetMatchInfo(data.m_byPlayState + 1, kLegacyMatchDuration, data.m_wRemainSec, data.m_wMaxKillMonster,
                 data.m_wCurKillMonster);
    m_Objective = GameLogic::Events::BloodCastleObjective::FromState(
        data.m_byPlayState, data.m_wMaxKillMonster, data.m_wCurKillMonster, data.m_byItemType, data.m_wIndex);

    // No weapon owner is sent as 0x00FF by OpenMU or 0xFFFF by original servers.
    constexpr WORD kOpenMuNoOwner = 0x00FF;
    constexpr WORD kOriginalNoOwner = 0xFFFF;
    constexpr BYTE kFirstQuestWeapon = 1;
    constexpr BYTE kLastQuestWeapon = 3;
    if (data.m_wIndex == kOpenMuNoOwner || data.m_wIndex == kOriginalNoOwner || data.m_byItemType < kFirstQuestWeapon ||
        data.m_byItemType > kLastQuestWeapon || CharactersClient == nullptr)
        return;

    constexpr WORD kObjectIndexMask = 0x7FFF;
    const int index = HangerBloodCastleQuestItem(data.m_wIndex & kObjectIndexMask);
    if (index >= 0 && index < MAX_CHARACTERS_CLIENT)
        CharactersClient[index].EtcPart = data.m_byItemType;
}

void CNewBloodCastleSystem::RenderMatchTimes(void)
{
    if (m_byMatchType > 0 && gMapManager.InBloodCastle() == true)
    {
        switch (m_byMatchType)
        {
        case 0:
        case 1:
        case 2:
        case 5:
            if (m_iMatchTime > 0)
            {
                if (!g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_BLOODCASTLE_TIME))
                {
                    g_pNewUISystem->Show(mu::ui::window::INTERFACE_BLOODCASTLE_TIME);
                }

                g_pBloodCastle->SetTime(m_iMatchTime);
                g_pBloodCastle->SetObjective(m_Objective);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_BLOODCASTLE_TIME))
        {
            g_pNewUISystem->Hide(mu::ui::window::INTERFACE_BLOODCASTLE_TIME);
        }
    }
}

void CNewBloodCastleSystem::CollectMatchResult(std::vector<MatchResultLine>& lines) const
{
    wchar_t lpszStr[256] = {};

    auto add = [&lines](const char* role, const wchar_t* text, MatchResultCell::Font font, DWORD color)
    { lines.push_back({role, {{text, 0, 0, font, color}}}); };

    const DWORD green = RGBA(128, 255, 128, 255);
    // One sentence per outcome; the theme wraps it, so every language keeps its own word order.
    add("message", m_iNumResult ? I18N::Game::BloodCastleQuestCompleted : I18N::Game::BloodCastleQuestFailed,
        MatchResultCell::Font::Normal, green);

    const MatchResult* pResult = &m_MatchResult[0];

    mu_swprintf(lpszStr, I18N::Game::RewardedExpD, pResult->m_dwExp);
    add("reward", lpszStr, MatchResultCell::Font::Bold, RGBA(210, 255, 210, 255));

    if (m_iNumResult)
    {
        mu_swprintf(lpszStr, I18N::Game::RewardedZenD, pResult->m_iZen);
        add("reward", lpszStr, MatchResultCell::Font::Bold, RGBA(255, 210, 210, 255));
    }

    mu_swprintf(lpszStr, I18N::Game::BloodCastlePointD, pResult->m_iScore);
    add("reward", lpszStr, MatchResultCell::Font::Bold, RGBA(210, 210, 255, 255));
}
