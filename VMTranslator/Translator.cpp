#include <iostream>
#include <fstream>
#include "Assembler.h"

using namespace std;
char* LexerLoc;
char* GrammarLoc;

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
        cerr << "Usage: ./translator.out <DFA file> <Grammar file> <input_vm_file_location> <output_assembly_location>" << endl;
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
    printParseTree(cerr, parseNode);
    cerr << endl;

    // AST Phase
    cerr << "AST PHASE" << endl;
    auto astNode = createAST(parseNode);
    cleanParseTree(parseNode);
    printAST(cerr,astNode);
    cerr << endl;

    // Type Checking
    cerr << "TYPE CHECKING PHASE" << endl;
    auto lst = getErrorList(astNode);
    if (lst.empty())
        cerr << "No errors found in type checking!" << endl;
    for (auto &x: lst)
        cerr << "Line number " << x.first << ": " << x.second << endl;

    if (!lst.empty())
        exit(-1);
    return 0;
    // Assembly Generation
    ofstream output_file{ argv[4] };

    if (!output_file)
    {
        cerr << "Error opening output file for binary!" << endl;
        exit(-1);
    }
    cerr << endl;
    cerr << "Writing assembly output" << endl;
    output_file << "@10\n"
                   "D = A\n"
                   "@LCL\n"
                   "M = D\n"
                   "\n"
                   "@256\n"
                   "D = A\n"
                   "@SP\n"
                   "M = D" << endl;
    writeAssembly(astNode, buffer.file_name, output_file);
    output_file << "A=-1\n0;JMP" << endl;
}