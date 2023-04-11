#include "Lexer.h"
#include <cassert>
#include <iomanip>
#include <sstream>
using namespace std;

DFA dfa;

std::ostream& operator<< (std::ostream& out, const Token& token)
{
	if (token.type == TokenType::TK_ERROR_LENGTH)
	{
		if (token.lexeme[0] >= '0' && token.lexeme[0] <= '9')
			out << "Line " << token.line_number << "\t\terror: Number must be less than 32767.";
		else
			out << "Line " << token.line_number << "\t\terror: Identifier length is greater than 30.";
	}
	else if (token.type == TokenType::TK_ERROR_SYMBOL)
		out << "Line " << token.line_number << "\t\terror: Unknwon Symbol <" << token.lexeme << ">.";
	else if (token.type == TokenType::TK_ERROR_PATTERN)
		out << "Line " << token.line_number << "\t\terror: Unknwon Pattern <" << token.lexeme << ">.";
	else
		out << "Line " << token.line_number << "\t\tToken: " << setw(20) << dfa.tokenType2tokenStr[(int)token.type] << "\tLexeme: " << token.lexeme;

	return out;
}

void loadDFA()
{
	std::ifstream dfaReader{ LexerLoc };
	assert(dfaReader);
	dfaReader >> dfa.num_tokens >> dfa.num_states >> dfa.num_transitions >> dfa.num_finalStates >> dfa.num_keywords;

	// Load Tokens
	dfa.tokenType2tokenStr.clear();
	dfa.tokenType2tokenStr.resize(dfa.num_tokens);
	dfa.tokenStr2tokenType.clear();
	for (int i = 0; i < dfa.num_tokens; ++i)
	{
		dfaReader >> dfa.tokenType2tokenStr[i];
		dfa.tokenStr2tokenType[dfa.tokenType2tokenStr[i]] = (TokenType)i;
	}

	// Load Transitions
	dfa.productions.clear();
	dfa.productions.assign(dfa.num_states, vector<int>(128, -1));

	for (int i = 0; i < dfa.num_transitions; ++i)
	{
		int from, to;
		string symbols;
		dfaReader >> from >> to >> symbols;

		for (char c : symbols)
			dfa.productions[from][c] = to;
	}

	// state 12 - accept state for comment
	for (int i = 0; i < 128; i++)
		dfa.productions[12][i] = 12;

	dfa.productions[12]['\n'] = -1;

	// state 13 - accept state for whitespace
	dfa.productions[0][' ']
		= dfa.productions[0]['\t']
		= dfa.productions[0]['\r']
		= dfa.productions[13][' ']
		= dfa.productions[13]['\t']
		= dfa.productions[13]['\r']
		= 13;

	// Load Final States
	dfa.finalStates.clear();
	dfa.finalStates.assign(dfa.num_states, TokenType::UNINITIALISED);

	for (int i = 0; i < dfa.num_finalStates; i++)
	{
		int state;
		string BUFF;
		dfaReader >> state >> BUFF;

		dfa.finalStates[state] = dfa.tokenStr2tokenType[BUFF];
	}

	// Load Keywords
	dfa.lookupTable.clear();
	dfa.keywordTokens.clear();
	for (int i = 0; i < dfa.num_keywords; ++i)
	{
		string keyword, token_name;
		dfaReader >> keyword >> token_name;
		dfa.lookupTable[keyword] = dfa.tokenStr2tokenType[token_name];
		dfa.keywordTokens.insert(token_name);
	}
}

void onTokenFromDFA(Token*& token, Buffer& buffer)
{
	if (token->type == TokenType::TK_COMMENT || token->type == TokenType::TK_WHITESPACE)
	{
		delete token;
		token = nullptr;
		return;
	}

	char* BUFF = new char[token->length + 1];
	for (int i = 0; i < token->length; i++)
		BUFF[i] = buffer.getChar(buffer.start_index - token->length + i);
	BUFF[token->length] = 0;
	token->lexeme = BUFF;

	if (token->type == TokenType::TK_SYMBOL)
	{
		auto res = dfa.lookupTable.find(token->lexeme);
		if (res != dfa.lookupTable.end())
			token->type = dfa.lookupTable.at(token->lexeme);
	}

	if (token->type == TokenType::TK_SYMBOL && token->length > 30)
		token->type = TokenType::TK_ERROR_LENGTH;

	if (token->type == TokenType::TK_NUM)
	{
		// make sure it fits in 15 bits
		if (token->lexeme.size() > 5)
			token->type = TokenType::TK_ERROR_LENGTH;
		else 
		{
			stringstream sstr{ token->lexeme };
			int x;
			sstr >> x;
			if (x > 32767)
				token->type = TokenType::TK_ERROR_LENGTH;
		}
	}
}

Token* getTokenFromDFA(Buffer& buffer)
{
	TokenType ttype;
	int start_index = buffer.start_index;
	int last_final = -1;
	int input_final_pos = start_index - 1;

	int len = 0;

	int cur_state = 0;
	// start index = index of character to read next

	while (true)
	{
		char input = buffer.getChar(start_index);

		last_final = cur_state;
		ttype = dfa.finalStates[cur_state];
		input_final_pos = start_index - 1;

		cur_state = dfa.productions[cur_state][input];

		if (cur_state == -1)    // return
		{
			if (input_final_pos == start_index - len - 1)
			{
				Token* token = new Token;
				token->type = TokenType::TK_ERROR_SYMBOL;
				token->length = 1;
				return token;
			}
			if (dfa.finalStates[last_final] == TokenType::UNINITIALISED && last_final != 0)
			{
				Token* token = new Token;
				token->type = TokenType::TK_ERROR_PATTERN;
				token->length = len;
				return token;
			}

			Token* token = new Token;
			token->type = ttype;
			token->length = input_final_pos - (start_index - len) + 1;
			return token;
		}

		start_index++;
		len++;
	}

	// this should not be reachable as our DFA is capable of handling every case
	assert(false);
}

Token* getNextToken(Buffer& buffer)
{
	while (buffer.getTopChar() != '\0')
	{
		if (buffer.getTopChar() == '\n')
		{
			buffer.line_number++;
			buffer.start_index++;
			continue;
		}

		Token* token = getTokenFromDFA(buffer);

		assert(token != nullptr);

		token->start_index = buffer.start_index;
		token->line_number = buffer.line_number;

		buffer.start_index += token->length;

		onTokenFromDFA(token, buffer);

		if (token == nullptr)
			continue;

		return token;
	}

	Token* end = new Token;
	end->type = TokenType::TK_EOF;
	end->line_number = buffer.line_number;
	return end;
}