export module compiler.lexer:structures;

import std;

// Forward Declarations
enum class ELexerError;

// Concepts
export template<typename T>
concept CETerminal = requires(T t)
{
    requires std::is_enum_v<T>;
    { T::eps };
    { T::TK_EOF };
};

export template <typename T>
concept CELexerSymbol = requires(T t)
{
    requires CETerminal<std::variant_alternative_t<0, T>>;
    requires std::is_same_v<ELexerError, std::variant_alternative_t<1, T>>;
};

export template <typename T>
concept CLexerToken = requires(T t, const T & u)
{
    requires CELexerSymbol<decltype(T::type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.after_construction(u) } -> std::same_as<void>;
    { t.discard() } -> std::same_as<bool>;
};

export template <typename T>
concept CLexerTypes = requires()
{
    requires std::same_as<typename T::ELexerSymbol, std::variant<typename T::ETerminal, ELexerError>>;
    requires CETerminal<typename T::ETerminal>;
    requires std::same_as<ELexerError, typename T::ELexerError>;
    requires CLexerToken<typename T::ILexerToken>;
};

// Type Definitions
export enum class ELexerError
{
    UNINITIALISED,
    ERR_SYMBOL,
    ERR_PATTERN
};

export template <CLexerToken LT>
struct LexerTypes
{
    using ELexerSymbol = decltype(LT::type);
    using ETerminal = std::variant_alternative_t<0, ELexerSymbol>;
    using ELexerError = std::variant_alternative_t<1, ELexerSymbol>;
    using ILexerToken = LT;
};

export struct TransitionInfo
{
    int from{};
    int to{};
    std::string_view pattern{};
    int default_transition_state{ -1 };
};

export template <CETerminal ETerminal>
struct FinalStateInfo
{
    int state_no;
    ETerminal token_type;
};