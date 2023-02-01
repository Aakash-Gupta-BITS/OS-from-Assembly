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
	TK_OB,
	TK_CB,
	TK_ASSIGN,
	TK_PLUS,
	TK_MINUS,
	TK_AND,
	TK_OR,
	TK_NOT,
	TK_AT,
	TK_SEMICOLON,
	TK_COMMENT,
	TK_WHITESPACE,
	TK_NUM,
	TK_SYMBOL,
	TK_M,
	TK_D,
	TK_MD,
	TK_A,
	TK_AM,
	TK_AD,
	TK_AMD,
	TK_JGT,
	TK_JEQ,
	TK_JGE,
	TK_JLT,
	TK_JNE,
	TK_JLE,
	TK_JMP,
	TK_SP,
	TK_LCL,
	TK_ARG,
	TK_THIS,
	TK_THAT,
	TK_REG,
	TK_SCREEN,
	TK_KBD,
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