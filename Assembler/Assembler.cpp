#include <iostream>
#include <bitset>
#include <map>
#include <string>
#include <sstream>
#include "Lexer.h"

using namespace std;

class Parser
{
    vector<vector<Token*>> tokens;
    vector<bitset<16>> binary;
    map<string, int> jump_locations;
    map<string, int> variables;


    static map<TokenType, unsigned short> predefined;
    static map<string, bitset<7>> comp_map;
    static map<TokenType, bitset<3>> jmp_map;

    int RAM_INDEX = 16;

    map<string, int> jmp_locations;
    map<string, int> variable_locations;

    void initialise_maps_if_empty()
    {
        if (predefined.size() == 0)
        {
            predefined[TokenType::TK_SP] = 0x0000;
            predefined[TokenType::TK_LCL] = 0x0001;
            predefined[TokenType::TK_ARG] = 0x0002;
            predefined[TokenType::TK_THIS] = 0x0003;
            predefined[TokenType::TK_THAT] = 0x0004;
            predefined[TokenType::TK_SCREEN] = 0x4000;
            predefined[TokenType::TK_KBD] = 0x6000;
        }

        if (jmp_map.size() == 0)
        {
            jmp_map[TokenType::TK_JGT] = 0b001;
            jmp_map[TokenType::TK_JEQ] = 0b010;
            jmp_map[TokenType::TK_JGE] = 0b011;
            jmp_map[TokenType::TK_JLT] = 0b100;
            jmp_map[TokenType::TK_JNE] = 0b101;
            jmp_map[TokenType::TK_JLE] = 0b110;
            jmp_map[TokenType::TK_JMP] = 0b111;
        }

        if (comp_map.size() == 0)
        {
            comp_map["0"] = 0b0'101010;
            comp_map["1"] = 0b0'111111;
            comp_map["-1"] = 0b0'111010;
            comp_map["D"] = 0b0'001100;
            comp_map["A"] = 0b0'110000;
            comp_map["!D"] = 0b0'001101;
            comp_map["!A"] = 0b0'110001;
            comp_map["-D"] = 0b0'001111;
            comp_map["-A"] = 0b0'110011;
            comp_map["D+1"] = 0b0'011111;
            comp_map["A+1"] = 0b0'110111;
            comp_map["D-1"] = 0b0'001110;
            comp_map["A-1"] = 0b0'110010;
            comp_map["D+A"] = 0b0'000010;
            comp_map["D-A"] = 0b0'010011;
            comp_map["A-D"] = 0b0'000111;
            comp_map["D&A"] = 0b0'000000;
            comp_map["D|A"] = 0b0'010101;


            comp_map["M"] = 0b1'110000;
            comp_map["!M"] = 0b1'110001;
            comp_map["-M"] = 0b1'110011;
            comp_map["M+1"] = 0b1'110111;
            comp_map["M-1"] = 0b1'110010;
            comp_map["D+M"] = 0b1'000010;
            comp_map["D-M"] = 0b1'010011;
            comp_map["M-D"] = 0b1'000111;
            comp_map["D&M"] = 0b1'000000;
            comp_map["D|M"] = 0b1'010101;
        }

    }

    void pass1_A(int index)
    {
        initialise_maps_if_empty();
        const vector<Token*>& line = tokens[index];
        if (line.size() != 2)
        {
            string code;
            for (auto& x : line)
                code += x->lexeme + " ";
            cerr << "Error in line having code: " << code << endl;
            exit(-1);
        }

        if (line[1]->type == TokenType::TK_NUM)
        {
            unsigned short num;
            stringstream sstr{ line[1]->lexeme };
            sstr >> num;
            binary[index] = num;
            binary[index][15] = 0;
        }
        else if (line[1]->type == TokenType::TK_SYMBOL)
            binary[index] = 0x0000;                     // note this - in pass 2, we will update this
        else if (predefined.find(line[1]->type) != predefined.end())
            binary[index] = predefined[line[1]->type];
        else if (line[1]->type == TokenType::TK_REG)
        {
            string s = line[1]->lexeme.substr(1);
            stringstream sstr{ s };
            int x;
            sstr >> x;
            binary[index] = x;
        }
        else
        {
            // this should never occur
            cerr << "Undefined case!" << endl;
            exit(-1);
        }

        // make sure the last bit is unset
        assert(!binary[index].test(15));
    }

    string get_dest_string(const vector<Token*>& line, int eq_index, const string& err_msg)
    {
        if (eq_index == -1)
            return "000";

        if (eq_index != 1)
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        if (line[0]->type != TokenType::TK_M &&
            line[0]->type != TokenType::TK_A &&
            line[0]->type != TokenType::TK_D &&
            line[0]->type != TokenType::TK_AM &&
            line[0]->type != TokenType::TK_MD &&
            line[0]->type != TokenType::TK_AD &&
            line[0]->type != TokenType::TK_AMD)
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        string dest_string = "000";
        if (line[0]->lexeme.find('M') != string::npos)
            dest_string[2] = '1';
        if (line[0]->lexeme.find('D') != string::npos)
            dest_string[1] = '1';
        if (line[0]->lexeme.find('A') != string::npos)
            dest_string[0] = '1';

        return dest_string;
    }

    string get_comp_string(const vector<Token*>& line, int comp_from, int comp_to, const string& err_msg)
    {
        string comp_string;
        for (int i = comp_from; i <= comp_to; ++i)
            comp_string += line[i]->lexeme;

        if (comp_map.find(comp_string) == comp_map.end())
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        return comp_map[comp_string].to_string();
    }

    string get_jump_string(const vector<Token*>& line, int semi_index, const string& err_msg)
    {
        if (semi_index == -1)
            return "000";

        if (semi_index != line.size() - 2)
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        if (line.back()->type != TokenType::TK_JGT &&
            line.back()->type != TokenType::TK_JEQ &&
            line.back()->type != TokenType::TK_JGE &&
            line.back()->type != TokenType::TK_JLT &&
            line.back()->type != TokenType::TK_JNE &&
            line.back()->type != TokenType::TK_JLE &&
            line.back()->type != TokenType::TK_JMP)
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        return jmp_map[line.back()->type].to_string();
    }

    void pass1_C(int index)
    {
        initialise_maps_if_empty();
        const vector<Token*>& line = tokens[index];

        // get separator indices
        int eq_index = -1;
        int semi_index = -1;
        for (int i = 0; i < line.size(); ++i)
            if (line[i]->type == TokenType::TK_ASSIGN)
                eq_index = i;
            else if (line[i]->type == TokenType::TK_SEMICOLON)
                semi_index = i;

        // prepare error msg to display
        string err_msg = "Error in line " + to_string(index + 1) + " having code : ";
        for (auto& x : line)
            err_msg += x->lexeme + " ";

        auto dest_string = get_dest_string(line, eq_index, err_msg);
        auto comp_string = get_comp_string(line, eq_index + 1, semi_index == -1 ? line.size() - 1 : semi_index - 1, err_msg);
        auto jmp_string = get_jump_string(line, semi_index, err_msg);

        binary[index] = bitset<16>("111" + comp_string + dest_string + jmp_string);
    }

    void pass1_L(int index)
    {
        // (..) - should be unique
        const vector<Token*>& line = tokens[index];

        string err_msg = "Error in line " + to_string(index + 1) + " having code : ";
        for (auto& x : line)
            err_msg += x->lexeme + " ";

        if (line.size() != 3 ||
            line[0]->type != TokenType::TK_OB ||
            line[1]->type != TokenType::TK_SYMBOL || 
            line[2]->type != TokenType::TK_CB)
        {
            cerr << err_msg << endl;
            exit(-1);
        }

        // it is guaranteed that the symbol is not one of predefined language symbols - check fot TK_SYMBOL

        if (jmp_locations.find(line[1]->lexeme) != jmp_locations.end())
        {
            cerr << "Symbol '" << line[1]->lexeme << "' is defined earlier!" << endl;
            exit(-1);
        }

        jmp_locations[line[1]->lexeme] = index;
    }

    void pass2_A(int index)
    {
        const vector<Token*>& line = tokens[index];
        if (line[1]->type != TokenType::TK_SYMBOL)
            return;

        bool is_jmp_location = (jmp_locations.find(line[1]->lexeme) != jmp_locations.end());
        if (is_jmp_location)
        {
            binary[index] = jmp_locations[line[1]->lexeme];
            assert(!binary[index].test(15));
            return;
        }

        if (variable_locations.find(line[1]->lexeme) == variable_locations.end())
            variable_locations[line[1]->lexeme] = RAM_INDEX++;

        binary[index] = variable_locations[line[1]->lexeme];
        assert(!binary[index].test(15));
    }

    void debug_output(int index)
    {
        cerr << "Line: " << tokens[index][0]->line_number << ":" << endl;
        for (auto& x : tokens[index])
            cerr << "\t" << *x << endl;
    }

public:
    Parser(Buffer &buffer)
    {
        auto token = getNextToken(buffer);

        if (token->type == TokenType::TK_ERROR_SYMBOL || token->type == TokenType::TK_ERROR_PATTERN || token->type == TokenType::TK_ERROR_LENGTH)
        {
            cerr << token << endl;
            exit(-1);
        }

        int delta = token->line_number;
        while (delta--)
            tokens.push_back({});

        tokens.back().push_back(token);

        while (token->type != TokenType::TK_EOF)
        {
            token = getNextToken(buffer);
            int delta = token->line_number - tokens.back().back()->line_number;
            while (delta--)
                tokens.push_back({});
            tokens.back().push_back(token);
        }
        tokens.back().pop_back();
        if (tokens.back().size() == 0)
            tokens.pop_back();

        binary = vector<bitset<16>>();
    }

    const vector<bitset<16>>& convert_to_binary()
    {
        if (binary.size() > 0)
            return binary;

        for (int i = 0; i < tokens.size(); ++i)
        {
            binary.push_back(bitset<16>(65535));
            if (tokens[i].size() == 0)
                continue;

            if (tokens[i][0]->type == TokenType::TK_AT)
                pass1_A(i);
            else if (tokens[i][0]->type == TokenType::TK_OB)
                pass1_L(i);
            else
                pass1_C(i);
        }

        for (int i = 0; i < tokens.size(); ++i)
        {
            if (tokens[i].size() == 0)
                continue;

            if (tokens[i][0]->type == TokenType::TK_AT)
                pass2_A(i);
        }

        return binary;
    }

    ~Parser()
    {
        for (auto& x : tokens)
            for (auto& y : x)
                delete y;
    }
};

map<TokenType, unsigned short> Parser::predefined;
map<string, bitset<7>> Parser::comp_map;
map<TokenType, bitset<3>> Parser::jmp_map;

int main()
{
    loadDFA();
    Buffer buffer("demo_program.asm");
    Parser p(buffer);
    auto b = p.convert_to_binary();
    for (auto& x : b)
        cout << x << endl;
}