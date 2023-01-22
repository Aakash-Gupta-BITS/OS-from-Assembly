#pragma once
#include "Buffer.h"
#include <fstream>
#include <set>
#include <string>
#include <map>
#include <vector>

extern const char* LexerLoc;

enum class TokenType
{
	TK_ASSIGNOP,
	TK_COMMENT,
	TK_FIELDID,
	TK_ID,
	TK_NUM,
	TK_RNUM,
	TK_FUNID,
	TK_RUID,
	TK_WITH,
	TK_PARAMETERS,
	TK_END,
	TK_WHILE,
	TK_UNION,
	TK_ENDUNION,
	TK_DEFINETYPE,
	TK_AS,
	TK_TYPE,
	TK_MAIN,
	TK_GLOBAL,
	TK_PARAMETER,
	TK_LIST,
	TK_SQL,
	TK_SQR,
	TK_INPUT,
	TK_OUTPUT,
	TK_INT,
	TK_REAL,
	TK_COMMA,
	TK_SEM,
	TK_COLON,
	TK_DOT,
	TK_ENDWHILE,
	TK_OP,
	TK_CL,
	TK_IF,
	TK_THEN,
	TK_ENDIF,
	TK_READ,
	TK_WRITE,
	TK_RETURN,
	TK_PLUS,
	TK_MINUS,
	TK_MUL,
	TK_DIV,
	TK_CALL,
	TK_RECORD,
	TK_ENDRECORD,
	TK_ELSE,
	TK_AND,
	TK_OR,
	TK_NOT,
	TK_LT,
	TK_LE,
	TK_EQ,
	TK_GT,
	TK_GE,
	TK_NE,
	TK_EOF,
	TK_WHITESPACE,
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