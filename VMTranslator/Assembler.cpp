#include "Assembler.h"
#include <iostream>
#include <format>
#include <string>
#include <sstream>
#include <string_view>
using namespace std;

void stack_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    out << endl;
    out << "  // " << node->token->lexeme << " " << node->children[0]->token->lexeme << " " << node->children[1]->token->lexeme << endl;
    auto type_stack = node->token->type;
    auto type_segment = node->children[0]->token->type;

    if (type_segment == TokenType::TK_CONSTANT)
    {
        out << "  @" << node->children[1]->token->lexeme << "  // D = " << node->children[1]->token->lexeme << endl;
        out << "  D = A" << endl;
        out << "  @SP  // RAM[SP] = D" << endl;
        out << "  A = M" << endl;
        out << "  M = D" << endl;
        out << "  @SP  // SP++" << endl;
        out << "  M = M + 1" << endl;
        return;
    }

    if (type_segment == TokenType::TK_POINTER)
    {
        cerr << "Line: " << node->token->line_number << ": Not handled in assembler yet!!" << endl;
        exit(-1);
    }

    string target_seg_name =
            type_segment == TokenType::TK_LOCAL ? "LCL" :
            type_segment == TokenType::TK_ARGUMENT ? "ARG" :
            type_segment == TokenType::TK_THAT ? "THAT" :
            type_segment == TokenType::TK_THIS ? "THIS" :
            type_segment == TokenType::TK_TEMP ? "R" :
            type_segment == TokenType::TK_STATIC ? (source_file_name + "." + node->children[1]->token->lexeme) :
            "";

    assert(!target_seg_name.empty());

    if (type_segment == TokenType::TK_TEMP)
    {
        stringstream sstr{node->children[1]->token->lexeme};
        int x;
        sstr >> x;
        target_seg_name += to_string(x + 5);
    }

    out << "  @" << target_seg_name << "  // addr <- " << target_seg_name << " + " << node->children[1]->token->lexeme << endl;
    out << "  D = " << ((type_segment == TokenType::TK_STATIC || type_segment == TokenType::TK_TEMP) ? "A" : "M") << endl;
    out << "  @" << node->children[1]->token->lexeme << endl;
    if (type_stack == TokenType::TK_POP)
    {
        out << "  D = D + A" << endl;
        out << "  @R15" << endl;
        out << "  M = D" << endl;
        out << "  @SP  // SP--" << endl;
        out << "  M = M - 1" << endl;
        out << "  A = M  // RAM[addr] <- RAM[SP]" << endl;
        out << "  D = M" << endl;
        out << "  @R15" << endl;
        out << "  A = M" << endl;
        out << "  M = D" << endl;
    }
    else
    {
        out << "  A = D + A" << endl;
        out << "  D = M" << endl;
        out << "  @SP  // SP++" << endl;
        out << "  M = M + 1" << endl;
        out << "  A = M  // RAM[addr] <- RAM[SP - 1]" << endl;
        out << "  A = A - 1" << endl;
        out << "  M = D" << endl;
    }
}

void arithmetic_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    auto type = node->token->type;
    out << endl << " // " << node->token->lexeme << endl;

    out << "  @SP" << endl;
    if (type != TokenType::TK_NEG)
    {
        out << "  AM = M - 1" << endl;
        out << "  D = M" << endl;
        out << "  A = A - 1" << endl;

        if (type == TokenType::TK_ADD)
            out << "  M = D + M" << endl;
        else if (type == TokenType::TK_SUB)
            out << "  M = M - D" << endl;
        else
            assert(false);
    }
    else
    {
        out << "  A = M - 1" << endl;
        out << "  M = -M" << endl;
    }
}

void logical_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    static int counter = 1;
    auto type = node->token->type;
    out << endl << " // " << node->token->lexeme << endl;

    out << "  @SP  // Load values" << endl;
    out << "  AM = M - 1" << endl;
    out << "  D = M" << endl;
    out << "  A = A - 1" << endl;
    out << "  D = D - M" << endl;
    out << "  @ALU_COMPARE_" << counter << "_ELSE  // Store in D" << endl;
    out << "  D;" << (type == TokenType::TK_EQ ? "JNE" : type == TokenType::TK_LT ? "JLE" : "JGE") << endl;
    out << "  D = 1" << endl;
    out << "  @ALU_COMPARE_" << counter << "_FINISH" << endl;
    out << "  0; JMP" << endl;
    out << "(ALU_COMPARE_" << counter << "_ELSE)" << endl;
    out << "  D = 0" << endl;
    out << "(ALU_COMPARE_" << counter << "_FINISH)" << endl;
    out << "  @SP // Store in memory" << endl;
    out << "  A = M - 1" << endl;
    out << "  M = D" << endl;
}

void boolean_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    auto type = node->token->type;
    out << endl << " // " << node->token->lexeme << endl;

    out << "  @SP" << endl;
    if (type != TokenType::TK_NOT)
    {
        out << "  AM = M - 1" << endl;
        out << "  D = M" << endl;
        out << "  A = A - 1" << endl;
        if (type == TokenType::TK_AND)
            out << "  M = D & M" << endl;
        else if (type == TokenType::TK_OR)
            out << "  M = D | M" << endl;
        else
            assert(false);
    }
    else
    {
        out << "  A = M - 1" << endl;
        out << "  D = 1" << endl;
        out << "  M = D - M" << endl;
    }
}

void branch_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    auto type = node->token->type;
    out << endl << " // " << node->token->lexeme;
    if (node->token->type == TokenType::TK_IF)
        out << "-goto";
    out << " " << node->children[0]->token->lexeme << endl;

    if (node->token->type == TokenType::TK_LABEL)
        out << "(" << node->children[0]->token->lexeme << ")" << endl;
    else if (node->token->type == TokenType::TK_GOTO)
        out << "  @" << node->children[0]->token->lexeme << endl << "  0; JMP" << endl;
    else if (node->token->type == TokenType::TK_IF)
    {
        out << "  @SP" << endl;
        out << "  AM = M - 1" << endl;
        out << "  D = M" << endl;
        out << "  @" << node->children[0]->token->lexeme << endl;
        out << "  D; JNE" << endl;
    }
    else
        assert(false);
}

void call_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    static int counter = 1;
    out << endl << "  // call " << node->children[0]->token->lexeme << " " << node->children[1]->token->lexeme << endl;

    auto target = node->children[0]->token->lexeme;
    string ret_label = "FUNC_CALL_" + target + "_" + to_string(counter++) + "_RET";
    vector<string> push_targets = { ret_label, "LCL", "ARG", "THIS", "THAT" };

    // Assumptions
    // 1. n args are already available in stack, n >= 0
    // 2. Every function will return exactly one value. if there are no args, we explictly have to push for one space
    // 3. return will work as expected.

    stringstream sstr {node->children[1]->token->lexeme};
    int nArgs;
    sstr >> nArgs;

    // If no arguments are mentioned, we need to make space for one return value
    if (nArgs == 0)
    {
        nArgs = 1;
        // simulate push constant 0
        ASTNode* push_node = new ASTNode{0, -1, new Token{TokenType::TK_PUSH, "push"}, {
                new ASTNode{0, -1, new Token(TokenType::TK_CONSTANT, "constant"), {}, nullptr},
                new ASTNode{0, -1, new Token(TokenType::TK_NUM, "0"), {}, nullptr}
        }, nullptr};
        stack_writer(push_node, source_file_name, out);
        delete push_node->token;
        delete push_node->children[0]->token;
        delete push_node->children[1]->token;
        delete push_node->children[0];
        delete push_node->children[1];
        delete push_node;
    }

    for (auto &x: push_targets)
    {
        out << "  @" << x << "  // push " << x << endl;
        out << (x == ret_label ? "  D = A" : "  D = M") << endl;
        out << "  @SP" << endl;
        out << "  AM = M + 1" << endl;
        out << "  A = A - 1" << endl;
        out << "  M = D" << endl;
    }

    out << "  @SP  // LCL = SP" << endl;
    out << "  D = M" << endl;
    out << "  @LCL" << endl;
    out << "  M = D" << endl;

    out << "  @" << nArgs + 5 << "  // ARG = SP - 5 - nArgs" << endl;
    out << "  D = D - A" << endl;
    out << "  @ARG" << endl;
    out << "  M = D" << endl;

    out << "  @" << target << " // JUMP to function" << endl;
    out << "  0; JMP" << endl;
    out << "(" << ret_label << ")" << endl;
}

void function_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    out << endl << "  // function " << node->children[0]->token->lexeme << " " << node->children[1]->token->lexeme << endl;
    out << "(" << node->children[0]->token->lexeme << ")" << endl;
}

void return_writer(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    out << endl << "  // return" << endl;
    out << "  @SP  // MEM[ARG] <- MEM[SP - 1]" << endl;
    out << "  A = M - 1" << endl;
    out << "  D = M" << endl;
    out << "  @ARG" << endl;
    out << "  A = M" << endl;
    out << "  M = D" << endl;
    out << "  D = A + 1  // SP = ARG + 1" << endl;
    out << "  @SP" << endl;
    out << "  M = D" << endl;

    // store return address in r15
    out << "  @LCL  // R15 <- return address" << endl;
    out << "  D = M" << endl;
    out << "  @5" << endl;
    out << "  A = D - A" << endl;
    out << "  D = M" << endl;
    out << "  @R15" << endl;
    out << "  M = D" << endl;

    vector<string> pop_targets = {"THAT", "THIS", "ARG", "LCL"};
    for (auto &target: pop_targets)
    {
        // load from lcl-1, lcl--, return popped value to target
        out << "  @LCL  // pop " << target << " using LCL" << endl;
        out << "  AM = M - 1" << endl;
        out << "  D = M" << endl;
        out << "  @" << target << endl;
        out << "  M = D" << endl;
    }

    // jump to return address
    out << "  @R15  // jump to return address" << endl;
    out << "  A = M" << endl;
    out << "  0;JMP" << endl;
}

void writeAssembly(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    assert(node);
    out << "  // BOOTSTRAP AREA" << endl;
    out << "  // SP = 256" << endl;
    out << "  @256" << endl;
    out << "  D = A" << endl;
    out << "  @SP" << endl;
    out << "  M = D" << endl << endl;
    out << "  // Call Sys.init" << endl;
    out << "  @Sys.init" << endl;
    out << "  0; JMP" << endl << endl;
    out << "(Sys.init)" << endl;
    out << "  // LCL = ARG = SP" << endl;
    out << "  @SP" << endl;
    out << "  D = A" << endl;
    out << "  @LCL" << endl;
    out << "  M = D" << endl;

    out << endl << "  // USER CODE" << endl;
    ASTNode* main_call_node = new ASTNode{0, -1, nullptr, {
            new ASTNode{0, -1, new Token(TokenType::UNINITIALISED, "Main.main"), {}, nullptr},
            new ASTNode{0, -1, new Token(TokenType::UNINITIALISED, "0"), {}, nullptr}
        }, nullptr};
    call_writer(main_call_node, source_file_name, out);
    delete main_call_node->children[0]->token;
    delete main_call_node->children[1]->token;
    delete main_call_node->children[0];
    delete main_call_node->children[1];
    delete main_call_node;
    main_call_node = nullptr;
    out << "  A = -1" << endl;
    out << "  0; JMP" << endl;

    for (auto func = node->children[0]; func; func = func->sibling)
    {
        function_writer(func, source_file_name, out);
        for (auto line = func->children[2]; line; line = line->sibling)
        {
            switch (line->token->type) {
                case TokenType::TK_PUSH:
                case TokenType::TK_POP:
                    stack_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_ADD:
                case TokenType::TK_SUB:
                case TokenType::TK_NEG:
                    arithmetic_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_LT:
                case TokenType::TK_EQ:
                case TokenType::TK_GT:
                    logical_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_AND:
                case TokenType::TK_OR:
                case TokenType::TK_NOT:
                    boolean_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_IF:
                case TokenType::TK_GOTO:
                case TokenType::TK_LABEL:
                    branch_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_CALL:
                    call_writer(line, source_file_name, out);
                    break;
                case TokenType::TK_RETURN:
                    return_writer(line, source_file_name, out);
                    break;
                default:
                    cerr << "Line: " << line->token->line_number << ": Not handled in assembler yet!!" << endl;
                    exit(-1);
            }
        }
    }
}