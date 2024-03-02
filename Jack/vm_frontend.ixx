export module jack.vm:frontend;
import std;
import compiler;
import helpers;

enum class Terminal
{
    eps,
    TK_EOF,
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
    TK_WHITESPACE
};

enum class NonTerminal
{
    start,
    func_start,
    line_start,
    line_end,
    op,
    segment,
    stack_op_name,
    seg_name,
    alu_op,
    arithmetic,
    logical,
    boolean,
    branch
};

struct LexerToken
{
    std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
    std::string_view lexeme{};
    int line_number = 1;

    static std::map<std::string_view, Terminal> terminal_map;

    constexpr void after_construction(const LexerToken& previous_token);

    constexpr bool discard() const
    {
        return type == Terminal::TK_WHITESPACE;
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

namespace jack::vm
{
    export auto get_ast(std::string_view vm_file_content) -> std::expected<ASTNode<LexerTypes<LexerToken>, NonTerminal>, std::string>;
}