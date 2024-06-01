module jack.compiler:backend;
import std;
import helpers;

using namespace jack::compiler;

namespace 
{
	struct ExpressionType
	{
		const FunctionEntry* enclosing_function{};
		ExpressionType(const FunctionEntry* fn) : enclosing_function{ fn } {}
		virtual ~ExpressionType() {}

		template <typename T>
		friend constexpr T& operator<<(T& out, const ExpressionType& tk)
		{
			return out << tk.get_out_str();
		}

	protected:
		virtual std::string get_out_str() const = 0;
	};

	std::unique_ptr<ExpressionType> get_expression_from_ast(ASTType, const FunctionEntry*);

	struct BinaryOp : ExpressionType
	{
		enum class OP { PLUS, MINUS, MULTIPLY, DIVIDE, BAND, BOR, LT, GR, EQ };

		const LexerToken token{};
		OP value{};

		BinaryOp(const FunctionEntry* fn, const LexerToken lt) :
			ExpressionType{ fn }, token{ lt }
		{
			if (lt.type == Terminal::TK_PLUS)
				value = OP::PLUS;
			else if (lt.type == Terminal::TK_MINUS)
				value = OP::MINUS;
			else if (lt.type == Terminal::TK_MULT)
				value = OP::MULTIPLY;
			else if (lt.type == Terminal::TK_DIV)
				value = OP::DIVIDE;
			else if (lt.type == Terminal::TK_AND)
				value = OP::BAND;
			else if (lt.type == Terminal::TK_OR)
				value = OP::BOR;
			else if (lt.type == Terminal::TK_LE)
				value = OP::LT;
			else if (lt.type == Terminal::TK_GE)
				value = OP::GR;
			else if (lt.type == Terminal::TK_EQ)
				value = OP::EQ;
			else
				std::unreachable();
		}

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << token.lexeme;
			return out.str();
		}
	};

	struct Expression;
	struct Term;

	struct SubroutineCall : ExpressionType
	{
		LexerToken prefix{};
		LexerToken method_name{};
		std::vector<std::unique_ptr<ExpressionType>> arguments{};

		SubroutineCall(const FunctionEntry* fn, ASTType ast) : ExpressionType{ fn }
		{
			const auto& identifier1 = ast->descendants[0];
			if (identifier1->descendants[0] == nullptr)
			{
				this->prefix = LexerToken(Terminal::eps, "");
				this->method_name = *identifier1->lexer_token.get();
			}
			else
			{
				this->prefix = *identifier1->lexer_token.get();
				this->method_name = *identifier1->descendants[0]->lexer_token.get();
			}

			for (auto it = ast->descendants.begin() + 1; it != ast->descendants.end(); ++it)
				this->arguments.emplace_back(get_expression_from_ast(std::move(*it), fn));
		}

		SubroutineCall(LexerToken prefix, LexerToken methodName, std::vector<std::unique_ptr<ExpressionType>> args, const FunctionEntry* fn) :
			ExpressionType{ fn }, prefix{ prefix }, method_name{ methodName }, arguments{ std::move(args) } { }

		protected:
			virtual std::string get_out_str() const override
			{
				constexpr_ostream out;
				if (prefix.type != Terminal::eps)
					out << prefix.lexeme << ".";
				out << method_name.lexeme << "(";
				for (const auto& arg : arguments)
					out << *arg << ", ";
				out << ")";
				return out.str();
			}
	};

	struct VarWithIndex : ExpressionType {
		LexerToken var_name;
		std::unique_ptr<ExpressionType> index_expression;

		VarWithIndex(const FunctionEntry* fn, ASTType ast) : ExpressionType{ fn }
		{
			var_name = *ast->lexer_token;
			index_expression = get_expression_from_ast(std::move(ast->descendants[0]->descendants[0]), fn);
		}

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << var_name.lexeme;
			if (index_expression != nullptr)
				out << "[" << *index_expression << "]";
			return out.str();
		}
	};

	struct UnaryOpWithTerm : ExpressionType
	{
		struct UnaryOp : ExpressionType
		{
			enum class OP { MINUS, NOT };

			const LexerToken token{};
			OP value{};

			UnaryOp(const FunctionEntry* fn, const LexerToken lt) :
				ExpressionType{ fn }, token{ lt }
			{
				if (lt.type == Terminal::TK_MINUS)
					value = OP::MINUS;
				else if (lt.type == Terminal::TK_NOT)
					value = OP::NOT;
				else
					std::unreachable();
			}

		protected:
			virtual std::string get_out_str() const override
			{
				constexpr_ostream out;
				out << token.lexeme;
				return out.str();
			}
		};

		std::unique_ptr<UnaryOp> unary_op;
		std::unique_ptr<Term> term;

		UnaryOpWithTerm(const FunctionEntry* fn, ASTType ast) : ExpressionType{ fn }
		{
			this->unary_op = std::make_unique<UnaryOp>(fn, *ast->lexer_token.get());
			this->term = std::make_unique<Term>(fn, std::move(ast->descendants[0]));
		}

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;

			return out.str();
		}
	};

	struct Term : ExpressionType
	{
		std::variant<std::unique_ptr<VarWithIndex>, std::unique_ptr<SubroutineCall>, std::unique_ptr<UnaryOpWithTerm>, std::unique_ptr<LexerToken>> term{};
	
		Term(const FunctionEntry* fn, ASTType ast) : ExpressionType{ fn }
		{
			if (ast->descendants.size() == 0 || ast->descendants[0] == nullptr)
			{
				switch (std::get<Terminal>(ast->lexer_token->type))
				{
					case Terminal::TK_IDENTIFIER:
					case Terminal::TK_NUM:
					case Terminal::TK_STR:
					case Terminal::TK_THIS:
					case Terminal::TK_NULL:
					case Terminal::TK_TRUE:
					case Terminal::TK_FALSE:
						term = std::make_unique<LexerToken>(*ast->lexer_token);
						return;
				}
				std::unreachable();
			}

			if (ast->lexer_token->type == Terminal::TK_MINUS ||
				ast->lexer_token->type == Terminal::TK_NOT)
			{
				this->term = std::make_unique<UnaryOpWithTerm>(fn, std::move(ast));
				return;
			}

			if (ast->descendants[0]->lexer_token->type == Terminal::TK_BRACKC)
			{
				this->term = std::make_unique<VarWithIndex>(fn, std::move(ast));
				return;
			}

			if (ast->descendants[0]->lexer_token->type == Terminal::TK_DOT)
			{
				const LexerToken prefix = *ast->lexer_token;
				const LexerToken method_name = *ast->descendants[0]->descendants[0]->lexer_token;
				std::vector<std::unique_ptr<ExpressionType>> arguments;
				for (auto it = ast->descendants.begin() + 1; it != ast->descendants.end(); ++it)
					arguments.emplace_back(get_expression_from_ast(std::move(*it), fn));
				this->term = std::make_unique<SubroutineCall>(prefix, method_name, std::move(arguments), fn);
				return;
			}

			const LexerToken prefix = LexerToken(Terminal::eps, "");
			const LexerToken method_name = *ast->lexer_token;
			std::vector<std::unique_ptr<ExpressionType>> arguments;
			for (auto it = ast->descendants.begin(); it != ast->descendants.end(); ++it)
				arguments.emplace_back(get_expression_from_ast(std::move(*it), fn));
			this->term = std::make_unique<SubroutineCall>(prefix, method_name, std::move(arguments), fn);
		}

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			std::visit([&out](const auto& arg) { 
				if constexpr (std::is_same_v<std::unique_ptr<LexerToken>, std::decay_t<decltype(arg)>>)
					out << ((LexerToken)*arg).lexeme;
				else
					out << *arg;
			}, term);
			return out.str();
		}
	};

	struct Expression : ExpressionType
	{
		struct OpEntry
		{
			std::unique_ptr<BinaryOp> op;
			std::unique_ptr<Expression> lterm;
			std::unique_ptr<Expression> rterm;

			OpEntry(auto&& op, auto&& lterm, auto&& rterm)
			{
				this->op = std::move(op);
				this->lterm = std::move(lterm);
				this->rterm = std::move(rterm);
			}

			OpEntry(OpEntry&& other) noexcept :
				op{ std::move(other.op) }, lterm{ std::move(other.lterm) }, rterm{ std::move(other.rterm) } { }
		};

		std::variant<std::unique_ptr<Term>, std::unique_ptr<OpEntry>> node;

		Expression(const FunctionEntry* fn, ASTType ast) : ExpressionType{ fn }
		{
			const std::array ops = { Terminal::TK_PLUS, Terminal::TK_MINUS, Terminal::TK_MULT, Terminal::TK_DIV, Terminal::TK_AND, Terminal::TK_OR, Terminal::TK_LE, Terminal::TK_GE, Terminal::TK_EQ };
			if (std::find(ops.begin(), ops.end(), ast->lexer_token->type) == ops.end())
				node = std::make_unique<Term>(fn, std::move(ast));
			else if (ast->lexer_token->type == Terminal::TK_MINUS)
				node = std::make_unique<Term>(fn, std::move(ast));
			else {
				auto op = std::make_unique<BinaryOp>(fn, *ast->lexer_token);
				auto lterm = std::make_unique<Expression>(fn, std::move(ast->descendants[0]));
				auto rterm = std::make_unique<Expression>(fn, std::move(ast->descendants[1]));
				node = std::make_unique<OpEntry>(std::move(op), std::move(lterm), std::move(rterm));
			}
		}
	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			std::visit([&out](const auto& arg) {
				if constexpr (std::is_same_v<std::unique_ptr<Term>, std::decay_t<decltype(arg)>>)
					out << *arg;
				else
					out << "(" << * arg->lterm << " " << *arg->op << " " << *arg->rterm << ")";
			}, node);
			return out.str();
		}
	};

	std::unique_ptr<ExpressionType> get_expression_from_ast(ASTType ast, const FunctionEntry* fn)
	{
		return std::make_unique<Expression>(fn, std::move(ast));
	}
}

namespace 
{
	std::unique_ptr<Statement> get_statement_from_ast(ASTType, const FunctionEntry*);

	struct LetStatement : Statement
	{
		const LexerToken let;
		std::string_view variable_name{};
		std::unique_ptr<ExpressionType> index_expression{};			// in case of array type
		std::unique_ptr<ExpressionType> expression{};

		LetStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, let{ *ast->lexer_token }
		{
			this->variable_name = ast->descendants[0]->lexer_token->lexeme;
			if (ast->descendants[0]->descendants[0] != nullptr)
				this->index_expression = get_expression_from_ast(std::move(ast->descendants[0]->descendants[0]), fn);
			this->expression = get_expression_from_ast(std::move(ast->descendants[1]), fn);
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
		std::unique_ptr<ExpressionType> expression{};

		ReturnStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, ret{ *ast->lexer_token }
		{
			if (ast->descendants[0] != nullptr)
				this->expression = get_expression_from_ast(std::move(ast->descendants[0]), fn);
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
		std::unique_ptr<SubroutineCall> call;

		DoStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, do_token{ *ast->lexer_token }
		{
			call = std::make_unique<SubroutineCall>(fn, std::move(ast->descendants[0]));
		}

		virtual ~DoStatement() {};

	protected:
		virtual std::string get_out_str() const override
		{
			constexpr_ostream out;
			out << "\tdo " << *call << ";";
			return out.str();
		}
	};

	struct IfStatement : Statement
	{
		const LexerToken if_token;
		std::unique_ptr<ExpressionType> condition{};
		std::vector<std::unique_ptr<Statement>> if_body;
		std::vector<std::unique_ptr<Statement>> else_body;

		IfStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, if_token{ *ast->lexer_token }
		{
			this->condition = get_expression_from_ast(std::move(ast->descendants[0]), fn);
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
		std::unique_ptr<ExpressionType> condition{};
		std::vector<std::unique_ptr<Statement>> body;

		WhileStatement(ASTType ast, const FunctionEntry* fn) : Statement{ fn }, while_token{ *ast->lexer_token }
		{
			this->condition = get_expression_from_ast(std::move(ast->descendants[0]), fn);
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
	
	bool valid_assignment_type(const Type* lhs, const Type* rhs)
	{
		if (lhs == nullptr || rhs == nullptr)
			return false;

		if (lhs == rhs)
			return true;

		return !(lhs->name == "int" || lhs->name == "char" || lhs->name == "boolean") && rhs->name == "null";
	}

	auto get_variable_loc(const FunctionEntry* fn, std::string_view var_name, const ExpressionType* array_index) -> std::tuple<const Type*, std::string_view, int>
	{
		const auto* enclosing_class = fn->enclosing_class_type;;
		const auto& class_static_vars = enclosing_class->static_variables;
		const auto& class_field_vars = enclosing_class->field_variables;
		const auto& params = fn->parameters;
		const auto& locals = fn->locals;
		const bool static_scope = fn->member_type == EFunctionType::FUNCTION;

		if (class_static_vars.find(var_name) != class_static_vars.end())
			return { class_static_vars.find(var_name)->second->type, "static", class_static_vars.find(var_name)->second->index };
			
		if (!static_scope && class_field_vars.find(var_name) != class_field_vars.end())
			return { class_field_vars.find(var_name)->second->type, "this", class_field_vars.find(var_name)->second->index };

		for (const auto& param : params)
			if (param->name == var_name)
				return { param->type, "argument", param->index };

		for (const auto& local : locals)
			if (local->name == var_name)
				return { local->type, "local", local->index };

		return { nullptr, "", 0 };
	}

	const Type* process(const SubroutineCall* call)
	{
		/*
		LexerToken prefix{};
		LexerToken method_name{};
		std::vector<std::unique_ptr<ExpressionType>> arguments{};
		*/

		// Y() -> can resolve to either this.Y() or static function Y()
		// X.Y() -> can resolve to (X is variable name, Y is method name) or (X is class name, Y is function name)

		const auto* enclosing_class = call->enclosing_function->enclosing_class_type;
		const auto* enclosing_function = call->enclosing_function;
		const auto& class_static_vars = enclosing_class->static_variables;
		const auto& class_field_vars = enclosing_class->field_variables;
		const auto& params = enclosing_function->parameters;
		const auto& locals = enclosing_function->locals;

		const auto& class_methods = enclosing_class->methods;
		const auto& class_constructors = enclosing_class->constructors;
		const auto& class_functions = enclosing_class->functions;
		
		if (call->prefix.lexeme == "")
		{

		}

		return nullptr;
	}

	const Type* process(const VarWithIndex* vwi)
	{
		const auto* index_type = process(vwi->index_expression.get());

		if (index_type->name != "int")
		{
			errors << vwi->enclosing_function->enclosing_class_type->name << "." << vwi->enclosing_function->name << ": Array index should be of type 'int'\n";
			return nullptr;
		}

		const auto [variable_type, segment, index] = get_variable_loc(vwi->enclosing_function, vwi->var_name.lexeme, vwi->index_expression.get());

		output << "\tpush " << segment << " " << index << "\n";
		return variable_type;
	}

	const Type* process(const UnaryOpWithTerm* op_with_term)
	{
		const auto* term_type = process(op_with_term->term.get());

		if (term_type == nullptr)
			errors << op_with_term->enclosing_function->enclosing_class_type->name << "." << op_with_term->enclosing_function->name << ": Type mismatch in unary operation: " << op_with_term->unary_op->token << "\n";

		if (op_with_term->unary_op->value == UnaryOpWithTerm::UnaryOp::OP::MINUS)
			output << "\tneg\n";
		else
			output << "\tnot\n";

		return term_type;
	}

	const Type* process(const Term* term)
	{
		if (std::holds_alternative<std::unique_ptr<VarWithIndex>>(term->term))
			return process(std::get<std::unique_ptr<VarWithIndex>>(term->term).get());
		else if (std::holds_alternative<std::unique_ptr<SubroutineCall>>(term->term))
			return process(std::get<std::unique_ptr<SubroutineCall>>(term->term).get());
		else if (std::holds_alternative<std::unique_ptr<UnaryOpWithTerm>>(term->term))
			return process(std::get<std::unique_ptr<UnaryOpWithTerm>>(term->term).get());
		else if (std::holds_alternative<std::unique_ptr<LexerToken>>(term->term))
		{
			const auto& token = std::get<std::unique_ptr<LexerToken>>(term->term).get();

			if (token->type == Terminal::TK_NUM)
			{
				output << "\tpush constant " << token->lexeme << "\n";
				return symbol_table->classes.find("int")->second.get();
			}
			else if (token->type == Terminal::TK_STR)
			{
				output << "\tpush constant " << token->lexeme.size() << "\n";
				output << "\tcall String.new 1\n";
				for (const auto& ch : token->lexeme)
				{
					output << "\tpush constant " << (int)ch << "\n";
					output << "\tcall String.appendChar 2\n";
				}
				return symbol_table->classes.find("String")->second.get();
			}
			else if (token->type == Terminal::TK_TRUE)
			{
				output << "\tpush constant 1\n";
				return symbol_table->classes.find("boolean")->second.get();
			}
			else if (token->type == Terminal::TK_FALSE)
			{
				output << "\tpush constant 0\n";
				return symbol_table->classes.find("boolean")->second.get();
			}
			else if (token->type == Terminal::TK_NULL)
			{
				output << "\tpush constant 0\n";
				return symbol_table->classes.find("null")->second.get();
			}
			else if (token->type == Terminal::TK_THIS)
			{
				output << "\tpush pointer 0\n";
				return term->enclosing_function->enclosing_class_type;
			}
			else if (token->type == Terminal::TK_IDENTIFIER)
			{
				const auto [variable_type, segment, index] = get_variable_loc(term->enclosing_function, token->lexeme, nullptr);
				output << "\tpush " << segment << " " << index << "\n";
				return variable_type;
			}
			else
				std::unreachable();
		}
		else
			std::unreachable();
	}

	const Type* process(const Expression* exp)
	{
		if (std::holds_alternative<std::unique_ptr<Term>>(exp->node))
			return process(std::get<std::unique_ptr<Term>>(exp->node).get());

		// binary operation
		const auto op_entry = std::get<std::unique_ptr<Expression::OpEntry>>(exp->node).get();
		const Type* lterm_type = process(op_entry->lterm.get());
		const Type* rterm_type = process(op_entry->rterm.get());

		if (lterm_type != rterm_type || lterm_type == nullptr)
		{
			errors << exp->enclosing_function->enclosing_class_type->name << "." << exp->enclosing_function->name << ": Type mismatch in binary operation: " << op_entry->op->token << "\n";
			return nullptr;
		}

		// LHS TYPE = RHS TYPE
		switch (op_entry->op->value)
		{
		case BinaryOp::OP::PLUS:
		case BinaryOp::OP::MINUS:
		case BinaryOp::OP::MULTIPLY:
		case BinaryOp::OP::DIVIDE:
		case BinaryOp::OP::LT:
		case BinaryOp::OP::GR:
			if (lterm_type != symbol_table->classes.find("int")->second.get())
			{
				errors << exp->enclosing_function->enclosing_class_type->name << "." << exp->enclosing_function->name << ": Type should be of type 'int' on both sides of binary operation: " << op_entry->op->token << "\n";
				return nullptr;
			}
			break;
		case BinaryOp::OP::BAND:
		case BinaryOp::OP::BOR:
			if (lterm_type != symbol_table->classes.find("boolean")->second.get())
			{
				errors << exp->enclosing_function->enclosing_class_type->name << "." << exp->enclosing_function->name << ": Type should be of type 'bool' on both sides of binary operation: " << op_entry->op->token << "\n";
				return nullptr;
			}
			break;
		case BinaryOp::OP::EQ:
			// Types are already same
			break;
		default:
			std::unreachable();
		}

		switch (op_entry->op->value)
		{
		case BinaryOp::OP::PLUS:
			output << "\tadd\n";
			break;
		case BinaryOp::OP::MINUS:
			output << "\tsub\n";
			break;
		case BinaryOp::OP::MULTIPLY:
			output << "\tcall Math.multiply 2\n";
			break;
		case BinaryOp::OP::DIVIDE:
			output << "\tcall Math.divide 2\n";
			break;
		case BinaryOp::OP::LT:
			output << "\tlt\n";
			break;
		case BinaryOp::OP::GR:
			output << "\tgt\n";
			break;
		case BinaryOp::OP::BAND:
			output << "\tand\n";
			break;
		case BinaryOp::OP::BOR:
			output << "\tor\n";
			break;
		case BinaryOp::OP::EQ:
			output << "\teq\n";
			break;
		default:
			std::unreachable();
		}
		
		switch (op_entry->op->value)
		{
		case BinaryOp::OP::PLUS:
		case BinaryOp::OP::MINUS:
		case BinaryOp::OP::MULTIPLY:
		case BinaryOp::OP::DIVIDE:
			return symbol_table->classes.find("int")->second.get();
		case BinaryOp::OP::LT:
		case BinaryOp::OP::GR:
		case BinaryOp::OP::EQ:
		case BinaryOp::OP::BAND:
		case BinaryOp::OP::BOR:
			return symbol_table->classes.find("boolean")->second.get();
		default:
			std::unreachable();
		}

		return nullptr;
	}

	const Type* process(const ExpressionType* expression)
	{
		if (auto ptr = dynamic_cast<const SubroutineCall*>(expression))
			return process(ptr);
		else if (auto ptr = dynamic_cast<const VarWithIndex*>(expression))
			return process(ptr);
		else if (auto ptr = dynamic_cast<const UnaryOpWithTerm*>(expression))
			return process(ptr);
		else if (auto ptr = dynamic_cast<const Term*>(expression))
			return process(ptr);
		else if (auto ptr = dynamic_cast<const Expression*>(expression))
			return process(ptr);
		else
			std::unreachable();
	}

	void process(const LetStatement* let)
	{
		const auto expression_type = process(let->expression.get());
		const auto [variable_type, segment, index] = get_variable_loc(let->enclosing_function, let->variable_name, let->index_expression.get());

		if (!valid_assignment_type(variable_type, expression_type))
			errors << let->enclosing_function->enclosing_class_type->name << "." << let->enclosing_function->name << ": Type mismatch in let statement: " << let->let << "\n";

		output << "\tpop " << segment << " " << index << "\n";
	}

	void process(const ReturnStatement* ret)
	{
		output << "// return statement\n";
		if (ret->enclosing_function->member_type == EFunctionType::CONSTRUCTOR && (ret->expression != nullptr))
		{
			errors << ret->enclosing_function->enclosing_class_type->name << "." << ret->enclosing_function->name << ": Constructor should not return a value\n";
			return;
		}

		if (ret->enclosing_function->return_type == std::nullopt && (ret->expression != nullptr))
		{
			errors << ret->enclosing_function->enclosing_class_type->name << "." << ret->enclosing_function->name << ": void method should not return a value\n";
			return;
		}

		if (ret->enclosing_function->member_type == EFunctionType::CONSTRUCTOR)
		{
			output << "\tpush pointer 0\n\treturn\n";
			return;
		}

		const auto* fn_ret_type = ret->enclosing_function->return_type.value_or(nullptr);
		const auto* expression_type = ret->expression != nullptr ? process(ret->expression.get()) : nullptr;

		if (fn_ret_type != expression_type)
			errors << ret->enclosing_function->enclosing_class_type->name << "." << ret->enclosing_function->name << ": Type mismatch in return statement: " << ret->ret << "\n";

		if (fn_ret_type == nullptr)
			output << "\tpush constant 0\n";

		output << "\treturn\n";
	}

	void process(const DoStatement* do_stmt)
	{
		output << "// Do " << do_stmt->call.get()->method_name << "\n";
		process(do_stmt->call.get());
		output << "\tpop temp 0\n";
	}

	void process(const IfStatement* if_stmt)
	{

		output << "// If statement\n";
		static int if_counter = 0;
		const auto* if_condition_res = process(if_stmt->condition.get());
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
		output << "// While statement\n";
		static int while_counter = 0;
		output << "label WHILE_LABEL" << while_counter << "\n";
		const auto* while_condition_res = process(while_stmt->condition.get());
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
		if (fn->member_type == EFunctionType::METHOD)
			output << "\t// Method " << fn->enclosing_class_type->name << "." << fn->name << "\n";
		else if (fn->member_type == EFunctionType::CONSTRUCTOR)
			output << "\t// Constructor " << fn->enclosing_class_type->name << "." << fn->name << "\n";
		else
			output << "\t// Function " << fn->enclosing_class_type->name << "." << fn->name << "\n";

		if (fn->return_type != std::nullopt)
			output << "\t// Return type: " << fn->return_type.value()->name << "\n";
		else 
			output << "\t// Return type: void\n";

		for (const auto& param : fn->parameters)
			output << "\t// Param " << param->index << " " << param->type->name << " " << param->name << "\n";
		for (const auto& local : fn->locals)
			output << "\t// Local " << local->index << " " << local->type->name << " " << local->name << "\n";

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

		// If no constructor or void return type, return should be present
		if (fn->member_type != EFunctionType::CONSTRUCTOR && fn->return_type != std::nullopt)
		if (fn->statements.empty() || !dynamic_cast<const ReturnStatement*>(fn->statements.back().get()))
			errors << fn->enclosing_class_type->name << "." << fn->name << ": Missing return statement in the end\n";

		if (fn->return_type == std::nullopt)
			output << "\tpush constant 0\n\treturn\n";
		else if (fn->member_type == EFunctionType::CONSTRUCTOR)
			output << "\tpush pointer 0\n\treturn\n";
	}

	void process(const SymbolTable* symbol_table)
	{
		for (const auto& [class_name, class_type] : symbol_table->classes)
		{
			const auto& class_static_vars = class_type->static_variables;
			const auto& class_field_vars = class_type->field_variables;

			output << "// class " << class_name << "\n";
			for (const auto& [_, var] : class_static_vars)
				output << "\t// static " << var->index << " " << var->type->name << " " << class_name << "." << var->name << "\n";
			for (const auto& [_, var] : class_field_vars)
				output << "\t// field " << var->index << " " << var->type->name << " " << class_name << "." << var->name << "\n";

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
	table->classes["null"] = std::make_unique<Type>("null");

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
		auto& term = table->classes[class_name];
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
					if (term->member_types.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists\n";
						continue;
					}

					if (table->classes.contains(var_name))
					{
						error_logs << class_name << ": Member " << *(*identifier)->lexer_token << " already exists as a class\n";
						continue;
					}

					term->member_types[var_name] = prefix_type == Terminal::TK_FIELD ? EVariableType::FIELD : EVariableType::STATIC;
					if (prefix_type == Terminal::TK_FIELD)
						term->field_variables[var_name] = std::make_unique<VariableEntry>(EVariableType::FIELD, var_name, table->classes[var_type_name].get(), field_var_index++);
					else
						term->static_variables[var_name] = std::make_unique<VariableEntry>(EVariableType::STATIC, var_name, table->classes[var_type_name].get(), static_var_index++);
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
				auto func_entry = std::make_unique<FunctionEntry>(efntype, subroutineDec->lexer_token->lexeme, term.get());

				int param_index = 0;
				int local_index = 0;

				if (term->member_types.contains(func_entry->name))
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
					func_entry->parameters.push_back(std::make_unique<VariableEntry>(EVariableType::PARAMETER, "this", term.get(), param_index++));
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

						if (term->member_types.contains(param_name))
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

							if (term->member_types.contains(local_name))
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
					term->member_types[func_entry->name] = EFunctionType::CONSTRUCTOR;
					term->constructors[func_entry->name] = std::move(func_entry);

					if (func_entry->return_type != term.get())
						error_logs << class_name << ": " << func_entry->name << ": Constructor's return type should match the class it is part of.\n";
				}
				else if (subroutine_type == Terminal::TK_METHOD)
				{
					term->member_types[func_entry->name] = EFunctionType::METHOD;
					term->methods[func_entry->name] = std::move(func_entry);
				}
				else if (subroutine_type == Terminal::TK_FUNCTION)
				{
					term->member_types[func_entry->name] = EFunctionType::FUNCTION;
					term->functions[func_entry->name] = std::move(func_entry);
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
