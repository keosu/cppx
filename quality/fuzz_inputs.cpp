import std;
import cppx.serialization;
import cppx.string_utils;
import cppx.time_utils;
import cppx.cli;

void require(bool value, const char* message) {
  if (!value) throw std::logic_error(message);
}

void exercise(std::string_view input) {
  std::optional<cppx::json> parsed;
  try {
    parsed = cppx::json::parse(input);
  } catch (const cppx::json_parse_error&) {
  }
  if (parsed) {
    auto canonical = parsed->dump();
    require(cppx::json::parse(canonical).dump() == canonical, "JSON canonical round trip");
  }
  if (auto integer = cppx::parse_int(input))
    require(cppx::parse_int(std::to_string(integer.value())).value() == integer.value(),
            "Integer round trip");
  if (auto number = cppx::parse_double(input); number && std::isfinite(number.value())) {
    auto canonical = std::format("{:.17g}", number.value());
    require(cppx::parse_double(canonical).value() == number.value(), "Double round trip");
  }
  if (auto time = cppx::parse_iso8601(input)) {
    auto canonical = cppx::format_iso8601(time.value());
    require(cppx::parse_iso8601(canonical).value() == time.value(), "Time round trip");
  }
  cppx::argument_parser parser("fuzz");
  parser.add_option("value", "v", "value");
  parser.add_flag("verbose", "b", "verbose");
  auto tokens = cppx::split(input, " ");
  (void)parser.parse(tokens);
}

// Same entry point can be linked with Clang -fsanitize=fuzzer.
extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size) {
  if (size <= 65536) exercise(std::string_view(reinterpret_cast<const char*>(data), size));
  return 0;
}

#ifndef CPPX_LIBFUZZER
int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "Usage: fuzz_inputs CORPUS ITERATIONS SEED FAILURE_FILE\n";
    return 2;
  }
  std::string current;
  auto persist = [&] {
    std::ofstream failure(argv[4], std::ios::binary | std::ios::trunc);
    failure.write(current.data(), static_cast<std::streamsize>(current.size()));
    failure.close();
    require(static_cast<bool>(failure), "Cannot persist reproducer");
  };
  try {
    std::vector<std::string> seeds;
    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(argv[1]))
      if (entry.is_regular_file()) paths.push_back(entry.path());
    std::ranges::sort(paths);
    for (const auto& path : paths) {
      std::ifstream stream(path, std::ios::binary);
      seeds.emplace_back(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
      require(stream.eof() || !stream.fail(), "Cannot read corpus");
      current = seeds.back();
      persist();
      exercise(current);
    }
    require(!seeds.empty(), "Empty corpus");
    auto count = std::stoull(argv[2]);
    auto seed = std::stoull(argv[3]);
    std::mt19937_64 random(seed);
    for (size_t i = 0; i < count; ++i) {
      current = seeds[random() % seeds.size()];
      for (size_t edit = 0, edits = 1 + random() % 8; edit < edits; ++edit) {
        size_t position = random() % (current.size() + 1);
        switch (random() % 3) {
          case 0: current.insert(position, 1, static_cast<char>(random() % 256)); break;
          case 1:
            if (position < current.size()) current.erase(position, 1);
            break;
          default:
            if (position < current.size()) current[position] = static_cast<char>(random() % 256);
        }
      }
      // Persist the in-flight input before executing it: sanitizer aborts cannot be caught.
      persist();
      exercise(current);
    }
    std::filesystem::remove(argv[4]);
    std::cout << "{\"iterations\":" << count << ",\"seed\":" << seed
              << ",\"corpus_size\":" << seeds.size() << ",\"passed\":true}\n";
    return 0;
  } catch (const std::exception& error) {
    std::ofstream failure(argv[4], std::ios::binary | std::ios::trunc);
    failure.write(current.data(), static_cast<std::streamsize>(current.size()));
    std::cerr << error.what() << '\n';
    return 1;
  }
}
#endif
