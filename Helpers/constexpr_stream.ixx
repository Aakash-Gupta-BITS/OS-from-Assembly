export module helpers:constexpr_stream;
import std;

export class constexpr_ostream
{
	std::string output;
public:
	constexpr constexpr_ostream() = default;

	constexpr constexpr_ostream& operator<<(std::string_view str)
	{
		output += str;
		return *this;
	}

	constexpr constexpr_ostream& operator<<(const char* str)
	{
		output += str;
		return *this;
	}

	constexpr constexpr_ostream& operator<<(const char c)
	{
		output += c;
		return *this;
	}

	constexpr constexpr_ostream& operator<<(std::signed_integral auto i)
	{
		if (i == std::numeric_limits<decltype(i)>::min())
		{
			if constexpr (std::is_same_v<decltype(i), std::int8_t>)
				output += "-128";
			else if constexpr (std::is_same_v<decltype(i), std::int16_t>)
				output += "-32768";
			else if constexpr (std::is_same_v<decltype(i), std::int32_t>)
				output += "-2147483648";
			else if constexpr (std::is_same_v<decltype(i), std::int64_t>)
				output += "-9223372036854775808";
			return *this;
		}

		if (i == 0)
		{
			output += '0';
			return *this;
		}

		if (i < 0)
		{
			output += "-";
			i = -i;
		}

		std::string digits{};
		while (i)
		{
			const auto digit = i % 10;
			digits += static_cast<char>('0' + digit);
			i /= 10;
		}

		std::reverse(digits.begin(), digits.end());
		output += digits;
		return *this;
	}

	constexpr constexpr_ostream& operator<<(std::unsigned_integral auto i)
	{
		std::string digits{};
		
		if (i == 0)
		{
			output += '0';
			return *this;
		}

		while (i)
		{
			const auto digit = i % 10;
			digits += static_cast<char>('0' + digit);
			i /= 10;
		}

		std::reverse(digits.begin(), digits.end());
		output += digits;
		return *this;
	}

	constexpr constexpr_ostream& operator<<(const bool b)
	{
		output += b ? "true" : "false";
		return *this;
	}

	constexpr constexpr_ostream& operator<<(std::nullptr_t)
	{
		output += "nullptr";
		return *this;
	}

	template <typename T>
	constexpr constexpr_ostream& operator<<(const T* ptr)
	{
		if (ptr == nullptr)
		{
			output += "nullptr";
			return *this;
		}

		std::uint64_t address = std::bit_cast<std::uint64_t>(ptr);
		std::string hex_digits;
		while (address)
		{
			const auto digit = address % 16;
			if (digit < 10)
				hex_digits += static_cast<char>('0' + digit);
			else
				hex_digits += static_cast<char>('A' + digit - 10);
			address /= 16;
		}

		while (hex_digits.size() < 16)
			hex_digits += '0';
		std::reverse(hex_digits.begin(), hex_digits.end());
		output += "0x" + hex_digits;
		return *this;
	}

	template <typename T>
	constexpr constexpr_ostream& operator<<(const std::unique_ptr<T>& t)
	{
		return *this << t.get();
	}

	template <typename T>
	constexpr constexpr_ostream& operator<<(const std::shared_ptr<T>& t)
	{
		return *this << t.get();
	}

	constexpr auto str() const -> const std::string&
	{
		return output;
	}

	constexpr auto sv() const -> std::string_view
	{
		return output;
	}
};

export class null_ostream
{
public:
	constexpr null_ostream& operator<<(auto t)
	{
		return *this;
	}

	constexpr auto str() const -> const std::string&
	{
		return "";
	}

	constexpr auto sv() const -> std::string_view
	{
		return "";
	}
};

export class constexpr_iostream
{
	int start_index = -1;
	std::vector<std::string> input;

	constexpr std::string& pop_front()
	{
		if (start_index == input.size() - 1)
			throw "No more data found";
		return input[++start_index];
	}

	constexpr auto str_to_uint64(std::string_view str) const -> std::uint64_t
	{
		std::uint64_t result = 0;
		for (auto c : str)
		{
			if (c < '0' || c > '9')
				throw "Invalid character found in input stream";

			result *= 10;
			result += (std::uint8_t)(c - '0');
		}

		return result;
	}

public:
	constexpr constexpr_iostream() = default;

	template <typename T>
	constexpr auto& operator<<(T&& t)
	{
		constexpr_ostream stream{};
		stream << t;
		input.emplace_back(stream.str());
		return *this;
	}

	constexpr auto& operator>>(bool& b)
	{
		b = pop_front() == "true";
		return *this;
	}

	constexpr auto& operator>>(std::string& str)
	{
		str = pop_front();
		return *this;
	}

	constexpr auto& operator>>(std::unsigned_integral auto& i)
	{
		std::uint64_t result = str_to_uint64(pop_front());

		using itype = std::remove_cvref_t<decltype(i)>;

		if (result < std::numeric_limits<itype>::min() || result > std::numeric_limits<itype>::max())
			throw "Integer overflow";

		i = static_cast<itype>(result);
		return *this;
	}

	constexpr auto& operator>>(std::signed_integral auto& i)
	{
		auto str = std::string_view(pop_front());
		uint64_t result = str_to_uint64(str[0] == '-' ? str.substr(1) : str);

		using itype = std::remove_cvref_t<decltype(i)>;

		i = static_cast<itype>(result);
		if (str[0] == '-')
			i = -i;

		return *this;
	}
};