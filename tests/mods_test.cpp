#include "../src/controllers/Mods.h"

#include <XBase/Platform.h>
#include <XBase/Runtime.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {
std::string requested;
std::string opened;
bool failure = false;
XBase::Runtime::GameTarget game = XBase::Runtime::GameTarget::ViceCity;
}

namespace XBase::Platform {
bool DownloadText(const char* url, std::string& body) {
    requested = url;
    if (failure) return false;
    body = R"({"success":true,"data":{"items":[{"id":"one","slug":"named mod","short_id":"short1"},{"id":"two","slug":null,"short_id":"short2"}],"pagination":{"page":2,"limit":20,"total":30,"total_pages":2}}})";
    return true;
}
bool OpenExternal(const char* url) { opened = url; return true; }
bool ReadTextFile(const std::string&, std::string&) { return false; }
bool WriteTextFile(const std::string&, const std::string&) { return false; }
}
namespace XBase::Runtime {
GameTarget GetGameTarget() { return game; }
}

XBase::Json::Value Wait() {
    for (int attempt = 0; attempt < 100; ++attempt) {
        auto result = Controllers::Mods::Snapshot();
        if (!result["loading"].AsBool()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(false);
    return {};
}

int main() {
    namespace Mods = Controllers::Mods;
    Mods::Request(2, 999, "plugins & scripts");
    auto state = Wait();
    assert(state["items"].Size() == 2);
    assert(requested == "https://api.miomoe.cn/modx/mods?page=2&limit=20&game=Grand-Theft-Auto-Vice-City&type=plugins%20%26%20scripts");
    assert(Mods::Open("one") && opened == "https://gtamodx.com/mods/named%20mod");
    assert(Mods::Open("two") && opened == "https://gtamodx.com/mods/short2");
    assert(!Mods::Open("unknown"));
    assert(Mods::OpenGame() && opened == "https://gtamodx.com/game/Grand-Theft-Auto-Vice-City");
    game = XBase::Runtime::GameTarget::SanAndreas;
    Mods::Request(0, 0);
    state = Wait();
    assert(requested == "https://api.miomoe.cn/modx/mods?page=1&limit=1&game=Grand-Theft-Auto-San-Andreas");
    game = XBase::Runtime::GameTarget::III;
    assert(std::string(Mods::GameSlug()) == "Grand-Theft-Auto-III");
    failure = true;
    Mods::Request();
    assert(Wait()["error"].AsString() == "mods.failed");
    failure = false;
    Mods::Request();
    assert(Wait()["error"].AsString().empty());
    Mods::Shutdown();
    std::cout << "MOD game mapping, filter encoding, limit clamps, slug fallback, trusted browser links and failed request recovery passed\n";
}
