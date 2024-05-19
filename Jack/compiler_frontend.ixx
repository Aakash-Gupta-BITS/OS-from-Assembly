export module jack.compiler:frontend;
import std;
import helpers;
import compiler;

enum class Terminal
{
	eps,
	TK_EOF,
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
	TK_WHITESPACE
};

enum class NonTerminal
{
	start,
	_class,
	class_vars,
	subroutineDecs,
	class_var,
	class_var_prefix,
	type,
	more_identifiers,
	subroutineDec,
	subroutine_prefix,
	subroutine_type,
	parameters,
	more_parameters,
	subroutine_body,
	routine_vars,
	routine_var,
	statements,
	statement,
	let_statement,
	identifier_suffix,
	if_statement,
	else_statement,
	while_statement,
	do_statement,
	return_statement,
	return_suffix,
	expression,
	expression_suffix,
	term,
	term_sub_iden,
	op,
	subroutine_call,
	subroutine_scope,
	expression_list,
	more_expressions
};

struct LexerToken
{
	std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
	std::string_view lexeme{};
	int line_number = 1;

	static std::map<std::string_view, Terminal> keyword_map;

	constexpr void after_construction(const LexerToken& previous_token);

	constexpr bool discard() const
	{
		return type == Terminal::TK_WHITESPACE || type == Terminal::TK_COMMENT;
	}

	constexpr auto copy_to_unique_ptr() const
	{
		return std::make_unique<LexerToken>(*this);
	}

	template <typename T>
	friend constexpr T& operator<<(T& out, const LexerToken& tk)
	{
		return out
			<< "{ line_number: "
			<< tk.line_number
			<< ", type: "
			<< tk.type
			<< ", lexeme : "
			<< tk.lexeme
			<< " }";
	}
};

namespace jack::compiler
{
	export auto get_ast(std::string_view jack_file_content) -> std::expected<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>, std::string>;
}