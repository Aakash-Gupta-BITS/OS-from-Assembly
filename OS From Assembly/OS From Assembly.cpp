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

    jack::vm::get_ast(buffer.str());

    return 0;
}