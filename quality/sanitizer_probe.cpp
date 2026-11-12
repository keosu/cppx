import std;

// A clean control and intentional defects verify that instrumentation is active.
// Negative controls are invoked only by scripts/verify.py, never by xmake test.
int main(int argc, char** argv) {
  if (argc < 2) return std::vector<int>{42}.front() == 42 ? 0 : 1;
  std::string_view mode(argv[1]);
  if (mode == "address") {
    auto data = std::make_unique<int[]>(4);
    volatile int index = argc + 2;
    data[index] = 42;
    std::cout << data[index];
  } else if (mode == "undefined") {
    volatile int maximum = std::numeric_limits<int>::max();
    std::cout << maximum + argc;
  } else if (mode == "thread") {
    int shared = 0;
    auto increment = [&] {
      for (int n = 0; n < 10000; ++n) ++shared;
    };
    std::thread a(increment), b(increment);
    a.join();
    b.join();
    std::cout << shared;
  } else {
    return 2;
  }
}
