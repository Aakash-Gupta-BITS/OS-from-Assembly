export module jack.compiler:backend;
import std;
import compiler;
import :frontend;

enum EMemberType
{
	FIELD,
	STATIC,
	CONSTRUCTOR,
	FUNCTION,
	METHOD
};

enum EFuncVariableType
{
	PARAMETER,
	LOCAL
};

struct VariableEntry;
struct FunctionEntry;

struct Type
{
	const std::string_view name{};
	std::map<std::string_view, EMemberType> members{};
	std::map<std::string_view, std::unique_ptr<VariableEntry>> field_variables{};
	std::map<std::string_view, std::unique_ptr<VariableEntry>> static_variable{};
	std::map<std::string_view, std::unique_ptr<FunctionEntry>> constructors{};
	std::map<std::string_view, std::unique_ptr<FunctionEntry>> functions{};
	std::map<std::string_view, std::unique_ptr<FunctionEntry>> methods{};

	template <typename T>
	friend constexpr T& operator<<(T& out, const Type& tk)
	{
		out << tk.name << ":\n";
		for (const auto& [name, member] : tk.field_variables)
			out << "\tfield " << *member << "\n";

		for (const auto& [name, member] : tk.static_variable)
			out << "\tstatic " << *member << "\n";

		out << "\n";
		for (const auto& [name, member] : tk.constructors)
			out << "\tconstructor " << *member << "\n";

		for (const auto& [name, member] : tk.functions)
			out << "\tfunction " << *member << "\n";

		for (const auto& [name, member] : tk.methods)
			out << "\tmethod " << *member << "\n";

		return out;
	}
};

struct VariableEntry
{
	const std::string_view name{};
	const Type* type{};
	int index{};

	template <typename T>
	friend constexpr T& operator<<(T& out, const VariableEntry& tk)
	{
		out << "[" << tk.index << " " << tk.type->name << " " << tk.name << "]";
		return out;
	}
};

struct FunctionEntry
{
	const std::string_view name{};
	const Type* enclosing_class_type{};

	std::optional<const Type*> return_type = std::nullopt;

	std::vector<VariableEntry> parameters{};
	std::vector<VariableEntry> locals{};
	std::map<std::string_view, EFuncVariableType> members{};

	std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> statements{};

	template <typename T>
	friend constexpr T& operator<<(T& out, const FunctionEntry& tk)
	{
		if (tk.return_type == std::nullopt)
			out << "void ";
		else out << tk.return_type.value()->name << " ";

		out << tk.enclosing_class_type->name << "." << tk.name << "(";
		for (int i = 0; i < tk.parameters.size(); ++i)
		{
			out << tk.parameters[i];
			if (i != tk.parameters.size() - 1)
				out << ", ";
		}

		out << ")\n\t{\n";

		for (int i = 0; i < tk.locals.size(); ++i)
			out << "\t\tvar " << tk.locals[i] << "\n";

		out << "\t\t";
		if (!tk.statements)
			out << "NO BODY\n\t}";
		else
			out << *tk.statements << "\n\t}";

		return out;
	}
};

namespace jack::compiler
{
	export struct SymbolTable
	{
		std::map<std::string_view, std::unique_ptr<Type>> classes;

		static std::expected<std::unique_ptr<SymbolTable>, std::string> init_table(std::vector<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>>);
		std::expected<std::string, std::string> generate_vm_code() const;
	};
}