import std;
import jack;
import helpers;

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    jack::simulator::SimulatorConfig config{};

    config.debug_stream = constexpr_ostream{};
    config.ram = jack::load_file<std::int16_t, 24577>("memory_input.txt");
    config.instructions = jack::load_file<std::uint16_t, 32768>("instructions.txt");
    
    jack::simulator::simulate(config);

    std::ofstream file_writer(fs::current_path() / "memory_debug.txt");
    file_writer << std::get<constexpr_ostream>(config.debug_stream).str() << std::flush;
    file_writer.close();

    jack::save_file(fs::current_path() / "memory_output.txt", config.ram);

    return 0;
}