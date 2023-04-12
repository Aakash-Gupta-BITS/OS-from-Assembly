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

ASTNode* createAST(const ParseTreeNode* input, const ParseTreeNode* parent, ASTNode* inherited)
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
        // start ==> TK_FUNCTION TK_SYMBOL TK_NUM func_start
        node->children.resize(1);
        node->children[0] = createAST(input->children[0], input);
        node->children[0]->children.resize(3);
        node->children[0]->children[0] = createAST(input->children[1], input);
        node->children[0]->children[1] = createAST(input->children[2], input);
        node->children[0]->children[2] = createAST(input->children[3], input, node->children[0]);
    }
    else if (input->productionNumber == 1)
    {
        // start ==> TK_NEWLINE start
        delete node;
        return createAST(input->children[1], input);
    }
    else if (input->productionNumber == 2)
    {
        // start ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 3)
    {
        // func_start ==> TK_NEWLINE line_start
        delete node;
        return createAST(input->children[1], input, inherited);
    }
    else if (input->productionNumber == 4)
    {
        // func_start ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 5)
    {
        // line_start ==> op line_end
        delete node;
        auto op = createAST(input->children[0], input);
        op->sibling = createAST(input->children[1], input, inherited);
        return op;
    }
    else if (input->productionNumber == 6)
    {
        // line_start ==> TK_NEWLINE line_start
        delete node;
        return createAST(input->children[1], input, inherited);
    }
    else if (input->productionNumber == 7)
    {
        // line_start ==> TK_FUNCTION TK_SYMBOL TK_NUM func_start
        auto func = createAST(input->children[0], input);
        func->children.resize(3);
        func->children[0] = createAST(input->children[1], input);
        func->children[1] = createAST(input->children[2], input);
        func->children[2] = createAST(input->children[3], input, func);
        inherited->sibling = func;
        return nullptr;
    }
    else if (input->productionNumber == 8)
    {
        // line_start ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 9)
    {
        // line_end ==> TK_EOF
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 10)
    {
        // line_end ==> TK_NEWLINE line_start
        delete node;
        return createAST(input->children[1], input, inherited);
    }
    else if (input->productionNumber >= 11 && input->productionNumber <= 14)
    {
        // op ==> segment | alu_op | branch | TK_RETURN
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 15)
    {
        // op ==> TK_CALL TK_SYMBOL TK_NUM
        node->token = copy_token(input->children[0]->token);
        node->children.resize(2);
        node->children[0] = createAST(input->children[1], input);
        node->children[1] = createAST(input->children[2], input);
    }
    else if (input->productionNumber == 16)
    {
        // segment ==> stack_op_name seg_name TK_NUM
        node->token = copy_token(input->children[0]->children[0]->token);
        node->children.resize(2);
        node->children[0] = createAST(input->children[1], input);
        node->children[1] = createAST(input->children[2], input);
    }
    else if (input->productionNumber >= 17 && input->productionNumber <= 38)
    {
        delete node;
        return createAST(input->children[0], input);
    }
    else if (input->productionNumber == 39)
    {
        // branch ==> TK_LABEL TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[1], input);
    }
    else if (input->productionNumber == 40)
    {
        // branch ==> TK_GOTO TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[1], input);
    }
    else if (input->productionNumber == 41)
    {
        // branch ==> TK_IF TK_MINUS TK_GOTO TK_SYMBOL
        node->token = copy_token(input->children[0]->token);
        node->children.resize(1);
        node->children[0] = createAST(input->children[3], input);
    }

    return node;
}