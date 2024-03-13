module jack.vm:backend;

import :frontend;
import std;
import compiler;
import helpers;

namespace
{
    std::string type_check(std::string_view file_name, const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ast_root)
    {
        // 1. There should be Main.main, which accepts 0 arguments
        // 2. There should be no function named Sys.init
        // 3. There should be no function with repeated name
        // 4. Since forward function calls are allowed, we perform two passes
        constexpr_ostream error_stream;

        // Gather all function names and their number of arguments
        std::map<std::string_view, LexerToken> function_dictionary;
        for (auto& function_entry : ast_root->descendants)
        {
            const auto& function_name = function_entry->descendants[0]->lexer_token;
            const auto& nArgs = function_entry->descendants[1]->lexer_token;

            const auto& existing_fn_entry = function_dictionary.find(function_name->lexeme);
            if (existing_fn_entry != function_dictionary.end())
            {
                error_stream << file_name << " (" << function_name->line_number << "): Function " << function_name << " is already defined at line " << existing_fn_entry->second.line_number << '\n';
                continue;
            }

            function_dictionary[function_name->lexeme] = *nArgs;
        }

        // Function level checks
        if (const auto& main_entry = function_dictionary.find("Main.main"); main_entry == function_dictionary.end())
            error_stream << "Function Main.main is not defined\n";
        else if (main_entry->second.lexeme != "0")
            error_stream << file_name << " (" << main_entry->second.line_number << "): Function Main.main should accept 0 arguments\n";

        if (const auto& sys_entry = function_dictionary.find("Sys.init"); sys_entry != function_dictionary.end())
            error_stream << file_name << " (" << sys_entry->second.line_number << "): Function Sys.init is not allowed in user code.\n";

        // Go inside each function
        for (const auto& function_entry : ast_root->descendants)
        {
            const auto& function_name = function_entry->descendants[0]->lexer_token;
            const auto& nArgs = function_entry->descendants[1]->lexer_token;

            std::set<std::string_view> local_labels{};

            for (const auto& operation : function_entry->descendants[2]->descendants)
            {
                const auto& line_number = operation->lexer_token->line_number;

                if (operation->node_symbol_type == Terminal::TK_LABEL)
                {
                    if (const auto& symbol = operation->descendants[0]->lexer_token->lexeme; local_labels.contains(symbol))
                        error_stream << file_name << " (" << line_number << "): Label " << symbol << " is already defined inside function " << function_name;
                    else if (symbol.starts_with("ALU_COMPARE_"))
                        error_stream << file_name << " (" << line_number << "): User defined symbol name must not start with 'ALU_COMPARE_'\n";
                    else if (symbol.starts_with("FUNC_CALL_"))
                        error_stream << file_name << " (" << line_number << "): User defined symbol name must not start with 'FUNC_CALL_'\n";
                    else
                        local_labels.insert(symbol);
                }
                else if (operation->node_symbol_type == Terminal::TK_CALL)
                {
                    // Calls must match existing function names
                    // Function calls must match the number of arguments
                    const auto& function_call_name = operation->descendants[0]->lexer_token->lexeme;
                    const auto& nArgs = operation->descendants[1]->lexer_token->lexeme;

                    if (const auto& ref = function_dictionary.find(function_call_name); ref == function_dictionary.end())
                        error_stream << file_name << " (" << line_number << "): Function " << function_call_name << " is not defined\n";
                    else if (ref->second.lexeme != nArgs)
                        error_stream << file_name << " (" << line_number << "): Function " << function_call_name << " should accept " << ref->second.lexeme << " arguments\n";
                }
                else if (operation->node_symbol_type == Terminal::TK_PUSH || operation->node_symbol_type == Terminal::TK_POP)
                {
                    const auto& segment = operation->descendants[0];
                    const auto& index = operation->descendants[1]->lexer_token->lexeme;

                    if (segment->node_symbol_type == Terminal::TK_CONSTANT && operation->node_symbol_type == Terminal::TK_POP)
                        error_stream << file_name << " (" << line_number << "): Popping a constant is invalid operation: pop constant " << index << "\n";
                    else if (segment->node_symbol_type == Terminal::TK_TEMP && !(index.size() == 1 && index[0] >= '0' && index[0] <= '7'))
                        error_stream << file_name << " (" << line_number << "): temp segment should have index in range [0, 7]: " << operation->node_symbol_type << " temp " << index << "\n";
                    else if (segment->node_symbol_type == Terminal::TK_POINTER && !(index == "0" || index == "1"))
                        error_stream << file_name << " (" << line_number << "): pointer segment should have index either 0 or 1: " << operation->node_symbol_type << " pointer " << index << "\n";
                }
            }

            // second pass for branch instructions for valid local names
            for (const auto& operation : function_entry->descendants[2]->descendants)
            {
                if (operation->node_symbol_type != Terminal::TK_GOTO && operation->node_symbol_type != Terminal::TK_IF)
                    continue;

                const auto& label = operation->descendants[0]->lexer_token->lexeme;
                const auto& line_number = operation->lexer_token->line_number;

                if (!local_labels.contains(label))
                    error_stream << file_name << " (" << line_number << "): Label " << label << " is not defined inside function " << function_name;
            }
        }

        return error_stream.str();
    }
}

namespace
{
    void stack_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, std::string_view file_name, auto& ostream)
    {
        const auto stack_type = ins->node_symbol_type;
        const auto segment_type = ins->descendants[0]->node_symbol_type;

        ostream << "\n  // " << ins->lexer_token->lexeme << " " << ins->descendants[0]->lexer_token->lexeme << " " << ins->descendants[1]->lexer_token->lexeme << "\n";

        if (segment_type == Terminal::TK_CONSTANT)
        {
            ostream << "  @" << ins->descendants[1]->lexer_token->lexeme << "\n";
            ostream << "  D = A\n";
            ostream << "  @SP  // RAM[SP] = D\n";
            ostream << "  A = M\n";
            ostream << "  M = D\n";
            ostream << "  @SP  // SP++\n";
            ostream << "  M = M + 1\n";
            return;
        }

        auto segment_display_name =
            segment_type == Terminal::TK_LOCAL ? std::string("LCL") :
            segment_type == Terminal::TK_ARGUMENT ? std::string("ARG") :
            segment_type == Terminal::TK_THIS ? std::string("THIS") :
            segment_type == Terminal::TK_THAT ? std::string("THAT") :
            segment_type == Terminal::TK_TEMP ? std::string("R") :
            segment_type == Terminal::TK_STATIC ? (std::string(file_name) + std::string(".") + std::string(ins->descendants[1]->lexer_token->lexeme)) :
            segment_type != Terminal::TK_POINTER ? std::string("") :
            ins->descendants[1]->lexer_token->lexeme == "0" ? std::string("THIS") : std::string("THAT");

        if (segment_display_name == "")
            std::terminate();

        if (segment_type == Terminal::TK_TEMP)
        {
            std::stringstream ss;
            ss << ins->descendants[1]->lexer_token->lexeme;
            int temp_index;
            ss >> temp_index;
            segment_display_name += std::to_string(5 + temp_index);
        }

        ostream << "  @" << segment_display_name << "  // addr <- " << segment_type << " + " << ins->descendants[1]->lexer_token->lexeme << "\n";

        if (segment_type != Terminal::TK_POINTER)
        {
            ostream << "  D = " << ((segment_type == Terminal::TK_STATIC || segment_type == Terminal::TK_TEMP) ? "A" : "M") << "\n";
            ostream << "  @" << ins->descendants[1]->lexer_token->lexeme << "\n";
        }
        else
            ostream << "  D = 0\n";

        // address = D + A
        if (stack_type == Terminal::TK_PUSH)
        {
            ostream << "  A = D + A\n";
            ostream << "  D = M\n";
            ostream << "  @SP  // SP++\n";
            ostream << "  M = M + 1\n";
            ostream << "  A = M  // RAM[SP - 1] <- RAM[addr]\n";
            ostream << "  A = A - 1\n";
            ostream << "  M = D\n";
        }
        else if (stack_type == Terminal::TK_POP)
        {
            ostream << "  D = D + A\n";
            ostream << "  @R15\n";
            ostream << "  M = D\n";
            ostream << "  @SP  // SP--\n";
            ostream << "  M = M - 1\n";
            ostream << "  A = M  // RAM[addr] <- RAM[SP]\n";
            ostream << "  D = M\n";
            ostream << "  @R15\n";
            ostream << "  A = M\n";
            ostream << "  M = D\n";
        }
        else
            std::terminate();
    }

    void arithmetic_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        const auto arithmetic_type = ins->node_symbol_type;

        ostream << "\n  // " << ins->lexer_token->lexeme << "\n";
        ostream << "  @SP\n";

        if (arithmetic_type == Terminal::TK_NEG)
        {
            ostream << "  A = M - 1\n";
            ostream << "  M = -M\n";
            return;
        }

        ostream << "  AM = M - 1\n";
        ostream << "  D = M\n";
        ostream << "  A = A - 1\n";

        if (arithmetic_type == Terminal::TK_ADD)
            ostream << "  M = D + M\n";
        else if (arithmetic_type == Terminal::TK_SUB)
            ostream << "  M = M - D\n";
        else
            std::terminate();
    }

    void logical_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        static std::size_t counter = 1;

        const auto logical_type = ins->node_symbol_type;
        ostream << "\n  // " << ins->lexer_token->lexeme << "\n";
        ostream << "  @SP  // Load values\n";
        ostream << "  AM = M - 1\n";
        ostream << "  D = M\n";
        ostream << "  A = A - 1\n";
        ostream << "  D = D - M\n";
        ostream << "  @ALU_COMPARE_" << counter << "_ELSE  // Store in D\n";
        ostream << "  D; J" << (logical_type == Terminal::TK_EQ ? "N" : logical_type == Terminal::TK_LT ? "G" : "L") << "E\n";
        ostream << "  D = 1\n";
        ostream << "  @ALU_COMPARE_" << counter << "_FINISH\n";
        ostream << "  0; JMP\n";
        ostream << "(ALU_COMPARE_" << counter << "_ELSE)\n";
        ostream << "  D = 0\n";
        ostream << "(ALU_COMPARE_" << counter++ << "_FINISH)\n";
        ostream << "  @SP  // Store result\n";
        ostream << "  A = M - 1\n";
        ostream << "  M = D\n";
    }

    void boolean_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        const auto boolean_type = ins->node_symbol_type;

        ostream << "\n  // " << ins->lexer_token->lexeme << "\n";
        ostream << "  @SP\n";
        if (boolean_type == Terminal::TK_NOT)
        {
            ostream << "  A = M - 1\n";
            ostream << "  D = 1\n";
            ostream << "  M = D - M\n";
            return;
        }

        ostream << "  AM = M - 1\n";
        ostream << "  D = M\n";
        ostream << "  A = A - 1\n";

        ostream << (boolean_type == Terminal::TK_AND ? "  M = D & M\n" : "  M = D | M\n");
    }

    void branch_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        const auto branch_type = ins->node_symbol_type;

        ostream << "\n  // " << ins->lexer_token->lexeme;
        ostream << " " << ins->descendants[0]->lexer_token->lexeme << "\n";

        if (branch_type == Terminal::TK_LABEL)
            ostream << "(" << ins->descendants[0]->lexer_token->lexeme << ")\n";
        else if (branch_type == Terminal::TK_GOTO)
            ostream << "  @" << ins->descendants[0]->lexer_token->lexeme << "\n  0; JMP\n";
        else if (branch_type == Terminal::TK_IF)
        {
            ostream << "  @SP\n";
            ostream << "  AM = M - 1\n";
            ostream << "  D = M\n";
            ostream << "  @" << ins->descendants[0]->lexer_token->lexeme << "\n";
            ostream << "  D; JNE\n";
        }
        else
            std::terminate();
    }

    void call_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        static std::size_t counter = 1;

        const auto target = ins->descendants[0]->lexer_token->lexeme;
        const auto ret_label = std::string("FUNC_CALL_") + std::string(target) + std::string("_") + std::to_string(counter++) + std::string("_RET");
        const std::array<std::string, 5> push_targets = { ret_label, "LCL", "ARG", "THIS", "THAT" };
        auto nArgs = std::stoi(ins->descendants[1]->lexer_token->lexeme.data());

        ostream << "\n  // call " << target << " " << ins->descendants[1]->lexer_token->lexeme << "\n";

        if (nArgs == 0)
        {
            nArgs = 1;
            // simulate push constant 0
            ostream << "  @0\n";
            ostream << "  D = A\n";
            ostream << "  @SP  // RAM[SP] = D\n";
            ostream << "  A = M\n";
            ostream << "  M = D\n";
            ostream << "  @SP  // SP++\n";
            ostream << "  M = M + 1\n";
        }

        for (auto& x : push_targets)
        {
            ostream << "  @" << x << "  // push " << x << "\n";
            ostream << (x == ret_label ? "  D = A\n" : "  D = M\n");
            ostream << "  @SP\n";
            ostream << "  AM = M + 1\n";
            ostream << "  A = A - 1\n";
            ostream << "  M = D\n";
        }

        ostream << "  @SP  // LCL = SP\n";
        ostream << "  D = M\n";
        ostream << "  @LCL\n";
        ostream << "  M = D\n";

        ostream << "  @" << nArgs + 5 << "  // ARG = SP - nArgs - 5\n";
        ostream << "  D = D - A\n";
        ostream << "  @ARG\n";
        ostream << "  M = D\n";

        ostream << "  @" << target << "  // JUMP to function\n";
        ostream << "  0; JMP\n";
        ostream << "(" << ret_label << ")\n";
    }

    void function_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        const auto function_name = ins->descendants[0]->lexer_token->lexeme;
        const auto nArgs = std::stoi(ins->descendants[1]->lexer_token->lexeme.data());

        ostream << "\n  // function " << function_name << " " << nArgs << "\n";
        ostream << "(" << function_name << ")\n";
    }

    void return_writer(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ins, auto& ostream)
    {
        ostream << "\n  // return\n";
        ostream << "  @SP  // RAM[ARG] <- RAM[SP - 1]\n";
        ostream << "  A = M - 1\n";
        ostream << "  D = M\n";
        ostream << "  @ARG\n";
        ostream << "  A = M\n";
        ostream << "  M = D\n";
        ostream << "  D = A + 1  // SP = ARG + 1\n";
        ostream << "  @SP\n";
        ostream << "  M = D\n";

        ostream << "  @LCL  // R15 <- return address\n";
        ostream << "  D = M\n";
        ostream << "  @5\n";
        ostream << "  A = D - A\n";
        ostream << "  D = M\n";
        ostream << "  @R15\n";
        ostream << "  M = D\n";

        std::array pop_targets = { "THAT", "THIS", "ARG", "LCL" };
        for (auto& target : pop_targets)
        {
            ostream << "  @LCL  // pop " << target << " using LCL\n";
            ostream << "  AM = M - 1\n";
            ostream << "  D = M\n";
            ostream << "  @" << target << "\n";
            ostream << "  M = D\n";
        }

        ostream << "  @R15  // Jump to return address\n";
        ostream << "  A = M\n";
        ostream << "  0; JMP\n";
    }
}

auto jack::vm::generate_assembly(std::string_view file_name, std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> ast_root) -> std::expected<std::string, std::string>
{
    if (ast_root == nullptr)
        return std::unexpected("AST is null");

    auto str = type_check(file_name, ast_root);

    if (!str.empty())
        return std::unexpected(str);

    constexpr_ostream out;
    out << "  // BBOTSTRAP AREA\n";
    out << "  @256\n";
    out << "  D = A\n";
    out << "  @SP\n";
    out << "  M = D\n\n";
    out << "  // CALL Sys.init\n";
    out << "  @Sys.init\n";
    out << "  0; JMP\n\n";
    out << "(Sys.init)\n";
    out << "  // LCL = ARG = SP\n";
    out << "  @SP\n";
    out << "  D = A\n";
    out << "  @LCL\n";
    out << "  M = D\n\n";

    // Call Main.main here
    auto main_call = std::make_unique<ASTNode<LexerTypes<LexerToken>, NonTerminal>>(Terminal::TK_CALL);
    auto main_name = std::make_unique<LexerToken>(Terminal::TK_SYMBOL, "Main.main");
    auto main_args = std::make_unique<LexerToken>(Terminal::TK_NUM, "0");
    main_call->descendants.push_back(std::make_unique<ASTNode<LexerTypes<LexerToken>, NonTerminal>>(std::move(main_name)));
    main_call->descendants.push_back(std::make_unique<ASTNode<LexerTypes<LexerToken>, NonTerminal>>(std::move(main_args)));
    call_writer(std::move(main_call), out);

    // Resume user code
    out << "  // User Code\n";
    out << "  A = -1\n";
    out << "  0; JMP\n";

    for (auto& function : ast_root->descendants)
    {
        function_writer(function, out);
        for (auto& op : function->descendants[2]->descendants)
        {
            switch (std::get<Terminal>(op->node_symbol_type))
            {
            case Terminal::TK_PUSH:
            case Terminal::TK_POP:
                stack_writer(op, file_name, out);
                break;
            case Terminal::TK_ADD:
            case Terminal::TK_SUB:
            case Terminal::TK_NEG:
                arithmetic_writer(op, out);
                break;
            case Terminal::TK_EQ:
            case Terminal::TK_GT:
            case Terminal::TK_LT:
                logical_writer(op, out);
                break;
            case Terminal::TK_AND:
            case Terminal::TK_OR:
            case Terminal::TK_NOT:
                boolean_writer(op, out);
                break;
            case Terminal::TK_LABEL:
            case Terminal::TK_GOTO:
            case Terminal::TK_IF:
                branch_writer(op, out);
                break;
            case Terminal::TK_CALL:
                call_writer(op, out);
                break;
            case Terminal::TK_RETURN:
                return_writer(op, out);
                break;
            default:
                std::terminate();
            }
        }
    }

    return out.str();
}