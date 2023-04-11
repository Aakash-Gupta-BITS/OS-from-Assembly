#include <iostream>
#include <fstream>
#include "Parser.h"

using namespace std;
char* LexerLoc;

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        cerr << "Invalid number of arguments!" << endl;
        cerr << "Usage: ./assembler.out <DFA file> <input_assembly_location> <output_file_location>" << endl;
        exit(-1);
    }

    LexerLoc = argv[1];
    loadDFA();

    Buffer buffer(argv[2]);

    Parser p(buffer);
    auto b = p.convert_to_binary();
    ofstream output_file{ argv[3] };
    
    if (!output_file)
    {
        cerr << "Error opening output file for binary!" << endl;
        exit(-1);
    }

    for (auto& x : b)
        output_file << x << endl;

    p.print_symbol_table();
}