#include "SemanticAnalysis.h"
#include <sstream>
#include <string>
#include <map>
#include <set>
using namespace std;

void check_function_commands(ASTNode* start, vector<pair<int, string>> & errors)
{
    // Done - There should be Main.main
    // Done - Main.main accepts 0 arguments
    // Done - There should not be any function named Sys.init
    // Done - Function names should not be repeated
    // Done - Calls must match existing function names
    // Done - Calls must match exact number of arguments in the definition
    // Done - Two passes required - Forward function calls are allowed

    map<string, string> func_dict;
    for (auto func = start->children[0]; func; func = func->sibling)
    {
        string &name = func->children[0]->token->lexeme;
        if (func_dict.find(name) != func_dict.end())
        {
            errors.push_back({func->token->line_number, "Function name is repeated here: " + name});
            return;
        }

        if (name == "Main.main" && func->children[1]->token->lexeme != "0")
        {
            errors.push_back({func->token->line_number, "Number of arguments to Main.main function should be exactly 0"});
            return;
        }

        if (name == "Sys.init")
        {
            errors.push_back({func->token->line_number, "Sys.init function name is not allowed in the code"});
            return;
        }

        func_dict[name] = func->children[1]->token->lexeme;
    }

    if (func_dict.find("Main.main") == func_dict.end())
    {
        errors.push_back({-1, "No function found with name 'Main.main' in the VM!"});
        return;
    }

    // Check for call statement matches
    for (auto func = start->children[0]; func; func = func->sibling)
    {
        for (auto line = func->children[2]; line; line = line->sibling)
        {
            if (line->token->type != TokenType::TK_CALL)
                continue;

            auto &name = line->children[0]->token->lexeme;
            auto &count = line->children[1]->token->lexeme;

            if (func_dict.find(name) == func_dict.end())
                errors.push_back({line->token->line_number, "Function '" + name + "' is called but it is not defined anywhere"});
            else if (func_dict[name] != count)
                errors.push_back({line->token->line_number, "Function '" + name + "' accepts " + func_dict[name] + " arguments. " + count + " supplied"});
        }
    }
}

void check_branch_commands(ASTNode* func, vector<pair<int, string>>& errors)
{
    // Done - Check if label names are not repeated inside a local function
    // Done - Check if calls to jump are made to existing labels
    // Done - JUMP Label should not start with ["ALU_COMPARE_"]
    // Done - Two pass required
    set<string> label_names;
    vector<string> reserved_prefixes {"ALU_COMPARE_"};
    for (auto line = func->children[2]; line; line = line->sibling)
    {
        if (line->token->type != TokenType::TK_LABEL)
            continue;

        auto &name = line->children[0]->token->lexeme;
        if (label_names.find(name) != label_names.end())
            errors.push_back({line->token->line_number, "Label " + name + " is repeated inside the scope of function " + func->children[0]->token->lexeme});
        else
            label_names.insert(name);

        for (auto &reserved: reserved_prefixes)
            if (name.substr(0, reserved.size()) == reserved)
                errors.push_back({line->token->line_number, "Label " + name + " starts with reserved prefix " + reserved});
    }

    for (auto line = func->children[2]; line; line = line->sibling)
    {
        if (line->token->type != TokenType::TK_GOTO && line->token->type != TokenType::TK_IF)
            continue;

        auto &target = line->children[0]->token->lexeme;
        if (label_names.find(target) == label_names.end())
            errors.push_back({line->token->line_number, "Label " + target + " is not found inside local scope of function " + func->children[0]->token->lexeme});
    }
}

bool checkPushPop(ASTNode* node, string& err)
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

    // Step 1: Check function names match
    check_function_commands(root, errors);

    // Step 2: Check all other commands are okay
    for (auto func = root->children[0]; func; func = func->sibling)
    {
        // Step 2.1: Check branch commands are local to function
        check_branch_commands(func, errors);

        // Step 2.2: Check normal operation commands are semantically valid (static analysis)
        for (auto line = func->children[2]; line; line = line->sibling)
        {
            string err;
            if (line->token->type == TokenType::TK_POP || line->token->type == TokenType::TK_PUSH)
            {
                bool res = checkPushPop(line, err);
                if (!res)
                    errors.push_back({line->token->line_number, err});
            }
        }
    }

    return errors;
}