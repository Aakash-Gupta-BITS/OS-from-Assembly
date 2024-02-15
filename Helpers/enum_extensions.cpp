module helpers:enum_extensions;

// TESTING
enum class TestEnum
{
    A, B, C
};

static_assert(get_enum_array<TestEnum>().size() == 3);
static_assert(get_enum_array<TestEnum>()[0] == "A");
static_assert(get_enum_array<TestEnum>()[1] == "B");
static_assert(get_enum_array<TestEnum>()[2] == "C");