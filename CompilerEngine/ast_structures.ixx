export module compiler.ast:structures;

import helpers;
import compiler.lexer;
import compiler.parser;
import std;

export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ASTNode
{
    using ParserTypes = ParserTypes<LexerTypes, ENonTerminal>;
    using ETerminal = LexerTypes::ETerminal;
    using ILexerToken = LexerTypes::ILexerToken;
    using InternalNodeType = std::unique_ptr<ParseTreeNode<LexerTypes, ENonTerminal>>;
    using LeafType = std::unique_ptr<ILexerToken>;

    std::variant<ETerminal, ENonTerminal> node_symbol_type{};
    LeafType lexer_token{};
    std::vector<std::unique_ptr<ASTNode>> descendants{};

    constexpr ASTNode(std::variant<ETerminal, ENonTerminal> node_symbol_type)
        : node_symbol_type{ node_symbol_type }, lexer_token{ nullptr }
    {

    }

    constexpr ASTNode(LeafType leaf) :
        node_symbol_type{ std::get<ETerminal>(leaf->type) },
        lexer_token{ std::move(leaf) }
    {

    }

    constexpr ASTNode(ASTNode&& other) noexcept
		: node_symbol_type{ other.node_symbol_type },
		descendants{ std::move(other.descendants) },
        lexer_token{ std::move(other.lexer_token) }
	{

	}

    template<typename ostream>
    constexpr friend ostream& operator<<(ostream& os, const ASTNode& node)
    {
        node.print(os, 0);
        return os;
    }

private:

    template<typename ostream>
    constexpr void print(ostream& os, int depth = 0) const
    {
        for (int i = 0; i < depth; ++i) os << "\t";
        os << node_symbol_type << ": "; 
        
        if (lexer_token) 
            os << *lexer_token;
        os << '\n';
        
        for (const auto& descendant : descendants)
            if (descendant)
                descendant->print(os, depth + 1);
    }
};

export template <typename T>
concept IsASTNode = requires(T t)
{
    [] <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
        (ASTNode<LexerTypes, ENonTerminal>&) {}(t);
};