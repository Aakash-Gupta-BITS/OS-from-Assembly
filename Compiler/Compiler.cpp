#include <iostream>
#include <fstream>
#include "SymbolTable.h"

using namespace std;
char* LexerLoc;
char* GrammarLoc;

void printParseTree(std::ostream& out, const ParseTreeNode* node)
{
    out << *node << endl;
    for (auto& x : node->children)
        printParseTree(out, x);
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
        cerr << "ERROR WHILE PROCESSING INPUT FILE! EXITING!" << endl;
        exit(-1);
    }

    // AST
    GlobalTable::getInstance()->add_class(parseNode);
    GlobalTable::getInstance()->check_names();
    cleanParseTree(parseNode);

    cerr << endl;
}