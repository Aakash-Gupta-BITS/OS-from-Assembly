#pragma once
#include <vector>
#include <string>
#include <string_view>
#include "Parser.h"

enum class VariableType
{
    INTEGER,
    CHAR,
    BOOLEAN,
    VOID,
    USER
};

enum class VariableCategory
{
    UNINITIALISED,
    STATIC,
    FIELD,
    ARGUMENT,
    LOCAL
};

enum class MethodCategory
{
    CONSTRUCTOR,
    FUNCTION,
    METHOD
};


struct ASTNode
{
    int sym_index = 0;

    Token* token = nullptr;
    std::vector<ASTNode*> children {};
    ASTNode* sibling = nullptr;

    friend std::ostream& operator<<(std::ostream&, const ASTNode&);

    ~ASTNode()
    {
        delete token;

        for (auto &x: children)
            delete x;
        delete sibling;
    }
};


class ClassLevelTable;
class GlobalTable;
ASTNode* createAST(const ParseTreeNode* input, const ParseTreeNode* parent = nullptr, ASTNode* inherited = nullptr);

struct VariableEntry
{
    std::string var_name;
    std::string var_type_name;
    VariableType var_type;
    VariableCategory var_category;
    int index;
    ClassLevelTable* type_entry;
};

class FunctionEntry
{
    MethodCategory category;
    std::string function_name;

    std::string return_type_name;
    VariableType return_type;
    ClassLevelTable* return_type_entry;

    std::vector<VariableEntry*> parameters;
    std::vector<VariableEntry*> locals;
    ASTNode* body;

    std::set<std::string_view> local_names;
public:
    explicit FunctionEntry(ASTNode*);
    friend class GlobalTable;
};

class ClassLevelTable
{
    std::string class_name;
    std::vector<VariableEntry*> variables;
    std::vector<FunctionEntry*> functions;

    std::set<std::string_view> member_names;
public:
    explicit ClassLevelTable(ASTNode*);
    friend class GlobalTable;
    friend class FunctionEntry;
};

class GlobalTable
{
    static GlobalTable* singleton_;

    GlobalTable() = default;
    std::vector<ClassLevelTable*> classes;

    std::map<std::string_view, ClassLevelTable*> class_type_map;
    bool check_names_called = false;
public:

    GlobalTable(GlobalTable &other) = delete;
    void operator=(const GlobalTable &) = delete;

    friend ASTNode* createAST(const ParseTreeNode* input, const ParseTreeNode* parent, ASTNode* inherited);

    static GlobalTable *getInstance()
    {
        if (GlobalTable::singleton_ == nullptr)
            GlobalTable::singleton_ = new GlobalTable();
        return GlobalTable::singleton_;
    }
    void add_class(ParseTreeNode*);
    void check_names();
};
