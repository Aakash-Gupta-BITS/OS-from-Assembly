#pragma once
#include "Buffer.h"
#include <fstream>
#include <set>
#include <string>
#include <map>
#include <utility>
#include <vector>

extern char* LexerLoc;

enum class TokenType
{
    TK_COMMENT,
    TK_CLASS,
    TK_CONSTRUCTOR,
    TK_FUNCTION,
    TK_METHOD,
    TK_FIELD,
    TK_STATIC,
    TK_VAR,
    TK_INT,
    TK_CHAR,
    TK_BOOLEAN,
    TK_VOID,
    TK_TRUE,
    TK_FALSE,
    TK_NULL,
    TK_THIS,
    TK_LET,
    TK_DO,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_RETURN,
    TK_CURO,
    TK_CURC,
    TK_PARENO,
    TK_PARENC,
    TK_BRACKO,
    TK_BRACKC,
    TK_DOT,
    TK_COMMA,
    TK_SEMICOLON,
    TK_PLUS,
    TK_MINUS,
    TK_MULT,
    TK_DIV,
    TK_AND,
    TK_OR,
    TK_LE,
    TK_GE,
    TK_EQ,
    TK_NOT,
    TK_NUM,
    TK_STR,
    TK_IDENTIFIER,
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

    Token(TokenType type = TokenType::UNINITIALISED, std::string lex = "") : type{type}, lexeme{std::move(lex)}
    {

    }
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