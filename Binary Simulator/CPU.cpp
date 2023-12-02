#include <cstdint>
#include <ostream>
#include <iostream>
#include <bitset>
#include <fstream>
#include <string>
#include <bit>
#include <array>
#include <functional>

using namespace std;

// Make sure we are using 2's complement representation.
// https://stackoverflow.com/questions/64842669/how-to-test-if-a-target-has-twos-complement-integers-with-the-c-preprocessor
#if (-1 & 3) == 1
    static_assert(false, "The encoding is sign-and-magnitude. This will compile on 2's complement.");
#elif (-1 & 3) == 2
    static_assert(false, "The encoding is one’s complement. This will compile on 2's complement.");
#elif (-1 & 3) == 3
    // 2's complement system
#else
    static_assert(false, "The encoding is not possible in C standard. This will compile on 2's complement.");
#endif

#define DEBUG_MODE  // Define DEBUG_MODE to enable debug features
#ifdef DEBUG_MODE
#define CONSTEXPR
#define DEBUG cerr
#else
#define CONSTEXPR constexpr
#define DEBUG ;//
#endif

const uint16_t INSTRUCTION_COUNT = 1 << 15;	// number of instructions
const uint16_t DATA_COUNT = 1 << 15;		// number of fields in data memory
const uint16_t VALID_DATA_LIMIT = 0x6000;

enum class InstructionType : uint8_t
{
	A,
	C,
	ERROR
};

class Hardware
{
private:
	// Registers
	int16_t D {};
	int16_t A {};
	uint16_t PC {};

	[[nodiscard]] static constexpr InstructionType get_instruction_type(uint16_t ins)
	{
		bool A_instruction = !(ins >> 15);
		bool D_instruction = ((ins >> 13) == 7);

		if (A_instruction == D_instruction)
			return InstructionType::ERROR;

		if (A_instruction)
			return InstructionType::A;

		return InstructionType::C;
	}

	[[nodiscard]] constexpr int16_t ALU_a_eq_0(uint8_t c) const
	{
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
            std::exit(-1);
		}
	}

    [[nodiscard]] constexpr int16_t ALU_a_neq_0(uint8_t c) const
	{
        auto mem_location = bit_cast<uint16_t>(A);
		const auto& M = data[mem_location];
		switch (c)
		{
		case 0b110000:
			return M;

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
            std::exit(-1);
		}
	}

	[[nodiscard]] static constexpr bool should_jump(uint8_t j, int16_t alu_out)
	{
		switch (j)
		{
		case 0b000:
			return false;
		case 0b001:
			return alu_out > 0;
		case 0b010:
			return alu_out == 0;
		case 0b011:
			return alu_out >= 0;
		case 0b0100:
			return alu_out < 0;
		case 0b0101:
			return alu_out != 0;
		case 0b110:
			return alu_out <= 0;
		case 0b111:
            return true;
        default:
            std::exit(-1);
		}
	}

	constexpr void compute(uint16_t ins)
	{
		uint8_t j = ins & 07;
		uint8_t d = (ins & 070) >> 3;
		uint8_t c = (ins & 07700) >> 6;
		uint8_t a = (ins & 010000) >> 12;

		// get value
		int16_t alu_out = (a == 0 ? ALU_a_eq_0(c) : ALU_a_neq_0(c));

		// get jump
		if (should_jump(j, alu_out))
			PC = A - 1;	// PC = PC + 1 will be executed later

		// get destination
		if (d & 0b001)
			data[A] = alu_out;
		if (d & 0b010)
			D = alu_out;
		if (d & 0b100)
			A = alu_out;
	}

public:
    // Memory elements
    array<uint16_t, INSTRUCTION_COUNT> instructions {};
    array<int16_t, DATA_COUNT> data {};

    CONSTEXPR void execute_next()
	{
		if (is_finished())
		{
			DEBUG << "end" << endl;
			return;
		}

		// fetch the instruction
		auto ins = instructions[PC];

		DEBUG << bitset<16>(ins) << "\t";

		// Decode the instruction
		switch (get_instruction_type(ins))
		{
		case InstructionType::A:
			A = bit_cast<int16_t>(ins);
			break;

		case InstructionType::C:
			if (ins != 0xffff)
			    compute(ins);
			break;

		default:
			std::exit(-1);	// Invalid Instruction
		}

		// Update PC
		PC = PC + 1;
		if (ins == 0xffff)
			DEBUG << "nop" << endl;
		else
			DEBUG << A << "\t" << D << "\t" << data[A] << "\t" << PC << endl;
	}

	[[nodiscard]] constexpr bool is_finished() const
	{
		return this->PC == bit_cast<uint16_t>((int16_t)-1);
	}

	void print_data(ostream& out)
	{
		out << "A:  " << A << endl;
		out << "D:  " << D << endl;
		out << "PC: " << PC << endl;
		for (int i = 0; i <= VALID_DATA_LIMIT; ++i)
			out << i << "\t" << bitset<16>(data[i]) << "\t(" << data[i] << ")" << endl;
	}
};

void load_file_to_memory(char* path, const function<void(size_t, uint16_t)>& f)
{
	ifstream file{ path };

    if (!file)
    {
        cerr << "Unable to open file: " << path << endl;
        std::exit(-1);
    }

	for (size_t i = 0; ; ++i)
	{
		string s;
		std::getline(file >> std::ws, s);

		if (file.eof()) break;

        uint16_t val = 0;
		for (char c : s)
		{
			if (c != '0' && c != '1')
				continue;

            val <<= 1;
            val |= (c - '0');
		}
        f(i, val);
	}
}

int main(int argc, char** argv)
{
	// format: ./simulator.out instruction_file_loc memory_file_loc memory_dump_loc
	// cerr: where debug output will be shown

	if (argc != 3 && argc != 4)
	{
		cerr << "format: ./simulator.out instruction_file_loc memory_dump_loc [memory_input_file_loc]" << endl;
		std::exit(-1);
	}

    Hardware hd{};
	load_file_to_memory(argv[1], [&](size_t index, uint16_t val) {
        hd.instructions[index] = val;
    });

    if (argc == 4)
        load_file_to_memory(argv[3], [&](size_t index, uint16_t val) {
            hd.data[index] = bit_cast<int16_t>(val);
        });

	// simulate
	while (!hd.is_finished())
		hd.execute_next();

	// to print "end"
	hd.execute_next();

	cerr << "Flushing output to a dump file..." << endl;

	// dump
	ofstream output{ argv[argc - 1] };
	hd.print_data(output);
	output.close();

	cerr << "Flushing output done..." << endl;
	return 0;
}