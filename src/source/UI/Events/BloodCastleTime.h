
#ifndef _NEWUIBLOODCASTLE_H_
#define _NEWUIBLOODCASTLE_H_

#pragma once

#include "UI/Core/WindowObject.h"
#include "GameLogic/Events/BloodCastleObjective.h"
#include "UI/Events/EventTimerView.h"
#include "UI/Core/WindowManager.h"

#include <string>

namespace mu::ui::window
{
// The Blood Castle time HUD. blood_castle_time.rml draws it (EventTimerView); C++ keeps the time, the kill
// count and when it is shown.
class CBloodCastle : public CObject
{
public:
    enum IMAGE_LIST
    {
        IMAGE_BLOODCASTLE_TIME_WINDOW = BITMAP_INTERFACE_NEW_BLOODCASTLE_BEGIN // newui_Figure_blood.tga (124, 81)
    };

private:
    enum BLOODCASTLE_TIME_WINDOW_SIZE
    {
        BLOODCASTLE_TIME_WINDOW_WIDTH = 124,
        BLOODCASTLE_TIME_WINDOW_HEIGHT = 81,
    };

    enum
    {
        BC_TIME_STATE_NORMAL = 1,
        BC_TIME_STATE_IMMINENCE,
    };

private:
    CManager* m_pNewUIMng;


    wchar_t m_szTime[256]; // 시간
    int m_iTime;           // 시간
    int m_iTimeState;      // 시간상태( 기본, 임박 )
    GameLogic::Events::BloodCastleObjective m_Objective;
    std::wstring m_ObjectiveText;
    std::string m_ObjectiveLocale;
    EventTimerView m_View{"blood_castle_time", "Data/Interface/RmlUi/blood_castle_time.rml"};

public:
    CBloodCastle();
    virtual ~CBloodCastle();

    bool Create(CManager* pNewUIMng);
    void Release();

    bool UpdateMouseEvent() override;
    Rml::ElementDocument* GetPlacedDocument() const override { return m_View.Document(); }
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    bool BtnProcess();

    float GetLayerDepth() override; //. 1.2f

    void OpenningProcess();
    void ClosingProcess();


private:
    void SyncView();

public:
    void SetTime(int m_iTime);
    void SetObjective(const GameLogic::Events::BloodCastleObjective& objective);
};
}

#endif // _NEWUIBLOODCASTLE_H_
