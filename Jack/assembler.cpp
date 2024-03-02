module jack.assembler;
import compiler;
import std;

using namespace std;

namespace
{
    enum class Terminal
    {
        eps,
        TK_EOF,
        TK_OB,
        TK_CB,
        TK_ASSIGN,
        TK_PLUS,
        TK_MINUS,
        TK_AND,
        TK_OR,
        TK_NOT,
        TK_AT,
        TK_SEMICOLON,
        TK_COMMENT,
        TK_WHITESPACE,
        TK_NUM,
        TK_ZERO,
        TK_ONE,
        TK_SYMBOL,
        TK_M,
        TK_D,
        TK_DM,
        TK_A,
        TK_AM,
        TK_AD,
        TK_ADM,
        TK_JGT,
        TK_JEQ,
        TK_JGE,
        TK_JLT,
        TK_JNE,
        TK_JLE,
        TK_JMP,
        TK_SP,
        TK_LCL,
        TK_ARG,
        TK_THIS,
        TK_THAT,
        TK_REG,
        TK_SCREEN,
        TK_KBD
    };

    enum class NonTerminal
    {
        start,
        
        empty,
        at,
        symbol,
        c_type,

        failure_invalid_instruction,
        failure_symbol_not_found,
        failure_symbol_repeat
    };

    struct LexerToken
    {
        std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
        std::string_view lexeme{};
        int line_number = 1;

        constexpr void after_construction(const LexerToken& previous_token)
        {
            line_number = previous_token.line_number +
                (int)std::count(previous_token.lexeme.begin(), previous_token.lexeme.end(), '\n');

            if (type == Terminal::TK_NUM)
            {
                if (lexeme == "0")
                    type = Terminal::TK_ZERO;
                else if (lexeme == "1")
                    type = Terminal::TK_ONE;
            }

            if (type != Terminal::TK_SYMBOL)
                return;

            if (lexeme.size() == 1)
            {
                if (lexeme[0] == 'M')
                    type = Terminal::TK_M;
                else if (lexeme[0] == 'D')
                    type = Terminal::TK_D;
                else if (lexeme[0] == 'A')
                    type = Terminal::TK_A;
            }
            else if (lexeme.size() == 2)
            {
                if (lexeme == "DM")
                    type = Terminal::TK_DM;
                else if (lexeme == "AM")
                    type = Terminal::TK_AM;
                else if (lexeme == "AD")
                    type = Terminal::TK_AD;
                else if (lexeme == "SP")
                    type = Terminal::TK_SP;
                else if (lexeme[0] == 'R' && lexeme[1] >= '0' && lexeme[1] <= '9')
                    type = Terminal::TK_REG;
            }
            else if (lexeme.size() == 3)
            {
                if (lexeme == "ADM")
                    type = Terminal::TK_ADM;
                else if (lexeme == "ARG")
                    type = Terminal::TK_ARG;
                else if (lexeme[0] == 'J')
                {
                    if (lexeme == "JGT")
                        type = Terminal::TK_JGT;
                    else if (lexeme == "JEQ")
                        type = Terminal::TK_JEQ;
                    else if (lexeme == "JGE")
                        type = Terminal::TK_JGE;
                    else if (lexeme == "JLT")
                        type = Terminal::TK_JLT;
                    else if (lexeme == "JNE")
                        type = Terminal::TK_JNE;
                    else if (lexeme == "JLE")
                        type = Terminal::TK_JLE;
                    else if (lexeme == "JMP")
                        type = Terminal::TK_JMP;
                }
                else if (lexeme[0] == 'R' && lexeme[1] == '1' && lexeme[2] >= '0' && lexeme[2] <= '5')
                    type = Terminal::TK_REG;
                else if (lexeme == "LCL")
                    type = Terminal::TK_LCL;
                else if (lexeme == "KBD")
                    type = Terminal::TK_KBD;
            }
            else if (lexeme == "THIS")
                type = Terminal::TK_THIS;
            else if (lexeme == "THAT")
                type = Terminal::TK_THAT;
            else if (lexeme == "SCREEN")
                type = Terminal::TK_SCREEN;
        }

        constexpr bool discard() const
        {
            return type == Terminal::TK_COMMENT || type == Terminal::TK_WHITESPACE;
        }

        constexpr auto copy_to_unique_ptr() const
        {
            return std::make_unique<LexerToken>(*this);
        }
        
        template <typename T>
        friend constexpr T& operator<<(T& out, const LexerToken& tk)
        {
            return out 
                << "{ line_number: " 
                << tk.line_number
                << ", type: " 
                << tk.type 
                << ", lexeme : "
                << tk.lexeme 
                << " }";
        }
    };

    using ASTType = ASTNode<LexerTypes<LexerToken>, NonTerminal>;

    consteval auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return std::array
                {
                    TransitionInfo{0, 1, "("},
                    TransitionInfo{0, 2, ")"},
                    TransitionInfo{0, 3, "="},
                    TransitionInfo{0, 4, "+"},
                    TransitionInfo{0, 5, "-"},
                    TransitionInfo{0, 6, "&"},
                    TransitionInfo{0, 7, "|"},
                    TransitionInfo{0, 8, "!"},
                    TransitionInfo{0, 9, "@"},
                    TransitionInfo{0, 10, ";"},
                    TransitionInfo{0, 11, "/"},
                    TransitionInfo{11, 12, "/"},
                    TransitionInfo{.from = 12, .to = -1, .pattern = "\r\n", .default_transition_state = 12},
                    TransitionInfo{0, 13, " \r\n\t"},
                    TransitionInfo{13, 13, " \r\n\t"},
                    TransitionInfo{0, 14, "0123456789"},
                    TransitionInfo{14, 14, "0123456789"},
                    TransitionInfo{0, 15, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:"},
                    TransitionInfo{15, 15, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:0123456789*/"},
                };
            };

        constexpr auto final_states = []()
            {
                return std::array
                {
                    FinalStateInfo{1, TK_OB},
                    FinalStateInfo{2, TK_CB},
                    FinalStateInfo{3, TK_ASSIGN},
                    FinalStateInfo{4, TK_PLUS},
                    FinalStateInfo{5, TK_MINUS},
                    FinalStateInfo{6, TK_AND},
                    FinalStateInfo{7, TK_OR},
                    FinalStateInfo{8, TK_NOT},
                    FinalStateInfo{9, TK_AT},
                    FinalStateInfo{10, TK_SEMICOLON},
                    FinalStateInfo{12, TK_COMMENT},
                    FinalStateInfo{13, TK_WHITESPACE},
                    FinalStateInfo{14, TK_NUM},
                    FinalStateInfo{15, TK_SYMBOL}
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }

    constexpr auto get_tokens(auto file_content)
    {
        constexpr auto lexer = get_lexer();
        std::vector<std::vector<LexerToken>> tokens;

        int line_number = 0;
        for (const auto& token : lexer(file_content))
        {
            int line_diff = token.line_number - line_number;
            while (line_diff-- > 0)
                tokens.push_back({});

            line_number = token.line_number;
            tokens.back().push_back(token);
        }

        tokens.back().pop_back();
        if (tokens.back().empty())
            tokens.pop_back();

        return tokens;
    }

    constexpr auto get_dest_ast(auto begin, auto end)
    {
        using enum Terminal;
        using enum NonTerminal;

        constexpr std::array<Terminal, 7> valid_dest = { TK_M, TK_D, TK_A, TK_DM, TK_AM, TK_AD, TK_ADM };

        if (begin == end)
            return std::make_unique<ASTType>(empty);

        if (begin + 1 != end)
			return std::make_unique<ASTType>(failure_invalid_instruction);

        if (std::find_if(valid_dest.begin(), valid_dest.end(), [&](const auto& tk) { return begin->type == tk; }) == valid_dest.end())
            return std::make_unique<ASTType>(failure_invalid_instruction);

        return std::make_unique<ASTType>(begin->copy_to_unique_ptr());
    }

    constexpr auto get_comp_ast(auto begin, auto end)
    {
        using enum Terminal;
        using enum NonTerminal;

        if (begin == end)
            return std::make_unique<ASTType>(failure_invalid_instruction);

        if (end - begin > 3)
            return std::make_unique<ASTType>(failure_invalid_instruction);

        constexpr std::array valid_comp = {
            "0", "1", "-1",
            "D", "A", "M",
            "!D", "!A", "!M",
            "-D", "-A", "-M",
            "D+1", "A+1", "M+1",
            "D-1", "A-1", "M-1",
            "D+A", "D+M", "D-A", "D-M", "A-D", "M-D",
            "D&A", "D&M", "D|A", "D|M"
        };

        std::string comp_lexeme;
        for (auto it = begin; it != end; ++it)
            comp_lexeme += it->lexeme;

        if (std::find(valid_comp.begin(), valid_comp.end(), comp_lexeme) == valid_comp.end())
            return std::make_unique<ASTType>(failure_invalid_instruction);

        if (begin + 1 == end)
            return std::make_unique<ASTType>(begin->copy_to_unique_ptr());

        if (begin + 2 == end)
        {
			auto node = std::make_unique<ASTType>(begin->copy_to_unique_ptr());
			node->descendants.push_back(std::make_unique<ASTType>((begin + 1)->copy_to_unique_ptr()));
			return std::move(node);
		}

        auto node = std::make_unique<ASTType>((begin + 1)->copy_to_unique_ptr());
        node->descendants.push_back(std::make_unique<ASTType>((begin)->copy_to_unique_ptr()));
        node->descendants.push_back(std::make_unique<ASTType>((begin + 2)->copy_to_unique_ptr()));
        return std::move(node);
    }

    constexpr auto get_jump_ast(auto begin, auto end)
    {
        using enum Terminal;
        using enum NonTerminal;

        if (begin == end)
            return std::make_unique<ASTType>(empty);

        if (begin + 1 != end)
            return std::make_unique<ASTType>(failure_invalid_instruction);

        constexpr std::array valid_jmp = {
            TK_JMP,
            TK_JGT,
            TK_JEQ,
            TK_JGE,
            TK_JLT,
            TK_JNE,
            TK_JLE
        };

        auto type = begin->type;

        if (std::find(valid_jmp.begin(), valid_jmp.end(), type) == valid_jmp.end())
            return std::make_unique<ASTType>(failure_invalid_instruction);

        return std::make_unique<ASTType>(begin->copy_to_unique_ptr());
    }

    constexpr auto get_ast_node_for_line(std::vector<LexerToken> line_tokens)
    {
        using ASTType = ASTNode<LexerTypes<LexerToken>, NonTerminal>;
        using enum Terminal;
        using enum NonTerminal;

        if (line_tokens.empty())
            return std::make_unique<ASTType>(empty);

        auto fail_node = std::make_unique<ASTType>(failure_invalid_instruction);

        if (line_tokens[0].type == TK_AT)
        {
            if (line_tokens.size() != 2)
                return std::move(fail_node);

            auto& token_type = line_tokens[1].type;
            if (token_type == TK_ZERO || token_type == TK_ONE)
                token_type = TK_NUM;

            const std::vector valid_types = { TK_NUM, TK_SYMBOL, TK_REG, TK_SP, TK_LCL, TK_ARG, TK_THIS, TK_THAT, TK_SCREEN, TK_KBD };

            if (std::find(valid_types.begin(), valid_types.end(), token_type) == valid_types.end())
                return std::move(fail_node);

            auto node = std::make_unique<ASTType>(at);
            node->descendants.emplace_back(std::make_unique<ASTType>(line_tokens[1].copy_to_unique_ptr()));
            return std::move(node);
        }

        if (line_tokens[0].type == TK_OB)
        {
            if (line_tokens.size() != 3 || line_tokens[1].type != TK_SYMBOL || line_tokens[2].type != TK_CB)
                return std::move(fail_node);

            auto node = std::make_unique<ASTType>(symbol);
            node->descendants.push_back(std::make_unique<ASTType>(line_tokens[1].copy_to_unique_ptr()));
            return std::move(node);
        }

        // Must be either = or ;, but not both
        std::size_t eq_count = 0;
        std::size_t semi_count = 0;
        for (int i = 0; i < line_tokens.size(); ++i)
        {
            eq_count += line_tokens[i].type == TK_ASSIGN;
            semi_count += line_tokens[i].type == TK_SEMICOLON;
        }

        if (!((eq_count == 1) ^ (semi_count == 1)))
            return std::move(fail_node);

        if (line_tokens.size() < 3)
            return std::move(fail_node);

        auto node = std::make_unique<ASTType>(c_type);

        auto comp_end = std::find_if(line_tokens.begin(), line_tokens.end(), [](const auto& tk) { return tk.type == TK_SEMICOLON; });
        auto dest_node = get_dest_ast(line_tokens.begin(), line_tokens.begin() + eq_count);
        auto comp_node = get_comp_ast(line_tokens.begin() + (2 * eq_count), comp_end);
        auto jump_node = get_jump_ast(comp_end + semi_count, line_tokens.end());

        if (dest_node->node_symbol_type == failure_invalid_instruction ||
            comp_node->node_symbol_type == failure_invalid_instruction ||
            jump_node->node_symbol_type == failure_invalid_instruction)
			return std::move(fail_node);

        if (eq_count == 1 && dest_node->node_symbol_type == empty)
            return std::move(fail_node);
        else if (semi_count == 1 && jump_node->node_symbol_type == empty)
            return std::move(fail_node);

        node->descendants.push_back(std::move(dest_node));
        node->descendants.push_back(std::move(comp_node));
        node->descendants.push_back(std::move(jump_node));

		return std::move(node);
    }
}

constexpr auto jack::assembler::generate_binary(std::string_view file_content) -> std::expected<std::vector<std::uint16_t>, std::string>
{
    const auto tokens = get_tokens(file_content);

    constexpr_ostream errors;
    vector<pair<string_view, std::uint16_t>> symbols, ram_bindings;
    int RAM_INDEX = 16;

    auto root = std::make_unique<ASTType>(NonTerminal::start);
    for (const auto& [line_number, line_tokens] : tokens | std::views::enumerate)
    {
        root->descendants.push_back(get_ast_node_for_line(line_tokens));
        const auto& ast = root->descendants.back();

        if (ast->node_symbol_type == NonTerminal::empty)
            continue;

        if (ast->node_symbol_type == NonTerminal::failure_invalid_instruction)
        {
            errors << "Invalid instruction on line " << line_number + 1 << ": '";
            for (const auto& token : line_tokens)
                errors << token.lexeme << " ";
            errors << "'\n";

			continue;
		}

        if (ast->node_symbol_type == NonTerminal::at)
            continue;

        if (ast->node_symbol_type == NonTerminal::symbol)
        {
            // this should be the first time we've seen this symbol
            if (std::find_if(symbols.begin(), symbols.end(), [&](const auto& sym) { return sym.first == line_tokens[1].lexeme; }) != symbols.end())
            {
				errors << "Symbol '" << line_tokens[1].lexeme << "' repeated on line " << line_number + 1 << "\n";
				continue;
			}

            symbols.push_back({ line_tokens[1].lexeme, (std::uint16_t)(line_number) });
			continue;
        }
    }

    std::vector<std::uint16_t> binary;
    constexpr std::uint16_t nop = 0xFFFF;

    if (!errors.sv().empty())
        return std::unexpected(std::move(errors.str()));

    for (const auto& ast : root->descendants)
    {
        // No 'NonTerminal::failure_invalid_instruction' is here
        if (ast->node_symbol_type == NonTerminal::empty)
        {
            binary.push_back(nop);
			continue;
        }

        if (ast->node_symbol_type == NonTerminal::at)
        {
            auto &symbol = ast->descendants[0];

            if (symbol->node_symbol_type == Terminal::TK_NUM)
            {
                std::uint16_t num{};
                std::from_chars(symbol->lexer_token->lexeme.data(), symbol->lexer_token->lexeme.data() + symbol->lexer_token->lexeme.size(), num);
                num &= 0x7FFF;

                constexpr_ostream oss;
                oss << num;
                if (symbol->lexer_token->lexeme == oss.sv())
                {
                    binary.push_back(num);
                    continue;
				}

                binary.push_back(nop);
                errors << "Invalid number on line " << symbol->lexer_token->line_number << ": " << symbol->lexer_token->lexeme << "\n";
                continue;
            }

            if (symbol->node_symbol_type == Terminal::TK_SYMBOL)
            {
                const auto symbol_location = std::find_if(symbols.begin(), symbols.end(), [&](const auto& sym) { return sym.first == symbol->lexer_token->lexeme; });
                if (symbol_location != symbols.end())
                {
                    binary.push_back(symbol_location->second);
					continue;
                }

                const auto ram_location = std::find_if(ram_bindings.begin(), ram_bindings.end(), [&](const auto& sym) { return sym.first == symbol->lexer_token->lexeme; });
                if (ram_location != ram_bindings.end())
                {
					binary.push_back(ram_location->second);
                    continue;
                }

                ram_bindings.push_back({ symbol->lexer_token->lexeme, RAM_INDEX });
                binary.push_back(RAM_INDEX++);
                continue;
            }
            
            if (symbol->node_symbol_type == Terminal::TK_SP)
                binary.push_back(0);
            else if (symbol->node_symbol_type == Terminal::TK_LCL)
                binary.push_back(1);
			else if (symbol->node_symbol_type == Terminal::TK_ARG)
				binary.push_back(2);
			else if (symbol->node_symbol_type == Terminal::TK_THIS)
				binary.push_back(3);
			else if (symbol->node_symbol_type == Terminal::TK_THAT)
				binary.push_back(4);
            else if (symbol->node_symbol_type == Terminal::TK_REG)
            {
				std::uint16_t num{};
				std::from_chars(symbol->lexer_token->lexeme.data() + 1, symbol->lexer_token->lexeme.data() + symbol->lexer_token->lexeme.size(), num);
				num &= 0x7FFF;

				binary.push_back(num);
			}
			else if (symbol->node_symbol_type == Terminal::TK_SCREEN)
				binary.push_back(0x4000);
			else if (symbol->node_symbol_type == Terminal::TK_KBD)
				binary.push_back(0x6000);
            else
            {
                errors << "Invalid symbol on line " << symbol->lexer_token->line_number << ": " << symbol->lexer_token->lexeme << "\n";
                binary.push_back(nop);
            }
            continue;
        }
    
        if (ast->node_symbol_type == NonTerminal::symbol)
        {
			binary.push_back(nop);
			continue;
		}

        std::bitset<3> dest{};
        std::bitset<7> comp{};
        std::bitset<3> jump{};

        if (ast->descendants[0]->node_symbol_type != NonTerminal::empty)
        {
            const auto& dest_node = ast->descendants[0];
            if (dest_node->node_symbol_type == Terminal::TK_M)
                dest = 0b001;
            else if (dest_node->node_symbol_type == Terminal::TK_D)
                dest = 0b010;
            else if (dest_node->node_symbol_type == Terminal::TK_A)
                dest = 0b100;
			else if (dest_node->node_symbol_type == Terminal::TK_DM)
				dest = 0b011;
			else if (dest_node->node_symbol_type == Terminal::TK_AM)
				dest = 0b101;
			else if (dest_node->node_symbol_type == Terminal::TK_AD)
				dest = 0b110;
			else if (dest_node->node_symbol_type == Terminal::TK_ADM)
				dest = 0b111;
            else
                std::terminate();
        }

        if (ast->descendants[2]->node_symbol_type != NonTerminal::empty)
        {
            const auto& jump_node = ast->descendants[2];
            if (jump_node->node_symbol_type == Terminal::TK_JGT)
                jump = 0b001;
            else if (jump_node->node_symbol_type == Terminal::TK_JEQ)
                jump = 0b010;
            else if (jump_node->node_symbol_type == Terminal::TK_JGE)
                jump = 0b011;
            else if (jump_node->node_symbol_type == Terminal::TK_JLT)
                jump = 0b100;
            else if (jump_node->node_symbol_type == Terminal::TK_JNE)
                jump = 0b101;
            else if (jump_node->node_symbol_type == Terminal::TK_JLE)
                jump = 0b110;
            else if (jump_node->node_symbol_type == Terminal::TK_JMP)
                jump = 0b111;
            else
                std::terminate();
        }
    
        string comp_lexeme = "";
        const auto& comp_node = ast->descendants[1];
        if (comp_node->descendants.size() == 1)
            comp_lexeme = std::string(comp_node->lexer_token->lexeme) + std::string(comp_node->descendants[0]->lexer_token->lexeme);
        else if (comp_node->descendants.size() == 2)
            comp_lexeme = std::string(comp_node->descendants[0]->lexer_token->lexeme) + std::string(comp_node->lexer_token->lexeme) + std::string(comp_node->descendants[1]->lexer_token->lexeme);
        else
            comp_lexeme = comp_node->lexer_token->lexeme;

        if (comp_lexeme == "0")
            comp = 0b0101010;
		else if (comp_lexeme == "1")
			comp = 0b0111111;
		else if (comp_lexeme == "-1")
			comp = 0b0111010;
		else if (comp_lexeme == "D")
			comp = 0b0001100;
		else if (comp_lexeme == "A")
			comp = 0b0110000;
        else if (comp_lexeme == "M")
            comp = 0b1110000;
        else if (comp_lexeme == "!D")
            comp = 0b0001101;
		else if (comp_lexeme == "!A")
			comp = 0b0110001;
		else if (comp_lexeme == "!M")
			comp = 0b1110001;
		else if (comp_lexeme == "-D")
			comp = 0b0001111;
		else if (comp_lexeme == "-A")
			comp = 0b0110011;
		else if (comp_lexeme == "-M")
			comp = 0b1110011;
		else if (comp_lexeme == "D+1")
			comp = 0b0011111;
		else if (comp_lexeme == "A+1")
			comp = 0b0110111;
		else if (comp_lexeme == "M+1")
			comp = 0b1110111;
		else if (comp_lexeme == "D-1")
			comp = 0b0001110;
		else if (comp_lexeme == "A-1")
			comp = 0b0110010;
		else if (comp_lexeme == "M-1")
			comp = 0b1110010;
		else if (comp_lexeme == "D+A")
			comp = 0b0000010;
		else if (comp_lexeme == "D+M")
			comp = 0b1000010;
		else if (comp_lexeme == "D-A")
			comp = 0b0010011;
		else if (comp_lexeme == "D-M")
			comp = 0b1010011;
		else if (comp_lexeme == "A-D")
			comp = 0b0000111;
		else if (comp_lexeme == "M-D")
			comp = 0b1000111;
		else if (comp_lexeme == "D&A")
			comp = 0b0000000;
		else if (comp_lexeme == "D&M")
			comp = 0b1000000;
		else if (comp_lexeme == "D|A")
			comp = 0b0010101;
		else if (comp_lexeme == "D|M")
			comp = 0b1010101;
		else
			std::terminate();

        std::uint16_t instruction = 0b111 << 13;
		instruction |= comp.to_ulong() << 6;
		instruction |= dest.to_ulong() << 3;
		instruction |= jump.to_ulong();

		binary.push_back(instruction);
    }

    if (!errors.sv().empty())
        return std::unexpected(std::move(errors.str()));

    return binary;
}