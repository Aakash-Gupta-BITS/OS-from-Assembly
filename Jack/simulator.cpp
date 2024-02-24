module jack.simulator;


// Make sure we are using 2's complement representation.
// https://stackoverflow.com/questions/64842669/how-to-test-if-a-target-has-twos-complement-integers-with-the-c-preprocessor
#if (-1 & 3) == 1
static_assert(false, "The system encoding is sign-and-magnitude. This program only compiles on two's complement system.");
#elif (-1 & 3) == 2
static_assert(false, "The system encoding is one’s complement. This program only compiles on two's complement system.");
#elif (-1 & 3) != 3
static_assert(false, "The system encoding is not possible in C standard. This program only compiles on two's complement system.");
#endif

import jack.utils;
import std;

using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;

const uint16_t NOP = 0b1111'1111'1111'1111;
const uint16_t TERMINATION_PC_ADDRESS = 0b1111'1111'1111'1111;

namespace jack::simulator
{
	enum class InstructionType : uint8_t
	{
		A,
		C,
		ERROR
	};

    [[nodiscard]]
	constexpr static InstructionType get_instruction_type(uint16_t ins)
	{
		bool A_instruction = !(ins >> 15);
		bool C_instruction = ((ins >> 13) == 7);

		if (A_instruction == C_instruction)
			return InstructionType::ERROR;

		return A_instruction ? InstructionType::A : InstructionType::C;
	}

    [[nodiscard]]
    constexpr static auto ALU_a_0(SimulatorConfig& config, uint8_t c) -> int16_t
    {
        auto& A = config.A;
        auto& D = config.D;

        switch (c)
        {
        case 0b101010:
            return 0;

        case 0b111111:
            return 1;

        case 0b111010:
            return -1;

        case 0b001100:
            return D;

        case 0b110000:
            return A;

        case 0b001101:
            return (int16_t)~D;

        case 0b110001:
            return (int16_t)~A;

        case 0b001111:
            return (int16_t)-D;

        case 0b110011:
            return (int16_t)-A;

        case 0b011111:
            return (int16_t)(D + 1);

        case 0b110111:
            return (int16_t)(A + 1);

        case 0b001110:
            return (int16_t)(D - 1);

        case 0b110010:
            return (int16_t)(A - 1);

        case 0b000010:
            return (int16_t)(D + A);

        case 0b010011:
            return (int16_t)(D - A);

        case 0b000111:
            return (int16_t)(A - D);

        case 0b000000:
            return (int16_t)(A & D);

        case 0b010101:
            return (int16_t)(D | A);

        default:
            throw std::runtime_error(std::format("Invalid instruction passed for comp bits: 0b0{:06b}\n", c));
        }
    }

    [[nodiscard]]
    constexpr static auto ALU_a_1(SimulatorConfig& config, uint8_t c)
    {
        auto& M = config.ram[config.A];
        auto& D = config.D;

        switch (c)
        {
        case 0b110000:
            return (int16_t)M;

        case 0b110001:
            return (int16_t)~M;

        case 0b110011:
            return (int16_t)-M;

        case 0b110111:
            return (int16_t)(M + 1);

        case 0b110010:
            return (int16_t)(M - 1);

        case 0b000010:
            return (int16_t)(D + M);

        case 0b010011:
            return (int16_t)(D - M);

        case 0b000111:
            return (int16_t)(M - D);

        case 0b000000:
            return (int16_t)(M & D);

        case 0b010101:
            return (int16_t)(D | M);

        default:
            throw std::runtime_error(std::format("Invalid instruction passed for comp bits: 0b1{:06b}\n",
                c));
        }
    }

    [[nodiscard]]
    constexpr static bool should_jump(uint8_t j, int16_t alu_out)
    {
        switch (j & 0b111)
        {
        case 0b000:
            return false;

        case 0b001:
            return alu_out > 0;

        case 0b010:
            return alu_out == 0;

        case 0b011:
            return alu_out >= 0;

        case 0b100:
            return alu_out < 0;

        case 0b101:
            return alu_out != 0;

        case 0b110:
            return alu_out <= 0;

        case 0b111:
            return true;
        }

        throw std::runtime_error("Switch should have covered all jump cases.");
    }

	constexpr static void cpu_one_step(SimulatorConfig& config)
	{
		auto& [D, A, PC, ram, instructions, debug_loc] = config;

		uint16_t ins = instructions[PC];
		while (ins == NOP)
			ins = instructions[PC];

		auto ins_type = get_instruction_type(ins);

		if (ins_type == InstructionType::A)
		{
			A = std::bit_cast<int16_t>(ins);
			PC = PC + 1;
            return;
		}

		if (ins_type != InstructionType::C)
			throw std::runtime_error(std::format("Invalid instruction type encountered: 0x{:04X}", ins));

		uint8_t j = ins & 07;
		uint8_t d = (ins & 070) >> 3;
		uint8_t c = (ins & 07700) >> 6;
		uint8_t a = (ins & 010000) >> 12;

        // get value
        int16_t alu_out = (a == 0 ? ALU_a_0(config, c) : ALU_a_1(config, c));

        // get jump
        if (should_jump(j, alu_out))
            PC = A;	// PC = PC + 1 will be executed later
        else
            PC += 1;

        // get destination
        if (d & 0b001)
            ram[A] = alu_out;
        if (d & 0b010)
            D = alu_out;
        if (d & 0b100)
            A = alu_out;
	}

	constexpr void simulate(SimulatorConfig& config)
	{
		auto&[D, A, PC, ram, instructions, debug_stream] = config;

        if (debug_stream.has_value())
        {
            debug_stream.value() << "________________________________________________________________________________\n";
            debug_stream.value() << std::format("|{:<23}|{:^13}|{:^13}|{:^13}|{:^13}|\n", "Instruction (Executed)", "Register PC", "Register A", "Register D", "Memory[A]");
            debug_stream.value() << "|-----------------------|-------------|-------------|-------------|-------------|\n";
        }

		while (config.PC != TERMINATION_PC_ADDRESS)
		{
            if (debug_stream.has_value())
            {
                if (A == std::bit_cast<int16_t>(TERMINATION_PC_ADDRESS))
                    debug_stream.value() << std::format("|{:016b}{:<7}|{:^13}|{:^13}|{:^13}|{:^13}|\n", instructions[PC], "", PC, A, D, "-");
                else
                    debug_stream.value() << std::format("|{:016b}{:<7}|{:^13}|{:^13}|{:^13}|{:^13}|\n", instructions[PC], "", PC, A, D, ram[A]);
            }

			cpu_one_step(config);
		}

        if (debug_stream.has_value())
            debug_stream.value() << "|_______________________|_____________|_____________|_____________|_____________|\n";
	}
}