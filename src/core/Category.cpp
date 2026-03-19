#include "Category.hpp"

namespace os {

namespace {

static const char* kIconDir = "streamline-vectors/core/pop/interface-essential";

std::vector<Category> makeDefaultCategories() {
    std::vector<Category> cats;

    {
        Category c;
        c.id = ID_CATEGORY_SYSTEM;
        c.name = "system";
        c.label = "System & Settings";
        c.description = "Core system utilities, configuration, and background settings.";
        c.icon = ImageSet(Path(kIconDir, "cog-1.svg"));
        cats.push_back(c);
    }

    {
        Category c;
        c.id = ID_CATEGORY_ACCESSORIES;
        c.name = "accessories";
        c.label = "Accessories";
        c.description = "Small productivity tools like Notepad, Calendar, Paint, and StopWatch.";
        c.icon = ImageSet(Path(kIconDir, "blank-notepad.svg"));
        cats.push_back(c);
    }

    {
        Category c;
        c.id = ID_CATEGORY_GAME;
        c.name = "game";
        c.label = "Games";
        c.description = "Games and entertainment.";
        c.icon = ImageSet(Path(kIconDir, "bomb.svg"));
        cats.push_back(c);
    }

    return cats;
}

const std::vector<Category>& categoryStorage() {
    static std::vector<Category> cats = makeDefaultCategories();
    return cats;
}

} // namespace

const std::vector<Category>& getAllCategories() {
    return categoryStorage();
}

const Category* getCategoryById(CategoryId id) {
    const auto& cats = categoryStorage();
    for (const auto& c : cats) {
        if (c.id == id) {
            return &c;
        }
    }
    return nullptr;
}

} // namespace os

