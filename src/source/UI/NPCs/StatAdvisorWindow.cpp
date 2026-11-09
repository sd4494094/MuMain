#include "stdafx.h"
#include "UI/NPCs/StatAdvisorWindow.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowCommon.h"
#include "UI/RmlBridge/RmlNumericInputFilter.h"
#include "UI/RmlBridge/RmlNativeTextSize.h"
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <algorithm>

namespace mu::ui::window
{
CStatAdvisorWindow* g_pStatAdvisorWindow = nullptr;

void CStatAdvisorWindow::Create(CManager* manager)
{
    manager->AddUIObj(INTERFACE_STAT_ADVISOR, this);
    m_View.Ensure();
}
void CStatAdvisorWindow::Release()
{
    m_Active = m_AwaitingOpen = false;
    m_View.Release();
}
void CStatAdvisorWindow::Built()
{
    auto* document = m_View.Document();
    UI::RmlBridge::AttachNumericInputFilter(document);
}
void CStatAdvisorWindow::Bind(Rml::DataModelConstructor& c, Model& m)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("current", &Row::current);
    row.RegisterMember("amount", &Row::amount);
    row.RegisterMember("result", &Row::result);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("rows", &m.rows);
    c.Bind("page", &m.page);
    c.Bind("text_px", &m.textPx);
    c.Bind("title", &m.title);
    c.Bind("message", &m.message);
    c.Bind("feedback", &m.feedback);
    c.Bind("available", &m.availableText);
    c.Bind("total", &m.totalText);
    c.Bind("remaining", &m.remainingText);
    c.Bind("pending", &m.pending);
    c.Bind("valid", &m.valid);
    c.Bind("can_confirm", &m.canConfirm);
    c.Bind("reset", &m.reset);
    c.Bind("allocate", &m.allocate);
    c.Bind("leave", &m.leave);
    c.Bind("back", &m.back);
    c.Bind("clear", &m.clear);
    c.Bind("next", &m.next);
    c.Bind("confirm_reset", &m.confirmReset);
    c.Bind("confirm_allocation", &m.confirmAllocation);
    c.Bind("attribute_label", &m.attributeLabel);
    c.Bind("current_label", &m.currentLabel);
    c.Bind("increase_label", &m.increaseLabel);
    c.Bind("result_label", &m.resultLabel);
    c.BindEventCallback("advisor_action", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.size() != 1 || !m_Active)
            return;
        const int action = args[0].Get<int>(-1);
        if (action == 7)
            Close();
        else if (action == 8 && !m_View.GetModel().pending)
            Clear();
        else if (action >= 1 && action <= 6 && !m_View.GetModel().pending)
        {
            Recalculate();
            if (action == 4 && !m_View.GetModel().valid)
                return;
            if ((action == 2 || action == 5) && !m_View.GetModel().canConfirm)
                return;
            Send(static_cast<std::uint8_t>(action));
        }
    });
}
void CStatAdvisorWindow::Begin()
{
    if (!SocketClient || SceneFlag != MAIN_SCENE)
        return;
    m_SessionId.clear();
    m_Reply = {};
    m_Active = m_AwaitingOpen = true;
    m_ConnectionId = SocketClient->GetId();
    m_View.GetModel().rows.clear();
    Send(0);
}
void CStatAdvisorWindow::SetPending(bool pending)
{
    m_View.GetModel().pending = pending;
    m_View.MarkDirty("pending");
}
void CStatAdvisorWindow::Send(std::uint8_t operation)
{
    if (!SocketClient || !SocketClient->IsConnected())
        return;
    std::array<std::string, 5> amounts{};
    if (operation == 4)
        for (const auto& row : m_View.GetModel().rows)
            amounts[row.id] = row.amount;
    const auto bytes = Network::MakeStatAdvisorRequest(operation, m_SessionId, m_Reply.token, amounts);
    if (!bytes)
        return;
    SetPending(true);
    m_SentAt = GetTickCount();
    SocketClient->Send(bytes->data(), static_cast<int>(bytes->size()));
}
void CStatAdvisorWindow::Receive(Network::StatAdvisorPacket reply)
{
    if (!m_Active || !SocketClient || SocketClient->GetId() != m_ConnectionId ||
        (!m_AwaitingOpen && reply.sessionId != m_SessionId) || SceneFlag != MAIN_SCENE)
        return;
    m_AwaitingOpen = false;
    m_SessionId = reply.sessionId;
    if (reply.page == 4)
    {
        m_Active = false;
        m_View.Hide();
        return;
    }
    if (!m_View.Ensure())
    {
        Close();
        return;
    }
    UI::RmlBridge::SyncNativeTextSize(m_View.Binder());
    auto& m = m_View.GetModel();
    auto oldRows = std::move(m.rows);
    m_Reply = std::move(reply);
    m.page = m_Reply.page;
    m_FocusAllocationInput = m.page == 2;
    m.title = m_Reply.title;
    m.message = m_Reply.message;
    m.canConfirm = m_Reply.canConfirm;
    m.available = m_Reply.available;
    m.rows.clear();
    for (const auto& value : m_Reply.rows)
    {
        Row row{value.id,      value.current, value.maximum,
                value.current, value.name,    (value.amount == 0 ? std::string{} : std::to_string(value.amount))};
        const auto old = std::find_if(oldRows.begin(), oldRows.end(), [&](const Row& r) { return r.id == value.id; });
        if (m.page == 2 && !m_Reply.clearInputs && old != oldRows.end())
            row.amount = old->amount;
        m.rows.push_back(std::move(row));
    }
    const auto& l = m_Reply.labels;
    m.reset = l.at("Reset"); m.allocate = l.at("Allocate"); m.leave = l.at("Leave");
    m.back = l.at("Back"); m.clear = l.at("Clear"); m.next = l.at("Next");
    m.confirmReset = l.at("ConfirmReset"); m.confirmAllocation = l.at("ConfirmAllocation");
    m.attributeLabel = l.at("Attribute"); m.currentLabel = l.at("Current");
    m.increaseLabel = l.at("Increase"); m.resultLabel = l.at("Result");
    for (const auto* key : {"rows", "page", "title", "message", "can_confirm", "reset", "allocate", "leave", "back",
                            "clear", "next", "confirm_reset", "confirm_allocation", "attribute_label", "current_label",
                            "increase_label", "result_label"})
        m_View.MarkDirty(key);
    SetPending(false);
    Recalculate();
    auto* document = m_View.Document();
    if (!document->IsVisible())
        document->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    if (auto* body = document->GetElementById("advisor-body"))
        body->SetScrollTop(0.f);
}
void CStatAdvisorWindow::Clear()
{
    for (auto& row : m_View.GetModel().rows)
        row.amount.clear();
    m_View.MarkDirty("rows");
    Recalculate();
}
void CStatAdvisorWindow::Recalculate()
{
    auto& m = m_View.GetModel();
    std::int64_t total = 0;
    bool valid = true, rowsChanged = false;
    for (auto& row : m.rows)
    {
        std::uint32_t amount;
        const bool parsed = Network::ReadStatAdvisorAmount(row.amount, amount);
        const auto result = static_cast<std::int64_t>(row.current) + amount;
        valid = valid && parsed && result <= row.maximum;
        total += amount;
        const int display = parsed && result <= INT_MAX ? static_cast<int>(result) : row.current;
        if (row.result != display)
        {
            row.result = display;
            rowsChanged = true;
        }
    }
    const auto sync = [this](auto& field, const auto& value, const char* key)
    {
        if (field != value)
        {
            field = value;
            m_View.MarkDirty(key);
        }
    };
    const auto& l = m_Reply.labels;
    if (l.empty())
        return;
    sync(m.valid, valid && total > 0 && total <= m.available, "valid");
    sync(m.availableText, l.at("Available") + ": " + std::to_string(m.available), "available");
    sync(m.totalText, l.at("Total") + ": " + std::to_string(total), "total");
    sync(m.remainingText, l.at("Remaining") + ": " + std::to_string(static_cast<std::int64_t>(m.available) - total), "remaining");
    sync(m.feedback, m.pending ? l.at("Waiting") : (!valid || total > m.available) ? l.at("InvalidInput") : std::string{}, "feedback");
    if (rowsChanged)
        m_View.MarkDirty("rows");
}
void CStatAdvisorWindow::Close()
{
    m_Active = m_AwaitingOpen = false;
    m_SessionId.clear();
    m_View.Hide();
    if (SocketClient)
        SocketClient->ToGameServer()->SendCloseNpcRequest();
}
void CStatAdvisorWindow::SyncInputState()
{
    auto* document = m_View.Document();
    if (!document)
        return;
    const auto& model = m_View.GetModel();
    const bool disabled = model.pending || model.page != 2;
    m_InputElements.clear();
    document->QuerySelectorAll(m_InputElements, ".advisor-input");
    for (auto* element : m_InputElements)
        if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element))
            if (input->IsDisabled() != disabled)
                input->SetDisabled(disabled);
}
bool CStatAdvisorWindow::Update()
{
    if (!m_Active)
        return true;
    if (SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected() || SocketClient->GetId() != m_ConnectionId)
    {
        m_Active = m_AwaitingOpen = false;
        m_View.Hide();
        return true;
    }
    // Recover a lost response by fetching authoritative state; never resend a confirmation.
    constexpr DWORD ReplyTimeoutMs = 10000;
    if (m_View.GetModel().pending && GetTickCount() - m_SentAt > ReplyTimeoutMs)
    {
        Send(m_AwaitingOpen ? 0 : 3);
    }
    UI::RmlBridge::SyncNativeTextSize(m_View.Binder());
    SyncInputState();
    if (m_FocusAllocationInput)
    {
        auto* document = m_View.Document();
        auto* input = document ? document->QuerySelector(".advisor-input") : nullptr;
        if (input && input->IsVisible(true))
            m_FocusAllocationInput = !input->Focus();
    }
    Recalculate();
    return true;
}
bool CStatAdvisorWindow::UpdateKeyEvent()
{
    if (!m_Active)
        return true;
    if (IsPress(VK_ESCAPE))
        Close();
    // Enter in a numeric field must never skip the review/explicit-confirm click.
    return false;
}
} // namespace mu::ui::window
