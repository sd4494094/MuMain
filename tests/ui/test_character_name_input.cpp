#include <doctest.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/TextInputContext.h>
#include <RmlUi/Core/TextInputHandler.h>

#include "Character/CharacterName.h"
#include "UI/RmlBridge/RmlCharacterNameInput.h"

namespace
{
class NameInputFixture
{
    struct Renderer : Rml::RenderInterface
    {
        Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override
        {
            return 1;
        }
        void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
        void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
        Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override
        {
            return 0;
        }
        Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override
        {
            return 0;
        }
        void ReleaseTexture(Rml::TextureHandle) override {}
        void EnableScissorRegion(bool) override {}
        void SetScissorRegion(Rml::Rectanglei) override {}
    } m_Renderer;

    struct System : Rml::SystemInterface
    {
        void GetClipboardText(Rml::String& text) override
        {
            text = clipboard;
        }
        Rml::String clipboard;
    } m_System;

    struct Editor : Rml::TextInputHandler
    {
        void OnActivate(Rml::TextInputContext* value) override
        {
            active = value;
        }
        void OnDeactivate(Rml::TextInputContext* value) override
        {
            if (active == value)
                active = nullptr;
        }
        void OnDestroy(Rml::TextInputContext* value) override
        {
            if (active == value)
                active = nullptr;
        }
        Rml::TextInputContext* active = nullptr;
    } m_Editor;

public:
    NameInputFixture()
    {
        Rml::SetRenderInterface(&m_Renderer);
        Rml::SetSystemInterface(&m_System);
        Rml::SetTextInputHandler(&m_Editor);
        REQUIRE(Rml::Initialise());
        m_Context = Rml::CreateContext("name-input", {640, 480});
        REQUIRE(m_Context);
        auto constructor = m_Context->CreateDataModel("character_name");
        REQUIRE(constructor);
        UI::RmlBridge::BindCharacterNameInput(constructor, name);
        Load();
    }

    ~NameInputFixture()
    {
        Rml::Shutdown();
        Rml::SetRenderInterface(nullptr);
        Rml::SetSystemInterface(nullptr);
        Rml::SetTextInputHandler(nullptr);
    }

    void Load()
    {
        m_Document = m_Context->LoadDocumentFromMemory(
            "<rml><head></head><body data-model='character_name'>"
            "<input id='name' type='text' maxlength='10' data-value='char_name'/></body></rml>");
        REQUIRE(m_Document);
        m_Document->Show();
        m_Context->Update();
        m_Field = dynamic_cast<Rml::ElementFormControlInput*>(m_Document->GetElementById("name"));
        REQUIRE(m_Field);
        m_Field->Focus();
    }

    void Type(const Rml::String& text)
    {
        m_Context->ProcessTextInput(text);
        m_Context->Update();
    }

    void Paste(const Rml::String& text)
    {
        m_System.clipboard = text;
        m_Context->ProcessKeyDown(Rml::Input::KI_V, Rml::Input::KM_CTRL);
        m_Context->ProcessKeyUp(Rml::Input::KI_V, Rml::Input::KM_CTRL);
        m_Context->Update();
    }

    void Replace(const Rml::String& text)
    {
        m_Field->Select();
        if (text.empty())
        {
            m_Context->ProcessKeyDown(Rml::Input::KI_BACK, 0);
            m_Context->ProcessKeyUp(Rml::Input::KI_BACK, 0);
            m_Context->Update();
        }
        else
            Type(text);
    }

    void ComposeAndCommit(const Rml::String& text)
    {
        REQUIRE(m_Editor.active);
        auto* input = m_Editor.active;
        int start = 0;
        int end = 0;
        input->GetSelectionRange(start, end);
        // Match SDL's pre-edit, empty edit, then committed TextInput sequence.
        input->SetText(text, start, end);
        const int compositionEnd = start + static_cast<int>(Rml::StringUtilities::LengthUTF8(text));
        input->SetCompositionRange(start, compositionEnd);
        input->SetText(Rml::StringView(), start, compositionEnd);
        input->SetCompositionRange(start, start);
        input->SetCursorPosition(start);
        input->CommitComposition(Rml::StringView());
        Type(text);
    }

    void Reload()
    {
        m_Document->Close();
        m_Context->Update();
        Load();
    }

    Rml::String Value() const
    {
        return m_Field->GetValue();
    }
    Rml::String name;

private:
    Rml::Context* m_Context = nullptr;
    Rml::ElementDocument* m_Document = nullptr;
    Rml::ElementFormControlInput* m_Field = nullptr;
};
} // namespace

TEST_CASE("character names fit the shared Season Six name policy [ui][character_name]")
{
    for (const std::string name : {"血雨星", "血雨星A", "Abc", "1234567890"})
        CHECK(Character::IsValidName(name));
    for (const std::string name :
         {"", "血雨", "血雨星辰", "血雨星辰天", "12345678901", "Name\n", "Name!", "血 雨星", "玩家😀", "@Abc"})
        CHECK_FALSE(Character::IsValidName(name));
    CHECK_FALSE(Character::IsValidName(std::string("A\0B", 3)));
    CHECK_FALSE(Character::IsValidName("\xED\xA0\x80"));
}

TEST_CASE("the live name input bounds typing paste replacement and reload [ui][rmlui][character_name]")
{
    NameInputFixture fixture;
    fixture.Type("血雨星辰天");
    CHECK(fixture.name == "血雨星");
    CHECK(fixture.Value() == fixture.name);
    fixture.Type("A");
    CHECK(fixture.Value() == "血雨星A");
    fixture.Type("B");
    CHECK(fixture.Value() == "血雨星A");
    fixture.Replace("123456789012345");
    CHECK(fixture.Value() == "1234567890");
    CHECK(fixture.name == fixture.Value());
    fixture.Replace("");
    fixture.Paste("血雨星辰天");
    CHECK(fixture.Value() == "血雨星");
    CHECK(fixture.name == fixture.Value());
    fixture.Reload();
    CHECK(fixture.Value() == "血雨星");
    fixture.Type("BCDEFGHIJKLMNOP");
    CHECK(fixture.Value().size() <= Character::MaximumNameBytes);
    CHECK(fixture.name == fixture.Value());
}

TEST_CASE("IME committed names obey the byte limit after pre-edit [ui][rmlui][character_name]")
{
    NameInputFixture fixture;
    fixture.ComposeAndCommit("血雨星辰天");
    CHECK(fixture.name == "血雨星");
    CHECK(fixture.Value() == fixture.name);
    fixture.ComposeAndCommit("辰天");
    CHECK(fixture.Value() == "血雨星");
    CHECK(fixture.name == fixture.Value());
}
