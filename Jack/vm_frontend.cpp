module jack.vm:frontend;

import std;

namespace
{
    consteval auto get_lexer()
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

    auto op_to_ast(auto parse_ptr) -> std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>
    {
        using ParseNodeType = decltype(parse_ptr)::element_type;
        using ASTType = ASTNode<LexerTypes<LexerToken>, NonTerminal>;

        const auto& descendants = parse_ptr->descendants;
        const auto& descendant_token_to_ast = [&](std::size_t index) { return std::make_unique<ASTType>(parse_ptr->extract_child_leaf(index)); };
        const auto& descendant_nt = [&](std::size_t index) { return parse_ptr->extract_child_node(index); };
        const auto& is_descendant_token = [&](std::size_t index) { return std::holds_alternative<ParseNodeType::LeafType>(descendants[index]); };

        if (descendants.size() == 3)
        {
            // op -> TK_CALL TK_SYMBOL TK_NUM
            auto ast_token = descendant_token_to_ast(0);
            ast_token->descendants.emplace_back(descendant_token_to_ast(1));
            ast_token->descendants.emplace_back(descendant_token_to_ast(2));
            return ast_token;
        }

        // op -> segment
        // op -> alu_op
        // op -> branch
        // op -> TK_RETURN

        if (is_descendant_token(0))
			return descendant_token_to_ast(0);

        auto descendant = descendant_nt(0);
        if (descendant->node_type == NonTerminal::segment)
        {
            // segment -> stack_op_name seg_name TK_NUM
            // stack_op_name -> TK_POP
            // stack_op_name -> TK_PUSH
            // seg_name -> TK_LOCAL
            // seg_name -> TK_ARGUMENT
            // seg_name -> TK_STATIC
            // seg_name -> TK_CONSTANT
            // seg_name -> TK_THIS
            // seg_name -> TK_THAT
            // seg_name -> TK_POINTER
            // seg_name -> TK_TEMP

            auto ast_token = std::make_unique<ASTType>(descendant->extract_child_node(0)->extract_child_leaf(0));
            ast_token->descendants.emplace_back(std::make_unique<ASTType>(descendant->extract_child_node(1)->extract_child_leaf(0)));
            ast_token->descendants.emplace_back(std::make_unique<ASTType>(descendant->extract_child_leaf(2)));
			return ast_token;
		}

        if (descendant->node_type == NonTerminal::alu_op)
        {
            // alu_op -> arithmetic
            // alu_op -> logical
            // alu_op -> boolean
            // arithmetic -> TK_ADD
            // arithmetic -> TK_SUB
            // arithmetic -> TK_NEG
            // logical -> TK_EQ
            // logical -> TK_GT
            // logical -> TK_LT
            // boolean -> TK_AND
            // boolean -> TK_OR
            // boolean -> TK_NOT

            return std::make_unique<ASTType>(descendant->extract_child_node(0)->extract_child_leaf(0));
        }

        if (descendant->node_type == NonTerminal::branch)
        {
            // branch -> TK_LABEL TK_SYMBOL
            // branch -> TK_GOTO TK_SYMBOL
            // branch -> TK_IF TK_MINUS TK_GOTO TK_SYMBOL
            auto& branch = descendant;
            auto ast_token = std::make_unique<ASTType>(branch->extract_child_leaf(0));
            ast_token->descendants.emplace_back(std::make_unique<ASTType>(branch->extract_child_leaf(branch->descendants.size() - 1)));
            return ast_token;
		}

        std::unreachable();
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

    if (auto it = terminal_map.find(lexeme); it != terminal_map.end())
        type = it->second;
}

std::map<std::string_view, Terminal> LexerToken::terminal_map;

auto jack::vm::get_ast(std::string_view vm_file_content) -> std::expected<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>, std::string>
{
    constexpr auto parser = get_parser();

    auto result = parser(vm_file_content);

    if (!result.errors.empty())
        return std::unexpected(result.errors);
    
    using ParseNodeType = decltype(result.root)::element_type;
    using ASTType = ASTNode<LexerTypes<LexerToken>, NonTerminal>;

    // Tail recursive AST construction
    auto parse_ptr = std::move(result.root);
    auto ast_root = std::make_unique<ASTType>(NonTerminal::start);
    auto ast_ptr = ast_root.get();

    while (parse_ptr != nullptr)
    {
        const auto& node_type = parse_ptr->node_type;
        const auto& descendants = parse_ptr->descendants;
        const auto& descendant_size = parse_ptr->descendants.size();
        const auto& descendant_token_to_ast = [&](std::size_t index) { return std::make_unique<ASTType>(parse_ptr->extract_child_leaf(index)); };
        const auto& is_descendant_token = [&](std::size_t index) { return std::holds_alternative<ParseNodeType::LeafType>(descendants[index]); };

        if (node_type == NonTerminal::start)
        {
            // start -> TK_FUNCTION TK_SYMBOL TK_NUM func_start
            // start -> TK_NEWLINE start
            // start -> TK_EOF

            if (descendant_size == 1)
                break;

            if (descendant_size == 2)
                goto LAST_AND_CONTINUE;

            ast_root->descendants.emplace_back(descendant_token_to_ast(0));
            ast_ptr = ast_root->descendants.back().get();
            ast_ptr->descendants.emplace_back(descendant_token_to_ast(1));
            ast_ptr->descendants.emplace_back(descendant_token_to_ast(2));
            ast_ptr->descendants.emplace_back(std::make_unique<ASTType>(NonTerminal::func_start));
            ast_ptr = ast_ptr->descendants.back().get();
            goto LAST_AND_CONTINUE;
        }

        if (node_type == NonTerminal::func_start)
        {
			// func_start -> TK_NEWLINE line_start
			// func_start -> TK_EOF
			if (descendant_size == 1)
				break;

			goto LAST_AND_CONTINUE;
		}

        if (node_type == NonTerminal::line_start)
        {
            // line_start -> op line_end
			// line_start -> TK_NEWLINE line_start
			// line_start -> TK_FUNCTION TK_SYMBOL TK_NUM func_start
			// line_start -> TK_EOF

			if (descendant_size == 1)
				break;

            if (descendant_size == 2)
            {
                if (is_descendant_token(0))
                    goto LAST_AND_CONTINUE;

                // Process operation here
                ast_ptr->descendants.emplace_back(op_to_ast(parse_ptr->extract_child_node(0)));
                goto LAST_AND_CONTINUE;
            }

            ast_root->descendants.emplace_back(descendant_token_to_ast(0));
            ast_ptr = ast_root->descendants.back().get();
            ast_ptr->descendants.emplace_back(descendant_token_to_ast(1));
            ast_ptr->descendants.emplace_back(descendant_token_to_ast(2));
            ast_ptr->descendants.emplace_back(std::make_unique<ASTType>(NonTerminal::func_start));
            ast_ptr = ast_ptr->descendants.back().get();
            goto LAST_AND_CONTINUE;
        }

        if (node_type == NonTerminal::line_end)
        {
			// line_end -> TK_EOF
			// line_end -> TK_NEWLINE line_start

			if (descendant_size == 1)
				break;

			goto LAST_AND_CONTINUE;
        }

        std::unreachable();

    LAST_AND_CONTINUE:
        parse_ptr = std::move(parse_ptr->extract_child_node(parse_ptr->descendants.size() - 1));
    }

    return ast_root;
}