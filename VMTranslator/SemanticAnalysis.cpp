#include "SemanticAnalysis.h"
#include <sstream>

using namespace std;

bool checkStack(ASTNode* node, string& err)
{
    stringstream sstr{node->children[1]->token->lexeme};
    int x;
    sstr >> x;

    bool invalid = false;
    if (node->children[0]->token->type == TokenType::TK_POINTER)
    {
        invalid = (x < 0 || x > 1);
        err = invalid ? "Pointer segment only support integer with values 0 and 1" : "";
    }
    else if (node->children[0]->token->type == TokenType::TK_TEMP)
    {
        invalid = (x < 0 || x > 7);
        err = invalid ? "Temp segment only support integer with values in range [0, 7]" : "";
    }
    else if (node->children[0]->token->type == TokenType::TK_CONSTANT)
    {
        invalid = node->token->type == TokenType::TK_POP;
        err = invalid ? "POP Operation is not allowed with constant segment" : "";
    }

    return !invalid;
}

vector<pair<int, string>> getErrorList(ASTNode* root)
{
    assert(root != nullptr);
    vector<pair<int, string>> errors;

    for (auto line = root->children[0]; line; line = line->sibling)
    {
        string err;
        if (line->token->type == TokenType::TK_POP || line->token->type == TokenType::TK_PUSH)
        {
            bool res = checkStack(line, err);
            if (!res)
                errors.push_back({line->token->line_number, err});
        }
    }

    return errors;
}