import std;
import jack;
import helpers;

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    fs::path path = fs::current_path() / "program.asm";

    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string s = buffer.str();

    jack::assembler::generate_binary(s);

    return 0;
}