export module jack.vm:backend;
import :frontend;

import compiler;
import std;
import helpers;

namespace jack::vm
{
    export auto generate_assembly(std::string_view file_name, std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> ast_root) -> std::expected<std::string, std::string>;
}
