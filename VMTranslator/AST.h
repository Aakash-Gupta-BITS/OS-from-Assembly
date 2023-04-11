#pragma once
#include "Parser.h"
#include <vector>

struct TypeLog;
std::ostream& operator<<(std::ostream&, const TypeLog&);

struct ASTNode
{
    int isLeaf = 0;
    int sym_index = 0;

    Token* token = nullptr;
    std::vector<ASTNode*> children;
    ASTNode* sibling = nullptr;

    friend std::ostream& operator<<(std::ostream&, const ASTNode&);
};

ASTNode* createAST(const ParseTreeNode*, const ParseTreeNode* parent = nullptr);