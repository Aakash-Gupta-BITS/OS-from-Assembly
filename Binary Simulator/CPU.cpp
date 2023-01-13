#include <cstdint>
#include <ostream>
#include <iostream>
#include <cassert>
#include <bitset>
#include <fstream>
#include <string>

using namespace std;

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
	// Memory elements
	uint16_t *instructions;
	int16_t *data;

	// Registers
	int16_t D = 0;
	int16_t A = 0;
	uint16_t PC = 0;

	InstructionType get_instruction_type(const uint16_t& ins) const
	{
		bool A_instruction = !(ins >> 15);
		bool D_instruction = ((ins >> 13) == 7);

		if (A_instruction == D_instruction)
			return InstructionType::ERROR;

		if (A_instruction)
			return InstructionType::A;

		return InstructionType::C;
	}

	int16_t ALU_a_eq_0(const uint8_t& c)
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

	int16_t ALU_a_neq_0(const uint8_t& c)
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

	void compute(const uint16_t& ins)
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
	Hardware(const uint16_t *instructions, const int16_t *data)
	{
		this->instructions = new uint16_t[INSTRUCTION_COUNT];
		this->data = new int16_t[DATA_COUNT];

		for (int i = 0; i < INSTRUCTION_COUNT; ++i)
			this->instructions[i] = instructions[i];

		for (int i = 0; i < DATA_COUNT; ++i)
			this->data[i] = data[i];
	}

	void reset()
	{
		PC = 0;
		cerr << "reset" << endl;
		cerr << "Instruction     \tA\tD\tMem[A]\tPC" << endl;
	}

	void execute_next()
	{
		if (is_finished())
		{
			cerr << "end" << endl;
			return;
		}

		// fetch the instruction
		auto ins = instructions[PC];

		cerr << bitset<16>(ins) << "\t";

		// Decode the instruction
		switch (get_instruction_type(ins))
		{
		case InstructionType::A:
			A = ins;
			break;

		case InstructionType::C:
			compute(ins);
			break;

		default:
			assert(0);	// Invalid Instruction
		}

		// Update PC
		PC = PC + 1;
		cerr << A << "\t" << D << "\t" << data[A] << "\t" << PC << endl;
	}

	inline bool is_finished()
	{
		return this->PC == uint16_t(-1);
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

void load_file_to_memory(char* path, void* dest, int len)
{
	uint16_t* target = (uint16_t*)dest;

	ifstream file{ path };
	for (int i = 0; i < len; ++i)
	{
		string s;
		std::getline(file >> std::ws, s);

		if (file.eof()) break;

		target[i] = 0;
		for (char c : s)
		{
			if (c != '0' && c != '1')
				continue;

			target[i] <<= 1;
			target[i] |= (c - '0');
		}
	}

	file.close();
}

int main(int argc, char** argv)
{
	// format: ./simulator.out instruction_file_loc memory_file_loc memory_dump_loc
	// cerr: where debug output will be shown

	uint16_t *instructions = new uint16_t[INSTRUCTION_COUNT];
	int16_t *data = new int16_t[DATA_COUNT];

	// load
	load_file_to_memory(argv[1], instructions, INSTRUCTION_COUNT);
	load_file_to_memory(argv[2], data, DATA_COUNT);

	// simulate
	Hardware hd(instructions, data);
	hd.reset();
	while (!hd.is_finished())
		hd.execute_next();
	hd.execute_next();

	// dump
	ofstream output{ argv[3] };
	hd.print_data(output);
	output.close();

	// unallocate
	delete[] instructions;
	delete[] data;
	return 0;
}