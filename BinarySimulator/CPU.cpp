#include <array>
#include <bit>
#include <bitset>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

using namespace std;

// Make sure we are using 2's complement representation.
// https://stackoverflow.com/questions/64842669/how-to-test-if-a-target-has-twos-complement-integers-with-the-c-preprocessor
#if (-1 & 3) == 1
static_assert(false, "The system encoding is sign-and-magnitude. This program only compiles on two's complement system.");
#elif (-1 & 3) == 2
static_assert(false, "The system encoding is one’s complement. This program only compiles on two's complement system.");
#elif (-1 & 3) != 3
static_assert(false, "The system encoding is not possible in C standard. This program only compiles on two's complement system.");
#endif

const uint16_t RAM_SIZE  = 0x4000;
const uint16_t SCREEN_SIZE = 0X2000;
const uint16_t DATA_COUNT = RAM_SIZE + SCREEN_SIZE + 1;
const uint16_t INSTRUCTION_COUNT = 0x8000;

const auto TERMINATION_PC_ADDRESS = bit_cast<uint16_t>((int16_t) - 1);
const auto NOP = bit_cast<uint16_t>((int16_t) - 1);

enum class InstructionType : uint8_t
{
    A,
    C,
    ERROR
};

struct DATA_MEMORY
{
    array<int16_t, RAM_SIZE> ram{};
    array<int16_t, SCREEN_SIZE> screen{};
    int16_t keyboard{};

    constexpr int16_t& operator[](uint16_t address)
    {
        if ((address & 0b1100'0000'0000'0000) == 0)
            return ram[address];

        if ((address & 0b1110'0000'0000'0000) == 0b0100'0000'0000'0000)
            return screen[address & 0b0001'1111'1111'1111];

        if (address == 24576)
            return keyboard;

        throw std::runtime_error(format("Trying to access invalid data memory location: 0x{:04X}\n",
                                        address));
    }

    constexpr const int16_t operator[](uint16_t address) const
    {
        if ((address & 0b1100'0000'0000'0000) == 0)
            return ram[address];

        if ((address & 0b1110'0000'0000'0000) == 0b0100'0000'0000'0000)
            return screen[address & 0b0001'1111'1111'1111];

        if (address == 24576)
            return keyboard;

        throw std::runtime_error(format("Trying to access invalid data memory location: 0x{:04X}\n",
                                        address));
    }
};

struct INSTRUCTION_MEMORY
{
    array<uint16_t, INSTRUCTION_COUNT> rom{};

    constexpr uint16_t& operator[](uint16_t address) noexcept
    {
        return rom.at(address);
    }

    constexpr const uint16_t& operator[](uint16_t address) const noexcept
    {
        return rom.at(address);
    }
};

struct REGISTERS
{
    int16_t D {};
    int16_t A {};
    uint16_t PC {};
};

[[nodiscard]]
constexpr InstructionType get_instruction_type(uint16_t ins)
{
    bool A_instruction = !(ins >> 15);
    bool D_instruction = ((ins >> 13) == 7);

    if (A_instruction == D_instruction)
        return InstructionType::ERROR;

    if (A_instruction)
        return InstructionType::A;

    return InstructionType::C;
}

[[nodiscard]]
constexpr int16_t ALU_a_0(REGISTERS& regs, uint8_t c)
{
    auto& A = regs.A;
    auto& D = regs.D;

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
            throw std::runtime_error(format("Invalid instruction passed for comp bits: 0b0{:06b}\n",
                                            c));
    }
}

[[nodiscard]]
constexpr int16_t ALU_a_1(REGISTERS& regs, DATA_MEMORY& dm, uint8_t c)
{
    auto& M = dm[bit_cast<uint16_t>(regs.A)];
    auto& D = regs.D;

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
            throw std::runtime_error(format("Invalid instruction passed for comp bits: 0b1{:06b}\n",
                                            c));
    }
}

[[nodiscard]]
constexpr bool should_jump(uint8_t j, int16_t alu_out)
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

struct Motherboard
{
    REGISTERS regs{};
    DATA_MEMORY dm{};
    INSTRUCTION_MEMORY im{};

    struct STATUS
    {
        const REGISTERS& registers;
        const uint16_t ins;
        const optional<int16_t> memory{};
    };

    struct sentinel {};

    struct iterator
    {
        DATA_MEMORY& dm;
        const INSTRUCTION_MEMORY& im;
        REGISTERS& regs;

        constexpr bool operator!=(sentinel) const { return regs.PC != TERMINATION_PC_ADDRESS; }
        constexpr iterator& operator++()
        {
            uint16_t ins = im[regs.PC];
            while (ins == NOP)
                ins = im[regs.PC];

            auto ins_type = get_instruction_type(ins);
            if (ins_type == InstructionType::A)
            {
                regs.A = bit_cast<int16_t>(ins);
                regs.PC = regs.PC + 1;
                return *this;
            }
            if (ins_type != InstructionType::C)
                throw runtime_error(format("Invalid instruction type encountered: 0x{:04X}", ins));


            uint8_t j = ins & 07;
            uint8_t d = (ins & 070) >> 3;
            uint8_t c = (ins & 07700) >> 6;
            uint8_t a = (ins & 010000) >> 12;

            // get value
            int16_t alu_out = (a == 0 ? ALU_a_0(regs, c) : ALU_a_1(regs, dm, c));

            // get jump
            if (should_jump(j, alu_out))
                regs.PC = regs.A;	// PC = PC + 1 will be executed later
            else
                regs.PC += 1;

            // get destination
            if (d & 0b001)
                dm[regs.A] = alu_out;
            if (d & 0b010)
                regs.D = alu_out;
            if (d & 0b100)
                regs.A = alu_out;

            return *this;
        }

        const constexpr STATUS operator*() const
        {
            if (regs.A == bit_cast<int16_t>(TERMINATION_PC_ADDRESS))
                return { regs, im[regs.PC] };

            return { regs, im[regs.PC], dm[regs.A] };
        }
    };

    constexpr iterator begin() { return iterator{ this->dm, this->im, this->regs }; }
    static constexpr sentinel end() { return {}; }
};

static_assert(sizeof(DATA_MEMORY) == (DATA_COUNT << 1));
static_assert(sizeof(INSTRUCTION_MEMORY) == (INSTRUCTION_COUNT << 1));
static_assert(sizeof(REGISTERS) == 6);
static_assert(sizeof(Motherboard) == sizeof(DATA_MEMORY) + sizeof(INSTRUCTION_MEMORY) + sizeof(REGISTERS));

struct Config
{
    string instruction_file_loc{};
    string memory_dump_loc{};
    string memory_input_loc{};

    static void load_file(string path, const function<void(size_t, uint16_t)>& f)
    {
        if (path.empty())
            return;

        ifstream file{ path };

        if (!file)
            throw runtime_error(format("Unable to open file: {}", path));

        string line;
        int i = 0;
        while (std::getline(file, line))
        {
            uint16_t val = 0;
            for (char c : line)
            {
                if (c != '0' && c != '1')
                    continue;

                val <<= 1;
                val |= (c - '0');
            }
            f(i, val);
            ++i;
        }
    }

    Config(int argc, char** argv)
    {
        if (argc < 2 || argc > 4)
        {
            cerr << "format: ./simulator.out instruction_file_loc [memory_dump_loc] [memory_input_loc]" << endl;
            std::exit(-1);
        }

        instruction_file_loc = argv[1];
        if (argc >= 3)
            memory_dump_loc = argv[2];
        if (argc == 4)
            memory_input_loc = argv[3];
    }

    void load_motherboard(Motherboard& mbd) const
    {
        load_file(memory_input_loc, [&](size_t index, uint16_t val) {
            mbd.dm[index] = bit_cast<int16_t>(val);
        });

        load_file(instruction_file_loc, [&](size_t index, uint16_t val) {
            mbd.im[index] = bit_cast<int16_t>(val);
        });
    }

    void dump_contents(Motherboard& mbd) const
    {
        if (memory_dump_loc.empty())
            return;

        // dump
        ofstream out{ memory_dump_loc };
        out << "A:  " << mbd.regs.A << endl;
        out << "D:  " << mbd.regs.D << endl;
        out << "PC: " << mbd.regs.PC << endl;
        out << "Memory Contents: " << endl;

        for (int i = 0; i < DATA_COUNT; ++i)
            if (mbd.dm[i] != 0)
                out << i << "\t" << bitset<16>(mbd.dm[i]) << "\t(" << mbd.dm[i] << ")" << endl;
    }
};

int main(int argc, char** argv)
{
    Motherboard mbd{};
    Config config(argc, argv);
    config.load_motherboard(mbd);

    std::cerr << std::format("{:<23}{:<13}{:<13}{:<13}{}\n",
                             "Instruction (Executed)", "Register PC",
                             "Register A", "Register D", "Memory[A]");

    try
    {
        for (const auto &[R, I, M] : mbd)
            std::cerr << std::format("{:<23}{:<13}{:<13}{:<13}{}\n", I, R.PC, R.A, R.D, (M.has_value() ? to_string(M.value()) : "-"));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught exception: '" << e.what() << "'\n";
        std::terminate();
    }

    std::cerr << "\nFINISHED EXECUTION" << std::endl;

	cerr << "Flushing output to a dump file." << endl;
    config.dump_contents(mbd);
	cerr << "Flushing output done." << endl;

	return 0;
}