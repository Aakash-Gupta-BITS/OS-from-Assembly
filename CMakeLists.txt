cmake_minimum_required(VERSION 3.1)
project(VMTranslator)

add_executable(Compiler.out Compiler/Compiler.cpp Compiler/Lexer.cpp Compiler/Parser.cpp Compiler/SymbolTable.cpp
)
add_executable(VMTranslator.out "VMTranslator/Translator.cpp" "VMTranslator/AST.cpp" "VMTranslator/Lexer.cpp" "VMTranslator/Parser.cpp" VMTranslator/SemanticAnalysis.cpp VMTranslator/Assembler.cpp Compiler/SymbolTable.h
)
add_executable(Assembler.out "Assembler/Assembler.cpp" "Assembler/Lexer.cpp" "Assembler/Parser.cpp"
)
add_executable(CPU.out "BinarySimulator/CPU.cpp"
)
target_compile_features(Compiler.out PRIVATE cxx_std_20)
target_compile_features(VMTranslator.out PRIVATE cxx_std_20)
target_compile_features(Assembler.out PRIVATE cxx_std_20)
target_compile_features(CPU.out PRIVATE cxx_std_20)
