#include <expected>
#include <format>
#include <iostream>
#include <ranges>
#include <string>
#include <version>

int main() {
  std::expected<int, std::string> value = 42;
  int sum = 0;
  for (int number : std::views::iota(1, 4)) sum += number;
  if (!value || std::format("{}:{}", *value, sum) != "42:6") return 1;

#if defined(_MSC_FULL_VER)
  std::cout << "msvc=" << _MSC_FULL_VER << '\n';
#endif
#if defined(__clang__)
  std::cout << "clang=" << __clang_version__ << '\n';
#elif defined(__GNUC__)
  std::cout << "gcc=" << __VERSION__ << '\n';
#endif
#if defined(_MSVC_STL_UPDATE)
  std::cout << "msvc_stl=" << _MSVC_STL_UPDATE << '\n';
#elif defined(_LIBCPP_VERSION)
  std::cout << "libcxx=" << _LIBCPP_VERSION << '\n';
#elif defined(__GLIBCXX__)
  std::cout << "libstdcxx=" << __GLIBCXX__ << '\n';
#endif
  std::cout << "C++23 API probe passed\n";
}
