#include <iostream>
#include <fstream>
#include "AST.h"

using namespace std;
char* LexerLoc;
char* GrammarLoc;

void printParseTree(std::ostream& out, const ParseTreeNode* node)
{
    out << *node << endl;
    for (auto& x : node->children)
        printParseTree(out, x);
}

void printAST(std::ostream& out, ASTNode* node, int tab = 0)
{
    if (node == nullptr)
        return;

    for (int i = 0; i < tab; ++i)
        out << '\t';
    out << *node << endl;

    for (auto& child : node->children)
        printAST(out, child, tab + 1);

    printAST(out, node->sibling, tab);
}

void cleanParseTree(ParseTreeNode* node)
{
    delete node->token;

    for (auto& x : node->children)
        cleanParseTree(x);

    delete node;
}

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        cerr << "Invalid number of arguments!" << endl;
        cerr << "Usage: ./compiler.out <DFA file> <Grammar file> <input_file_location> <output_bytecode_location>" << endl;
        exit(-1);
    }

    LexerLoc = argv[1];
    GrammarLoc = argv[2];
    loadDFA();
    loadParser();
    Buffer buffer(argv[3]);

    bool error = false;
    auto parseNode = parseInputSourceCode(buffer, error);
    if (error)
    {
        cerr << "ERROR WHILE PROCESSING INPUT FILE! EXITING!"<<endl;
        exit(-1);
    }

    // AST
    auto node = createAST(parseNode);
    printAST(cerr, node);

    cerr << endl;
}