import std;
import cppx.serialization;

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  try {
    std::ifstream input(argv[1]);
    if (!input) throw std::runtime_error("Cannot open differential inputs");
    std::ofstream output(argv[2]);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    std::string line;
    while (std::getline(input, line)) output << cppx::json::parse(line).dump() << '\n';
    if (input.bad() || !input.eof()) throw std::runtime_error("Incomplete differential input");
    output.close();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
