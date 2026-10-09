#include "Mods.h"

#include "controllers/Mods.h"
#include "ui/Widget.h"
#include "utils/I18n.h"

#include <XBase/UI.h>

#include <cstdio>
#include <functional>
#include <string>

namespace Pages::Mods {
namespace {
struct CategoryOption {
    const char* value;
    const char* labelKey;
};

constexpr CategoryOption kCategories[] = {
    {"", "mods.allCategories"},
    {"plugins & scripts", "mods.categoryPlugins"},
    {"vehicles", "mods.categoryVehicles"},
    {"peds", "mods.categoryPeds"},
    {"maps", "mods.categoryMaps"},
    {"graphics", "mods.categoryGraphics"},
    {"tools", "mods.categoryTools"},
};

const CategoryOption& CurrentCategory(const std::string& value) {
    for (const CategoryOption& option : kCategories) {
        if (value == option.value) return option;
    }
    return kCategories[0];
}

void DrawCategoryPicker(const std::string& current, bool disabled,
    const std::function<void(const char*)>& onChanged) {
    const CategoryOption& selected = CurrentCategory(current);
    XBase::UI::Disabled(disabled, [&] {
        XBase::UI::Combo("##ModsCategory", I18n::T(selected.labelKey), [&] {
            for (const CategoryOption& option : kCategories) {
                if (XBase::UI::Selectable(I18n::T(option.labelKey), current == option.value)) {
                    onChanged(option.value);
                }
            }
        });
    });
}
}

void Draw() {
    namespace Catalog = Controllers::Mods;
    XBase::Json::Value state = Catalog::Snapshot();
    if (!state["requested"].AsBool()) {
        Catalog::Request();
        state = Catalog::Snapshot();
    }

    const bool busy = state["loading"].AsBool();
    const int page = state["requestedPage"].AsInt(1);
    const int limit = state["requestedLimit"].AsInt(12);
    const std::string type = state["type"].AsString();
    const auto& pagination = state["pagination"];

    XBase::UI::TextDisabled(Catalog::GameSlug());
    XBase::UI::BeginGroupBox("ModsToolbar");
    XBase::UI::Text(I18n::T("mods.category"));
    DrawCategoryPicker(type, busy, [&](const char* next) {
        Catalog::Request(1, limit, next);
    });
    XBase::UI::SameLine();
    XBase::UI::Text(I18n::T("mods.pageSize"));
    XBase::UI::SameLine();
    if (!busy && UI::Button("6###ModsLimit6")) Catalog::Request(1, 6, type);
    XBase::UI::SameLine();
    if (!busy && UI::Button("12###ModsLimit12")) Catalog::Request(1, 12, type);
    XBase::UI::SameLine();
    if (!busy && UI::Button("20###ModsLimit20")) Catalog::Request(1, 20, type);
    XBase::UI::SameLine();
    if (!busy && UI::Button(I18n::T("mods.refresh"))) Catalog::Request(page, limit, type);
    XBase::UI::EndGroupBox();

    if (UI::Button(I18n::T("mods.gamePage"))) Catalog::OpenGame();
    if (busy) XBase::UI::TextDisabled(I18n::T("mods.loading"));
    if (!state["error"].AsString().empty()) {
        UI::TextWarning(I18n::T("mods.failed"));
        return;
    }

    XBase::UI::Separator();
    const auto& items = state["items"];
    if (!busy && items.Size() == 0) XBase::UI::TextWrapped(I18n::T("mods.empty"));

    XBase::UI::Columns(2, "ModsGrid", false);
    for (std::size_t index = 0; index < items.Size(); ++index) {
        const auto& item = items[index];
        const std::string id = item["id"].AsString();
        const std::string title = item["title"].AsString();
        const std::string cardId = title + "###ModCard_" + id;
        XBase::UI::Group(cardId.c_str(), [&] {
            const std::string category = item["category"]["name"].AsString();
            if (!category.empty()) XBase::UI::TextDisabled(category.c_str());
            XBase::UI::TextWrapped(title.c_str());
            std::string author = item["reposted"].AsBool()
                ? item["original_author"].AsString() : item["author"]["name"].AsString();
            if (author.empty()) author = I18n::T("mods.unknownAuthor");
            XBase::UI::TextDisabled(I18n::T("mods.author"), author.c_str());
            const std::string summary = item["summary"].AsString();
            if (!summary.empty()) XBase::UI::TextWrapped(summary.c_str());

            char stats[180]{};
            std::snprintf(stats, sizeof(stats), "%s %d  ·  %s %d  ·  %s %d  ·  %s %d",
                I18n::T("mods.downloads"), item["downloads"].AsInt(),
                I18n::T("mods.views"), item["views"].AsInt(),
                I18n::T("mods.likes"), item["like_count"].AsInt(),
                I18n::T("mods.comments"), item["comments_count"].AsInt());
            XBase::UI::TextDisabled(stats);
            const std::string button = std::string(I18n::T("mods.download")) + "###ModOpen_" + id;
            if (UI::Button(button.c_str()) && !Catalog::Open(id)) {
                UI::TextWarning(I18n::T("mods.openFailed"));
            }
        });
        XBase::UI::NextColumn();
    }
    XBase::UI::Columns(1);

    XBase::UI::Separator();
    XBase::UI::Text(I18n::T("mods.pagination"), pagination["page"].AsInt(1), pagination["total_pages"].AsInt());
    XBase::UI::Disabled(busy || page <= 1, [&] {
        if (UI::Button(I18n::T("mods.previous"))) Catalog::Request(page - 1, limit, type);
    });
    XBase::UI::SameLine();
    XBase::UI::Disabled(busy || page >= pagination["total_pages"].AsInt(), [&] {
        if (UI::Button(I18n::T("mods.next"))) Catalog::Request(page + 1, limit, type);
    });
}
}
