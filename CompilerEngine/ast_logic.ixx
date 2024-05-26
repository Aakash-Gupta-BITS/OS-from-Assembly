export module compiler.ast:logic;

import std;

import :structures;
import compiler.lexer;
import compiler.parser;
import helpers;

using std::unique_ptr;

template<typename vtype, int array_size>
class ASTVisitor
{
	template <IsParseTreeNode ptn, IsASTNode astn>
	constexpr auto visitor(unique_ptr<ptn> node, unique_ptr<astn> inherited) const -> unique_ptr<astn>
	{
		if (!node)
			return nullptr;

		std::function<unique_ptr<astn>(unique_ptr<ptn>, unique_ptr<astn>)> converter =
			[this](unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
			{
				return this->visitor(std::move(node), std::move(inherited));
			};

		return std::visit(
			[&](auto& _class_to_call) -> unique_ptr<astn>
			{
				using visitor_type = std::remove_cvref_t<decltype(_class_to_call)>;
				return visitor_type::to_ast(converter, std::move(node), std::move(inherited));
			},
			m_visitors[static_cast<int>(node->node_type)]
		);
	}

public:
	const std::array<vtype, array_size> m_visitors;

	template <IsParseTreeNode ptn, IsASTNode astn>
	constexpr auto visit(unique_ptr<ptn> node) const -> unique_ptr<astn>
	{
		if (!node)
			return nullptr;

		return visitor<ptn, astn>(std::move(node), nullptr);
	}
};

export template<CENonTerminal ENonTerminal, typename... Types>
constexpr auto build_visitor(std::pair<ENonTerminal, Types> ...args)
{
	using vtype = variant_unique<Types...>;

	constexpr auto array_size = get_enum_size<ENonTerminal>();
	std::vector<std::pair<ENonTerminal, vtype>> arr{ args... };

	std::array<vtype, array_size> m_visitors;
	std::array<int, array_size> counts{};

	for (auto& [key, value] : arr)
		m_visitors[static_cast<int>(key)] = value, counts[static_cast<int>(key)]++;
	
	for (auto& x : counts)
		if (x == 0)
			throw std::runtime_error("Mapping of visitor not found for atleast one Non Terminal.");
		else if (x > 1)
			throw std::runtime_error("There exist atleast two mappings for single Non Terminal.");
	
	return ASTVisitor<vtype, array_size> {
		.m_visitors = m_visitors
	};
}