#pragma once
#include "Lexer.h"
#include <bitset>

class Parser
{
    std::vector<std::vector<Token*>> tokens;
    std::vector<std::bitset<16>> binary;

    std::map<std::string, int> jmp_locations;
    std::map<std::string, int> variable_locations;

    static std::map<TokenType, unsigned short> predefined;
    static std::map<std::string, std::bitset<7>> comp_map;
    static std::map<TokenType, std::bitset<3>> jmp_map;

    int RAM_INDEX = 16;


    void initialise_maps_if_empty();
    void pass1_A(int index);
    std::string get_dest_string(const std::vector<Token*>& line, int eq_index, const std::string& err_msg);
    std::string get_comp_string(const std::vector<Token*>& line, int comp_from, int comp_to, const std::string& err_msg);
    std::string get_jump_string(const std::vector<Token*>& line, int semi_index, const std::string& err_msg);
    void pass1_C(int index);
    void pass1_L(int index);
    void pass2_A(int index);
    void debug_output(int index);

public:
    Parser(Buffer& buffer);
    const std::vector<std::bitset<16>>& convert_to_binary();
    void print_symbol_table() const;
    ~Parser();
};