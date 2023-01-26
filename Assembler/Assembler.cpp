#include <iostream>
#include "Lexer.h"

using namespace std;

class Parser
{
    Buffer* buffer;
    Token* last_token;

public:
    Parser(Buffer* buffer) : buffer{ std::move(buffer) }
    {
        this->last_token = nullptr;
    }

    vector<Token*> get_next_line()
    {
        if (last_token == nullptr)
            last_token = getNextToken(*buffer);

        if (last_token->type == TokenType::TK_EOF)
            return { last_token };

        vector<Token*> tokens{ last_token };
        auto new_token = getNextToken(*buffer);
        for (; new_token->line_number == last_token->line_number; new_token = getNextToken(*buffer))
            tokens.push_back(new_token);

        last_token = new_token;
        return tokens;
    }
};

int main()
{
    loadDFA();
    Buffer* buffer = new Buffer("demo_program.asm");
    Parser p(buffer);

    while (true)
    {
        auto tokens = p.get_next_line();
        if (tokens[0]->type == TokenType::TK_EOF)
            break;

        cout << "Line: " << tokens[0]->line_number << ": " << endl;
        for (auto& x : tokens)
        {
            cout << "\t" << *x << endl;
            delete x;
        }
    }

    delete buffer;
}