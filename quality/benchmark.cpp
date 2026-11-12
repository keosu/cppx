#include "metrics.h"
#ifdef _WIN32
  #include <malloc.h>
#endif

import std;
import cppx.serialization;
import cppx.concurrent;
import cppx.logging;
import cppx.file_system;

namespace {
std::atomic<size_t> allocations = 0, allocated_bytes = 0;
void record(size_t size) {
  ++allocations;
  allocated_bytes += size;
}
}  // namespace
void* operator new(size_t size) {
  if (auto memory = std::malloc(std::max<size_t>(size, 1))) {
    record(size);
    return memory;
  }
  throw std::bad_alloc();
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, size_t) noexcept { std::free(memory); }
void* operator new(size_t size, std::align_val_t alignment) {
  auto align = static_cast<size_t>(alignment);
  void* memory = nullptr;
#ifdef _WIN32
  memory = _aligned_malloc(std::max<size_t>(size, 1), align);
#else
  if (posix_memalign(&memory, align, std::max<size_t>(size, 1)) != 0) memory = nullptr;
#endif
  if (!memory) throw std::bad_alloc();
  record(size);
  return memory;
}
void* operator new[](size_t size, std::align_val_t alignment) {
  return ::operator new(size, alignment);
}
void operator delete(void* memory, std::align_val_t) noexcept {
#ifdef _WIN32
  _aligned_free(memory);
#else
  std::free(memory);
#endif
}
void operator delete[](void* memory, std::align_val_t alignment) noexcept {
  ::operator delete(memory, alignment);
}
void operator delete(void* memory, size_t, std::align_val_t alignment) noexcept {
  ::operator delete(memory, alignment);
}
void operator delete[](void* memory, size_t, std::align_val_t alignment) noexcept {
  ::operator delete(memory, alignment);
}

size_t peak_resident_bytes() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS counters{};
  if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    throw std::runtime_error("Cannot measure peak memory");
  return counters.PeakWorkingSetSize;
#else
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0) throw std::runtime_error("Cannot measure peak memory");
  #ifdef __APPLE__
  return static_cast<size_t>(usage.ru_maxrss);
  #else
  return static_cast<size_t>(usage.ru_maxrss) * 1024;
  #endif
#endif
}

template <typename F>
cppx::json measure(std::string name, size_t count, F operation) {
  using clock = std::chrono::steady_clock;
  for (size_t i = 0; i < 100; ++i) operation();
  std::vector<double> times;
  times.reserve(count);
  auto before_count = allocations.load(), before_bytes = allocated_bytes.load();
  auto start = clock::now();
  for (size_t i = 0; i < count; ++i) {
    auto before = clock::now();
    operation();
    times.push_back(std::chrono::duration<double, std::micro>(clock::now() - before).count());
  }
  auto elapsed = std::chrono::duration<double>(clock::now() - start).count();
  auto total_count = allocations.load() - before_count;
  auto total_bytes = allocated_bytes.load() - before_bytes;
  std::ranges::sort(times);
  auto percentile = [&](double fraction) {
    return times[static_cast<size_t>(std::ceil(fraction * count)) - 1];
  };
  auto result = cppx::json::object();
  result["name"] = name;
  result["operations"] = count;
  result["seconds"] = elapsed;
  result["operations_per_second"] = count / elapsed;
  result["p50_us"] = percentile(.50);
  result["p95_us"] = percentile(.95);
  result["p99_us"] = percentile(.99);
  result["allocation_calls"] = total_count;
  result["allocated_bytes"] = total_bytes;
  result["process_peak_resident_bytes"] = peak_resident_bytes();
  return result;
}

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  try {
    size_t count = std::stoull(argv[1]);
    if (count < 100) return 2;
    auto report = cppx::json::object();
    auto results = cppx::json::array();
    std::string document =
        R"({"id":18446744073709551615,"name":"cppx","values":[1,2,3,4,5],"ok":true})";
    report["json_input_bytes"] = document.size();
    results.push_back(measure("json_parse_dump", count, [&] {
      auto output = cppx::json::parse(document).dump();
      if (output.empty()) throw std::runtime_error("Empty JSON output");
    }));
    cppx::thread_pool pool(2, 64);
    results.push_back(measure("pool_submit_get_single_flight", count, [&] {
      if (pool.submit([] { return 42; }).get() != 42) throw std::runtime_error("Pool mismatch");
    }));
    cppx::channel<int> requests(64), responses(64);
    std::jthread receiver([&] {
      while (auto value = requests.receive()) responses.send(*value);
      responses.close();
    });
    // Close on exceptions too, before jthread joins a blocked receiver.
    struct close_guard {
      cppx::channel<int>& queue;
      ~close_guard() { queue.close(); }
    } guard{requests};
    results.push_back(measure("channel_round_trip", count, [&] {
      requests.send(42);
      if (responses.receive() != 42) throw std::runtime_error("Channel mismatch");
    }));
    cppx::temp_dir directory("benchmark");
    auto log = cppx::create_logger("benchmark");
    log->remove_console_sink();
    log->add_file_sink(directory.path() / "log", 0);
    results.push_back(measure("file_log_write_flush", count, [&] {
      log->info("representative log record {}", 42);
      log->flush();
    }));
    log->remove_file_sink();
    report["schema_version"] = 1;
    report["hardware_threads"] = std::thread::hardware_concurrency();
    report["pointer_bits"] = sizeof(void*) * 8;
#if defined(__clang__)
    report["compiler"] = std::string("clang ") + __clang_version__;
#elif defined(_MSC_FULL_VER)
    report["compiler"] = std::string("msvc ") + std::to_string(_MSC_FULL_VER);
#else
    report["compiler"] = std::string("gcc ") + __VERSION__;
#endif
    report["results"] = std::move(results);
    std::ofstream output(argv[2]);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << report.dump(2) << '\n';
    output.close();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
