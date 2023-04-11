#pragma once
#include "Buffer.h"
#include <fstream>
#include <set>
#include <string>
#include <map>
#include <vector>

extern char* LexerLoc;

enum class TokenType
{
    TK_PUSH,
    TK_POP,
    TK_LOCAL,
    TK_ARGUMENT,
    TK_STATIC,
    TK_CONSTANT,
    TK_THIS,
    TK_THAT,
    TK_POINTER,
    TK_TEMP,
    TK_NUM,
    TK_SYMBOL,
    TK_ADD,
    TK_SUB,
    TK_MINUS,
    TK_NEG,
    TK_EQ,
    TK_GT,
    TK_LT,
    TK_AND,
    TK_OR,
    TK_NOT,
    TK_LABEL,
    TK_GOTO,
    TK_IF,
    TK_FUNCTION,
    TK_CALL,
    TK_RETURN,
    TK_NEWLINE,
    TK_WHITESPACE,
    TK_EOF,
    TK_ERROR_SYMBOL,
    TK_ERROR_PATTERN,
    TK_ERROR_LENGTH,
	UNINITIALISED
};

struct Token
{
	TokenType type = TokenType::UNINITIALISED;
	std::string lexeme;
	int line_number = 0;
	int start_index = 0;
	int length = 0;

	friend std::ostream& operator<<(std::ostream&, const Token&);
};

struct DFA
{
	int num_tokens;
	int num_states;
	int num_transitions;
	int num_finalStates;
	int num_keywords;

	std::vector<std::vector<int>> productions;
	std::vector<TokenType> finalStates;
	std::vector<std::string> tokenType2tokenStr;
	std::map<std::string, TokenType> tokenStr2tokenType;
	std::map<std::string, TokenType> lookupTable;
	std::set<std::string> keywordTokens;

	DFA() : num_tokens{ 0 }, num_states{ 0 }, num_transitions{ 0 }, num_finalStates{ 0 }, num_keywords{ 0 }
	{

	}
};

extern DFA dfa;

void loadDFA();
Token* getNextToken(Buffer&);