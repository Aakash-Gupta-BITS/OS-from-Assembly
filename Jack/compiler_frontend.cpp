module jack.compiler:frontend;
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
				TransitionInfo{0, 1, "{"},
				TransitionInfo{0, 2, "}"},
				TransitionInfo{0, 3, "("},
				TransitionInfo{0, 4, ")"},
				TransitionInfo{0, 5, "["},
				TransitionInfo{0, 6, "]"},
				TransitionInfo{0, 7, "."},
				TransitionInfo{0, 8, ","},
				TransitionInfo{0, 9, ";"},
				TransitionInfo{0, 10, "+"},
				TransitionInfo{0, 11, "-"},
				TransitionInfo{0, 12, "*"},
				TransitionInfo{0, 13, "/"},
				TransitionInfo{13, 25, "/"},
				TransitionInfo{13, 26, "*"},
				TransitionInfo{0, 14, "&"},
				TransitionInfo{0, 15, "|"},
				TransitionInfo{0, 16, "<"},
				TransitionInfo{0, 17, ">"},
				TransitionInfo{0, 18, "="},
				TransitionInfo{0, 19, "~"},
				TransitionInfo{0, 20, "0123456789"},
				TransitionInfo{20, 20, "0123456789"},
				TransitionInfo{0, 21, "\""},
				TransitionInfo{.from = 21, .to = -1, .pattern = "\r\n\"", .default_transition_state = 21},
				TransitionInfo{21, 22, "\""},
				TransitionInfo{0, 23, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPLKJHGFDSAZXCVBNM_"},
				TransitionInfo{23, 23, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_1234567890"},
				TransitionInfo{0, 24, "\t\r\n "},
				TransitionInfo{24, 24, "\t\r\n "},
				TransitionInfo{.from = 25, .to = -1, .pattern = "\n", .default_transition_state = 25},
				TransitionInfo{.from = 26, .to = 27, .pattern = "*", .default_transition_state = 26},
				TransitionInfo{.from = 27, .to = -1, .pattern = "*/", .default_transition_state = 26},
				TransitionInfo{27, 26, "*"},
				TransitionInfo{27, 28, "/"}
			};
		};

		constexpr auto final_states = []()
		{
			return std::array
			{
				FinalStateInfo{1, TK_CURO},
				FinalStateInfo{2, TK_CURC},
				FinalStateInfo{3, TK_PARENO},
				FinalStateInfo{4, TK_PARENC},
				FinalStateInfo{5, TK_BRACKO},
				FinalStateInfo{6, TK_BRACKC},
				FinalStateInfo{7, TK_DOT},
				FinalStateInfo{8, TK_COMMA},
				FinalStateInfo{9, TK_SEMICOLON},
				FinalStateInfo{10, TK_PLUS},
				FinalStateInfo{11, TK_MINUS},
				FinalStateInfo{12, TK_MULT},
				FinalStateInfo{13, TK_DIV},
				FinalStateInfo{14, TK_AND},
				FinalStateInfo{15, TK_OR},
				FinalStateInfo{16, TK_LE},
				FinalStateInfo{17, TK_GE},
				FinalStateInfo{18, TK_EQ},
				FinalStateInfo{19, TK_NOT},
				FinalStateInfo{20, TK_NUM},
				FinalStateInfo{22, TK_STR},
				FinalStateInfo{23, TK_IDENTIFIER},
				FinalStateInfo{24, TK_WHITESPACE},
				FinalStateInfo{25, TK_COMMENT},
				FinalStateInfo{28, TK_COMMENT}
			};
		};

		return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
	}

	consteval auto get_parser()
	{
		using enum NonTerminal;
		using enum Terminal;
		using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;

		constexpr auto lexer = get_lexer();

		return build_parser([]()
		{
			return std::array
			{
				PI(start, _class),
				PI(_class, TK_CLASS, TK_IDENTIFIER, TK_CURO, class_vars, subroutineDecs, TK_CURC, TK_EOF),
				PI(class_vars, class_var, class_vars),
				PI(class_vars, eps),
				PI(subroutineDecs, subroutineDec, subroutineDecs),
				PI(subroutineDecs, eps),
				PI(class_var, class_var_prefix, type, TK_IDENTIFIER, more_identifiers, TK_SEMICOLON),
				PI(class_var_prefix, TK_STATIC),
				PI(class_var_prefix, TK_FIELD),
				PI(type, TK_INT),
				PI(type, TK_CHAR),
				PI(type, TK_BOOLEAN),
				PI(type, TK_IDENTIFIER),
				PI(more_identifiers, TK_COMMA, TK_IDENTIFIER, more_identifiers),
				PI(more_identifiers, eps),
				PI(subroutineDec, subroutine_prefix, subroutine_type, TK_IDENTIFIER, TK_PARENO, parameters, TK_PARENC, subroutine_body),
				PI(subroutine_prefix, TK_CONSTRUCTOR),
				PI(subroutine_prefix, TK_FUNCTION),
				PI(subroutine_prefix, TK_METHOD),
				PI(subroutine_type, type),
				PI(subroutine_type, TK_VOID),
				PI(parameters, type, TK_IDENTIFIER, more_parameters),
				PI(parameters, eps),
				PI(more_parameters, TK_COMMA, type, TK_IDENTIFIER, more_parameters),
				PI(more_parameters, eps),
				PI(subroutine_body, TK_CURO, routine_vars, statements, TK_CURC),
				PI(routine_vars, routine_var, routine_vars),
				PI(routine_vars, eps),
				PI(routine_var, TK_VAR, type, TK_IDENTIFIER, more_identifiers, TK_SEMICOLON),
				PI(statements, statement, statements),
				PI(statements, eps),
				PI(statement, let_statement),
				PI(statement, if_statement),
				PI(statement, while_statement),
				PI(statement, do_statement),
				PI(statement, return_statement),
				PI(let_statement, TK_LET, TK_IDENTIFIER, identifier_suffix, TK_EQ, expression, TK_SEMICOLON),
				PI(identifier_suffix, eps),
				PI(identifier_suffix, TK_BRACKO, expression, TK_BRACKC),
				PI(if_statement, TK_IF, TK_PARENO, expression, TK_PARENC, TK_CURO, statements, TK_CURC, else_statement),
				PI(else_statement, eps),
				PI(else_statement, TK_ELSE, TK_CURO, statements, TK_CURC),
				PI(while_statement, TK_WHILE, TK_PARENO, expression, TK_PARENC, TK_CURO, statements, TK_CURC),
				PI(do_statement, TK_DO, subroutine_call, TK_SEMICOLON),
				PI(return_statement, TK_RETURN, return_suffix, TK_SEMICOLON),
				PI(return_suffix, eps),
				PI(return_suffix, expression),
				PI(expression, term, expression_suffix),
				PI(expression_suffix, op, term, expression_suffix),
				PI(expression_suffix, eps),
				PI(term, TK_NUM),
				PI(term, TK_STR),
				PI(term, TK_TRUE),
				PI(term, TK_FALSE),
				PI(term, TK_NULL),
				PI(term, TK_THIS),
				PI(term, TK_IDENTIFIER, term_sub_iden),
				PI(term_sub_iden, eps),
				PI(term_sub_iden, TK_PARENO, expression_list, TK_PARENC),
				PI(term_sub_iden, TK_DOT, TK_IDENTIFIER, TK_PARENO, expression_list, TK_PARENC),
				PI(term_sub_iden, TK_BRACKO, expression, TK_BRACKC),
				PI(term, TK_PARENO, expression, TK_PARENC),
				PI(term, TK_MINUS, term),
				PI(term, TK_NOT, term),
				PI(op, TK_PLUS),
				PI(op, TK_MINUS),
				PI(op, TK_MULT),
				PI(op, TK_DIV),
				PI(op, TK_AND),
				PI(op, TK_OR),
				PI(op, TK_LE),
				PI(op, TK_GE),
				PI(op, TK_EQ),
				PI(subroutine_call, TK_IDENTIFIER, subroutine_scope, TK_PARENO, expression_list, TK_PARENC),
				PI(subroutine_scope, TK_DOT, TK_IDENTIFIER),
				PI(subroutine_scope, eps),
				PI(expression_list, expression, more_expressions),
				PI(expression_list, eps),
				PI(more_expressions, TK_COMMA, expression, more_expressions),
				PI(more_expressions, eps)
			};
		}, [] { return lexer; });
	}

	auto compile_ast(auto parse_ptr, std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> inherited) -> decltype(inherited)
	{
		using ParseNodeType = decltype(parse_ptr)::element_type;
		using ASTType = decltype(inherited)::element_type;

		const auto& descendant_token = [&](std::size_t index) { return parse_ptr->extract_child_leaf(index); };
		const auto& descendant_nt = [&](std::size_t index) { return parse_ptr->extract_child_node(index); };
		const auto& descendant_token_to_ast = [&](std::size_t index) { return std::make_unique<ASTType>(descendant_token(index)); };
		const auto& is_descendant_token = [&](std::size_t index) { return std::holds_alternative<ParseNodeType::LeafType>(parse_ptr->descendants[index]); };
		const auto& descendant_count = [&]() { return parse_ptr->descendants.size(); };

		if (parse_ptr == nullptr) std::terminate();

		const NonTerminal& node_type = parse_ptr->node_type;

		if (node_type == NonTerminal::start) 
		{
			// start => _class
			return compile_ast(descendant_nt(0), {});
		}
		else if (node_type == NonTerminal::_class)
		{
			// class => TK_CLASS TK_IDENTIFIER TK_CURO class_vars subroutineDecs TK_CURC TK_EOF
			auto ast = std::make_unique<ASTType>(NonTerminal::_class, descendant_token(1));
			ast->descendants.push_back(compile_ast(descendant_nt(3), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(4), {}));
			return ast;
		}
		else if (node_type == NonTerminal::class_vars)
		{
			// class_vars => class_var class_vars
			// class_vars => eps
			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::class_vars);

			inherited->descendants.push_back(compile_ast(descendant_nt(0), {}));
			return compile_ast(descendant_nt(1), std::move(inherited));
		}
		else if (node_type == NonTerminal::subroutineDecs)
		{
			// subroutineDecs => subroutineDec subroutineDecs
			// subroutineDecs => eps

			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::subroutineDecs);

			inherited->descendants.push_back(compile_ast(descendant_nt(0), {}));
			return compile_ast(descendant_nt(1), std::move(inherited));
		}
		else if (node_type == NonTerminal::class_var)
		{
			// class_var => class_var_prefix type TK_IDENTIFIER more_identifiers TK_SEMICOLON
			auto ast = std::make_unique<ASTType>(NonTerminal::class_var);
			ast->descendants.push_back(compile_ast(descendant_nt(0), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			ast->descendants.push_back(std::make_unique<ASTType>(descendant_token(2)));

			auto more_ids = compile_ast(descendant_nt(3), {});
			if (more_ids == nullptr)
				return ast;

			for (auto &desc: more_ids->descendants)
				ast->descendants.push_back(std::move(desc));

			return ast;
		}
		else if (node_type == NonTerminal::class_var_prefix)
		{
			// class_var_prefix => TK_STATIC
			// class_var_prefix => TK_FIELD
			return descendant_token_to_ast(0);
		}
		else if (node_type == NonTerminal::type)
		{
			// type => TK_INT
			// type => TK_CHAR
			// type => TK_BOOLEAN
			// type => TK_IDENTIFIER
			return descendant_token_to_ast(0);
		}
		else if (node_type == NonTerminal::more_identifiers)
		{
			// more_identifiers => TK_COMMA TK_IDENTIFIER more_identifiers
			// more_identifiers => eps
			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::more_identifiers);

			inherited->descendants.push_back(descendant_token_to_ast(1));
			return compile_ast(descendant_nt(2), std::move(inherited));
		}
		else if (node_type == NonTerminal::subroutineDec)
		{
			// subroutineDec => subroutine_prefix subroutine_type TK_IDENTIFIER TK_PARENO parameters TK_PARENC subroutine_body
			auto ast = std::make_unique<ASTType>(compile_ast(descendant_nt(0), {})->node_symbol_type, descendant_token(2));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(4), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(6), {}));
			return ast;
		}
		else if (node_type == NonTerminal::subroutine_prefix)
		{
			// subroutine_prefix => TK_CONSTRUCTOR
			// subroutine_prefix => TK_FUNCTION
			// subroutine_prefix => TK_METHOD
			return descendant_token_to_ast(0);
		}
		else if (node_type == NonTerminal::subroutine_type)
		{
			// subroutine_type => type
			// subroutine_type => TK_VOID
			if (is_descendant_token(0))
				return descendant_token_to_ast(0);
			return compile_ast(descendant_nt(0), {});
		}
		else if (node_type == NonTerminal::parameters)
		{
			// parameters => type TK_IDENTIFIER more_parameters
			// parameters => eps
			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::parameters);

			auto param = compile_ast(descendant_nt(0), {});
			param->descendants.push_back(std::make_unique<ASTType>(descendant_token(1)));
			inherited->descendants.push_back(std::move(param));
			return compile_ast(descendant_nt(2), std::move(inherited));
		}
		else if (node_type == NonTerminal::more_parameters)
		{
			// more_parameters => TK_COMMA type TK_IDENTIFIER more_parameters
			// more_parameters => eps

			if (descendant_count() == 0)
				return inherited;

			auto param = compile_ast(descendant_nt(1), {});
			param->descendants.push_back(std::make_unique<ASTType>(descendant_token(2)));
			inherited->descendants.push_back(std::move(param));
			return compile_ast(descendant_nt(3), std::move(inherited));
		}
		else if (node_type == NonTerminal::subroutine_body)
		{
			// subroutine_body => TK_CURO routine_vars statements TK_CURC
			auto ast = std::make_unique<ASTType>(NonTerminal::subroutine_body);
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(2), {}));
			return ast;
		}
		else if (node_type == NonTerminal::routine_vars)
		{
			// routine_vars => routine_var routine_vars
			// routine_vars => eps
			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::routine_vars);

			inherited->descendants.push_back(compile_ast(descendant_nt(0), {}));
			return compile_ast(descendant_nt(1), std::move(inherited));
		}
		else if (node_type == NonTerminal::routine_var)
		{
			// routine_var => TK_VAR type TK_IDENTIFIER more_identifiers TK_SEMICOLON
			auto ast = std::make_unique<ASTType>(NonTerminal::routine_var);
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			ast->descendants.push_back(std::make_unique<ASTType>(descendant_token(2)));

			auto identifiers = compile_ast(descendant_nt(3), {});
			if (identifiers == nullptr)
				return ast;

			for (auto &desc: identifiers->descendants)
				ast->descendants.push_back(std::move(desc));

			return ast;
		}
		else if (node_type == NonTerminal::statements)
		{
			// statements => statement statements
			// statements => eps
			if (descendant_count() == 0)
				return inherited;

			if (inherited == nullptr)
				inherited = std::make_unique<ASTType>(NonTerminal::statements);

			inherited->descendants.push_back(compile_ast(descendant_nt(0), {}));
			return compile_ast(descendant_nt(1), std::move(inherited));
		}
		else if (node_type == NonTerminal::statement)
		{
			// statement => let_statement
			// statement => if_statement
			// statement => while_statement
			// statement => do_statement
			// statement => return_statement
			return compile_ast(descendant_nt(0), {});
		}
		else if (node_type == NonTerminal::let_statement)
		{
			// let_statement => TK_LET TK_IDENTIFIER identifier_suffix TK_EQ expression TK_SEMICOLON
			auto identifier = std::make_unique<ASTType>(descendant_token(1));
			identifier->descendants.push_back(compile_ast(descendant_nt(2), {}));

			auto ast = std::make_unique<ASTType>(descendant_token(0));
			ast->descendants.push_back(std::move(identifier));
			ast->descendants.push_back(compile_ast(descendant_nt(4), {}));
			return ast;
		}
		else if (node_type == NonTerminal::identifier_suffix)
		{
			// identifier_suffix => eps
			// identifier_suffix => TK_BRACKO expression TK_BRACKC
			if (descendant_count() == 0)
				return inherited;

			return compile_ast(descendant_nt(1), {});
		}
		else if (node_type == NonTerminal::if_statement)
		{
			// if_statement => TK_IF TK_PARENO expression TK_PARENC TK_CURO statements TK_CURC else_statement
			auto ast = std::make_unique<ASTType>(descendant_token(0));
			ast->descendants.push_back(compile_ast(descendant_nt(2), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(5), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(7), {}));
			return ast;
		}
		else if (node_type == NonTerminal::else_statement)
		{
			// else_statement => eps
			// else_statement => TK_ELSE TK_CURO statements TK_CURC
			if (descendant_count() == 0)
				return {};

			return compile_ast(descendant_nt(2), {});
		}
		else if (node_type == NonTerminal::while_statement)
		{
			// while_statement => TK_WHILE TK_PARENO expression TK_PARENC TK_CURO statements TK_CURC
			auto ast = std::make_unique<ASTType>(descendant_token(0));
			ast->descendants.push_back(compile_ast(descendant_nt(2), {}));
			ast->descendants.push_back(compile_ast(descendant_nt(5), {}));
			return ast;
		}
		else if (node_type == NonTerminal::do_statement)
		{
			// do_statement => TK_DO subroutine_call TK_SEMICOLON
			auto ast = std::make_unique<ASTType>(descendant_token(0));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return ast;
		}
		else if (node_type == NonTerminal::subroutine_call)
		{
			// subroutine_call => TK_IDENTIFIER subroutine_scope TK_PARENO expression_list TK_PARENC
			auto ast = std::make_unique<ASTType>(NonTerminal::subroutine_call);
			auto identifier = std::make_unique<ASTType>(descendant_token(0));
			identifier->descendants.push_back(compile_ast(descendant_nt(1), {}));
			ast->descendants.push_back(std::move(identifier));

			auto expression_list = compile_ast(descendant_nt(3), {});

			if (expression_list == nullptr)
				return ast;

			for	(auto &desc: expression_list->descendants)
				ast->descendants.push_back(std::move(desc));

			return ast;
		}
		else if (node_type == NonTerminal::return_statement)
		{
			// return_statement => TK_RETURN return_suffix TK_SEMICOLON
			auto ast = std::make_unique<ASTType>(descendant_token(0));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return ast;
		}
		else if (node_type == NonTerminal::return_suffix)
		{
			// return_suffix => eps
			// return_suffix => expression
			if (descendant_count() == 0)
				return {};

			return compile_ast(descendant_nt(0), {});
		}
		else if (node_type == NonTerminal::subroutine_scope)
		{
			// subroutine_scope => TK_DOT TK_IDENTIFIER
			// subroutine_scope => eps
			if (descendant_count() == 0)
				return {};

			return descendant_token_to_ast(1);
		}
		else if (node_type == NonTerminal::expression_list)
		{
			// expression_list => expression more_expressions
			// expression_list => eps
			if (descendant_count() == 0)
				return inherited;

			if (!inherited)
				inherited = std::make_unique<ASTType>(NonTerminal::expression_list);

			inherited->descendants.push_back(compile_ast(descendant_nt(0), {}));
			return compile_ast(descendant_nt(1), std::move(inherited));
		}
		else if (node_type == NonTerminal::more_expressions)
		{
			// more_expressions => TK_COMMA expression more_expressions
			// more_expressions => eps
			if (descendant_count() == 0)
				return inherited;

			inherited->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return compile_ast(descendant_nt(2), std::move(inherited));
		}
		else if (node_type == NonTerminal::expression)
		{
			// expression => term expression_suffix
			auto term = compile_ast(descendant_nt(0), {});
			return compile_ast(descendant_nt(1), std::move(term));
		}
		else if (node_type == NonTerminal::expression_suffix)
		{
			// expression_suffix => op term expression_suffix
			// expression_suffix => eps
			if (descendant_count() == 0)
				return inherited;

			auto ast = compile_ast(descendant_nt(0), {});
			ast->descendants.push_back(std::move(inherited));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return compile_ast(descendant_nt(2), std::move(ast));
		}
		else if (node_type == NonTerminal::op)
		{
			// op => TK_PLUS
			// op => TK_MINUS
			// op => TK_MULT
			// op => TK_DIV
			// op => TK_AND
			// op => TK_OR
			// op => TK_LE
			// op => TK_GE
			// op => TK_EQ

			return descendant_token_to_ast(0);
		}
		else if (node_type == NonTerminal::term)
		{
			// term => TK_NUM
			// term => TK_STR
			// term => TK_TRUE
			// term => TK_FALSE
			// term => TK_NULL
			// term => TK_THIS
			// term => TK_PARENO expression TK_PARENC
			// term => TK_IDENTIFIER term_sub_iden
			// term => TK_MINUS term
			// term => TK_NOT term
			if (descendant_count() == 1)
				return descendant_token_to_ast(0);

			auto first_child = descendant_token(0);

			if (first_child->type == Terminal::TK_PARENO)
				return compile_ast(descendant_nt(1), {});

			auto ast = std::make_unique<ASTType>(std::move(first_child));
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return ast;
		}
		else if (node_type == NonTerminal::term_sub_iden)
		{
			// term_sub_iden => eps
			// term_sub_iden => TK_PARENO expression_list TK_PARENC
			// term_sub_iden => TK_DOT TK_IDENTIFIER TK_PARENO expression_list TK_PARENC
			// term_sub_iden => TK_BRACKO expression TK_BRACKC
			if (descendant_count() == 0)
				return {};

			auto first_child = descendant_token(0);
			if (first_child->type == Terminal::TK_PARENO)
				return compile_ast(descendant_nt(1), {});

			auto ast = std::make_unique<ASTType>(std::move(first_child));
			if (first_child->type == Terminal::TK_DOT)
			{
				ast->descendants.push_back(descendant_token_to_ast(1));
				ast->descendants.push_back(compile_ast(descendant_nt(3), {}));
				return ast;
			}
			ast->descendants.push_back(compile_ast(descendant_nt(1), {}));
			return ast;
		}

		std::unreachable();
	}
}

std::map<std::string_view, Terminal> LexerToken::keyword_map;

constexpr void LexerToken::after_construction(const LexerToken& previous_token) 
{
	line_number = previous_token.line_number + (int)std::count(previous_token.lexeme.begin(), previous_token.lexeme.end(), '\n');
	
	constexpr std::array keywords = {
		"class",
		"constructor", 
		"function",
		"method",
		"field",
		"static",
		"var", 
		"int", 
		"char",
		"boolean",
		"void", 
		"true", 
		"false", 
		"null", 
		"this", 
		"let", 
		"do",
		"if",
		"else", 
		"while", 
		"return"
	};

	constexpr std::array<Terminal, keywords.size()> keyword_types = {
		Terminal::TK_CLASS,
		Terminal::TK_CONSTRUCTOR,
		Terminal::TK_FUNCTION,
		Terminal::TK_METHOD,
		Terminal::TK_FIELD,
		Terminal::TK_STATIC,
		Terminal::TK_VAR,
		Terminal::TK_INT,
		Terminal::TK_CHAR,
		Terminal::TK_BOOLEAN,
		Terminal::TK_VOID,
		Terminal::TK_TRUE,
		Terminal::TK_FALSE,
		Terminal::TK_NULL,
		Terminal::TK_THIS,
		Terminal::TK_LET,
		Terminal::TK_DO,
		Terminal::TK_IF,
		Terminal::TK_ELSE,
		Terminal::TK_WHILE,
		Terminal::TK_RETURN
	};
	
	if (std::is_constant_evaluated())
	{
		for (int i = 0; i < keywords.size(); ++i)
			if (lexeme == keywords[i])
				type = keyword_types[i];

		return;
	}

	if (keyword_map.empty())
		for (int i = 0; i < keywords.size(); ++i)
			keyword_map[keywords[i]] = keyword_types[i];

	if (auto it = keyword_map.find(lexeme); it != keyword_map.end())
		type = it->second;
}

auto jack::compiler::get_ast(std::string_view file_content) -> std::expected<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>, std::string>
{
	constexpr auto parser = get_parser();
	auto result = parser(file_content);

	if (!result.errors.empty())
		return std::unexpected(result.errors);

	return compile_ast(std::move(result.root), {});
}