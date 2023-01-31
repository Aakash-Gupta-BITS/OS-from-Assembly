#include <iostream>
#include <bitset>
#include <map>
#include <sstream>
#include "Lexer.h"

using namespace std;

class Parser
{
    vector<vector<Token*>> tokens;
    vector<bitset<16>> binary;
    map<string, int> jump_locations;
    map<string, int> variables;

    void pass1_A(int index)
    {
        static map<TokenType, unsigned short> predefined;
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

        vector<Token*> line = tokens[index];
        if (line.size() != 2)
        {
            string code;
            for (auto& x : line)
                code += x->lexeme + " ";
            cerr << "Error in line having code: " << code << endl;
            exit(-1);
        }

        binary.push_back(bitset<16>());

        if (line[1]->type == TokenType::TK_NUM)
        {
            unsigned short num;
            stringstream sstr{ line[1]->lexeme };
            sstr >> num;
            binary.back() = num;
            binary.back()[15] = 0;
        }
        else if (line[1]->type == TokenType::TK_SYMBOL)
        {
            cerr << "Not handled yet!" << endl;
            exit(-1);
        }
        else if (predefined.find(line[1]->type) != predefined.end())
            binary.back() = predefined[line[1]->type];
        else if (line[1]->type == TokenType::TK_REG)
        {
            string s = line[1]->lexeme.substr(1);
            stringstream sstr{ s };
            int x;
            sstr >> binary.back();
        }
        else
        {
            // this should never occur
            cerr << "Undefined case!" << endl;
            exit(-1);
        }

        // make sure the last bit is unset
        assert(!binary.back().test(15));
    }

    void pass1_C(int index)
    {

    }

    void pass1_L(int index)
    {

    }

    void debug_output(int index)
    {
        cerr << "Line: " << tokens[index][0]->line_number << ":" << endl;
        for (auto& x : tokens[index])
            cerr << "\t" << x << endl;
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

        tokens.push_back({ token });

        while (token->type != TokenType::TK_EOF)
        {
            token = getNextToken(buffer);
            if (token->line_number != tokens.back().back()->line_number)
                tokens.push_back({});
            tokens.back().push_back(token);
        }
        binary = vector<bitset<16>>(tokens.size());
    }

    void pass1()
    {
        for (int i = 0; i < tokens.size(); ++i)
        {
            debug_output(i);

            if (tokens[i][0]->type == TokenType::TK_AT)
                pass1_A(i);
            else if (tokens[i][0]->type == TokenType::TK_OB)
                pass1_L(i);
            else
                pass1_C(i);
        }
    }

    ~Parser()
    {
        for (auto& x : tokens)
            for (auto& y : x)
                delete y;
    }
};


int main()
{
    loadDFA();
    Buffer buffer("demo_program.asm");
    Parser p(buffer);
}