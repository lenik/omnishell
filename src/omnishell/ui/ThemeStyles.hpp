#ifndef THEME_STYLES_HPP
#define THEME_STYLES_HPP

#include <bas/ui/arch/ImageSet.hpp>

namespace ThemeStyles {

constexpr std::string_view hi_micro = "heroicons/micro";
constexpr std::string_view hi_mini = "heroicons/mini";
constexpr std::string_view hi_normal = "heroicons/normal";
constexpr std::string_view hi_solid = "heroicons/solid";

constexpr std::string_view slv_block_arrows = "streamline-vectors/block/arrows";
constexpr std::string_view slv_block_commerce = "streamline-vectors/block/commerce";
constexpr std::string_view slv_block = "streamline-vectors/block";
constexpr std::string_view slv_block_leisure = "streamline-vectors/block/leisure";
constexpr std::string_view slv_block_technology = "streamline-vectors/block/technology";
constexpr std::string_view slv_block_wellness = "streamline-vectors/block/wellness";
constexpr std::string_view slv_core_duo = "streamline-vectors/core/duo";
constexpr std::string_view slv_core_flat = "streamline-vectors/core/flat";
constexpr std::string_view slv_core_gradient = "streamline-vectors/core/gradient";
constexpr std::string_view slv_core_line = "streamline-vectors/core/line";
constexpr std::string_view slv_core_neon = "streamline-vectors/core/neon";
constexpr std::string_view slv_core_pop = "streamline-vectors/core/pop";
constexpr std::string_view slv_core_remix = "streamline-vectors/core/remix";
constexpr std::string_view slv_core_solid = "streamline-vectors/core/solid";
constexpr std::string_view slv_covid = "streamline-vectors/covid";
constexpr std::string_view slv_cyber_duotone = "streamline-vectors/cyber/duotone";
constexpr std::string_view slv_cyber_line = "streamline-vectors/cyber/line";
constexpr std::string_view slv_flex_duo = "streamline-vectors/flex/duo";
constexpr std::string_view slv_flex_flat = "streamline-vectors/flex/flat";
constexpr std::string_view slv_flex_gradient = "streamline-vectors/flex/gradient";
constexpr std::string_view slv_flex_line = "streamline-vectors/flex/line";
constexpr std::string_view slv_flex_neon = "streamline-vectors/flex/neon";
constexpr std::string_view slv_flex_pop = "streamline-vectors/flex/pop";
constexpr std::string_view slv_flex_remix = "streamline-vectors/flex/remix";
constexpr std::string_view slv_flex_solid = "streamline-vectors/flex/solid";
constexpr std::string_view slv_freebies_freemojis = "streamline-vectors/freebies-freemojis";
constexpr std::string_view slv_freehand_duotone = "streamline-vectors/freehand/duotone";
constexpr std::string_view slv_freehand_line = "streamline-vectors/freehand/line";
constexpr std::string_view slv_guidance = "streamline-vectors/guidance";
constexpr std::string_view slv_kameleon_colors = "streamline-vectors/kameleon/colors";
constexpr std::string_view slv_kameleon_duo = "streamline-vectors/kameleon/duo";
constexpr std::string_view slv_kameleon_pop = "streamline-vectors/kameleon/pop";
constexpr std::string_view slv_logos_block = "streamline-vectors/logos/block";
constexpr std::string_view slv_logos_line = "streamline-vectors/logos/line";
constexpr std::string_view slv_logos_solid = "streamline-vectors/logos/solid";
constexpr std::string_view slv_memes = "streamline-vectors/memes";
constexpr std::string_view slv_pixel = "streamline-vectors/pixel";
constexpr std::string_view slv_plump_duo = "streamline-vectors/plump/duo";
constexpr std::string_view slv_plump_flat = "streamline-vectors/plump/flat";
constexpr std::string_view slv_plump_gradient = "streamline-vectors/plump/gradient";
constexpr std::string_view slv_plump_line = "streamline-vectors/plump/line";
constexpr std::string_view slv_plump_neon = "streamline-vectors/plump/neon";
constexpr std::string_view slv_plump_pop = "streamline-vectors/plump/pop";
constexpr std::string_view slv_plump_remix = "streamline-vectors/plump/remix";
constexpr std::string_view slv_plump_solid = "streamline-vectors/plump/solid";
constexpr std::string_view slv_sharp_duo = "streamline-vectors/sharp/duo";
constexpr std::string_view slv_sharp_flat = "streamline-vectors/sharp/flat";
constexpr std::string_view slv_sharp_gradient = "streamline-vectors/sharp/gradient";
constexpr std::string_view slv_sharp_line = "streamline-vectors/sharp/line";
constexpr std::string_view slv_sharp_neon = "streamline-vectors/sharp/neon";
constexpr std::string_view slv_sharp_pop = "streamline-vectors/sharp/pop";
constexpr std::string_view slv_sharp_remix = "streamline-vectors/sharp/remix";
constexpr std::string_view slv_sharp_solid = "streamline-vectors/sharp/solid";
constexpr std::string_view slv_stickies_colors = "streamline-vectors/stickies/colors";
constexpr std::string_view slv_stickies_duo = "streamline-vectors/stickies/duo";
constexpr std::string_view slv_ultimate_bold = "streamline-vectors/ultimate/bold";
constexpr std::string_view slv_ultimate_colors = "streamline-vectors/ultimate/colors";
constexpr std::string_view slv_ultimate_regular = "streamline-vectors/ultimate/regular";

// inline std::string slv(std::string_view tail) {
//     return std::string(slv_core_pop) + "/" + std::string(tail);
// }

// inline std::string slv_if0(std::string_view tail) {
//     return std::string(slv_core_pop) + "/interface-essential/" + std::string(tail);
// }

// inline std::string if0(std::string_view tail) {
//     return std::string("interface-essential") + "/" + std::string(tail);
// }

}; // namespace ThemeStyles

#endif // THEME_STYLES_HPP