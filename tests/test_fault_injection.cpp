import std;
import cppx.concurrent;
import cppx.file_system;
import cppx.time_utils;
import cppx.logging;
import cppx.fault_injection;

using namespace std::chrono_literals;
using cppx::testing::fail_after;
using cppx::testing::fault;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
template <typename F>
void must_throw(F&& operation) {
  bool threw = false;
  try {
    operation();
  } catch (const std::system_error&) {
    threw = true;
  }
  cppx::testing::clear_fault();
  require(threw, "Injected operation did not report a system/stream error");
}

int main() {
  try {
    for (size_t after : {0u, 1u, 3u}) {
      fail_after(fault::thread_create, after);
      must_throw([] { cppx::thread_pool pool(4); });
      std::atomic<int> completed = 0;
      fail_after(fault::thread_create, after);
      must_throw([&] { cppx::parallel_for(0, 4, [&](int) { ++completed; }, 4); });
      require(completed == static_cast<int>(after), "Partial parallel workers were not joined");
    }
    cppx::timer timer;
    fail_after(fault::thread_create);
    must_throw([&] { timer.start_once(0ms, [] {}); });
    require(!timer.is_running(), "Timer retained running state after creation failure");
    std::promise<void> called;
    auto ready = called.get_future();
    timer.start_once(0ms, [&] { called.set_value(); });
    require(ready.wait_for(2s) == std::future_status::ready, "Timer did not recover");
    timer.stop();

    cppx::temp_dir directory("faults");
    auto path = directory.path() / "data";
    require(cppx::write_file(path, "original").is_ok(), "Fixture write failed");
    fail_after(fault::file_read);
    require(!cppx::read_file_text(path), "Text read failure was lost");
    fail_after(fault::file_read);
    require(!cppx::read_file_binary(path), "Short read was lost");
    fail_after(fault::file_read);
    require(!cppx::read_lines(path), "Line read failure was lost");
    for (auto point : {fault::file_write, fault::file_flush, fault::file_close, fault::replace}) {
      fail_after(point);
      require(!cppx::write_file_atomic(path, "replacement"), "Atomic failure was lost");
      require(cppx::read_file_text(path).value() == "original", "Atomic failure replaced target");
      require(std::distance(std::filesystem::directory_iterator(directory.path()),
                            std::filesystem::directory_iterator{}) == 1,
              "Atomic failure leaked staging file");
    }
    for (auto point : {fault::file_write, fault::file_flush, fault::file_close}) {
      fail_after(point);
      require(!cppx::write_file(path, "data"), "Text write failure was lost");
      fail_after(point);
      require(!cppx::write_file_binary(path, {std::byte{1}}), "Binary write failure was lost");
      fail_after(point);
      require(!cppx::write_lines(path, {"line"}), "Line write failure was lost");
    }
    for (auto point : {fault::log_write, fault::log_flush}) {
      auto log = cppx::create_logger("faults");
      log->remove_console_sink();
      log->add_file_sink(directory.path() / "log");
      fail_after(point);
      must_throw([&] {
        log->info("record");
        log->flush();
      });
    }
    auto log = cppx::create_logger("scope-fault");
    log->remove_console_sink();
    log->set_level(cppx::log_level::debug);
    log->add_file_sink(directory.path() / "scope-log");
    {
      cppx::log_scope scope(log, "injected failure");
      fail_after(fault::log_write);
    }
    require(log->last_error() != nullptr, "Scope destructor failure was lost");
    std::cout << "Fault injection checks passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
