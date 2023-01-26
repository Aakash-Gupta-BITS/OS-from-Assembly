#include <iostream>
#include <bitset>
#include "Lexer.h"

using namespace std;

enum class InstructionType
{
    A_INSTRUCTION,
    C_INSTRUCTION,
    L_INSTRUCTION
};

struct ASTNode
{
    Token* token = nullptr;
    InstructionType type;
    std::vector<ASTNode*> children;
    ASTNode* sibling = nullptr;
};

class Parser
{
    Buffer* buffer;
    Token* last_token;

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

public:
    Parser(Buffer* buffer) : buffer{ std::move(buffer) }
    {
        this->last_token = nullptr;
    }

};

int main()
{
    loadDFA();
    Buffer* buffer = new Buffer("demo_program.asm");
    
    delete buffer;
}