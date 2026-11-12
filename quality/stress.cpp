#include "metrics.h"

import std;
import cppx.concurrent;
import cppx.time_utils;
import cppx.file_system;
import cppx.logging;

using namespace std::chrono_literals;
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
size_t resident_bytes() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS counters{};
  if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    throw std::runtime_error("GetProcessMemoryInfo failed");
  return counters.WorkingSetSize;
#elif defined(__linux__)
  std::ifstream statm("/proc/self/statm");
  size_t pages = 0, resident = 0;
  if (!(statm >> pages >> resident)) throw std::runtime_error("Cannot read process memory");
  return resident * static_cast<size_t>(sysconf(_SC_PAGESIZE));
#else
  return 0;  // Unavailable, never used to claim a leak-free result.
#endif
}
std::pair<size_t, size_t> handles_and_threads() {
#ifdef _WIN32
  DWORD handles = 0;
  if (!GetProcessHandleCount(GetCurrentProcess(), &handles))
    throw std::runtime_error("Cannot measure process handles");
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot snapshot threads");
  THREADENTRY32 entry{};
  entry.dwSize = sizeof(entry);
  size_t threads = 0;
  if (Thread32First(snapshot, &entry)) {
    do {
      if (entry.th32OwnerProcessID == GetCurrentProcessId()) ++threads;
    } while (Thread32Next(snapshot, &entry));
  }
  CloseHandle(snapshot);
  if (threads == 0) throw std::runtime_error("Cannot measure process threads");
  return {handles, threads};
#elif defined(__linux__)
  size_t handles = 0, threads = 0;
  for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator("/proc/self/fd"))
    ++handles;
  for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator("/proc/self/task"))
    ++threads;
  return {handles, threads};
#else
  return {0, 0};
#endif
}
void round(size_t iteration) {
  cppx::channel<int> channel(1 + iteration % 17);
  std::atomic<size_t> accepted = 0, received = 0;
  std::atomic<long long> sent_sum = 0, received_sum = 0;
  std::vector<std::jthread> receivers, senders;
  for (int i = 0; i < 3; ++i)
    receivers.emplace_back([&] {
      while (auto value = channel.receive()) {
        ++received;
        received_sum += *value;
      }
    });
  for (int i = 0; i < 4; ++i)
    senders.emplace_back([&, i] {
      for (int j = 0; j < 200; ++j) {
        int value = i * 200 + j;
        if (channel.send_for(value, 10ms)) {
          ++accepted;
          sent_sum += value;
        } else
          break;
      }
    });
  std::this_thread::sleep_for(std::chrono::microseconds(100 + iteration % 1000));
  channel.close();
  senders.clear();
  receivers.clear();
  require(accepted == received && sent_sum == received_sum, "Channel lost accepted data");

  cppx::thread_pool pool(3, 8);
  std::atomic<size_t> submitted = 0, completed = 0;
  std::vector<std::jthread> producers;
  for (int i = 0; i < 3; ++i)
    producers.emplace_back([&] {
      for (int j = 0; j < 60; ++j) {
        std::future<int> result;
        try {
          result = pool.submit_for(10ms, [&] {
            ++completed;
            return 42;
          });
        } catch (const std::runtime_error&) {
          break;
        }
        ++submitted;
        require(result.get() == 42, "Pool result mismatch");
      }
    });
  std::this_thread::sleep_for(200us);
  pool.close();
  producers.clear();
  pool.shutdown();
  require(submitted == completed, "Pool lost accepted work");

  cppx::timer timer;
  std::atomic<size_t> callbacks = 0;
  timer.start_periodic(1ms, [&] { ++callbacks; });
  std::jthread stop1([&] { timer.stop(); }), stop2([&] { timer.stop(); });
  stop1.join();
  stop2.join();
  auto stopped_at = callbacks.load();
  require(!timer.is_running(), "Timer still running after joined stops");

  cppx::temp_dir directory("stress");
  auto log = cppx::create_logger("stress");
  log->remove_console_sink();
  log->add_file_sink(directory.path() / "log", 1024);
  std::jthread levels([&] {
    for (int i = 0; i < 200; ++i)
      log->set_level(i % 2 ? cppx::log_level::info : cppx::log_level::debug);
  });
  std::vector<std::jthread> writers;
  for (int i = 0; i < 2; ++i)
    writers.emplace_back([&] {
      for (int j = 0; j < 10; ++j) log->info("message {}", j);
    });
  writers.clear();
  levels.join();
  log->flush();
  log->remove_file_sink();
  auto path = directory.path() / "data";
  require(cppx::write_file_atomic(path, "complete").is_ok(), "Atomic write failed");
  require(cppx::read_file_text(path).value() == "complete", "Atomic content mismatch");
  require(callbacks == stopped_at, "Timer callback ran after stop returned");
}
int main(int argc, char** argv) {
  if (argc != 3) return 2;
  try {
    auto seconds = std::stoull(argv[1]);
    if (seconds == 0) return 2;
    std::ofstream metrics(argv[2]);
    metrics.exceptions(std::ios::badbit | std::ios::failbit);
    auto start = std::chrono::steady_clock::now(), next = start;
    size_t iterations = 0;
    do {
      round(iterations++);
      auto now = std::chrono::steady_clock::now();
      if (now >= next) {
        auto elapsed = std::chrono::duration<double>(now - start).count();
        auto [handles, threads] = handles_and_threads();
        metrics << "{\"elapsed_seconds\":" << elapsed << ",\"rounds\":" << iterations
                << ",\"resident_bytes\":" << resident_bytes() << ",\"open_handles\":" << handles
                << ",\"threads\":" << threads << "}\n"
                << std::flush;
        next = now + 1s;
      }
    } while (std::chrono::steady_clock::now() - start < std::chrono::seconds(seconds));
    std::cout << "{\"seconds\":" << seconds << ",\"rounds\":" << iterations
              << ",\"passed\":true}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
