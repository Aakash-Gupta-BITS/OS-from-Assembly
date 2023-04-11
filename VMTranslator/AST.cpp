#include "AST.h"
#include <iostream>
#include <cassert>
using namespace std;

std::ostream& operator<<(std::ostream& out, const ASTNode& node)
{
    out <<
        "{ symbol: '" <<
        parser.symbolType2symbolStr[node.sym_index] <<
        "', lexeme: '" <<
        (node.token ? node.token->lexeme : "") <<
        "'";

    out << " }";
    return out;
}

Token* copy_token(Token* input)
{
    assert(input != nullptr);
    assert(input->length > 0);

    Token* out = new Token;
    out->type = input->type;
    out->lexeme = input->lexeme;
    out->line_number = input->line_number;
    out->start_index = input->line_number;
    out->length = input->length;
    return out;
}

ASTNode* createAST(const ParseTreeNode* input, const ParseTreeNode* parent)
{
    assert(input != nullptr);

    if (input->isLeaf)
    {
        ASTNode* node = new ASTNode;
        node->sym_index = input->symbol_index;
        node->token = copy_token(input->token);
        node->isLeaf = 1;
        return node;
    }

    ASTNode* node = new ASTNode;
    node->isLeaf = 0;
    node->sym_index = input->symbol_index;

    if (input->productionNumber == 0)
    {
        // start ==> lines
        node->children.resize(1);
        node->children[0] = createAST(input->children[0], input);
    }
    else if (input->productionNumber == 1)
    {
        // lines ==> op line_end_state
        delete node;
        auto op = createAST(input->children[0], input);
        op->sibling = createAST(input->children[1], input);
        return op;
    }
    else if (input->productionNumber == 2)
    {
        // lines ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 3)
    {
        // lines ==> TK_NEWLINE lines
        delete node;
        return createAST(input->children[1], input);
    }
    else if (input->productionNumber == 4)
    {
        // line_end_state ==> TK_NEWLINE lines
        delete node;
        return createAST(input->children[1], input);
    }
    else if (input->productionNumber == 5)
    {
        // line_end_state ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 6)
    {
        // op ==> segment
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 7)
    {
        // op ==> alu_op
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 8)
    {
        // segment ==> stack_op_name seg_name TK_NUM
        node->token = copy_token(input->children[0]->children[0]->token);
        node->children.resize(2);
        node->children[0] = createAST(input->children[1], input);
        node->children[1] = createAST(input->children[2], input);
    }
    else if (input->productionNumber >= 9 && input->productionNumber <= 30)
    {
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 31)
    {
        // op ==> branch
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 32)
    {
        // branch ==> TK_LABEL TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[1], input);
    }
    else if (input->productionNumber == 33)
    {
        // branch ==> TK_GOTO TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[1], input);
    }
    else if (input->productionNumber == 34)
    {
        // branch ==> TK_IF TK_MINUS TK_GOTO TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[3], input);
    }

    return node;
}