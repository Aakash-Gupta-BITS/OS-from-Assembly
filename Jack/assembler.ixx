export module jack.assembler;

import std;
import helpers;

namespace jack::assembler
{
    export constexpr auto generate_binary(std::string_view) -> std::expected<std::vector<std::uint16_t>, std::string>;
}