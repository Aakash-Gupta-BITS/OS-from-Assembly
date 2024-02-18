export module jack:simulator;
import std;
import helpers;

namespace fs = std::filesystem;
const std::size_t RAM_SIZE = 0x4000;
const std::size_t SCREEN_SIZE = 0X2000;
const std::size_t INSTRUCTION_COUNT = 0x8000;

namespace jack::simulator
{
	export struct SimulatorConfig
	{
		std::int16_t D{};
		std::int16_t A{};
		std::uint16_t PC{};
		std::array<std::int16_t, RAM_SIZE + SCREEN_SIZE + 1> ram{};
		std::array<std::uint16_t, INSTRUCTION_COUNT> instructions{};
		std::optional<constexpr_ostream> debug_stream{};
	};

	export constexpr void simulate(SimulatorConfig& config);
}