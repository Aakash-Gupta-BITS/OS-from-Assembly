module jack.vm:frontend;

import std;

namespace
{
    constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return std::array
                {
                    TransitionInfo{0, 1, "-"},
                    TransitionInfo{0, 2, "0123456789"},
                    TransitionInfo{2, 2, "0123456789"},
                    TransitionInfo{0, 3, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:"},
                    TransitionInfo{3, 3, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:0123456789"},
                    TransitionInfo{0, 4, " \t\r "},
                    TransitionInfo{4, 4, " \t\r "},
                    TransitionInfo{0, 5, "\n"}
                };
            };

        constexpr auto final_states = []()
            {
                return std::array
                {
                    FinalStateInfo{1, TK_MINUS},
                    FinalStateInfo{2, TK_NUM},
                    FinalStateInfo{3, TK_SYMBOL},
                    FinalStateInfo{4, TK_WHITESPACE},
                    FinalStateInfo{5, TK_NEWLINE}
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }

    consteval auto get_parser()
    {
        using enum NonTerminal;
        using enum Terminal;
        using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;

        constexpr auto lxr = get_lexer();

        return build_parser([]()
            {
                return std::array
                {
                    PI(start, TK_FUNCTION, TK_SYMBOL, TK_NUM, func_start),
                    PI(start, TK_NEWLINE, start),
                    PI(start, TK_EOF),
                    PI(func_start, TK_NEWLINE, line_start),
                    PI(func_start, TK_EOF),
                    PI(line_start, op, line_end),
                    PI(line_start, TK_NEWLINE, line_start),
                    PI(line_start, TK_FUNCTION, TK_SYMBOL, TK_NUM, func_start),
                    PI(line_start, TK_EOF),
                    PI(line_end, TK_EOF),
                    PI(line_end, TK_NEWLINE, line_start),
                    PI(op, segment),
                    PI(op, alu_op),
                    PI(op, branch),
                    PI(op, TK_RETURN),
                    PI(op, TK_CALL, TK_SYMBOL, TK_NUM),
                    PI(segment, stack_op_name, seg_name, TK_NUM),
                    PI(stack_op_name, TK_POP),
                    PI(stack_op_name, TK_PUSH),
                    PI(seg_name, TK_LOCAL),
                    PI(seg_name, TK_ARGUMENT),
                    PI(seg_name, TK_STATIC),
                    PI(seg_name, TK_CONSTANT),
                    PI(seg_name, TK_THIS),
                    PI(seg_name, TK_THAT),
                    PI(seg_name, TK_POINTER),
                    PI(seg_name, TK_TEMP),
                    PI(alu_op, arithmetic),
                    PI(alu_op, logical),
                    PI(alu_op, boolean),
                    PI(arithmetic, TK_ADD),
                    PI(arithmetic, TK_SUB),
                    PI(arithmetic, TK_NEG),
                    PI(logical, TK_EQ),
                    PI(logical, TK_GT),
                    PI(logical, TK_LT),
                    PI(boolean, TK_AND),
                    PI(boolean, TK_OR),
                    PI(boolean, TK_NOT),
                    PI(branch, TK_LABEL, TK_SYMBOL),
                    PI(branch, TK_GOTO, TK_SYMBOL),
                    PI(branch, TK_IF, TK_MINUS, TK_GOTO, TK_SYMBOL)
                };
            }, []() { return lxr; });
    }
}

constexpr void LexerToken::after_construction(const LexerToken& previous_token)
{
    line_number = previous_token.line_number + (previous_token.type == Terminal::TK_NEWLINE ? 1 : 0);

    constexpr std::array keywords = {
        "push",
        "pop",
        "local",
        "argument",
        "static",
        "constant",
        "this",
        "that",
        "pointer",
        "temp",
        "add",
        "sub",
        "neg",
        "eq",
        "gt",
        "lt",
        "and",
        "or",
        "not",
        "if",
        "label",
        "goto",
        "function",
        "call",
        "return"
    };

    constexpr std::array<Terminal, keywords.size()> keyword_types = {
        Terminal::TK_PUSH,
        Terminal::TK_POP,
        Terminal::TK_LOCAL,
        Terminal::TK_ARGUMENT,
        Terminal::TK_STATIC,
        Terminal::TK_CONSTANT,
        Terminal::TK_THIS,
        Terminal::TK_THAT,
        Terminal::TK_POINTER,
        Terminal::TK_TEMP,
        Terminal::TK_ADD,
        Terminal::TK_SUB,
        Terminal::TK_NEG,
        Terminal::TK_EQ,
        Terminal::TK_GT,
        Terminal::TK_LT,
        Terminal::TK_AND,
        Terminal::TK_OR,
        Terminal::TK_NOT,
        Terminal::TK_IF,
        Terminal::TK_LABEL,
        Terminal::TK_GOTO,
        Terminal::TK_FUNCTION,
        Terminal::TK_CALL,
        Terminal::TK_RETURN
    };

    if (std::is_constant_evaluated())
    {
        for (int i = 0; i < keywords.size(); ++i)
            if (lexeme == keywords[i])
                type = keyword_types[i];

        return;
    }

    if (terminal_map.empty())
        for (int i = 0; i < keywords.size(); ++i)
            terminal_map[keywords[i]] = keyword_types[i];

    auto it = terminal_map.find(lexeme);
    if (it != terminal_map.end())
        type = it->second;
}

std::map<std::string_view, Terminal> LexerToken::terminal_map;

auto jack::vm::get_ast(std::string_view vm_file_content) -> std::expected<ASTNode<LexerTypes<LexerToken>, NonTerminal>, std::string>
{
    constexpr auto parser = get_parser();
    auto result = parser(vm_file_content);

    if (!result.errors.empty())
        return std::unexpected(result.errors);


    return ASTNode<LexerTypes<LexerToken>, NonTerminal>(Terminal::TK_ADD);
}