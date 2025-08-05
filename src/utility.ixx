export module uni_cpp:utility;

export import std;

namespace upp::impl
{
    template<typename T, typename... Ts>
    constexpr bool is_any_of = (std::is_same_v<T, Ts> || ...);
} // namespace upp::impl