"""Export compiled official localization tables, and exercise every locale switch."""
import json
import re
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
out = root / ".localization-output"
gen = out / "generated"
groups = ("Game", "Dialog", "Metadata")
source = ['#include "All.cpp"'] + [f'#include "{g}.cpp"' for g in ("Game", "Dialog", "Metadata", "Editor")]
source += [r"""
#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>
std::string utf8(const wchar_t* value) {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(value);
}
std::string utf8(const char* value) { return value; }
template<typename T> void hex(const T* value) {
    for (unsigned char c : utf8(value)) {
        constexpr char digits[] = "0123456789abcdef";
        std::cout << digits[c >> 4] << digits[c & 15];
    }
}
int main() {
    assert(I18N::LocaleIndex("zh-CN") == 10);
    assert(I18N::GetAvailableLocales().size() == 12);
"""]
for group in groups:
    source += [f"""
    for (int locale = 0; locale < 12; ++locale) {{
        I18N::SetLocale(I18N::GetAvailableLocales()[locale]);
        for (const auto& slot : I18N::{group}::kSlots) assert(*slot.dest == slot.values[locale]);
    }}
    I18N::SetLocale("zh-CN");
    for (const auto& slot : I18N::{group}::kSlots) {{
        std::cout << "{group}\\t"; hex(slot.values[0]); std::cout << "\\t";
        hex(*slot.dest); std::cout << "\\n";
    }}
"""]
source += ["}"]
(out / "locale_harness.cpp").write_text("\n".join(source))
subprocess.run(["g++", "-std=c++20", "-O0", "-I"+str(gen), str(out/"locale_harness.cpp"), "-o", str(out/"locale_harness")], check=True)
run = subprocess.run([str(out/"locale_harness")], capture_output=True, text=True, check=True)
data = {}
for group in groups:
    cpp = (gen/(group+".cpp")).read_text()
    identifiers = re.findall(r"^    \{ &(\w+), \{ ", cpp, re.M)
    rows = [s.split("\t") for s in run.stdout.splitlines() if s.startswith(group+"\t")]
    assert len(rows) == len(identifiers)
    data[group] = [{"identifier": ident, "en": bytes.fromhex(row[1]).decode("utf-8"), "zh": bytes.fromhex(row[2]).decode("utf-8")} for ident, row in zip(identifiers,rows)]
(out/"compiled-locales.json").write_text(json.dumps(data, ensure_ascii=False,indent=2))
print("COMPILED_LOCALE_SWITCH_TEST_PASS", {g: len(v) for g,v in data.items()})
