export module compiler.parser:parser;

import :structures;
import :parse_table;
import compiler.lexer;
import helpers;

import std;

enum class ETokenConsumed
{
	CONSUMED,
	NOT_CONSUMED,
	STACK_EMPTY
};

template <CParserTypes ParserTypes>
class ParserStack
{
	using ETerminal = ParserTypes::ETerminal;
	using ENonTerminal = ParserTypes::ENonTerminal;
	using ELexerSymbol = ParserTypes::ELexerSymbol;
	using EParserSymbol = ParserTypes::EParserSymbol;
	using ParseTreeNode = ParserTypes::ParseTreeNode;
	using ILexerToken = ParserTypes::ILexerToken;

	using InternalNodeType = ParseTreeNode::InternalNodeType;

	std::vector<EParserSymbol> stack{};
	std::unique_ptr<ParseTreeNode> root;

	ParseTreeNode* parent;
	int child_index;

	constexpr_ostream& log;
	constexpr_ostream& err;

	constexpr auto on_terminal_matched(CLexerToken auto& token)
	{
		parent->descendants[child_index] = std::make_unique<std::decay_t<decltype(token)>>(token);
		this->pop();
		return ETokenConsumed::CONSUMED;
	}

	constexpr auto on_terminal_not_matched(CLexerToken auto& token)
	{
		err << "Parser error (" << token << ") Invalid token encountered with stack top " << top() << "\n";
		this->pop();
		return ETokenConsumed::NOT_CONSUMED;
	}

	constexpr auto on_non_terminal_expand(CLexerToken auto& token, IsParseTable auto& parse_table)
	{
		const auto stack_top = std::get<ENonTerminal>(this->top());
		const auto input_terminal = std::get<ETerminal>(token.type);

		const auto production_number = parse_table.parse_table[(int)stack_top][(int)input_terminal];
		const auto& production = parse_table.productions[production_number];

		// empty production
		if (production.size == 1 && production.production[0] == ETerminal::eps)
		{
			this->pop();
			return ETokenConsumed::NOT_CONSUMED;
		}

		auto& node = parent ? *(std::get<InternalNodeType>(parent->descendants[child_index]).get()) : *root.get();
		node.production_number = production_number;

		for (auto [index, symbol] : production.production | std::views::take(production.size) | std::views::enumerate)
			if (std::holds_alternative<ETerminal>(symbol))
				node.descendants.push_back(std::make_unique<ILexerToken>());
			else
				node.descendants.push_back(std::make_unique<ParseTreeNode>(
					std::get<ENonTerminal>(symbol),
					&node,
					(int)index
				));

		stack.pop_back();
		for (auto& symbol : production.production | std::views::take(production.size) | std::views::reverse)
			stack.push_back(symbol);

		parent = &node;
		child_index = 0;
		return ETokenConsumed::NOT_CONSUMED;
	}

	constexpr auto on_non_terminal_error(CLexerToken auto& token)
	{
		err << "Parser error (" << token << ") Invalid token encountered with stack top " << top() << "\n";
		return ETokenConsumed::CONSUMED;
	}

	constexpr auto on_non_terminal_sync(CLexerToken auto& token)
	{
		err << "Parser error (" << token << ") Invalid token encountered with stack top " << top() << "\n";
		this->pop();
		return ETokenConsumed::NOT_CONSUMED;
	}

	constexpr auto pop()
	{
		stack.pop_back();
		while (child_index == parent->descendants.size() - 1)
		{
			child_index = parent->parent_child_index;
			parent = parent->parent;

			if (parent == nullptr)
				return;
		}
		++child_index;
	}

public:
	constexpr ParserStack(auto& log, auto& err) : log{ log }, err{ err }
	{
		stack.push_back(ENonTerminal::start);
		root = std::make_unique<ParseTreeNode>(ENonTerminal::start);

		parent = nullptr;
		child_index = 0;
	}

	constexpr inline auto top() const
	{
		return stack.back();
	}

	constexpr inline auto empty() const
	{
		return stack.empty();
	}

	constexpr inline auto& get_root()
	{
		return root;
	}

	constexpr auto process_token(CLexerToken auto& token, IsParseTable auto& parse_table)
	{
		log << "Stack: ";
		for (auto& symbol : stack)
			log << symbol << " ";
		log << "(top)\nToken: " << token << "\n\n";
		
		if (this->empty())
		{
			err << "Parser error (" << token << "): Input source code is syntactically incorrect.\n";
			return ETokenConsumed::STACK_EMPTY;
		}

		const auto token_type = std::get<ETerminal>(token.type);

		// Stack top is terminal
		if (auto stored_type = std::get_if<ETerminal>(&stack.back()); stored_type)
		{
			if (*stored_type == token_type)
				return on_terminal_matched(token);

			return on_terminal_not_matched(token);
		}

		// Stack top is non terminal
		const auto non_terminal = std::get<ENonTerminal>(this->top());
		const auto prod_number = parse_table.parse_table[(int)non_terminal][(int)token_type];

		if (prod_number == -1)
			return on_non_terminal_error(token);

		if (prod_number == -2)
			return on_non_terminal_sync(token);

		return on_non_terminal_expand(token, parse_table);
	}
};

template <CParserTypes ParserTypes, CLexerTypes LexerTypes, int num_states, int max_prod_len, int num_productions>
class Parser
{
	using ETerminal = ParserTypes::ETerminal;
	using ENonTerminal = ParserTypes::ENonTerminal;
	using ELexerSymbol = std::variant<ETerminal, ELexerError>;

	struct ParserOutput
	{
		std::unique_ptr<typename ParserTypes::ParseTreeNode> root;
		std::string logs;
		std::string errors;
	};

public:
	Lexer<LexerTypes, num_states> lexer{};
	ParseTable<ParserTypes, max_prod_len, num_productions> parse_table{};

	constexpr auto operator()(std::string_view source_code) const
	{
		constexpr_ostream clog, cerr{};
		ParserStack<ParserTypes> stack(clog, cerr);

		for (auto token : lexer(source_code))
		{
			if (std::holds_alternative<ELexerError>(token.type))
			{
				cerr << "Lexer error: " << token << "\n";
				continue;
			}

			while (stack.process_token(token, parse_table) == ETokenConsumed::NOT_CONSUMED);
		}

		if (!stack.empty())
			cerr << "Parser error: No more tokens but parsing still left!\n";
		else
			clog << "Parser: Parsing complete\n";

		ParserOutput output{
			.root = std::move(stack.get_root()),
			.logs = std::string(clog.sv()),
			.errors = std::string(cerr.sv())
		};

		return output;
	}
};

template <typename T>
concept IsParser = requires(T t)
{
	[] <CParserTypes ParserTypes, CLexerTypes LexerTypes, int num_states, int max_prod_len, int num_productions>
		(Parser<ParserTypes, LexerTypes, num_states, max_prod_len, num_productions>&) {}(t);
};

export template <typename ostream>
constexpr ostream& operator<<(ostream& out, const IsParser auto& parser)
{
    return out << parser.lexer << "\n" << parser.parse_table;
}

export constexpr auto build_parser(auto production_callback, auto lexer_callback)
{
	return Parser
	{
		.lexer = lexer_callback(),
		.parse_table = build_parse_table(production_callback)
	};
}