import std;
import jack;
import helpers;

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    jack::simulator::SimulatorConfig config{};

    config.ram = jack::load_file<std::int16_t, 24577>("memory_input.txt");
    config.instructions = jack::load_file<std::uint16_t, 32768>("instructions.txt");
    
    jack::simulator::simulate(config);

    jack::save_file(fs::current_path() / "memory_output.txt", config.ram);

    return 0;
}