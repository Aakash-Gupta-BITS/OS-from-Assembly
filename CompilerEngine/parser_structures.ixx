export module compiler.parser:structures;

import helpers;
import compiler.lexer;
import std;

// Concepts
export template<typename T>
concept CENonTerminal = requires(T t)
{
	requires std::is_enum_v<T>;
	{ T::start };
};

export template <typename T>
concept CEParserSymbol = requires(T t)
{
    requires CETerminal<std::variant_alternative_t<0, T>>;
    requires CENonTerminal<std::variant_alternative_t<1, T>>;
};

// Structures
export template <CLexerTypes _LexerTypes, CENonTerminal _ENonTerminal>
struct ParseTreeNode
{
    using LexerTypes = _LexerTypes;
    using ENonTerminal = _ENonTerminal;

    using LeafType = std::unique_ptr<typename LexerTypes::ILexerToken>;
    using InternalNodeType = std::unique_ptr<ParseTreeNode>;

    const ENonTerminal node_type{};

    ParseTreeNode* const parent{};
    const int parent_child_index{ -1 };

    int production_number { -1 };

    std::vector<std::variant<LeafType, InternalNodeType>> descendants{};

    constexpr inline auto extract_child_leaf(std::size_t index)
    {
        return std::move(std::get<LeafType>(descendants[index]));
    }

    constexpr inline auto extract_child_node(std::size_t index)
    {
        return std::move(std::get<InternalNodeType>(descendants[index]));
    }

    template<typename ostream>
    constexpr friend ostream& operator<<(ostream& os, const ParseTreeNode& node)
    {
        node.print(os, 0);
        return os;
    }

private:

    template<typename ostream>
    constexpr void print(ostream& os, int depth = 0) const
    {
        for (int i = 0; i < depth; ++i) os << "\t";
        os << node_type << '\n';
        for (const auto& descendant : descendants)
            if (std::holds_alternative<LeafType>(descendant))
            {
                for (int i = 0; i <= depth; ++i) os << "\t";
                os << *std::get<LeafType>(descendant) << '\n';
            }
            else
                std::get<InternalNodeType>(descendant)->print(os, depth + 1);
    }
};

export template <typename T>
concept IsParseTreeNode = requires(T t)
{
    [] <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
        (ParseTreeNode<LexerTypes, ENonTerminal>&) {}(t);
};

export template <CLexerTypes _LexerTypes, CENonTerminal _ENonTerminal, int max_prod_len = 30>
struct ProductionInfo
{
    using LexerTypes = _LexerTypes;
    using ETerminal = LexerTypes::ETerminal;
    using ENonTerminal = _ENonTerminal;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    const ENonTerminal start;
    const std::array<EParserSymbol, max_prod_len> production;
    const std::size_t size;

    template <typename StartType, typename... ProdType>
    constexpr ProductionInfo(StartType start, ProdType... production)
        : start(start), production{ production... }, size(sizeof...(production))
    {
        static_assert(sizeof...(production) > 0, "Can't have a production with no expansion.");
    }
};

export template <typename T>
concept IsProductionInfo = requires(T t)
{
    []<CLexerTypes LexerTypes, CENonTerminal ENonTerminal, int max_prod_len>
        (ProductionInfo<LexerTypes, ENonTerminal, max_prod_len>&) {}(t);
};

// Parser types
export template <typename T>
concept CParserTypes = requires()
{
    requires CLexerTypes<LexerTypes<typename T::ILexerToken>>;
    requires CLexerToken<typename T::ILexerToken>;
    requires CENonTerminal<typename T::ENonTerminal>;
    requires std::same_as<typename T::EParserSymbol, std::variant<typename T::ETerminal, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ProductionInfo, ProductionInfo<LexerTypes<typename T::ILexerToken>, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ParseTreeNode, ParseTreeNode<LexerTypes<typename T::ILexerToken>, typename T::ENonTerminal>>;
};

export template <CLexerTypes LexerTypes, CENonTerminal ENT>
struct ParserTypes
{
    using ELexerSymbol = LexerTypes::ELexerSymbol;
    using ETerminal = LexerTypes::ETerminal;
    using ELexerError = LexerTypes::ELexerError;

    using ILexerToken = LexerTypes::ILexerToken;

    using ENonTerminal = ENT;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    using ProductionInfo = ProductionInfo<LexerTypes, ENonTerminal>;
    using ParseTreeNode = ParseTreeNode<LexerTypes, ENonTerminal>;
};