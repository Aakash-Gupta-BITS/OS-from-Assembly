#include <cstdint>
#include <ostream>
#include <iostream>
#include <cassert>
#include <bitset>
using namespace std;

const uint16_t INSTRUCTION_COUNT = 1 << 15;	// number of instructions
const uint16_t DATA_COUNT = 1 << 15;		// number of fields in data memory

enum class InstructionType : uint8_t
{
	A,
	C,
	ERROR
};

enum class Destination : uint8_t
{
	NA = 0,
	M = 1,
	D = 2,
	MD = 3,
	A = 4,
	AM = 5,
	AD = 6,
	AMD = 7
};

class Hardware
{
private:
	// Memory elements
	uint16_t instructions[INSTRUCTION_COUNT];
	int16_t data[DATA_COUNT];

	// Registers
	int16_t D;
	int16_t A;
	int16_t PC;

	InstructionType get_instruction_type(const uint16_t& ins) const
	{
		bool A_instruction = ~(ins >> 15);
		bool D_instruction = ((ins >> 13) == 7);

		if (A_instruction == D_instruction)
			return InstructionType::ERROR;

		if (A_instruction)
			return InstructionType::A;

		return InstructionType::C;
	}

	void execute_A(const uint16_t& ins)
	{
		A = ins;
	}

	int16_t execute_C_a_eq0(const uint8_t& c)
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
			return ~D;

		case 0b110001:
			return ~A;

		case 0b001111:
			return -D;

		case 0b110011:
			return -A;

		case 0b011111:
			return D + 1;

		case 0b110111:
			return A + 1;

		case 0b001110:
			return D - 1;

		case 0b110010:
			return A - 1;

		case 0b000010:
			return D + A;

		case 0b010011:
			return D - A;

		case 0b000111:
			return A - D;

		case 0b000000:
			return A & D;

		case 0b010101:
			return D | A;
		}
		assert(0);
	}

	int16_t execute_C_a_neq0(const uint8_t& c)
	{
		const int16_t M = data[A];
		switch (c)
		{
		case 0b110000:
			return M;

		case 0b110001:
			return ~M;

		case 0b110011:
			return -M;

		case 0b110111:
			return M + 1;

		case 0b110010:
			return M - 1;

		case 0b000010:
			return D + M;

		case 0b010011:
			return D - M;

		case 0b000111:
			return M - D;

		case 0b000000:
			return M & D;

		case 0b010101:
			return D | M;
		}
		assert(0);
	}

	bool should_jump(const uint8_t& j, const int16_t& alu_out)
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
		}

		// this should not be executed
		assert(0);
	}

	void execute_C(const uint16_t& ins)
	{
		uint8_t j = ins & 07;
		uint8_t d = (ins & 070) >> 3;
		uint8_t c = (ins & 07700) >> 6;
		uint8_t a = (ins & 010000) >> 12;

		// get value
		int16_t alu_out = (a == 0 ? execute_C_a_eq0(c) : execute_C_a_neq0(c));

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
	Hardware() 
	{

	}

	void reset()
	{
		PC = D = A = 0;
	}

	void execute_next()
	{
		// fetch the instruction
		auto ins = instructions[PC];

		// Decode the instruction
		switch (get_instruction_type(ins))
		{
		case InstructionType::A:
			execute_A(ins);
			break;

		case InstructionType::C:
			execute_C(ins);
			break;

		default:
			assert(0);	// Invalid Instruction
		}

		// Update PC
		PC = PC + 1;
	}
};

int main(int argc, char** argv)
{
	// format: ./simulator.out instruction_file_loc memory_file_loc
	// debug output will be printed on cerr
}