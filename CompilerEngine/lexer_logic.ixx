export module compiler.lexer:lexer;

import :dfa;
import :structures;
import helpers;

import std;

template <CLexerTypes LexerTypes, int num_states>
struct Lexer;

template <typename T>
concept IsLexer = requires(T t)
{
    [] <CLexerTypes LexerTypes, int num_states>(Lexer<LexerTypes, num_states>&) {}(t);
};

template <CLexerTypes LexerTypes, int num_states>
class LexerStringWrapper
{
    using ETerminal = LexerTypes::ETerminal;
    using ELexerSymbol = LexerTypes::ELexerSymbol;
    using ELexerError = LexerTypes::ELexerError;
    using ILexerToken = LexerTypes::ILexerToken;
    
    const DFA<ELexerSymbol, num_states>& dfa{};
    const std::string_view source_code{};

    struct sentinel {};
    class Iterator
    {
        const LexerStringWrapper& lexer_string;
        ILexerToken token;
        std::size_t cur_position{ 0 };
        int end_passed_count = 0;

        constexpr auto get_token_from_dfa()
        {
            auto tk = lexer_string.dfa.get_next_token<ILexerToken>(lexer_string.source_code, cur_position);

            if (!std::get_if<ELexerError>(&tk.type))
                cur_position += tk.lexeme.size();
            else if (tk.type == ELexerError::ERR_SYMBOL || tk.type == ELexerError::ERR_PATTERN)
                cur_position += 1;
            else
                throw "Guddi pakdo developer ki";

            tk.after_construction(token);

            return tk;
        }

    public:
        constexpr Iterator(const LexerStringWrapper& lsw) : lexer_string{ lsw }, cur_position{ 0 }
        {
            do
            {
                token = get_token_from_dfa();
            } while (token.discard() && token.type != ETerminal::TK_EOF);
        }
        constexpr bool operator!=(sentinel) const
        {
            // Pass atleast one TK_EOF so that parser can be sure that the input has ended
            return end_passed_count < 2;
        }
        constexpr const auto& operator++() 
        {
            do
            {
                token = get_token_from_dfa();
            } while (token.discard() && token.type != ETerminal::TK_EOF);

            end_passed_count += token.type == ETerminal::TK_EOF;
            return *this;
        }
        constexpr const auto& operator*() const
        {
            return token;
        }
    };

public:
    constexpr LexerStringWrapper(const IsLexer auto& lexer, std::string_view source_code) :
        dfa{ lexer.dfa },
        source_code{ source_code }
    {}

    constexpr auto begin() const
    {
        return Iterator(*this);
    }

    constexpr auto end() const
    {
        return sentinel{};
    }
};

export template <CLexerTypes LexerTypes, int num_states>
struct Lexer
{
    using ETerminal = LexerTypes::ETerminal;
    using ELexerSymbol = LexerTypes::ELexerSymbol;
     
    const DFA<ELexerSymbol, num_states> dfa;

    constexpr auto operator()(std::string_view source_code) const
    {
        return LexerStringWrapper<LexerTypes, num_states>(*this, source_code);
    }
};

export template <typename ostream>
constexpr ostream& operator<<(ostream& out, const IsLexer auto& lexer)
{
    out << lexer.dfa << "\n";
    return out;
}

export template <CLexerTypes LexerTypes>
constexpr auto build_lexer(auto transition_callback, auto final_states_callback)
{
    auto dfa = build_dfa<LexerTypes::ELexerSymbol>(transition_callback, final_states_callback);
   
    return Lexer<LexerTypes, dfa.state_count>{ dfa };
}