export module jack:utils;
import std;

namespace fs = std::filesystem;

namespace jack
{
	export template <std::integral int_type, std::size_t N> 
		auto load_file(fs::path file_path) -> std::array<int_type, N>
	{
		std::array<int_type, N> input{};

		std::ifstream file{ file_path };
		if (!file.is_open())
			throw fs::filesystem_error("Unable to open file for reading", file_path, std::make_error_code(std::errc::no_such_file_or_directory));

		std::string line;
		std::size_t i = 0;
		while (std::getline(file, line))
		{
			auto &val = input[i++];

			for (char c : line)
			{
				if (c != '0' && c != '1')
					continue;

				val <<= 1;
				val |= (c - '0');
			}
		}

		return input;
	}

	export template <std::integral int_type, std::size_t N>
		auto save_file(fs::path file_path, std::array<int_type, N> const& output) -> void
	{
		std::ofstream file{ file_path };

		if (!file.is_open())
			throw fs::filesystem_error("Unable to open file for writing", file_path, std::make_error_code(std::errc::no_such_file_or_directory));

		for (auto &x: output)
			file << std::format("{:0{}b}\n", x, 8 * sizeof(int_type));
		file << std::flush;
	}
}