module jack.compiler:backend;
import std;
import helpers;

using namespace jack::compiler;

namespace 
{
	std::unique_ptr<Statement> get_statement_from_ast(ASTType, const FunctionEntry*);

	struct LetStatement : Statement
	{
		std::string_view variable_name{};
		const LexerToken let;
		ASTType index_expression{};			// in case of array type
		ASTType expression{};

		LetStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, let{ *ast->lexer_token }
		{
			this->variable_name = ast->descendants[0]->lexer_token->lexeme;
			if (ast->descendants[0]->descendants[0] != nullptr)
				this->index_expression = std::move(ast->descendants[0]->descendants[0]);
			this->expression = std::move(ast->descendants[1]);
		}

		virtual ~LetStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "let " << variable_name;

			if (index_expression != nullptr)
				out << "[" << *index_expression << "]";
			out << " = " << *expression << ";";
			return out.str();
		}
	};

	struct ReturnStatement : Statement
	{
		const LexerToken ret;
		ASTType expression{};

		ReturnStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, ret{ *ast->lexer_token }
		{
			this->expression = std::move(ast->descendants[0]);
		}
		virtual ~ReturnStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "return";
			if (expression != nullptr)
				out << " " << *expression;
			out << ";";
			return out.str();
		}
	};

	struct DoStatement : Statement
	{
		// either method_name() or prefix_name.method_name()
		const LexerToken do_token;
		std::string_view prefix_name;
		std::string_view method_name;
		std::vector<ASTType> arguments{};

		DoStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, do_token{ *ast->lexer_token }
		{
			const auto& identifier1 = ast->descendants[0];
			if (identifier1->descendants[0] == nullptr)
			{
				this->prefix_name = "";
				this->method_name = identifier1->lexer_token->lexeme;
			}
			else
			{
				this->prefix_name = identifier1->lexer_token->lexeme;
				this->method_name = identifier1->descendants[0]->lexer_token->lexeme;
			}

			for (auto it = ast->descendants.begin() + 1; it != ast->descendants.end(); ++it)
				this->arguments.push_back(std::move(*it));
		}

		virtual ~DoStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "do ";
			if (!prefix_name.empty())
				out << prefix_name << ".";
			out << method_name << "(";
			for (auto it = arguments.begin(); it != arguments.end(); ++it)
			{
				out << **it;
				if (it != arguments.end() - 1)
					out << ", ";
			}
			out << ");";
			return out.str();
		}
	};

	struct IfStatement : Statement
	{
		const LexerToken if_token;
		ASTType condition{};
		std::vector<std::unique_ptr<Statement>> if_body;
		std::vector<std::unique_ptr<Statement>> else_body;

		IfStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, if_token{ *ast->lexer_token }
		{
			this->condition = std::move(ast->descendants[0]);
			if (ast->descendants[1] != nullptr)
				for (auto it = ast->descendants[1]->descendants.begin(); it != ast->descendants[1]->descendants.end(); ++it)
					this->if_body.push_back(get_statement_from_ast(std::move(*it), fn));

			if (ast->descendants[2] != nullptr)
				for (auto it = ast->descendants[2]->descendants.begin(); it != ast->descendants[2]->descendants.end(); ++it)
					this->else_body.push_back(get_statement_from_ast(std::move(*it), fn));
		}

		virtual ~IfStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "if (" << *condition << ") {\n";
			for (const auto& statement : if_body)
				out << *statement << "\n";
			out << "}";
			if (!else_body.empty())
			{
				out << " else {\n";
				for (const auto& statement : else_body)
					out << *statement << "\n";
				out << "}";
			}
			return out.str();
		}
	};

	struct WhileStatement : Statement
	{
		const LexerToken while_token;
		ASTType condition{};
		std::vector<std::unique_ptr<Statement>> body;

		WhileStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, while_token{ *ast->lexer_token }
		{
			this->condition = std::move(ast->descendants[0]);
			for (auto it = ast->descendants[1]->descendants.begin(); it != ast->descendants[1]->descendants.end(); ++it)
				this->body.push_back(get_statement_from_ast(std::move(*it), fn));
		}

		virtual ~WhileStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "while (" << *condition << ") {\n";
			for (const auto& statement : body)
				out << *statement << "\n";
			out << "}";
			return out.str();
		}
	};

	std::unique_ptr<Statement> get_statement_from_ast(ASTType ast, const FunctionEntry* fn)
	{
		switch (std::get<Terminal>(ast->node_symbol_type))
		{
		case Terminal::TK_LET:
			return std::make_unique<LetStatement>(std::move(ast), fn);
		case Terminal::TK_RETURN:
			return std::make_unique<ReturnStatement>(std::move(ast), fn);
		case Terminal::TK_DO:
			return std::make_unique<DoStatement>(std::move(ast), fn);
		case Terminal::TK_IF:
			return std::make_unique<IfStatement>(std::move(ast), fn);
		case Terminal::TK_WHILE:
			return std::make_unique<WhileStatement>(std::move(ast), fn);
		default:
			std::unreachable();
		}
	}
}

struct AssemblyGenerator
{
	const SymbolTable* symbol_table{};
	constexpr_ostream errors;
	constexpr_ostream output;
	
	std::tuple<const Type*, std::string_view, int> get_variable_loc(const FunctionEntry* fn, std::string_view var_name, const ASTType& array_index)
	{
		return { nullptr, "", 0 };
	}

	// Process expression and return its type
	const Type* process(const FunctionEntry* fn, const ASTType& ast)
	{
		constexpr std::array segments = { "local", "argument", "this", "static" };
		return nullptr;
	}

	void process(const LetStatement* let)
	{
		const auto expression_type = process(let->enclosing_function, let->expression);
		const auto [variable_type, segment, index] = get_variable_loc(let->enclosing_function, let->variable_name, let->index_expression);

		if (expression_type != variable_type || expression_type == nullptr)
			errors << let->enclosing_function->enclosing_class_type->name << "." << let->enclosing_function->name << ": Type mismatch in let statement: " << let->let << "\n";

		output << "\tpop " << segment << " " << index << "\n";
	}

	void process(const ReturnStatement* ret)
	{
		if (ret->enclosing_function->member_type == EFunctionType::CONSTRUCTOR && (ret->expression != nullptr)) 
		{
			errors << ret->enclosing_function->enclosing_class_type->name << "." << ret->enclosing_function->name << ": Constructor should not return a value\n";
			return;
		}

		if (ret->enclosing_function->member_type == EFunctionType::CONSTRUCTOR)
		{
			output << "\tpush pointer 0\n\treturn\n";
			return;
		}

		const auto* fn_ret_type = ret->enclosing_function->return_type.value_or(nullptr);
		const auto* expression_type = ret->expression != nullptr ? process(ret->enclosing_function, ret->expression) : nullptr;

		if (fn_ret_type != expression_type)
			errors << ret->enclosing_function->enclosing_class_type->name << "." << ret->enclosing_function->name << ": Type mismatch in return statement: " << ret->ret << "\n";

		if (fn_ret_type == nullptr)
			output << "\tpush constant 0\n";

		output << "\treturn\n";
	}

	void process(const DoStatement* do_stmt)
	{

	}

	void process(const IfStatement* if_stmt)
	{
		static int if_counter = 0;
		const auto* if_condition_res = process(if_stmt->enclosing_function, if_stmt->condition);
		if (if_condition_res != symbol_table->classes.find("boolean")->second.get())
			errors << if_stmt->enclosing_function->enclosing_class_type->name << "." << if_stmt->enclosing_function->name << ": If condition should be of type boolean: " << if_stmt->if_token << "\n";

		output << "\tnot\n";
		output << "\tif-goto IF_LABEL" << if_counter << "\n";
		for (const auto& stmt : if_stmt->if_body)
			process(stmt.get());
		output << "\tgoto ELSE_LABEL" << if_counter << "\n";
		output << "label IF_LABEL" << if_counter << "\n";
		for (const auto& stmt : if_stmt->else_body)
			process(stmt.get());
		output << "label ELSE_LABEL" << if_counter++ << "\n";
	}

	void process(const WhileStatement* while_stmt)
	{
		static int while_counter = 0;
		output << "label WHILE_LABEL" << while_counter << "\n";
		const auto* while_condition_res = process(while_stmt->enclosing_function, while_stmt->condition);
		if (while_condition_res != symbol_table->classes.find("boolean")->second.get())
			errors << while_stmt->enclosing_function->enclosing_class_type->name << "." << while_stmt->enclosing_function->name << ": While condition should be of type boolean: " << while_stmt->while_token << "\n";

		output << "\tnot\n";
		output << "\tif-goto WHILE_END_LABEL" << while_counter << "\n";
		for (const auto& stmt : while_stmt->body)
			process(stmt.get());
		output << "\tgoto WHILE_LABEL" << while_counter << "\n";
		output << "label WHILE_END_LABEL" << while_counter++ << "\n";
	}

	void process(const Statement* stmt)
	{
		if (const auto let = dynamic_cast<const LetStatement*>(stmt))
			process(let);
		else if (const auto ret = dynamic_cast<const ReturnStatement*>(stmt))
			process(ret);
		else if (const auto do_stmt = dynamic_cast<const DoStatement*>(stmt))
			process(do_stmt);
		else if (const auto if_stmt = dynamic_cast<const IfStatement*>(stmt))
			process(if_stmt);
		else if (const auto while_stmt = dynamic_cast<const WhileStatement*>(stmt))
			process(while_stmt);
		else
			std::unreachable();
	}

	void process(const FunctionEntry* fn)
	{
		output << "function " << fn->enclosing_class_type->name << "." << fn->name << " " << fn->locals.size() << "\n";
		if (fn->member_type == EFunctionType::METHOD)
		{
			output << "\tpush argument 0\n";
			output << "\tpop pointer 0\n";
		}
		else if (fn->member_type == EFunctionType::CONSTRUCTOR)
		{
			output << "\tpush constant " << fn->enclosing_class_type->field_variables.size() << "\n";
			output << "\tcall Memory.alloc 1\n";
			output << "\tpop pointer 0\n";
		}

		for (const auto& stmt : fn->statements)
			process(stmt.get());
	}

	void process(const SymbolTable* symbol_table)
	{
		for (const auto& [class_name, class_type] : symbol_table->classes)
		{
			if (class_name == "int" || class_name == "char" || class_name == "boolean")
				continue;

			for (const auto& [_, con] : class_type->constructors)
				process(con.get());

			for (const auto& [_, fn] : class_type->functions)
				process(fn.get());

			for (const auto& [_, met] : class_type->methods)
				process(met.get());
		}
	}
};

std::expected<std::unique_ptr<SymbolTable>, std::string> SymbolTable::init_table(std::vector<ASTType> class_asts)
{
	int static_var_index = 0;
	auto table = std::make_unique<SymbolTable>();
	constexpr_ostream error_logs;

	table->classes["int"] = std::make_unique<Type>("int");
	table->classes["char"] = std::make_unique<Type>("char");
	table->classes["boolean"] = std::make_unique<Type>("boolean");

	// Init type map
	for (auto& class_ast : class_asts) 
	{
		auto class_name = class_ast->lexer_token->lexeme;
		if (table->classes.contains(class_name))
		{
			error_logs << "Class " << class_name << " already exists\n";
			continue;
		}

		table->classes[class_name] = std::make_unique<Type>(class_name);
	}

	// Init type members
	for (auto& class_ast : class_asts)
	{
		const auto class_name = class_ast->lexer_token->lexeme;
		auto& type = table->classes[class_name];
		const auto& class_vars = class_ast->descendants[0];
		const auto& subroutineDecs = class_ast->descendants[1];

		if (class_vars != nullptr)
		{
			int field_var_index = 0;

			const auto& class_var = class_vars->descendants;
			for (const auto& var : class_var)
			{
				const auto prefix_type = std::get<Terminal>(var->descendants[0]->lexer_token->type);
				const auto var_type_name = var->descendants[1]->lexer_token->lexeme;

				if (!table->classes.contains(var_type_name))
				{
					error_logs << class_name << ": Type " << *var->descendants[1]->lexer_token << " does not exist\n";
					continue;
				}

				for (auto identifier = var->descendants.begin() + 2; identifier != var->descendants.end(); ++identifier)
				{
					const auto var_name = (*identifier)->lexer_token->lexeme;
					if (type->member_types.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists\n";
						continue;
					}

					if (table->classes.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists as a class\n";
						continue;
					}

					type->member_types[var_name] = prefix_type == Terminal::TK_FIELD ? EVariableType::FIELD : EVariableType::STATIC;
					if (prefix_type == Terminal::TK_FIELD)
						type->field_variables[var_name] = std::make_unique<VariableEntry>(EVariableType::FIELD, var_name, table->classes[var_type_name].get(), field_var_index++);
					else
						type->static_variable[var_name] = std::make_unique<VariableEntry>(EVariableType::STATIC, var_name, table->classes[var_type_name].get(), static_var_index++);
				}
			}
		}

		if (subroutineDecs != nullptr)
		{
			for (const auto& subroutineDec : subroutineDecs->descendants)
			{
				const auto subroutine_type = std::get<Terminal>(subroutineDec->node_symbol_type);
				const auto& subroutine_return_type_name = subroutineDec->descendants[0]->lexer_token->lexeme;
				const auto& params_ast = subroutineDec->descendants[1];
				const auto& locals_ast = subroutineDec->descendants[2]->descendants[0];
				auto& body_ast = subroutineDec->descendants[2]->descendants[1];
				const EFunctionType efntype = subroutine_type == Terminal::TK_CONSTRUCTOR ? EFunctionType::CONSTRUCTOR :
					subroutine_type == Terminal::TK_METHOD ? EFunctionType::METHOD : EFunctionType::FUNCTION;
				auto func_entry = std::make_unique<FunctionEntry>(efntype, subroutineDec->lexer_token->lexeme, type.get());

				int param_index = 0;
				int local_index = 0;

				if (type->member_types.contains(func_entry->name))
				{
					error_logs << class_name << ": Member " << *subroutineDec->lexer_token << " already exists\n";
					continue;
				}

				if (table->classes.contains(func_entry->name))
				{
					error_logs << class_name << ": Subroutine " << *subroutineDec->lexer_token << " already exists as a class\n";
					continue;
				}

				if (!table->classes.contains(subroutine_return_type_name) && subroutine_return_type_name != "void")
				{
					error_logs << class_name << ": " << func_entry->name << ": Return Type " << *subroutineDec->descendants[0]->lexer_token << " does not exist\n";
					continue;
				}
				else if (subroutine_return_type_name != "void")
					func_entry->return_type = table->classes[subroutine_return_type_name].get();

				if (subroutine_type == Terminal::TK_METHOD)
				{
					func_entry->parameters.push_back(std::make_unique<VariableEntry>(EVariableType::PARAMETER, "this", type.get(), param_index++));
					func_entry->fn_member_types["this"] = EVariableType::PARAMETER;
				}

				if (params_ast != nullptr)
				{
					for (const auto& param : params_ast->descendants)
					{
						const auto& param_type_name = param->lexer_token->lexeme;
						const auto& param_name = param->descendants[0]->lexer_token->lexeme;

						if (!table->classes.contains(param_type_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Type " << *param->descendants[0]->lexer_token << " does not exist\n";
							continue;
						}

						if (table->classes.contains(param_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Member " << *param->descendants[0]->lexer_token << " already exists as a class\n";
							continue;
						}

						if (type->member_types.contains(param_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Member " << *param->descendants[0]->lexer_token << " already exists in enclosing class\n";
							continue;
						}

						if (func_entry->fn_member_types.contains(param_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Member " << *param->descendants[0]->lexer_token << " already exists in method\n";
							continue;
						}

						func_entry->parameters.push_back(std::make_unique<VariableEntry>(EVariableType::PARAMETER, param_name, table->classes[param_type_name].get(), param_index++));
						func_entry->fn_member_types[param_name] = EVariableType::PARAMETER;
						func_entry->fn_member_entry[param_name] = func_entry->parameters.back().get();
					}
				}

				if (locals_ast != nullptr)
				{
					for (const auto& local_entry : locals_ast->descendants)
					{
						const auto& local_type_name = local_entry->descendants[0]->lexer_token->lexeme;
						if (!table->classes.contains(local_type_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Type " << *local_entry->descendants[0]->lexer_token << " does not exist\n";
							continue;
						}

						for (auto local_name_ptr = local_entry->descendants.begin() + 1; local_name_ptr != local_entry->descendants.end(); ++local_name_ptr)
						{
							const auto& local_name = (*local_name_ptr)->lexer_token->lexeme;
							if (table->classes.contains(local_name))
							{
								error_logs << class_name << ": " << func_entry->name << ": Member " << *(*local_name_ptr)->lexer_token << " already exists as a class\n";
								continue;
							}

							if (type->member_types.contains(local_name))
							{
								error_logs << class_name << ": " << func_entry->name << ": Member " << *(*local_name_ptr)->lexer_token << " already exists in enclosing class\n";
								continue;
							}

							if (func_entry->fn_member_types.contains(local_name))
							{
								error_logs << class_name << ": " << func_entry->name << ": Member " << *(*local_name_ptr)->lexer_token << " already exists in method\n";
								continue;
							}

							func_entry->locals.push_back(std::make_unique<VariableEntry>(EVariableType::LOCAL, local_name, table->classes[local_type_name].get(), local_index++));
							func_entry->fn_member_types[local_name] = EVariableType::LOCAL;
							func_entry->fn_member_entry[local_name] = func_entry->locals.back().get();
						}
					}
				}

				if (body_ast)
					for (auto it = body_ast->descendants.begin(); it != body_ast->descendants.end(); ++it)
						func_entry->statements.push_back(get_statement_from_ast(std::move(*it), func_entry.get()));

				if (subroutine_type == Terminal::TK_CONSTRUCTOR)
				{
					type->member_types[func_entry->name] = EFunctionType::CONSTRUCTOR;
					type->constructors[func_entry->name] = std::move(func_entry);

					if (func_entry->return_type != type.get())
						error_logs << class_name << ": " << func_entry->name << ": Constructor's return type should match the class it is part of.\n";
				}
				else if (subroutine_type == Terminal::TK_METHOD)
				{
					type->member_types[func_entry->name] = EFunctionType::METHOD;
					type->methods[func_entry->name] = std::move(func_entry);
				}
				else if (subroutine_type == Terminal::TK_FUNCTION)
				{
					type->member_types[func_entry->name] = EFunctionType::FUNCTION;
					type->functions[func_entry->name] = std::move(func_entry);
				}
			}
		}
	}

	if (static_var_index >= 255 - 16 + 1)
		error_logs << "Static variables exceed 240!\n";

	if (!table->classes.contains("Main"))
		error_logs << "Main.main class does not exist\n";
	else if (!table->classes["Main"]->member_types.contains("main") || table->classes["Main"]->member_types["main"] != EFunctionType::FUNCTION)
		error_logs << "Main.main class does not have a main function\n";
	else if (table->classes["Main"]->functions["main"]->parameters.size() != 0)
		error_logs << "Main.main function should not have parameters\n";
	else if (table->classes["Main"]->functions["main"]->return_type != std::nullopt)
		error_logs << "Main.main function should have a void return type\n";
	
	if (table->classes.contains("Sys") && table->classes["Sys"]->member_types.contains("init"))
		error_logs << "Sys class should not have an init function\n";

	if (error_logs.has_data())
		return std::unexpected(error_logs.str());

	return table;
}

std::expected<std::string, std::string> SymbolTable::generate_vm_code() const
{
	AssemblyGenerator generator(this);
	generator.process(this);
	if (generator.errors.has_data())
		return std::unexpected(generator.errors.str());
	return generator.output.str();
}