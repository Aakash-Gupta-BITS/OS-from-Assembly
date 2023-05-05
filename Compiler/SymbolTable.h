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
    std::vector<ASTNode*> children;
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

struct VariableEntry
{
    std::string var_name;
    VariableType var_type;
    VariableCategory var_category;
    int index;
    ClassLevelTable* type_entry;
};

struct FunctionEntry
{
    MethodCategory category;
    std::string function_name;
    VariableType return_type;
    ClassLevelTable* return_type_entry;
    std::vector<VariableEntry*> parameters;
    std::vector<VariableEntry*> locals;
    ASTNode* body;

    explicit FunctionEntry(ASTNode*);
};

class ClassLevelTable
{
    std::string class_name;
    std::vector<VariableEntry*> variables;
    std::vector<FunctionEntry*> functions;

public:
    explicit ClassLevelTable(ASTNode*);
    friend class GlobalTable;
};

class GlobalTable
{
    static GlobalTable* singleton_;

    GlobalTable() = default;
public:
    std::vector<ClassLevelTable*> classes;

    GlobalTable(GlobalTable &other) = delete;
    void operator=(const GlobalTable &) = delete;

    static GlobalTable *getInstance()
    {
        if (GlobalTable::singleton_ == nullptr)
            GlobalTable::singleton_ = new GlobalTable();
        return GlobalTable::singleton_;
    }
    void add_class(ParseTreeNode*);
};
