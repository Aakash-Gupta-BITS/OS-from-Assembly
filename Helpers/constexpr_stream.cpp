module helpers:constexpr_stream;


namespace ConstexprStreamTest
{
	consteval static bool test_c_style_array()
	{
		constexpr_ostream stream{};
		stream << "Hello, world!";
		return stream.sv() == "Hello, world!";
	}

	consteval static bool test_string()
	{
		constexpr_ostream stream{};
		std::string str = "Hello";
		stream << str;

		constexpr_iostream iostream;
		iostream << "Hemkd";
		std::string out;
		iostream >> out;

		return stream.sv() == "Hello" && out == "Hemkd";
	}

	consteval static bool test_char()
	{
		constexpr_ostream stream{};
		stream << 'a' << 'b';
		return stream.sv() == "ab";
	}

	consteval static bool test_signed_int()
	{
		constexpr_ostream stream{};
		stream
			<< std::int8_t{ -128 }
			<< std::int16_t{ -32768 }
			<< std::int32_t{ -2147483648 }
			<< std::int64_t{ -9223372036854775807l }
			<< " "
			<< 159;

		constexpr_iostream iostream;
		iostream << -9223372036854775807l;
		std::int64_t i;
		iostream >> i;

		return stream.sv() == "-128-32768-2147483648-9223372036854775807 159" && i == -9223372036854775807l;
	}

	consteval static bool test_unsigned_int()
	{
		constexpr_ostream stream{};
		stream
			<< std::uint8_t{ 255 }
			<< std::uint16_t{ 65535 }
			<< std::uint32_t{ 4294967295 }
			<< std::uint64_t{ 18446744073709551615ul }
			<< " "
			<< 159u;

		constexpr_iostream iostream;
		iostream << 18446744073709551615ul;
		std::uint64_t i;
		iostream >> i;
		return stream.sv() == "25565535429496729518446744073709551615 159" && i == 18446744073709551615ul;
	}

	consteval static bool test_bool()
	{
		constexpr_ostream ostream{};
		ostream << true << false;

		constexpr_iostream iostream;
		iostream << true << false;

		bool b1, b2;
		iostream >> b1 >> b2;

		return ostream.sv() == "truefalse" && b1 && !b2;
	}

	consteval static bool test_nullptr()
	{
		constexpr_ostream stream{};
		stream << nullptr;
		return stream.sv() == "nullptr";
	}

	consteval static bool test_pointer()
	{
		constexpr_ostream stream{};
		void* ptr = nullptr;
		stream << ptr;
		// idk how to cast an address to void* in constexpr
		// Maybe relevant: https://stackoverflow.com/questions/70543372/how-to-cast-from-const-void-in-a-constexpr-expression
		return stream.sv() == "nullptr";
	}

	static_assert(test_c_style_array(), "C-style array test failed!");
	static_assert(test_string(), "String test failed!");
	static_assert(test_char(), "Char test failed!");
	static_assert(test_signed_int(), "Signed int test failed!");
	static_assert(test_unsigned_int(), "Unsigned int test failed!");
	static_assert(test_bool(), "Bool test failed!");
	static_assert(test_nullptr(), "Nullptr test failed!");
	static_assert(test_pointer(), "Void test failed!");
}