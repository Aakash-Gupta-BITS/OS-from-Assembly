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
            type_segment == TokenType::TK_THAT ? "ARG" :
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
    out << "  D = M" << endl;
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

void writeAssembly(ASTNode* node, const string& source_file_name, std::ostream& out)
{
    assert(node);

    for (auto line = node->children[0]; line; line = line->sibling)
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
            default:
                cerr << "Line: " << line->token->line_number << ": Not handled in assembler yet!!" << endl;
                exit(-1);
        }
    }
}