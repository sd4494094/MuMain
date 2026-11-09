#pragma once
#include "Network/Server/StatAdvisorPacket.h"
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

namespace mu::ui::window
{
class CManager;
class CStatAdvisorWindow : public CObject
{
public:
    void Create(CManager* manager);
    void Release();
    void Begin();
    void Receive(Network::StatAdvisorPacket reply);
    Rml::ElementDocument* GetPlacedDocument() const override
    {
        return m_View.Document();
    }
    Rml::ElementDocument* GetFillDocument() const override
    {
        return m_View.Document();
    }
    bool Render() override { return true; }
    bool Update() override;
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    bool IsVisible() const override { return m_Active; }
    float GetLayerDepth() override { return 60.f; }
    float GetKeyEventOrder() override { return 110.f; }
    bool TakesTypingFrom(const Rml::ElementDocument* document) const override
    {
        return document && document == m_View.Document();
    }

private:
    struct Row
    {
        int id = 0, current = 0, maximum = 0, result = 0;
        Rml::String name, amount = "0";
    };
    struct Model
    {
        float textPx = 12.f;
        int page = 0, available = 0, selectedAttribute = -1;
        Rml::String title, message, feedback, availableText, totalText, remainingText;
        Rml::String reset, allocate, leave, back, clear, next, confirmReset, confirmAllocation;
        Rml::String attributeLabel, currentLabel, increaseLabel, resultLabel;
        bool pending = false, valid = false, canConfirm = false;
        std::vector<Row> rows;
    };
    void Bind(Rml::DataModelConstructor& constructor, Model& model);
    void Built();
    void Send(std::uint8_t operation);
    void Close();
    void Recalculate();
    void SyncInputState();
    void Clear();
    void SelectAttribute(int id);
    void SetPending(bool pending);
    Rml::ElementList m_InputElements;
    Network::StatAdvisorPacket m_Reply;
    std::string m_SessionId;
    bool m_Active = false, m_AwaitingOpen = false;
    bool m_FocusAllocationInput = false;
    DWORD m_SentAt = 0;
    int m_ConnectionId = 0;
    UI::RmlBridge::ThemedView<Model> m_View{"stat_advisor",
        [this](Rml::DataModelConstructor& c, Model& model) { Bind(c, model); },
        {{"Data/Interface/RmlUi/stat_advisor.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document,
         .afterBuild = [this] { Built(); }}};
};
extern CStatAdvisorWindow* g_pStatAdvisorWindow;
} // namespace mu::ui::window
