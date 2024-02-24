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

    auto [binary, errors] = jack::assembler::generate_binary(s);


    fs::path bin_path = fs::current_path() / "program.out";

    std::ofstream ofile(bin_path);
    for (auto& x : binary)
        ofile << std::bitset<16>(x) << endl;

    return 0;
}