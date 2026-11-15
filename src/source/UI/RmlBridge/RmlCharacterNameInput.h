#pragma once

#include <RmlUi/Core/DataModelHandle.h>
#include <string>

namespace UI::RmlBridge
{
// RmlUi's dirty model writes the bounded value back into the edit buffer.
void BindCharacterNameInput(Rml::DataModelConstructor& constructor, std::string& name);
} // namespace UI::RmlBridge
