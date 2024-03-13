import std;
import jack;
import helpers;

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    fs::path path = fs::current_path() / "program.vm";

    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    auto file_content = buffer.str();

    auto ast = jack::vm::get_ast(file_content);
    cout << jack::vm::generate_assembly(std::move(ast.value())).error();

    return 0;
}