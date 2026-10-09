#include "Mods.h"

#include <XBase/Platform.h>
#include <XBase/Runtime.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <mutex>

namespace Controllers::Mods {
namespace {
using XBase::Json::Value;
std::mutex s_mutex;
std::future<Value> s_request;
Value s_data = Value::Parse("{\"items\":[],\"pagination\":{\"page\":1,\"limit\":12,\"total\":0,\"total_pages\":0}}");
bool s_loading = false;
bool s_requested = false;
int s_page = 1;
int s_limit = 12;
std::string s_type;
std::string s_error;

std::string Encode(const std::string& value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
            result += static_cast<char>(c);
        } else {
            result += '%';
            result += hex[c >> 4];
            result += hex[c & 15];
        }
    }
    return result;
}

void Poll() {
    if (!s_loading || s_request.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    Value response;
    try {
        response = s_request.get();
    } catch (...) {
        s_error = "mods.failed";
    }
    s_loading = false;
    const Value& data = response["data"];
    if (!response["success"].AsBool() || !data["items"].IsArray()
        || !data["pagination"].IsObject()) {
        s_error = "mods.failed";
        return;
    }
    s_data = data;
    s_error.clear();
}
}

const char* GameSlug() {
    switch (XBase::Runtime::GetGameTarget()) {
    case XBase::Runtime::GameTarget::ViceCity: return "Grand-Theft-Auto-Vice-City";
    case XBase::Runtime::GameTarget::SanAndreas: return "Grand-Theft-Auto-San-Andreas";
    case XBase::Runtime::GameTarget::III: return "Grand-Theft-Auto-III";
    default: return "";
    }
}

void Request(int page, int limit, const std::string& type) {
    std::lock_guard<std::mutex> lock(s_mutex);
    Poll();
    if (s_loading) return;
    s_page = std::max(1, page);
    s_limit = std::clamp(limit, 1, 20);
    s_type = type;
    s_error.clear();
    s_requested = true;
    s_loading = true;
    const std::string url = "https://api.miomoe.cn/modx/mods?page=" + std::to_string(s_page)
        + "&limit=" + std::to_string(s_limit) + "&game=" + Encode(GameSlug())
        + (type.empty() ? "" : "&type=" + Encode(type));
    try {
        s_request = std::async(std::launch::async, [url] {
            std::string body;
            if (!XBase::Platform::DownloadText(url.c_str(), body)) return Value{};
            return Value::Parse(body);
        });
    } catch (...) {
        s_loading = false;
        s_error = "mods.failed";
    }
}

Value Snapshot() {
    std::lock_guard<std::mutex> lock(s_mutex);
    Poll();
    Value result = s_data;
    result.Set("loading", Value(s_loading));
    result.Set("requested", Value(s_requested));
    result.Set("error", Value(s_error));
    result.Set("gameSlug", Value(GameSlug()));
    result.Set("type", Value(s_type));
    result.Set("requestedPage", Value(s_page));
    result.Set("requestedLimit", Value(s_limit));
    return result;
}

bool Open(const std::string& id) {
    std::string link;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        Poll();
        const Value& items = s_data["items"];
        for (std::size_t index = 0; index < items.Size(); ++index) {
            const Value& item = items[index];
            if (item["id"].AsString() != id) continue;
            std::string slug = item["slug"].AsString();
            if (slug.empty()) slug = item["short_id"].AsString();
            if (!slug.empty()) link = "https://gtamodx.com/mods/" + Encode(slug);
            break;
        }
    }
    return !link.empty() && XBase::Platform::OpenExternal(link.c_str());
}

bool OpenGame() {
    const std::string link = "https://gtamodx.com/game/" + Encode(GameSlug());
    return XBase::Platform::OpenExternal(link.c_str());
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_request.valid()) s_request.wait();
    s_loading = false;
}
}
