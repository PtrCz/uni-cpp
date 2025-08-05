export module uni_cpp:utility;

export import std;

namespace upp::impl
{
    template<typename T, typename U, typename... Ts>
    struct is_any_of_t : is_any_of_t<T, Ts...>
    {
    };

    template<typename T, typename... Ts>
    struct is_any_of_t<T, T, Ts...> : std::true_type
    {
    };

    template<typename T, typename U>
    struct is_any_of_t<T, U> : std::false_type
    {
    };

    template<typename T, typename... Ts>
    constexpr bool is_any_of = is_any_of_t<Ts...>::value;

} // namespace upp::impl