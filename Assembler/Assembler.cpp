#include <iostream>
#include <bitset>
#include "Lexer.h"

using namespace std;

enum class InstructionType
{
    NA,
    A_INSTRUCTION,
    C_INSTRUCTION,
    L_INSTRUCTION
};

enum class ASTNodeType
{
    NA,
    A_INSTRUCTION,
    C_INSTRUCTION,
    L_INSTRUCTION,
    VALUE,
    OPERATOR
};

struct ASTNode
{
    Token* token = nullptr;
    ASTNodeType type = ASTNodeType::NA;
    std::vector<ASTNode*> children;
};

class Parser
{
    Buffer* buffer;
    Token* last_token;
    vector<Token*> current_tokens;
    InstructionType current_type;

    void deduce_current_type()
    {
        // not correct for all cases
        if (current_tokens[0]->type == TokenType::TK_AT)
            current_type = InstructionType::A_INSTRUCTION;
        else if (current_tokens[0]->type == TokenType::TK_OB)
            current_type = InstructionType::L_INSTRUCTION;
        else
            current_type = InstructionType::C_INSTRUCTION;
    }

    void move_next()
    {
        if (last_token == nullptr)
            last_token = getNextToken(*buffer);

        if (last_token->type == TokenType::TK_EOF)
            return;

        current_tokens = { last_token };
        auto new_token = getNextToken(*buffer);
        for (; new_token->line_number == last_token->line_number; new_token = getNextToken(*buffer))
            current_tokens.push_back(new_token);

        last_token = new_token;
        deduce_current_type();
    }

    bool has_next()
    {
        return last_token == nullptr || last_token->type != TokenType::TK_EOF;
    }

    ASTNode* make_A_Instruction() const
    {
        return new ASTNode
        {
            current_tokens[0],
            ASTNodeType::A_INSTRUCTION,
            {
                new ASTNode
                {
                    current_tokens[1],
                    ASTNodeType::VALUE
                }
            }
        };
    }

    ASTNode* make_L_Instruction() const
    {
        return new ASTNode
        {
            nullptr,
            ASTNodeType::L_INSTRUCTION,
            {
                new ASTNode
                {
                    current_tokens[1],
                    ASTNodeType::VALUE
                }
            }
        };
    }

    ASTNode* make_C_Instruction() const
    {
        int eq_index = -1, colon_index = -1;
        for (int i = 0; i < current_tokens.size(); ++i)
            if (current_tokens[i]->type == TokenType::TK_ASSIGN)
                eq_index = i;
            else if (current_tokens[i]->type == TokenType::TK_SEMICOLON)
                colon_index = i;

        ASTNode* dest, *comp, *jump;
        dest = comp = jump = nullptr;

        if (eq_index != -1)
        {
            dest = new ASTNode;
            dest->token = current_tokens[0];
            dest->type = ASTNodeType::VALUE;
        }

        if (colon_index != -1)
        {
            jump = new ASTNode;
            jump->token = current_tokens[colon_index + 1];
            jump->type = ASTNodeType::VALUE;
        }

        int from = eq_index == -1 ? 0 : eq_index + 1;
        int to = colon_index == -1 ? current_tokens.size() - 1 : colon_index - 1;
        string comp_string = "";
        for (int i = from; i <= to; ++i)
            comp_string += current_tokens[i]->lexeme;
        comp = new ASTNode;
        comp->token = new Token
        {
            TokenType::UNINITIALISED,
            comp_string,
            current_tokens[from]->line_number,
            current_tokens[from]->start_index,
            current_tokens[to]->start_index + current_tokens[to]->length - current_tokens[from]->start_index
        };

        return new ASTNode
        {
            nullptr,
            ASTNodeType::C_INSTRUCTION,
            {
                dest,
                comp,
                jump
            }
        };
    }

public:
    Parser(Buffer* buffer) : buffer{ buffer }
    {
        this->last_token = nullptr;
        current_type = InstructionType::NA;
    }

    ASTNode* getAST(bool& hasError)
    {
        ASTNode* root = new ASTNode;

        while (has_next())
        {
            move_next();

            switch (current_type)
            {
            case InstructionType::NA:
                cerr << "Invalid instruction found! Token info for the line is provided below: " << endl;
                for (auto& x : current_tokens)
                {
                    cout << *x << endl;
                    delete x;
                }

                hasError = true;
                return root;

            case InstructionType::A_INSTRUCTION:
                root->children.push_back(make_A_Instruction());
                break;

            case InstructionType::C_INSTRUCTION:
                root->children.push_back(make_C_Instruction());
                break;
            
            case InstructionType::L_INSTRUCTION:
                root->children.push_back(make_L_Instruction());
                break;
            
            default:
                assert(0);
            }
        }

        return root;
    }

    ~Parser()
    {
        if (last_token != nullptr)
            delete last_token;
    }
};


int main()
{
    loadDFA();
    Buffer* buffer = new Buffer("demo_program.asm");
    Parser p(buffer);
    bool has_error;

    auto ast = p.getAST(has_error);

    delete buffer;
}