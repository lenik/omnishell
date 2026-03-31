#ifndef OMNISHELL_CORE_CATEGORY_HPP
#define OMNISHELL_CORE_CATEGORY_HPP

#include <bas/ui/arch/ImageSet.hpp>

#include <string>
#include <vector>

namespace os {

// Logical application categories for the shell UI
enum CategoryId {
    ID_CATEGORY_NONE = 0,
    ID_CATEGORY_SYSTEM = 1,
    ID_CATEGORY_ACCESSORIES = 2,
    ID_CATEGORY_GAME = 3,
};

struct Category {
    CategoryId id;
    std::string name;        // Internal name, e.g. "system"
    std::string label;       // Display label, e.g. "System & Settings"
    std::string description; // Human-friendly description
    ImageSet icon;           // Optional icon representing this category
};

// Get all known categories (stable order)
const std::vector<Category>& getAllCategories();

// Lookup a category by its id, nullptr if not found
const Category* getCategoryById(CategoryId id);

} // namespace os

#endif // OMNISHELL_CORE_CATEGORY_HPP

