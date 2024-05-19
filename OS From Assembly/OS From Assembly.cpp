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
    cout << **ast << endl;
    cout << *jack::vm::generate_assembly("program.vm", std::move(ast.value()));

    return 0;
}