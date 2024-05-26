module jack.compiler:backend;
import std;
import helpers;

using namespace jack::compiler;

std::expected<std::unique_ptr<SymbolTable>, std::string> SymbolTable::init_table(std::vector<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>> class_asts)
{
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
			int static_var_index = 0;

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
					if (type->members.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists\n";
						continue;
					}

					if (table->classes.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists as a class\n";
						continue;
					}

					type->members[var_name] = prefix_type == Terminal::TK_FIELD ? EMemberType::FIELD : EMemberType::STATIC;
					if (prefix_type == Terminal::TK_FIELD)
						type->field_variables[var_name] = std::make_unique<VariableEntry>(var_name, table->classes[var_type_name].get(), field_var_index++);
					else
						type->static_variable[var_name] = std::make_unique<VariableEntry>(var_name, table->classes[var_type_name].get(), static_var_index++);
				}
			}
		}

		if (subroutineDecs != nullptr)
		{
			for (const auto& subroutineDec : subroutineDecs->descendants)
			{
				auto func_entry = std::make_unique<FunctionEntry>(subroutineDec->lexer_token->lexeme, type.get());
				const auto subroutine_type = std::get<Terminal>(subroutineDec->node_symbol_type);
				const auto& subroutine_return_type_name = subroutineDec->descendants[0]->lexer_token->lexeme;
				const auto& params_ast = subroutineDec->descendants[1];
				const auto& locals_ast = subroutineDec->descendants[2]->descendants[0];
				auto& body_ast = subroutineDec->descendants[2]->descendants[1];

				int param_index = 0;
				int local_index = 0;

				if (type->members.contains(func_entry->name))
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

						if (type->members.contains(param_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Member " << *param->descendants[0]->lexer_token << " already exists in enclosing class\n";
							continue;
						}

						if (func_entry->members.contains(param_name))
						{
							error_logs << class_name << ": " << func_entry->name << ": Member " << *param->descendants[0]->lexer_token << " already exists in method\n";
							continue;
						}

						func_entry->parameters.push_back(VariableEntry(param_name, table->classes[param_type_name].get(), param_index++));
						func_entry->members[param_name] = EFuncVariableType::PARAMETER;
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

							if (type->members.contains(local_name))
							{
								error_logs << class_name << ": " << func_entry->name << ": Member " << *(*local_name_ptr)->lexer_token << " already exists in enclosing class\n";
								continue;
							}

							if (func_entry->members.contains(local_name))
							{
								error_logs << class_name << ": " << func_entry->name << ": Member " << *(*local_name_ptr)->lexer_token << " already exists in method\n";
								continue;
							}

							func_entry->locals.push_back(VariableEntry(local_name, table->classes[local_type_name].get(), local_index++));
							func_entry->members[local_name] = EFuncVariableType::LOCAL;
						}
					}
				}

				func_entry->statements = std::move(body_ast);

				if (subroutine_type == Terminal::TK_CONSTRUCTOR)
				{
					type->members[func_entry->name] = EMemberType::CONSTRUCTOR;
					type->constructors[func_entry->name] = std::move(func_entry);
				}
				else if (subroutine_type == Terminal::TK_METHOD)
				{
					type->members[func_entry->name] = EMemberType::METHOD;
					type->methods[func_entry->name] = std::move(func_entry);
				}
				else if (subroutine_type == Terminal::TK_FUNCTION)
				{
					type->members[func_entry->name] = EMemberType::FUNCTION;
					type->functions[func_entry->name] = std::move(func_entry);
				}
			}
		}
	}

	if (!table->classes.contains("Main"))
		error_logs << "Main.main class does not exist\n";
	else if (!table->classes["Main"]->members.contains("main") || table->classes["Main"]->members["main"] != EMemberType::FUNCTION)
		error_logs << "Main.main class does not have a main function\n";
	else if (table->classes["Main"]->functions["main"]->parameters.size() != 0)
		error_logs << "Main.main function should not have parameters\n";
	else if (table->classes["Main"]->functions["main"]->return_type != std::nullopt)
		error_logs << "Main.main function should have a void return type\n";
	
	if (table->classes.contains("Sys") && table->classes["Sys"]->members.contains("init"))
		error_logs << "Sys class should not have an init function\n";

	if (error_logs.has_data())
		return std::unexpected(error_logs.str());

	return table;
}

std::expected<std::string, std::string> SymbolTable::generate_vm_code() const
{
	return "";
}