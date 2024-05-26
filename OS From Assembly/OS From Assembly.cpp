import std;
import jack;
import helpers;
import compiler;

using namespace std;
using namespace jack::compiler;

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    fs::path path1 = fs::current_path() / "program.jack";
    fs::path path2 = fs::current_path() / "Main.jack";

    std::vector<string> file_contents;
    std::vector<typename std::remove_cvref_t<decltype(get_ast("").value())>> arr;

    for (auto &path: {path1, path2})
    {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file_contents.push_back(buffer.str());

        auto ast = get_ast(file_contents.back());

        if (!ast.has_value())
        {
            std::cerr << ast.error() << std::endl;
            return 0;
        }

        arr.push_back(std::move(ast.value()));
    }

    auto res = SymbolTable::init_table(std::move(arr));

    if (!res.has_value())
        std::cerr << res.error() << std::endl;
    else
        for (auto &[class_name, class_entry]: res.value()->classes)
            std::cout << *class_entry << std::endl;

    return 0;
}