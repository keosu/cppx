import std;

int main() {
  std::expected<int, std::string> value = 42;
  std::vector<int> numbers{1, 2, 3};
  if (!value ||
      std::format("{}:{}", *value, std::accumulate(numbers.begin(), numbers.end(), 0)) != "42:6") {
    return 1;
  }
  std::cout << "import std probe passed\n";
}
