#include <iostream>
#include "Lexer.h"

using namespace std;

int main()
{
    loadDFA();
    Buffer bufer("demo_program.asm");

    Token* t;
    while ((t = getNextToken(bufer))->type != TokenType::TK_EOF)
    {
        cout << *t << endl;
    }

    std::cout << "Hello World!\n";
}