// Independent boundary checks: failures must remain visible in Release builds.
#include <cstdlib>
#include <ctime>

import std;
import cppx.error;
import cppx.serialization;
import cppx.time_utils;
import cppx.concurrent;
import cppx.file_system;
import cppx.logging;
import cppx.cli;
import cppx.math_utils;

namespace {
int failures = 0;
void check(bool condition, std::string_view label) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << label << '\n';
  }
}

template <typename F>
void rejects(F&& operation, std::string_view label) {
  bool threw = false;
  try {
    operation();
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, label);
}

void json_contract() {
  using cppx::json;
  check(json::parse(R"("\u4e2d\u6587\ud83d\ude00")").as_string() == "中文😀",
        "JSON Unicode escapes and surrogate pair");
  check(json::parse(json("中文😀").dump()).as_string() == "中文😀", "JSON UTF-8 round trip");
  check(json(9007199254740993LL).dump() == "9007199254740993", "JSON exact integer storage");
  check(json::parse("18446744073709551615").dump() == "18446744073709551615",
        "JSON uint64 maximum");
  for (const auto* input :
       {"01", "-01", "1.", "1e", "+1", "[1,]", "{\"a\":1,}", "true false", "\vnull", "\"a\nb\"",
        "\"\\ud800\"", "\"\\udc00\"", "\"\\ud800\\u0041\""}) {
    rejects([&] { json::parse(input); }, input);
  }
  rejects([] { json::parse(std::string("\"\xc0\xaf\"")); }, "JSON invalid UTF-8");
  rejects([] { json(std::numeric_limits<double>::infinity()).dump(); }, "JSON infinity");
  rejects([] { json(1e100).as_int(); }, "JSON out of range integer conversion");
  rejects([] { json(1.5).as_int(); }, "JSON fractional integer conversion");
}

void time_contract() {
  auto time = cppx::parse_iso8601("2026-01-01T00:00:00.123Z");
  check(time.is_ok() && cppx::format_iso8601(time.value()) == "2026-01-01T00:00:00.123Z",
        "UTC milliseconds round trip");
  auto offset = cppx::parse_iso8601("2026-01-01T05:30:00.123+05:30");
  check(time && offset && time.value() == offset.value(), "ISO positive offset");
  for (const auto* input : {"2025-02-29T00:00:00Z", "2026-01-01T24:00:00Z",
                            "2026-01-01T00:00:00Zjunk", "2026-01-01T00:00:00+24:00"}) {
    check(cppx::parse_iso8601(input).is_err(), input);
  }
  std::string format = "%Y trailing";
  check(cppx::format_time(std::chrono::system_clock::now(), std::string_view(format).substr(0, 2))
                .size() == 4,
        "format_time respects string_view length");
  check(cppx::parse_time("2026-02-30 12:00:00").is_err(), "local invalid date");
  check(cppx::parse_time("2026-01-01 00:00:00junk").is_err(), "local trailing input");
  auto negative = cppx::parse_iso8601("2025-12-31T18:30:00.123-05:30");
  check(time && negative && time.value() == negative.value(),
        "ISO negative offset and year rollover");
  auto leap = cppx::parse_iso8601("2000-02-29T12:34:56.789Z");
  check(leap && cppx::format_iso8601(leap.value()) == "2000-02-29T12:34:56.789Z", "valid leap day");
  auto before_epoch = cppx::parse_iso8601("1969-12-31T23:59:59.999Z");
  check(before_epoch && cppx::format_iso8601(before_epoch.value()) == "1969-12-31T23:59:59.999Z",
        "negative epoch fraction");
  check(cppx::format_time(std::chrono::system_clock::now(), std::string(300, 'x')) ==
            std::string(300, 'x'),
        "custom format grows output buffer");
  for (const auto* input : {"2026-01-01T00:00:00", "2026-01-01T00:00:00.Z", "2026-01-01T00:00:60Z",
                            "2026-01-01T00:00:00.1234567890Z", "2026-00-01T00:00:00Z"})
    check(cppx::parse_iso8601(input).is_err(), input);
  rejects([] { cppx::from_timestamp_sec(std::numeric_limits<long long>::max()); },
          "timestamp scaling overflow");
}

void local_time_contract() {
  // These process-local timezone changes run before any worker threads exist.
  const char* original = std::getenv("TZ");
  std::optional<std::string> saved = original ? std::optional<std::string>(original) : std::nullopt;
  auto set_zone = [](const char* zone) {
#ifdef _WIN32
    _putenv_s("TZ", zone ? zone : "");
    _tzset();
#else
    if (zone)
      setenv("TZ", zone, 1);
    else
      unsetenv("TZ");
    tzset();
#endif
  };
  set_zone("CST6CDT");
  auto summer = cppx::parse_time("2026-07-01 12:00:00");
  auto winter = cppx::parse_time("2026-01-01 12:00:00");
  check(summer && cppx::format_iso8601(summer.value()) == "2026-07-01T17:00:00.000Z",
        "local DST offset");
  check(winter && cppx::format_iso8601(winter.value()) == "2026-01-01T18:00:00.000Z",
        "local standard offset");
  check(cppx::parse_time("2026-03-08 02:30:00").is_err(), "DST gap rejected");
  auto fold = cppx::parse_time("2026-11-01 01:30:00");
  check(fold && cppx::format_time(fold.value()) == "2026-11-01 01:30:00",
        "DST fold civil round trip");
  auto utc = cppx::parse_iso8601("2026-07-01T12:00:00.123Z");
  set_zone("JST-9");
  auto other_zone = cppx::parse_iso8601("2026-07-01T12:00:00.123Z");
  check(utc && other_zone && utc.value() == other_zone.value(),
        "ISO parsing independent of host timezone");
  std::string pattern = "%Y-%m-%d %H:%M:%S ignored";
  auto sliced = cppx::parse_time("2026-07-01 12:00:00", std::string_view(pattern).substr(0, 17));
  check(sliced && cppx::format_iso8601(sliced.value()) == "2026-07-01T03:00:00.000Z",
        "parse_time respects format view length");
  set_zone(saved ? saved->c_str() : nullptr);
}

void cli_contract() {
  cppx::argument_parser parser("contract");
  parser.add_option("mode", "m", "mode").set_choices("mode", {"fast", "slow"});
  check(parser.parse({"--typo", "value"}).is_err(), "CLI unknown long option");
  check(parser.parse({"-m", "invalid"}).is_err(), "CLI short option choices");
  check(parser.parse({"--mode=fast"}).is_ok(), "CLI long option equals syntax");
  cppx::argument_parser positional("contract");
  positional.add_positional("file", "file");
  auto parsed = positional.parse({"--", "--data"});
  check(parsed && parsed.value().get_string_or("file", "") == "--data", "CLI end of options");
  auto help = positional.parse({"--help"});
  check(help && help.value().help_requested() &&
            help.value().help_text().find("--help") != std::string::npos,
        "CLI help returns to host and bypasses required arguments");
  auto literal_help = positional.parse({"--", "--help"});
  check(literal_help && !literal_help.value().help_requested(),
        "CLI help after delimiter is positional");
  auto command = parser.add_subcommand("child", "child help");
  command->add_positional("required", "required");
  auto child_help = parser.parse({"child", "--help"});
  check(child_help && child_help.value().help_requested() &&
            child_help.value().help_text().starts_with("child"),
        "CLI subcommand help");
  parser.add_flag("verbose", "v", "verbose");
  check(parser.parse({"--mode", "-v"}).is_err(), "option must not swallow known short flag");
  check(parser.parse({"--verbose=yes"}).is_err(), "flag rejects a value");
  rejects([&] { parser.add_flag("mode", "x", "duplicate"); }, "duplicate argument rejected");
  cppx::argument_parser number("numbers");
  number.add_option("value", "n", "number");
  check(number.parse({"-n", "-42"}).value().get_int("value").value() == -42,
        "negative option value");
}

void math_contract() {
  const auto limit = std::numeric_limits<int>::max();
  check(cppx::median(std::vector<int>{limit, limit}) == static_cast<double>(limit),
        "median integer overflow");
  check(cppx::magnitude(std::vector<int>{50000, 0}) == 50000.0, "magnitude integer overflow");
  check(cppx::inverse_lerp(std::numeric_limits<int>::min(), limit, 0) > 0.49,
        "inverse_lerp integer overflow");
  check(cppx::range(limit - 1, limit, 2) == std::vector<int>{limit - 1},
        "range positive step overflow");
  auto minimum = std::numeric_limits<int>::min();
  check(cppx::range(minimum + 1, minimum, -2) == std::vector<int>{minimum + 1},
        "range negative step overflow");
  check(cppx::range(0, minimum, minimum) == std::vector<int>{0}, "range minimum step");
  rejects([&] { cppx::sum(std::vector<int>{limit, 1}); }, "sum reports overflow");
  rejects([&] { cppx::product(std::vector<int>{minimum, -1}); }, "product reports overflow");
  rejects([&] { cppx::dot_product(std::vector<int>{limit}, std::vector<int>{2}); },
          "dot product reports overflow");
  rejects([&] { cppx::subtract(std::vector<unsigned>{0}, std::vector<unsigned>{1}); },
          "unsigned underflow");
  rejects([&] { cppx::abs(minimum); }, "absolute minimum reports overflow");
  check(cppx::abs(1U) == 1 && cppx::approx_equal(3, 3) && !cppx::approx_equal(minimum, limit, 1),
        "integral abs and approximate comparison");
  rejects([] { cppx::mean(std::vector<int>{}); }, "empty mean");
  check(cppx::mean(std::vector<double>(2, std::numeric_limits<double>::max())) ==
            std::numeric_limits<double>::max(),
        "mean avoids sum overflow");
  rejects([] { cppx::random_int(3, 1); }, "invalid integer distribution bounds");
  rejects([] { cppx::random_bool(2); }, "invalid probability");
  rejects([] { cppx::denormalize(std::numeric_limits<double>::infinity(), 0, 1); },
          "invalid denormalization");
}

void file_contract() {
  cppx::temp_dir directory("cppx_contract");
  auto log = cppx::create_logger("contract");
  log->remove_console_sink();
  rejects([&] { log->add_file_sink(directory.path() / "missing" / "log.txt"); },
          "logger reports file open failure");
  check(cppx::read_file_binary(directory.path()).is_err(), "binary read rejects directory");
}

struct no_default_error {
  explicit no_default_error(int value) : value(value) {}
  int value;
};
struct throwing_move {
  throwing_move() = default;
  throwing_move(const throwing_move&) = default;
  throwing_move(throwing_move&&) { throw std::runtime_error("move failure"); }
  throwing_move& operator=(const throwing_move&) = default;
  throwing_move& operator=(throwing_move&&) { throw std::runtime_error("move failure"); }
};
struct throwing_task {
  throwing_task() = default;
  throwing_task(throwing_task&&) { throw std::runtime_error("task capture failure"); }
  void operator()() const {}
};
static_assert(!std::is_nothrow_move_constructible_v<cppx::expected<throwing_move, int>>);
static_assert(!std::is_nothrow_move_assignable_v<cppx::expected<throwing_move, int>>);
static_assert(!std::is_copy_constructible_v<cppx::expected<std::unique_ptr<int>, int>>);

void error_contract() {
  auto same = cppx::expected<std::string, std::string>::ok("value");
  check(same.value() == "value", "expected T=E success");
  same = cppx::expected<std::string, std::string>::err("error");
  check(same.error() == "error", "expected T=E error and assignment");
  rejects([&] { (void)same.value(); }, "checked value access");
  auto moved =
      cppx::expected<std::unique_ptr<int>, std::unique_ptr<int>>::ok(std::make_unique<int>(7));
  auto mapped = std::move(moved).map([](std::unique_ptr<int> p) { return *p; });
  check(mapped.value() == 7, "move-only mapping");
  auto failed = cppx::expected<int, std::unique_ptr<int>>::err(std::make_unique<int>(8));
  auto void_error = std::move(failed).map([](int) {});
  check(void_error.is_err() && *void_error.error() == 8, "void mapping moves error");
  check(cppx::expected<void, no_default_error>::ok().is_ok(), "void non-default error");
  auto success = cppx::expected<int, int>::ok(3);
  int observed = 0;
  check(success.map([&](int n) { observed = n; }).is_ok() && observed == 3, "void mapping success");
  check(cppx::expected<void, int>::ok().map([] { return 4; }).value() == 4, "mapping from void");
  throwing_move original;
  auto result = cppx::expected<throwing_move, int>::ok(original);
  rejects([&] { auto copy = std::move(result); }, "throwing move propagates");
  auto target = cppx::expected<throwing_move, int>::err(9);
  rejects([&] { target = std::move(result); }, "throwing assignment propagates");
  check(target.is_err() && target.error() == 9, "failed assignment retains error state");
  auto void_failed = cppx::expected<void, std::unique_ptr<int>>::err(std::make_unique<int>(6));
  check(std::move(void_failed).map_err([](std::unique_ptr<int> e) { return *e; }).error() == 6,
        "void map_err moves error");
}

void json_boundaries() {
  using cppx::json;
  for (auto value : {std::numeric_limits<long long>::min(), -1LL, 0LL, 9007199254740993LL,
                     std::numeric_limits<long long>::max()}) {
    check(cppx::from_json_string<long long>(cppx::to_json_string(value)) == value,
          "signed integer serialization round trip");
  }
  auto maximum = std::numeric_limits<unsigned long long>::max();
  check(cppx::from_json_string<unsigned long long>(cppx::to_json_string(maximum)) == maximum,
        "unsigned integer serialization round trip");
  std::map<unsigned long long, long long> values{{maximum, -42}};
  check(cppx::from_json_string<decltype(values)>(cppx::to_json_string(values)) == values,
        "integer map key round trip");
  rejects([] { cppx::from_json_string<std::map<int, int>>(R"({"1x":2})"); }, "partial map key");
  rejects([] { cppx::from_json_string<std::map<int, int>>(R"({"1":2,"01":3})"); },
          "converted map key collision");
  for (const auto* input : {"18446744073709551616", "-9223372036854775809", "1e9999", "1e-9999",
                            R"({"x":1,"x":2})", R"("\u12")", "[", "{", "\""})
    rejects([&] { json::parse(input); }, input);
  for (auto bytes : {std::string("\x80"), std::string("\xed\xa0\x80"),
                     std::string("\xf4\x90\x80\x80"), std::string("\xe2\x82")}) {
    rejects([&] { json::parse("\"" + bytes + "\""); }, "invalid UTF-8 input");
    rejects([&] { json(bytes).dump(); }, "invalid UTF-8 output");
  }
  std::string controls;
  for (int i = 0; i < 32; ++i) controls += static_cast<char>(i);
  check(json::parse(json(controls).dump()).as_string() == controls,
        "all control escapes including NUL");
  rejects([] { json::parse(std::string("null\0", 5)); }, "embedded NUL trailing input");
  check(json::parse("[[0]]", {32, 2}).is_array(), "depth limit boundary");
  rejects([] { json::parse("[[[0]]]", {32, 2}); }, "depth limit exceeded");
  rejects([] { json::parse("null", {3, 2}); }, "input byte limit");
  try {
    json::parse("[1,]");
    check(false, "parse error expected");
  } catch (const cppx::json_parse_error& error) {
    check(error.position() == 3, "parse error byte offset");
  }
  for (double value : {std::numeric_limits<double>::max(),
                       std::numeric_limits<double>::denorm_min(), -0.0, 0.1, 1e20}) {
    auto roundtrip = json::parse(json(value).dump()).as_number();
    check(roundtrip == value && std::signbit(roundtrip) == std::signbit(value),
          "double exact round trip");
  }
  rejects([] { json(-1).as_uint64(); }, "negative unsigned conversion");
  rejects([] { json(std::ldexp(1.0, 63)).as_int64(); }, "double int64 upper exclusive bound");
  rejects([] { json(std::ldexp(1.0, 64)).as_uint64(); }, "double uint64 upper exclusive bound");
  rejects([] { json(1e100).as<float>(); }, "float narrowing overflow");
  rejects([] { (void)json(std::numeric_limits<double>::quiet_NaN()); }, "NaN construction");
  rejects([] { (void)json::array()[0]; }, "checked array index");
}

template <typename T>
bool ready(std::future<T>& future, std::string_view label) {
  bool done = future.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
  check(done, label);
  if (!done) {
    std::cerr.flush();
    std::_Exit(1);
  }
  return done;
}

void concurrent_contract() {
  using namespace std::chrono_literals;
  rejects([] { cppx::parallel_for(0, 8, [](int) { throw std::runtime_error("parallel"); }, 2); },
          "parallel_for propagates exceptions");
  std::vector<int> elements(8);
  rejects(
      [&] {
        cppx::parallel_for_each(elements, [](int&) { throw std::runtime_error("parallel"); }, 2);
      },
      "parallel_for_each propagates exceptions");
  std::atomic<int> completed = 0;
  rejects(
      [&] {
        cppx::parallel_invoke([] { throw std::runtime_error("parallel"); }, [&] { ++completed; });
      },
      "parallel_invoke propagates exceptions");
  check(completed == 1, "parallel_invoke joins all tasks before throwing");
  throwing_task bad_capture;
  std::atomic<bool> first_finished = false;
  rejects([&] { cppx::parallel_invoke([&] { first_finished = true; }, std::move(bad_capture)); },
          "partial parallel launch failure propagates");
  check(first_finished, "partial launch failure joins the started task");
  cppx::parallel_for(
      std::numeric_limits<int>::max() - 2, std::numeric_limits<int>::max(),
      [&](int) { ++completed; }, 8);
  check(completed == 3, "parallel_for extreme indices");
  cppx::thread_pool pool(1);
  auto nested = pool.submit([&] { return pool.submit([] { return 42; }).get(); });
  if (ready(nested, "nested single worker submission")) check(nested.get() == 42, "nested result");
  auto unique =
      pool.submit([](std::unique_ptr<int> value) { return *value; }, std::make_unique<int>(9));
  check(unique.get() == 9, "thread pool move-only argument");
  auto failed = pool.submit([]() -> int { throw std::runtime_error("task failure"); });
  rejects([&] { failed.get(); }, "task exception stored in future");
  auto self_join = pool.submit([&] { pool.shutdown(); });
  rejects([&] { self_join.get(); }, "worker shutdown rejects self join");
  pool.close();
  rejects([&] { pool.submit([] {}); }, "submission after close");
  pool.shutdown();
  pool.shutdown();

  cppx::thread_pool bounded(1, 1);
  std::promise<void> entered, release;
  auto gate = release.get_future().share();
  auto active = bounded.submit([&] {
    entered.set_value();
    gate.wait();
    return 1;
  });
  auto started = entered.get_future();
  ready(started, "bounded pool active task starts");
  auto queued = bounded.submit([] { return 2; });
  rejects([&] { bounded.submit_for(1ms, [] {}); }, "bounded pool timeout");
  auto waiting = std::async(std::launch::async, [&] {
    try {
      bounded.submit_for(10s, [] {});
      return false;
    } catch (const std::exception&) {
      return true;
    }
  });
  bounded.close();
  if (ready(waiting, "closing pool wakes blocked submitter"))
    check(waiting.get(), "closed submitter rejected");
  release.set_value();
  bounded.shutdown();
  check(active.get() == 1 && queued.get() == 2, "close drains accepted work");

  cppx::channel<int> channel(1);
  check(channel.send(7), "channel accepts value");
  check(!channel.send_for(8, 1ms), "channel send timeout");
  auto writer = std::async(std::launch::async, [&] { return channel.send(9); });
  channel.close();
  if (ready(writer, "channel close wakes writer")) check(!writer.get(), "closed writer rejected");
  check(channel.receive() == 7 && !channel.receive(), "closed channel drains");
  cppx::channel<int> empty;
  check(!empty.receive_for(1ms), "channel receive timeout");
  auto reader = std::async(std::launch::async, [&] { return empty.receive(); });
  empty.close();
  if (ready(reader, "channel close wakes reader")) check(!reader.get(), "closed empty channel");
  cppx::blocking_queue<int> queue;
  queue.push(5);
  queue.close();
  check(!queue.push(6) && queue.pop() == 5 && !queue.wait_pop(), "blocking queue close and drain");
  rejects([&] { queue.pop(); }, "legacy pop reports closed queue");
  cppx::atomic_counter counter(std::numeric_limits<long long>::max());
  check(counter.increment() == std::numeric_limits<long long>::min(),
        "atomic counter defined wrap");
}

void timer_contract() {
  using namespace std::chrono_literals;
  cppx::timer timer;
  std::atomic<int> calls = 0;
  timer.start_once(10s, [&] { ++calls; });
  auto start = std::chrono::steady_clock::now();
  timer.stop();
  check(std::chrono::steady_clock::now() - start < 1s && calls == 0 && !timer.is_running(),
        "timer stop interrupts delay");
  timer.stop();
  std::promise<void> called;
  auto done = called.get_future();
  timer.start_periodic(10s, [&] {
    ++calls;
    timer.stop();
    called.set_value();
  });
  ready(done, "callback can stop timer");
  timer.stop();
  check(!timer.is_running() && calls == 1, "self stop prevents subsequent callbacks");
  std::promise<void> inside_callback, release_callback;
  auto callback_entered = inside_callback.get_future();
  auto callback_gate = release_callback.get_future().share();
  timer.start_once(0ms, [&] {
    inside_callback.set_value();
    callback_gate.wait();
    timer.stop();
  });
  ready(callback_entered, "concurrent stop callback starts");
  auto external_stop = std::async(std::launch::async, [&] { timer.stop(); });
  release_callback.set_value();
  ready(external_stop, "external and callback stop do not deadlock");
  external_stop.get();
  std::promise<void> throwing;
  auto thrown = throwing.get_future();
  timer.start_once(0ms, [&] {
    throwing.set_value();
    throw std::runtime_error("callback failure");
  });
  ready(thrown, "throwing callback started");
  timer.stop();
  check(!timer.is_running() && timer.exception(), "callback exception retained");
  rejects([&] { timer.rethrow_if_failed(); }, "callback exception can be rethrown");
  std::promise<void> once;
  auto once_done = once.get_future();
  timer.start_once(0ms, [&] { once.set_value(); });
  ready(once_done, "one shot executes");
  auto deadline = std::chrono::steady_clock::now() + 2s;
  while (timer.is_running() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  check(!timer.is_running() && !timer.exception(),
        "one shot stops itself and restart clears exception");
  timer.stop();
  rejects([&] { timer.start_periodic(0ms, [] {}); }, "zero periodic interval rejected");
  rejects([&] { timer.start_once(-1ms, [] {}); }, "negative timer delay rejected");
  rejects([&] { timer.start_once(std::chrono::milliseconds::max(), [] {}); },
          "unrepresentable timer delay rejected");
  std::promise<void> self_destroyed;
  auto destroyed = self_destroyed.get_future();
  auto self = std::make_unique<cppx::timer>();
  self->start_once(0ms, [&] {
    self.reset();
    self_destroyed.set_value();
  });
  ready(destroyed, "timer can be destroyed from its callback");
  destroyed.get();
  // No references into this function survive the callback; the detached worker owns its state.
  std::promise<void> captured_done;
  auto captured_future = captured_done.get_future();
  auto captured_timer = std::make_shared<cppx::timer>();
  std::weak_ptr<cppx::timer> weak = captured_timer;
  captured_timer->start_once(
      0ms, [owner = captured_timer, &captured_done] { captured_done.set_value(); });
  captured_timer.reset();
  ready(captured_future, "callback with shared ownership executes");
  auto capture_deadline = std::chrono::steady_clock::now() + 2s;
  while (!weak.expired() && std::chrono::steady_clock::now() < capture_deadline)
    std::this_thread::yield();
  check(weak.expired(), "timer can be destroyed by final callback capture");
  {
    cppx::scope_timer scope("throwing", [](auto) { throw std::runtime_error("scope failure"); });
  }
}

void resource_contract() {
  using namespace std::chrono_literals;
  namespace fs = std::filesystem;
  cppx::temp_dir directory("cppx_resources");
  std::vector<std::future<cppx::temp_file>> creators;
  for (int i = 0; i < 16; ++i)
    creators.push_back(std::async(std::launch::async,
                                  [&] { return cppx::temp_file("parallel", directory.path()); }));
  std::set<fs::path> names;
  std::vector<cppx::temp_file> files;
  for (auto& future : creators) {
    files.push_back(future.get());
    check(fs::exists(files.back().path()) && names.insert(files.back().path()).second,
          "exclusive temp file ownership");
  }
  auto replaced = files[0].path();
  auto retained = files[1].path();
  files[0] = std::move(files[1]);
  check(!fs::exists(replaced) && fs::exists(retained) && files[1].path().empty(),
        "temp move assignment cleanup");
  files.clear();
  check(!fs::exists(retained), "temp moved resource final cleanup");
  rejects([&] { cppx::temp_file invalid("../escape", directory.path()); },
          "temp prefix cannot escape parent");
  rejects([&] { cppx::temp_dir invalid("missing", directory.path() / "absent"); },
          "temp creation failure");
  fs::path kept;
  {
    cppx::temp_file file("kept", directory.path());
    kept = file.path();
    file.keep();
    check(file.path() == kept, "keep preserves path");
  }
  check(fs::exists(kept), "keep releases cleanup ownership");
  auto target = directory.path() / "atomic.txt";
  check(cppx::write_file_atomic(target, "old").is_ok(), "atomic initial write");
  check(cppx::write_file_atomic(target, "new").is_ok() &&
            cppx::read_file_text(target).value() == "new",
        "atomic replacement");
  auto blocked = directory.path() / "blocked";
  fs::create_directory(blocked);
  check(cppx::write_file_atomic(blocked, "bad").is_err() && fs::is_directory(blocked),
        "failed replace preserves destination");
  for (const auto& entry : fs::directory_iterator(directory.path()))
    check(!entry.path().filename().string().starts_with("cppx_write_"), "atomic staging cleanup");

  auto log = cppx::create_logger("rotation");
  log->remove_console_sink();
  auto log_path = directory.path() / "日志.txt";
  log->add_file_sink(log_path, 1);
  for (int i = 0; i < 5; ++i) log->info("record-{}", i);
  log->flush();
  for (int i = 0; i < 5; ++i) {
    auto path = log_path;
    if (i) path += "." + std::to_string(i);
    auto content = cppx::read_file_text(path);
    check(content && content.value().find("record-" + std::to_string(4 - i)) != std::string::npos,
          "rotation preserves complete records in order");
  }
  std::vector<std::future<void>> logging;
  for (int i = 0; i < 4; ++i)
    logging.push_back(std::async(std::launch::async, [&, i] {
      for (int n = 0; n < 30; ++n) {
        log->set_level(n % 2 ? cppx::log_level::info : cppx::log_level::off);
        log->info("thread {} entry {}", i, n);
      }
    }));
  for (auto& future : logging) future.get();
  log->remove_file_sink();
  log->set_level(cppx::log_level::debug);
  auto broken = directory.path() / "broken.log";
  log->add_file_sink(broken, 1);
  {
    cppx::log_scope scope(log, "retained-error");
    // Occupied backup paths force rotation to fail during scope destruction.
    fs::create_directories(fs::path(broken.string() + ".4") / "child");
  }
  check(log->last_error() != nullptr, "scope destructor retains log failure");
  rejects([&] { log->flush(); }, "flush on failed rotation reports failure");
}
}  // namespace

int main() {
  json_contract();
  time_contract();
  local_time_contract();
  cli_contract();
  math_contract();
  file_contract();
  error_contract();
  json_boundaries();
  concurrent_contract();
  timer_contract();
  resource_contract();
  std::cout << "Core contract failures: " << failures << '\n';
  return failures == 0 ? 0 : 1;
}
