export module jack.vm:frontend;
import std;
import compiler;
import helpers;

enum class Terminal
{
    eps,
    TK_EOF,
    TK_PUSH,
    TK_POP,
    TK_LOCAL,
    TK_ARGUMENT,
    TK_STATIC,
    TK_CONSTANT,
    TK_THIS,
    TK_THAT,
    TK_POINTER,
    TK_TEMP,
    TK_NUM,
    TK_SYMBOL,
    TK_ADD,
    TK_SUB,
    TK_MINUS,
    TK_NEG,
    TK_EQ,
    TK_GT,
    TK_LT,
    TK_AND,
    TK_OR,
    TK_NOT,
    TK_LABEL,
    TK_GOTO,
    TK_IF,
    TK_FUNCTION,
    TK_CALL,
    TK_RETURN,
    TK_NEWLINE,
    TK_WHITESPACE
};

enum class NonTerminal
{
    start,
    func_start,
    line_start,
    line_end,
    op,
    segment,
    stack_op_name,
    seg_name,
    alu_op,
    arithmetic,
    logical,
    boolean,
    branch
};

struct LexerToken
{
    std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
    std::string_view lexeme{};
    int line_number = 1;

    static std::map<std::string_view, Terminal> terminal_map;

    constexpr void after_construction(const LexerToken& previous_token);

    constexpr bool discard() const
    {
        return type == Terminal::TK_WHITESPACE;
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

namespace
{
    std::string type_check(const std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>& ast_root)
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
                // TODO: error_stream << function_name->line_number << ": Function " << function_name << " is already defined at line " << existing_fn_entry->second.line_number << '\n';
                continue;
            }

            function_dictionary[function_name->lexeme] = *nArgs;
        }

        // Function level checks
        if (const auto& main_entry = function_dictionary.find("Main.main"); main_entry == function_dictionary.end())
            error_stream << "Function Main.main is not defined\n";
        else if (main_entry->second.lexeme != "0")
            ; // TODO: error_stream << main_entry->second.line_number << ": Function Main.main should accept 0 arguments\n";

        if (const auto& sys_entry = function_dictionary.find("Sys.init"); sys_entry == function_dictionary.end())
            ; // TODO: error_stream << sys_entry->second.line_number << ": Function Sys.init is not allowed in user code.\n";

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
                        ; // TODO: error_stream << line_number << ": Label " << symbol << " is already defined inside function " << function_name;
                    else if (symbol.starts_with("ALU_COMPARE_"))
                        ; // TODO: error_stream << line_number << ": User defined symbol name must not start with 'ALU_COMPARE_'\n";
                    else if (symbol.starts_with("FUNC_CALL_"))
                        ; // TODO: error_stream << line_number << ": User defined symbol name must not start with 'FUNC_CALL_'\n";
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
                        ; // TODO: error_stream << line_number << ": Function " << function_call_name << " is not defined\n";
					else if (ref->second.lexeme != nArgs)
                        ; // TODO: error_stream << line_number << ": Function " << function_call_name << " should accept " << ref->second.lexeme << " arguments\n";
                }
                else if (operation->node_symbol_type == Terminal::TK_PUSH || operation->node_symbol_type == Terminal::TK_POP)
                {
                    const auto& segment = operation->descendants[0];
                    const auto& index = operation->descendants[1]->lexer_token->lexeme;

                    if (segment->node_symbol_type == Terminal::TK_CONSTANT && operation->node_symbol_type == Terminal::TK_POP)
                        ; // TODO: error_stream << line_number << ": Popping a constant is invalid operation: pop constant " << index << "\n";
                    else if (segment->node_symbol_type == Terminal::TK_TEMP && !(index.size() == 1 && index[0] >= '0' && index[0] <= '7'))
                        ; // TODO: error_stream << line_number << ": temp segment should have index in range [0, 7]: " << operation->node_symbol_type << " temp " << index << "\n";
                    else if (segment->node_symbol_type == Terminal::TK_POINTER && !(index == "0" || index == "1"))
                        ; // TODO: error_stream << line_number << ": pointer segment should have index either 0 or 1: " << operation->node_symbol_type << " pointer " << index << "\n";
                
                    if (segment->node_symbol_type == Terminal::TK_POINTER)
                        ; // TODO: error_stream << line_number << ": pointer segment type is not supported yet.\n";
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
                    ; // TODO: error_stream << line_number << ": Label " << label << " is not defined inside function " << function_name;
            }
        }

        return error_stream.str();
    }
}

namespace jack::vm
{
    export auto get_ast(std::string_view vm_file_content) -> std::expected<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>, std::string>;

    export auto generate_assembly(std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> ast_root) -> std::expected<std::vector<std::string>, std::string>
    {
        if (ast_root == nullptr)
            return std::unexpected("AST is null");

        auto str = type_check(ast_root);

        if (!str.empty())
			return std::unexpected(str);

        return std::unexpected("Not implemented");
    }
}