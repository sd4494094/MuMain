#include "UI/RmlBridge/RmlCharacterNameInput.h"
#include "Character/CharacterName.h"

#include <RmlUi/Core/Variant.h>

namespace UI::RmlBridge
{
void BindCharacterNameInput(Rml::DataModelConstructor& constructor, std::string& name)
{
    constructor.BindFunc(
        "char_name", [&name](Rml::Variant& value) { value = name; },
        [&name](const Rml::Variant& value) { name = Character::LimitNameBytes(value.Get<Rml::String>()); });
}
} // namespace UI::RmlBridge
