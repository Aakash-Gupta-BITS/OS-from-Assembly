cmake_minimum_required(VERSION 3.1)
project(VMTranslator)

add_executable(Translator.out "VMTranslator/Translator.cpp" "VMTranslator/AST.cpp" "VMTranslator/Lexer.cpp" "VMTranslator/Parser.cpp" VMTranslator/SemanticAnalysis.cpp VMTranslator/SemanticAnalysis.h VMTranslator/Assembler.cpp VMTranslator/Assembler.h)
target_compile_features(Translator.out PRIVATE cxx_std_17)