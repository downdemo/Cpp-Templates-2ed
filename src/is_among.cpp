#include <tuple>
#include <type_traits>

namespace jc {

template <typename T, typename Tuple>
struct is_among;

template <typename T, template <typename...> class Tuple, typename... List>
struct is_among<T, Tuple<List...>>
    : std::disjunction<std::is_same<T, List>...> {};

template <typename T, typename Tuple>
inline constexpr bool is_among_v = is_among<T, Tuple>::value;

}  // namespace jc

static_assert(jc::is_among_v<int, std::tuple<char, int, double>>);
static_assert(!jc::is_among_v<float, std::tuple<char, int, double>>);

int main() {}