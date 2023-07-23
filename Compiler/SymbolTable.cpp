#include "SymbolTable.h"
#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>
#include <functional>
using namespace std;

GlobalTable* GlobalTable::singleton_ = nullptr;

Token* copy_token(Token* input)
{
    assert(input != nullptr);
    assert(input->length > 0);

    auto* out = new Token;
    out->type = input->type;
    out->lexeme = input->lexeme;
    out->line_number = input->line_number;
    out->start_index = input->line_number;
    out->length = input->length;
    return out;
}

ASTNode* createAST(const ParseTreeNode* input, const ParseTreeNode* parent, ASTNode* inherited)
{
    assert(input != nullptr);

    if (input->isLeaf)
    {
        auto* node = new ASTNode;
        node->sym_index = input->symbol_index;
        node->token = copy_token(input->token);
        return node;
    }

    auto* node = new ASTNode;
    node->sym_index = input->symbol_index;

    if (input->productionNumber == 0)
    {
        // class ==> TK_CLASS TK_IDENTIFIER TK_CURO class_vars subroutineDecs TK_CURC TK_EOF
        node->children = {
                createAST(input->children[1]),
                createAST(input->children[3]),
                createAST(input->children[4]),
        };
        auto class_entry = new ClassLevelTable(node);
        GlobalTable::getInstance()->classes.push_back(class_entry);
        return nullptr;
    }
    else if (input->productionNumber == 1)
    {
        // class_vars ==> class_var class_vars
        delete node;
        auto child = createAST(input->children[0]);
        child->sibling = createAST(input->children[1]);
        return child;
    }
    else if (input->productionNumber == 2)
    {
        // class_vars ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 3)
    {
        // subroutineDecs ==> subroutineDec subroutineDecs
        delete node;
        auto routine = createAST(input->children[0]);
        routine->sibling = createAST(input->children[1]);
        return routine;
    }
    else if (input->productionNumber == 4)
    {
        // subroutineDecs ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 5)
    {
        // class_var ==> class_var_prefix type TK_IDENTIFIER more_identifiers TK_SEMICOLON
        node->children = {
                createAST(input->children[0]),
                createAST(input->children[1]),
                createAST(input->children[2])
        };
        node->children[2]->sibling = createAST(input->children[3]);
    }
    else if (input->productionNumber >= 6 && input->productionNumber <= 11)
    {
        // class_var_prefix TK_STATIC
        // class_var_prefix TK_FIELD
        // type TK_INT
        // type TK_CHAR
        // type TK_BOOLEAN
        // type TK_IDENTIFIER
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 12)
    {
        // more_identifiers ==> TK_COMMA TK_IDENTIFIER more_identifiers
        delete node;
        node = createAST(input->children[1]);
        node->sibling = createAST(input->children[2]);
    }
    else if (input->productionNumber == 13)
    {
        // more_identifiers ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 14)
    {
        // subroutineDec ==> subroutine_prefix subroutine_type TK_IDENTIFIER TK_PARENO parameters TK_PARENC subroutine_body
        node->token = copy_token(input->children[2]->token);
        node->children = {
                createAST(input->children[0]),
                createAST(input->children[1]),
                createAST(input->children[4]),
                createAST(input->children[6])
        };
    }
    else if (input->productionNumber >= 15 && input->productionNumber <= 19)
    {
        // subroutine_prefix TK_CONSTRUCTOR
        // subroutine_prefix TK_FUNCTION
        // subroutine_prefix TK_METHOD
        // subroutine_type type
        // subroutine_type TK_VOID
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 20)
    {
        // parameters ==> type TK_IDENTIFIER more_parameters
        node->token = copy_token(input->children[1]->token);
        node->children = {
            createAST(input->children[0])
        };
        node->sibling = createAST(input->children[2]);
    }
    else if (input->productionNumber == 21)
    {
        // parameters ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 22)
    {
        // more_parameters ==> TK_COMMA type TK_IDENTIFIER more_parameters
        node->token = copy_token(input->children[2]->token);
        node->children = {
            createAST(input->children[1])
        };
        node->sibling = createAST(input->children[3]);
    }
    else if (input->productionNumber == 23)
    {
        // more_parameters ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 24)
    {
        // subroutine_body ==> TK_CURO routine_vars statements TK_CURC
        node->children = {
                createAST(input->children[1]),
                createAST(input->children[2]),
        };
    }
    else if (input->productionNumber == 25)
    {
        // routine_vars ==> routine_var routine_vars
        delete node;
        node = createAST(input->children[0]);
        node->sibling = createAST(input->children[1]);
    }
    else if (input->productionNumber == 26)
    {
        // routine_vars ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 27)
    {
        // routine_var ==> TK_VAR type TK_IDENTIFIER more_identifiers TK_SEMICOLON
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[1]),
                createAST(input->children[2])
        };
        node->children[1]->sibling = createAST(input->children[3]);
    }
    else if (input->productionNumber == 28)
    {
        // statements ==> statement statements
        delete node;
        node = createAST(input->children[0]);
        node->sibling = createAST(input->children[1]);
    }
    else if (input->productionNumber == 29)
    {
        // statements ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber >= 30 && input->productionNumber <= 34)
    {
        // statement let_statement
        // statement if_statement
        // statement while_statement
        // statement do_statement
        // statement return_statement
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 35)
    {
        // let_statement ==> TK_LET TK_IDENTIFIER identifier_suffix TK_EQ expression TK_SEMICOLON
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[1]),
                createAST(input->children[2]),
                createAST(input->children[4])
        };
    }
    else if (input->productionNumber == 36)
    {
        // identifier_suffix ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 37)
    {
        // identifier_suffix ==> TK_BRACKO expression TK_BRACKC
        delete node;
        return createAST(input->children[1]);
    }
    else if (input->productionNumber == 38)
    {
        // if_statement TK_IF TK_PARENO expression TK_PARENC TK_CURO statements TK_CURC else_statement
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[2]),
                createAST(input->children[5]),
                createAST(input->children[7])
        };
    }
    else if (input->productionNumber == 39)
    {
        // else_statement ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 40)
    {
        // else_statement ==> TK_ELSE TK_CURO statements TK_CURC
        delete node;
        return createAST(input->children[2]);
    }
    else if (input->productionNumber == 41)
    {
        // while_statement ==> TK_WHILE TK_PARENO expression TK_PARENC TK_CURO statements TK_CURC
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[2]),
                createAST(input->children[5])
        };
    }
    else if (input->productionNumber == 42)
    {
        // do_statement ==> TK_DO subroutine_call TK_SEMICOLON
        node->token = copy_token(input->children[0]->token);
        node->children = {createAST(input->children[1])};
    }
    else if (input->productionNumber == 43)
    {
        // return_statement ==> TK_RETURN return_suffix TK_SEMICOLON
        node->token = copy_token(input->children[0]->token);
        node->children = {createAST(input->children[1])};
    }
    else if (input->productionNumber == 44)
    {
        // return_suffix ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 45)
    {
        // return_suffix ==> expression
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 46)
    {
        // expression ==> term expression_suffix
        delete node;
        auto term = createAST(input->children[0]);
        auto suffix = createAST(input->children[1]);
        if (suffix == nullptr)
            return term;

        suffix->children[0] = term;
        return suffix;
    }
    else if (input->productionNumber == 47)
    {
        // expression_suffix ==> op term expression_suffix
        delete node;
        auto op = createAST(input->children[0]);
        auto term = createAST(input->children[1]);
        auto suffix = createAST(input->children[2]);

        if (suffix == nullptr)
            op->children = { nullptr, term };
        else
        {
            op->children = { nullptr, suffix };
            suffix->children[0] = term;
        }

        return op;
    }
    else if (input->productionNumber == 48)
    {
        // expression_suffix ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber >= 49 && input->productionNumber <= 54)
    {
        // term TK_NUM
        // term TK_STR
        // term TK_TRUE
        // term TK_FALSE
        // term TK_NULL
        // term TK_THIS
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 55)
    {
        // term ==> TK_IDENTIFIER term_sub_iden
        node->token = copy_token(input->children[0]->token);
        return createAST(input->children[1], nullptr, node);
    }
    else if (input->productionNumber == 56)
    {
        // term_sub_iden ==> eps
        delete node;
        return inherited;
    }
    else if (input->productionNumber == 57)
    {
        // term_sub_iden ==> TK_PARENO expression_list TK_PARENC
        node->token = copy_token(input->children[0]->token);
        node->children = {
                inherited,
                createAST(input->children[1])
        };
    }
    else if (input->productionNumber == 58)
    {
        // term_sub_iden ==> TK_DOT TK_IDENTIFIER TK_PARENO expression_list TK_PARENC
        node->token = copy_token(input->children[2]->token);

        auto caller = createAST(input->children[0]);
        caller->children = {
                inherited,
                createAST(input->children[1])
        };

        node->children = {
                caller,
                createAST(input->children[3])
        };
    }
    else if (input->productionNumber == 59)
    {
        // term_sub_iden ==> TK_BRACKO expression TK_BRACKC
        node->token = copy_token(input->children[0]->token);
        node->children = {
                inherited,
                createAST(input->children[1])
        };
    }
    else if (input->productionNumber == 60)
    {
        // term ==> TK_PARENO expression TK_PARENC
        delete node;
        return createAST(input->children[1]);
    }
    else if (input->productionNumber == 61)
    {
        // term ==> TK_MINUS term
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[1])
        };
    }
    else if (input->productionNumber == 62)
    {
        // term ==> TK_NOT term
        node->token = copy_token(input->children[0]->token);
        node->children = {
                createAST(input->children[1])
        };
    }
    else if (input->productionNumber >= 63 && input->productionNumber <= 71)
    {
        // op TK_PLUS
        // op TK_MINUS
        // op TK_MULT
        // op TK_DIV
        // op TK_AND
        // op TK_OR
        // op TK_LE
        // op TK_GE
        // op TK_EQ
        delete node;
        return createAST(input->children[0]);
    }
    else if (input->productionNumber == 72)
    {
        // subroutine_call ==> TK_IDENTIFIER subroutine_scope TK_PARENO expression_list TK_PARENC
        node->token = copy_token(input->children[2]->token);
        node->children = {
                createAST(input->children[1], nullptr, createAST(input->children[0])),
                createAST(input->children[3])
        };
    }
    else if (input->productionNumber == 73)
    {
        // subroutine_scope ==> TK_DOT TK_IDENTIFIER
        node->token = copy_token(input->children[0]->token);
        node->children = {
                inherited,
                createAST(input->children[1])
        };
    }
    else if (input->productionNumber == 74)
    {
        // subroutine_scope ==> eps
        delete node;
        return inherited;
    }
    else if (input->productionNumber == 75)
    {
        // expression_list ==> expression more_expressions
        delete node;
        auto expression = createAST(input->children[0]);
        expression->sibling = createAST(input->children[1]);
        return expression;
    }
    else if (input->productionNumber == 76)
    {
        // expression_list ==> eps
        delete node;
        return nullptr;
    }
    else if (input->productionNumber == 77)
    {
        // more_expressions ==> TK_COMMA expression more_expressions
        delete node;
        auto expression = createAST(input->children[1]);
        expression->sibling = createAST(input->children[2]);
        return expression;
    }
    else if (input->productionNumber == 78)
    {
        // more_expressions ==> eps
        delete node;
        return nullptr;
    }
    else
    {
        assert(false);
    }

    return node;
}

std::ostream& operator<<(std::ostream& out, const ASTNode& node)
{
    out <<
        "{ symbol: '" <<
        parser.symbolType2symbolStr[node.sym_index] <<
        "', lexeme: '" <<
        (node.token ? node.token->lexeme : "") <<
        "'";

    out << " }";
    return out;
}

VariableType extract_type(TokenType token_type)
{
    switch (token_type) {
        case TokenType::TK_VOID:
            return VariableType::VOID;

        case TokenType::TK_INT:
            return VariableType::INTEGER;

        case TokenType::TK_CHAR:
            return VariableType::CHAR;

        case TokenType::TK_BOOLEAN:
            return VariableType::BOOLEAN;

        case TokenType::TK_IDENTIFIER:
            return VariableType::USER;

        default: break;
    }

    cerr << "This should never occur! Check parsing rules!" << endl;
    assert(false);
}

FunctionEntry::FunctionEntry(ASTNode *node)
{
    switch (node->children[0]->token->type) {
        case TokenType::TK_METHOD:
            category = MethodCategory::METHOD;
            break;

        case TokenType::TK_FUNCTION:
            category = MethodCategory::FUNCTION;
            break;

        case TokenType::TK_CONSTRUCTOR:
            category = MethodCategory::CONSTRUCTOR;
            break;

        default:
            cerr << "This should never occur! Parsing rules are wrong!!" << endl;
            assert(false);
    }
    delete node->children[0];

    function_name = node->token->lexeme;
    delete node->token;

    return_type_name = node->children[1]->token->lexeme;
    return_type = extract_type(node->children[1]->token->type);
    delete node->children[1];
    return_type_entry = nullptr;

    int variable_index = 0;
    for (auto param = node->children[2]; param; param = param->sibling)
    {
        auto var = new VariableEntry();
        var->var_name = param->token->lexeme;
        var->var_type_name = param->children[0]->token->lexeme;
        var->var_type = extract_type(param->children[0]->token->type);
        var->var_category = VariableCategory::ARGUMENT;
        var->index = variable_index++;
        var->type_entry = nullptr;
        parameters.push_back(var);
    }
    delete node->children[2];

    for (auto local = node->children[3]->children[0]; local; local = local->sibling)
    {
        auto var_type = extract_type(local->children[0]->token->type);
        for (auto var_name = local->children[1]; var_name; var_name = var_name->sibling)
        {
            auto var = new VariableEntry();
            var->var_name = var_name->token->lexeme;
            var->var_type_name = local->children[0]->token->lexeme;
            var->var_type = var_type;
            var->var_category = VariableCategory::LOCAL;
            var->index = variable_index++;
            var->type_entry = nullptr;
            locals.push_back(var);
        }
    }
    delete node->children[3]->children[0];

    body = node->children[3]->children[1];
    node->children[3]->children[1] = nullptr;
    node->children[3]->children.clear();
    delete node->children[3];
}

ClassLevelTable::ClassLevelTable(ASTNode * node)
{
    assert(node != nullptr);

    this->class_name = node->children[0]->token->lexeme;
    delete node->children[0];

    int var_index = 0;
    for (auto var = node->children[1]; var; var = var->sibling)
    {
        auto var_type = extract_type(var->children[1]->token->type);

        auto var_category = (
                var->children[0]->token->type == TokenType::TK_STATIC ? VariableCategory::STATIC :
                var->children[0]->token->type == TokenType::TK_FIELD ? VariableCategory::FIELD :
                VariableCategory::UNINITIALISED);

        if (var_category == VariableCategory::UNINITIALISED)
        {
            cerr << "This should never happen! Check the parsing table!" << endl;
            assert(false);
        }

        for (auto var_name_entry = var->children[2]; var_name_entry; var_name_entry = var_name_entry->sibling)
        {
            auto entry = new VariableEntry();
            entry->var_name = var_name_entry->token->lexeme;
            entry->var_type_name = var->children[1]->token->lexeme;
            entry->var_type = var_type;
            entry->var_category = var_category;
            entry->index = var_index++;
            entry->type_entry = nullptr;
            variables.push_back(entry);
        }
    }

    for (auto var = node->children[2]; var; var = var->sibling)
        functions.push_back(new FunctionEntry(var));
}

void GlobalTable::add_class(ParseTreeNode* node)
{
    createAST(node);
}

void member_iterator(ASTNode* node, function<void(ASTNode*, bool)> fn)
{
    if (node == nullptr)
        return;

    auto& children = node->children;

    // presence of '(' in AST implies we are making a function call
    if (node->token->type == TokenType::TK_PARENO)
    {
        assert(children.size() == 2);
        fn(children[0], true);
        member_iterator(children[1], fn);
        assert(node->sibling == nullptr);
        return;
    }

    if (node->token->type == TokenType::TK_IDENTIFIER ||
        node->token->type == TokenType::TK_DOT)
    {
        fn(node, false);
        return;
    }

    for (auto& child : children)
        member_iterator(child, fn);

    member_iterator(node->sibling, fn);
}

void GlobalTable::check_names()
{
    assert(!check_names_called);
    check_names_called = true;

    class_type_map.clear();

    for (auto& name : { "int", "char", "boolean" })
        class_type_map[name] = nullptr;

    // Requirement #1: No two classes have same name. Their being `int` or `char` is not checked as this is handled via lexer and parser.
    for (auto& class_entry : classes)
    {
        if (class_type_map.find(class_entry->class_name) != class_type_map.end())
        {
            cerr << "Class " << class_entry->class_name << " is defined more than once!" << endl;
            assert(false);
        }
        class_type_map[class_entry->class_name] = class_entry;
    }

    // Requirement #2: No two members in a class can have same name. The member names should not match existing class names.
    for (auto& class_entry : classes)
    {
        auto& member_names = class_entry->member_names;

        for (auto& var : class_entry->variables)
        {
            if (member_names.find(var->var_name) != member_names.end())
            {
                cerr << "Class " << class_entry->class_name << " has two members with same name: " << var->var_name << endl;
                assert(false);
            }
            if (class_type_map.find(var->var_name) != class_type_map.end())
            {
                cerr << "Class " << class_entry->class_name << " has a variable with same name as a class: " << var->var_name << endl;
                assert(false);
            }
            member_names.insert(var->var_name);
        }
        for (auto& fun : class_entry->functions)
        {
            if (member_names.find(fun->function_name) != member_names.end())
            {
                cerr << "Class " << class_entry->class_name << " has a member with same name as a function: " << fun->function_name << endl;
                assert(false);
            }
            if (class_type_map.find(fun->function_name) != class_type_map.end())
            {
                cerr << "Class " << class_entry->class_name << " has a function with same name as a class: " << fun->function_name << endl;
                assert(false);
            }

            member_names.insert(fun->function_name);
        }
    }

    // Requirement #3: A local variable name and parameter name inside function can't be a class name, member name in existing class, same name.
    for (auto& class_entry : classes)
    {
        for (auto& func_entry : class_entry->functions)
        {
            auto& local_names = func_entry->local_names;
            vector<VariableEntry*> entries;
            entries.insert(entries.end(), func_entry->parameters.begin(), func_entry->parameters.end());
            entries.insert(entries.end(), func_entry->locals.begin(), func_entry->locals.end());

            for (auto& entry : entries)
            {
                // class name check
                if (class_type_map.find(entry->var_name) != class_type_map.end())
                {
                    cerr << "Class " << class_entry->class_name << " has a function " << func_entry->function_name << " with a parameter/local variable with same name as a class: " << entry->var_name << endl;
                    assert(false);
                }

                // member name check
                if (class_entry->member_names.find(entry->var_name) != class_entry->member_names.end())
                {
                    cerr << "Class " << class_entry->class_name << " has a function " << func_entry->function_name << " with a parameter/local variable with same name as a member: " << entry->var_name << endl;
                    assert(false);
                }

                // local name check
                if (local_names.find(entry->var_name) != local_names.end())
                {
                    cerr << "Class " << class_entry->class_name << " has a function " << func_entry->function_name << " with two parameters/local variables with same name: " << entry->var_name << endl;
                    assert(false);
                }

                local_names.insert(entry->var_name);
            }
        }
    }

    // Requirement #4: Types of class variable, function returns, local variables and parameters should be valid.
    for (auto& class_entry : classes)
    {
        // class variables
        for (auto& var : class_entry->variables)
        {
            if (class_type_map.find(var->var_type_name) == class_type_map.end())
            {
                cerr << "Class " << class_entry->class_name << " has a variable " << var->var_name << " with invalid type: " << var->var_type_name << endl;
                assert(false);
            }
            var->type_entry = class_type_map[var->var_type_name];
        }

        // function returns
        for (auto& fun : class_entry->functions)
        {
            if (class_type_map.find(fun->return_type_name) == class_type_map.end() && fun->return_type != VariableType::VOID)
            {
                cerr << "Class " << class_entry->class_name << " has a function " << fun->function_name << " with invalid return type: " << fun->return_type_name << endl;
                assert(false);
            }
            fun->return_type_entry = class_type_map[fun->return_type_name];
        }

        // function parameters and local variables
        for (auto& fun : class_entry->functions)
        {
            for (auto& param : fun->parameters)
            {
                if (class_type_map.find(param->var_type_name) == class_type_map.end())
                {
                    cerr << "Class " << class_entry->class_name << " has a function " << fun->function_name << " with a parameter " << param->var_name << " with invalid type: " << param->var_type_name << endl;
                    assert(false);
                }
                param->type_entry = class_type_map[param->var_type_name];
            }

            for (auto& local : fun->locals)
            {
                if (class_type_map.find(local->var_type_name) == class_type_map.end())
                {
                    cerr << "Class " << class_entry->class_name << " has a function " << fun->function_name << " with a local variable " << local->var_name << " with invalid type: " << local->var_type_name << endl;
                    assert(false);
                }
                local->type_entry = class_type_map[local->var_type_name];
            }
        }
    }

    // Requirement #5: Function body - Identifier usage should refer to existing valid name.
    for (auto& class_entry : classes)
    {
        map<string_view, ClassLevelTable*> valid_names;
        for (auto& x : class_entry->variables)
            valid_names[x->var_name] = x->type_entry;

        set<string_view> self_functions;
        for (auto& f : class_entry->functions)
            self_functions.insert(f->function_name);

        for (auto& func_entry : class_entry->functions)
        {
            for (auto& x : func_entry->parameters)
                valid_names[x->var_name] = x->type_entry;
            for (auto& x : func_entry->locals)
                valid_names[x->var_name] = x->type_entry;

            // we are allowing the following here: X | X.Y | X() | X.Y()
            // while the language parsing rules only allows for X | X() | X.Y()
            member_iterator(func_entry->body,
                [this, self_functions, class_entry, valid_names](ASTNode* member, bool is_function_call) {
                    const int& line_number = member->token->line_number;
                    const bool dot_type = member->token->type == TokenType::TK_DOT;
                    const auto& main_name = dot_type ?
                        member->children[0]->token->lexeme :
                        member->token->lexeme;
                    const auto& secondary_name = dot_type ? member->children[1]->token->lexeme : "";

                    bool X_user_class_type = (class_type_map.find(main_name) != class_type_map.end());
                    bool X_self_function = self_functions.find(main_name) != self_functions.end();
                    bool X_local_var = valid_names.find(main_name) != valid_names.end();

                    int count = (X_user_class_type ? 1 : 0) + (X_self_function ? 1 : 0) + (X_local_var ? 1 : 0);
                    assert(count <= 1);
                    if (count == 0)
                    {
                        cerr << "Line number " << line_number << ": Variable " << main_name << " is not found." << endl;
                        assert(false);
                    }

                    if (dot_type)
                    {
                        if (X_self_function)
                        {
                            cerr << "Line number " << line_number << ": Variable " << main_name << " can't be a member function name." << endl;
                            assert(false);
                        }

                        // checks for y
                        ClassLevelTable* class_ref = nullptr;
                        if (X_user_class_type)
                            class_ref = class_type_map[main_name];
                        else
                            class_ref = valid_names.at(main_name);

                        assert(class_ref != nullptr);
                        if (class_ref->member_names.find(secondary_name) == class_ref->member_names.end())
                        {
                            cerr << "Line number " << line_number << ": Member " << secondary_name << " doesn't exist in class " << class_ref->class_name << "." << endl;
                            assert(false);
                        }

                        bool is_variable = false;
                        for (auto& x : class_ref->variables)
                            if (x->var_name == secondary_name)
                                is_variable = true;

                        if (is_function_call && is_variable)
                        {
                            cerr << "Line number " << line_number << ": Member " << secondary_name << " is a member variable but is treated as function." << endl;
                            assert(false);
                        }

                        if (!is_function_call && !is_variable)
                        {
                            cerr << "Line number " << line_number << ": Member " << secondary_name << " is a member function but is treated as variable." << endl;
                            assert(false);
                        }
                    }
                    else
                    {
                        if (X_user_class_type)
                        {
                            cerr << "Line number " << line_number << ": Variable " << main_name << " can't be a user defined type." << endl;
                            assert(false);
                        }

                        if (is_function_call && X_local_var)
                        {
                            cerr << "Line number " << line_number << ": Variable " << main_name << " can't be called with ()." << endl;
                            assert(false);
                        }
                        
                        if (!is_function_call && X_self_function)
                        {
                            cerr << "Line number " << line_number << ": Function " << main_name << " is not called." << endl;
                            assert(false);
                        }
                    }
                });

            for (auto& x : func_entry->local_names)
                valid_names.erase(x);
        }
    }

    // Requirement #6: There should be a class named `Main`. This class should have a function named `main` with no arguments. Other classes can have `main` but that is not our concern.
    if (class_type_map.find("Main") == class_type_map.end())
    {
        cerr << "Class 'Main' not found." << endl;
        assert(false);
    }

    FunctionEntry* main_func = nullptr;
    for (auto& x : class_type_map["Main"]->functions)
        if (x->function_name == "main")
        {
            main_func = x;
            break;
        }

    if (main_func == nullptr)
    {
        cerr << "'main' function not found inside class 'Main'" << endl;
        assert(false);
    }

    if (main_func->parameters.size() != 0)
    {
        cerr << "'main' function inside 'Main' class should not accept any parameters" << endl;
        assert(false);
    }
}