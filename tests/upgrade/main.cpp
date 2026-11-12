import std;
import cppx.error;
import cppx.string_utils;
import cppx.file_system;
import cppx.serialization;
import cppx.concurrent;

// This intentionally uses the common 0.1 API and ASCII/small-integer storage schema.
// Newly widened integer/Unicode semantics need product-specific migration fixtures.
int main(int argc, char** argv) {
  if (argc != 3) return 2;
  try {
    int expected = std::stoi(argv[2]);
    int visits = 0;
    if (std::filesystem::exists(argv[1])) {
      auto content = cppx::read_file_text(argv[1]);
      if (content.is_err()) throw std::runtime_error("Cannot read upgrade fixture");
      visits = cppx::json::parse(content.value())["visits"].as_int();
    }
    if (visits != expected - 1) throw std::runtime_error("Previous phase state mismatch");
    cppx::thread_pool pool(2);
    auto next = pool.submit([visits] { return visits + 1; }).get();
    auto data = cppx::json::object();
    data["visits"] = next;
    data["schema"] = "v1";
    if (cppx::write_file(argv[1], data.dump(), false).is_err())
      throw std::runtime_error("Cannot write upgrade fixture");
    return next == expected ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
