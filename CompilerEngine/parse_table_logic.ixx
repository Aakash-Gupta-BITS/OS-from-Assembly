export module compiler.parser:parse_table;

import :structures;
import compiler.lexer;
import helpers;

import std;

export template <CParserTypes ParserTypes, int max_prod_len, int num_productions>
struct ParseTable
{
    using ETerminal = ParserTypes::ETerminal;
    using ENonTerminal = ParserTypes::ENonTerminal;
    using ELexerSymbol = std::variant<ETerminal, ELexerError>;
    using EParserSymbol = ParserTypes::EParserSymbol;

    using ProductionNumber = int;

    static constexpr auto num_terminals = get_enum_size<ETerminal>();
    static constexpr auto num_non_terminals = get_enum_size<ENonTerminal>();

    std::array<ProductionInfo<LexerTypes<typename ParserTypes::ILexerToken>, ENonTerminal, max_prod_len>, num_productions> productions{};
    std::array<std::array<ProductionNumber, num_terminals>, num_non_terminals> parse_table{};
};

export template <typename T>
concept IsParseTable = requires(T t)
{
    []<CParserTypes ParserTypes, int max_prod_len, int num_productions>(
        ParseTable<ParserTypes, max_prod_len, num_productions>) { }(t);
};

export template <typename ostream>
constexpr ostream& operator<<(ostream& out, const IsParseTable auto& pt)
{
    using ETerminal = std::decay_t<decltype(pt)>::ETerminal;
    using ENonTerminal = std::decay_t<decltype(pt)>::ENonTerminal;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    out << "=================Productions and Parse Table=================\n";
    out << "Number of Non Terminals: " << get_enum_size<ENonTerminal>() << '\n';
    out << "Productions\n";
    for (auto [prod_index, prod] : pt.productions | std::views::enumerate)
    {
        out << prod_index << ". " << prod.start << " -> ";
        for (auto& x : prod.production | std::views::take(prod.size))
            out << x << " ";
    	out << '\n';
    }
    out << R"(
Parse Table:
NonTerminal :: Terminal (Production Number)
NonTerminal :: Sync Set Terminal

)";

    for (const auto &[nt_index, productions]: 
        pt.parse_table
        | std::views::enumerate)
    {
        out << (ENonTerminal)nt_index << " :: ";
        for (const auto& [terminal_index, prod_number] : 
            productions 
            | std::views::enumerate 
            | std::views::filter([](auto x) { return std::get<1>(x) > -1; }))
			out << (ETerminal)terminal_index << "(" << prod_number << ") ";

        out << "\n" << (ENonTerminal)nt_index << " :: ";
        for (const auto& terminal_index : productions
            | std::views::enumerate
            | std::views::filter([](auto x) { return std::get<1>(x) == -2; })
            | std::views::transform([](auto x) { return std::get<0>(x); }))
            out << (ETerminal)terminal_index << " ";
        out << "\n\n";
    }

    return out;
}

template <CEParserSymbol EParserSymbol>
constexpr auto get_nullable_set(const auto& productions)
{
    using ETerminal = std::variant_alternative_t<0, EParserSymbol>;
    using ENonTerminal = std::variant_alternative_t<1, EParserSymbol>;

    // Returns an array of size num_non_terminals, where ith index=true means (Nonterminal)i is nullable
    std::bitset<get_enum_size<ENonTerminal>()> nullable_set;
    nullable_set.reset();

    // T -> eps means T is nullable
    for (auto& x : productions)
        if (x.size == 1 && x.production[0] == ETerminal::eps)
            nullable_set[(int)x.start] = true;

    // Go till convergence
    for (bool changed = true; std::exchange(changed, false); )
        for (auto& x : productions
            | std::views::filter([&](auto& x) { return !nullable_set[(int)x.start]; })
            | std::views::filter([&](auto& x) { return std::ranges::none_of(
                x.production
                | std::views::take(x.size)
                | std::views::transform([&](auto& y)
                    { return std::holds_alternative<ETerminal>(y) || !nullable_set[(int)std::get<ENonTerminal>(y)]; }),
                std::identity()); })
            )
            changed = nullable_set[(int)x.start] = true;

    return nullable_set;
}

template <CEParserSymbol EParserSymbol>
constexpr auto get_first_sets(const auto& productions, const auto& nullable_set)
{
    using ETerminal = std::variant_alternative_t<0, EParserSymbol>;
    using ENonTerminal = std::variant_alternative_t<1, EParserSymbol>;

    constexpr auto num_terminals = get_enum_size<ETerminal>();
    constexpr auto num_non_terminals = get_enum_size<ENonTerminal>();

    std::array<std::bitset<num_terminals>, num_non_terminals> first_set{};

    // Add eps to first set of nullables
    for (int i = 0; i < num_non_terminals; ++i)
        if (nullable_set[i])
            first_set[i][(int)ETerminal::eps] = true;

    // Iterate untill no update
    for (bool updated = true; std::exchange(updated, false); )
        for (const auto& x : productions)
        {
            auto& bits = first_set[(int)x.start];
            auto copy = bits;

            for (auto y : x.production | std::views::take(x.size))
            {
                if (std::holds_alternative<ETerminal>(y) && y != ETerminal::eps)
                {
                    bits[(int)std::get<ETerminal>(y)] = true;
                    break;
                }

                if (y == ETerminal::eps)
                {
                    bits[(int)std::get<ETerminal>(y)] = true;
                    continue;
                }

                int index = (int)std::get<ENonTerminal>(y);
                bits |= first_set[index];

                if (!nullable_set[index])
                    break;
            }

            updated = updated || (copy != bits);
        }

    return first_set;
}

template <CEParserSymbol EParserSymbol>
constexpr auto get_follow_sets(const auto& productions, const auto& nullable_set, const auto& first_set)
{
    using ETerminal = std::variant_alternative_t<0, EParserSymbol>;
    using ENonTerminal = std::variant_alternative_t<1, EParserSymbol>;

    constexpr auto num_terminals = get_enum_size<ETerminal>();
    constexpr auto num_non_terminals = get_enum_size<ENonTerminal>();

    std::array<std::bitset<num_terminals>, num_non_terminals> follow_set{};

    for (bool updated = true; std::exchange(updated, false); )
        for (auto& x : productions)
        {
            for (int j = 0; j < x.size; ++j)
            {
                if (std::holds_alternative<ETerminal>(x.production[j]))
                    continue;

                auto& symbol = x.production[j];
                auto& bits = follow_set[(int)std::get<ENonTerminal>(symbol)];
                auto copy = bits;

                for (int k = j + 1; k < x.size; ++k)
                {
                    auto& follow_symbol = x.production[k];
                    if (std::holds_alternative<ETerminal>(follow_symbol))
                    {
                        bits[(int)std::get<ETerminal>(follow_symbol)] = true;

                        if (follow_symbol != ETerminal::eps)
                            break;
                    }
                    else
                    {
                        bits |= first_set[(int)std::get<ENonTerminal>(follow_symbol)];

                        if (!nullable_set.test((int)std::get<ENonTerminal>(follow_symbol)))
                            break;
                    }

                    if (k == x.size - 1)
                        bits |= follow_set[(int)x.start];
                }

                if (j == x.size - 1)
                    bits |= follow_set[(int)x.start];

                updated = updated || (copy != bits);
            }
        }

    // remove eps from follow set
    for (auto& follow : follow_set)
        follow[(int)ETerminal::eps] = false;

    return follow_set;
}

export constexpr auto build_parse_table(auto production_callback)
{
    constexpr auto productions = production_callback();
    using ETerminal = typename std::remove_cvref_t<decltype(*productions.begin())>::ETerminal;
    using ENonTerminal = typename std::remove_cvref_t<decltype(*productions.begin())>::ENonTerminal;
    using _LexerTypes = typename std::remove_cvref_t<decltype(*productions.begin())>::LexerTypes;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    constexpr auto num_terminals = get_enum_size<ETerminal>();
    constexpr auto num_non_terminals = get_enum_size<ENonTerminal>();

    constexpr auto nullable_set = get_nullable_set<EParserSymbol>(productions);
    constexpr auto first_set = get_first_sets<EParserSymbol>(productions, nullable_set);
    constexpr auto follow_set = get_follow_sets<EParserSymbol>(productions, nullable_set, first_set);

    constexpr auto test_nullable = [](const auto& x)
        {
            return (x == ETerminal::eps)
                || (std::holds_alternative<ENonTerminal>(x) && nullable_set[(int)std::get<ENonTerminal>(x)]);
        };
    constexpr auto get_first = [](const auto& x)
        {
            std::bitset<num_terminals> bits{};
            if (std::holds_alternative<ETerminal>(x))
                bits[(int)std::get<ETerminal>(x)] = true;
            else
                bits = first_set[(int)std::get<ENonTerminal>(x)];
            return bits;
        };

    std::array<std::bitset<num_terminals>, productions.size()> selection_sets{};
    for (int i = 0; i < productions.size(); ++i)
    {
        auto& x = productions[i];
        auto& target = selection_sets[i];
        bool all_nullable = true;

        for (auto& y : x.production | std::views::take(x.size))
        {
            target |= get_first(y);

            if (!test_nullable(y))
            {
                all_nullable = false;
                break;
            }
        }

        if (all_nullable)
            target |= follow_set[(int)x.start];

        target[(int)ETerminal::eps] = false;
    }

    ParseTable<ParserTypes<LexerTypes<typename _LexerTypes::ILexerToken>, ENonTerminal>, productions[0].production.size(), productions.size()> parser
    {
        .productions = productions,
        .parse_table = {}
    };

    for (auto& row : parser.parse_table)
        row.fill(-1);

    for (int i = 0; i < productions.size(); ++i)
    {
        auto lhs = productions[i].start;
        for (int token = 0; token < selection_sets[i].size(); ++token)
        {
            if (!selection_sets[i][token])
                continue;

            if (parser.parse_table[(int)lhs][token] != -1)
                throw std::format("There is a problem for '{}' with the terminal '{}'.", get_enum_string(lhs), get_enum_string((ETerminal)token));

            parser.parse_table[(int)lhs][token] = i;
        }
    }

    // Fill for sync set: If j is in FOLLOW(i) and (i,j) is -1, then (i,j) = -2
    for (int i = 0; i < num_non_terminals; ++i)
    {
        for (int j = 0; j < num_terminals; ++j)
        {
            if (parser.parse_table[i][j] == -1 && follow_set[i][j])
                parser.parse_table[i][j] = -2;
        }
    }

    return parser;
}