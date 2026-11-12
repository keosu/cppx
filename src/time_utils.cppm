// cppx.time_utils - Time formatting, parsing, and utilities
// Provides ISO8601, RFC3339 formatting and scope timers

module;
#include <ctime>

export module cppx.time_utils;

import std;
import cppx.error;
#ifdef CPPX_FAULT_INJECTION
import cppx.fault_injection;
#endif

namespace cppx {

using std::chrono::duration_cast;
using std::chrono::hours;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::minutes;
using std::chrono::nanoseconds;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

// ============================================================================
// Time formatting
// ============================================================================

namespace time_detail {
inline bool local_tm(std::time_t time, std::tm& result) {
#ifdef _WIN32
  return ::localtime_s(&result, &time) == 0;
#else
  return ::localtime_r(&time, &result) != nullptr;
#endif
}
inline bool representable(seconds value, nanoseconds fraction = {}) {
  // Reserve one second at each endpoint so adding the fraction cannot overflow.
  return value >= std::chrono::ceil<seconds>(system_clock::duration::min()) &&
         value < std::chrono::floor<seconds>(system_clock::duration::max()) &&
         fraction.count() >= 0;
}
}  // namespace time_detail

// UTC output, truncated to milliseconds. Calendar arithmetic also handles pre-epoch dates.
export std::string format_iso8601(const system_clock::time_point& time_point) {
  auto day = std::chrono::floor<std::chrono::days>(time_point);
  std::chrono::year_month_day date(day);
  std::chrono::hh_mm_ss time(std::chrono::floor<milliseconds>(time_point - day));
  return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:03}Z", int(date.year()),
                     unsigned(date.month()), unsigned(date.day()), time.hours().count(),
                     time.minutes().count(), time.seconds().count(), time.subseconds().count());
}

export std::string format_rfc3339(const system_clock::time_point& time_point) {
  return format_iso8601(time_point);
}

// Custom formats use local time and must not contain embedded NUL characters.
export std::string format_time(const system_clock::time_point& time_point,
                               std::string_view format = "%Y-%m-%d %H:%M:%S") {
  if (format.empty()) return "";
  if (format.find('\0') != std::string_view::npos)
    throw std::invalid_argument("NUL in time format");
  std::tm tm{};
  auto time = system_clock::to_time_t(time_point);
  if (!time_detail::local_tm(time, tm)) throw std::out_of_range("Local time out of range");
  std::string owned_format(format);
  for (size_t size = 128; size <= 65536; size *= 2) {
    std::string out(size, '\0');
    auto count = std::strftime(out.data(), out.size(), owned_format.c_str(), &tm);
    if (count) {
      out.resize(count);
      return out;
    }
  }
  throw std::runtime_error("Time format failed or output exceeds 64 KiB");
}

export std::string now_iso8601() { return format_iso8601(system_clock::now()); }
export std::string now_rfc3339() { return format_rfc3339(system_clock::now()); }
export std::string now_formatted(std::string_view format = "%Y-%m-%d %H:%M:%S") {
  return format_time(system_clock::now(), format);
}

// RFC3339 subset: YYYY-MM-DDTHH:MM:SS[.1..9 digits](Z|+HH:MM|-HH:MM).
// Leap seconds are rejected; fractional seconds truncate to the system clock resolution.
export expected<system_clock::time_point, error_info> parse_iso8601(std::string_view str) {
  using R = expected<system_clock::time_point, error_info>;
  auto invalid = [] { return R::err(error_info("Invalid ISO8601 timestamp")); };
  auto number = [&](size_t pos, size_t count) -> int {
    if (pos + count > str.size()) return -1;
    int out = 0;
    for (size_t i = pos; i < pos + count; ++i) {
      if (str[i] < '0' || str[i] > '9') return -1;
      out = out * 10 + str[i] - '0';
    }
    return out;
  };
  if (str.size() < 20 || str[4] != '-' || str[7] != '-' || str[10] != 'T' || str[13] != ':' ||
      str[16] != ':')
    return invalid();
  int y = number(0, 4), m = number(5, 2), d = number(8, 2);
  int h = number(11, 2), min = number(14, 2), sec = number(17, 2);
  std::chrono::year_month_day date{std::chrono::year(y), std::chrono::month(unsigned(m)),
                                   std::chrono::day(unsigned(d))};
  if (y < 1 || !date.ok() || h < 0 || h > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
    return invalid();
  size_t pos = 19;
  int fraction = 0;
  if (str[pos] == '.') {
    ++pos;
    size_t digits = 0;
    while (pos < str.size() && str[pos] >= '0' && str[pos] <= '9') {
      if (++digits > 9) return invalid();
      fraction = fraction * 10 + str[pos++] - '0';
    }
    if (!digits) return invalid();
    while (digits++ < 9) fraction *= 10;
  }
  int offset = 0;
  if (pos < str.size() && str[pos] == 'Z')
    ++pos;
  else if (pos < str.size() && (str[pos] == '+' || str[pos] == '-')) {
    int sign = str[pos] == '+' ? 1 : -1;
    if (str.size() - pos != 6 || str[pos + 3] != ':') return invalid();
    int oh = number(pos + 1, 2), om = number(pos + 4, 2);
    if (oh < 0 || oh > 23 || om < 0 || om > 59) return invalid();
    offset = sign * (oh * 60 + om);
    pos += 6;
  } else
    return invalid();
  if (pos != str.size()) return invalid();
  auto whole = duration_cast<seconds>(std::chrono::sys_days(date).time_since_epoch()) + hours(h) +
               minutes(min - offset) + seconds(sec);
  if (!time_detail::representable(whole, nanoseconds(fraction))) return invalid();
  return R::ok(
      system_clock::time_point(duration_cast<system_clock::duration>(whole) +
                               duration_cast<system_clock::duration>(nanoseconds(fraction))));
}

// Local civil time: libc selects one offset in an ambiguous DST fold; gaps are rejected.
export expected<system_clock::time_point, error_info>
parse_time(std::string_view str, std::string_view format = "%Y-%m-%d %H:%M:%S") {
  using R = expected<system_clock::time_point, error_info>;
  auto invalid = [] { return R::err(error_info("Invalid local timestamp or format")); };
  if (format.empty() || format.find('\0') != std::string_view::npos) return invalid();
  std::tm tm{};
  tm.tm_isdst = -1;
  std::istringstream stream{std::string(str)};
  std::string owned_format(format);
  stream >> std::get_time(&tm, owned_format.c_str());
  if (stream.fail() || stream.peek() != std::char_traits<char>::eof()) return invalid();
  auto original = tm;
  auto value = std::mktime(&tm);
  std::tm verified{};
  if (!time_detail::local_tm(value, verified) || verified.tm_year != original.tm_year ||
      verified.tm_mon != original.tm_mon || verified.tm_mday != original.tm_mday ||
      verified.tm_hour != original.tm_hour || verified.tm_min != original.tm_min ||
      verified.tm_sec != original.tm_sec || !time_detail::representable(seconds(value)))
    return invalid();
  return R::ok(system_clock::from_time_t(value));
}

// ============================================================================
// Duration formatting
// ============================================================================

// Format duration in human-readable form
export std::string format_duration(milliseconds duration) {
  auto ms = duration.count();

  if (ms < 1000) {
    return std::format("{}ms", ms);
  }

  auto secs = ms / 1000;
  ms %= 1000;

  if (secs < 60) {
    return std::format("{}.{:03d}s", secs, ms);
  }

  auto mins = secs / 60;
  secs %= 60;

  if (mins < 60) {
    return std::format("{}m {}s", mins, secs);
  }

  auto hrs = mins / 60;
  mins %= 60;

  return std::format("{}h {}m {}s", hrs, mins, secs);
}

export template <typename Rep, typename Period>
std::string format_duration(std::chrono::duration<Rep, Period> duration) {
  return format_duration(duration_cast<milliseconds>(duration));
}

// ============================================================================
// Scope timer (RAII timing)
// ============================================================================

export class scope_timer {
 public:
  using callback_type = std::function<void(milliseconds)>;

  explicit scope_timer(std::string name, callback_type callback = nullptr)
      : name_(std::move(name)), callback_(std::move(callback)), start_(steady_clock::now()) {}

  ~scope_timer() {
    auto end = steady_clock::now();
    auto duration = duration_cast<milliseconds>(end - start_);

    if (callback_) {
      try {
        callback_(duration);
      } catch (...) { /* scope_timer callbacks are best effort */
      }
    }
  }

  // Get elapsed time without destroying the timer
  [[nodiscard]] milliseconds elapsed() const {
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now - start_);
  }

  // Reset the timer
  void reset() { start_ = steady_clock::now(); }

  [[nodiscard]] const std::string& name() const { return name_; }

 private:
  std::string name_;
  callback_type callback_;
  steady_clock::time_point start_;
};

// Helper to create scope timer with lambda
export auto time_scope(std::string name, auto callback) {
  return scope_timer(std::move(name), [callback = std::move(callback)](milliseconds duration) {
    callback(duration);
  });
}

// ============================================================================
// Timer class (for scheduled tasks)
// ============================================================================

export class timer {
 public:
  timer() = default;
  ~timer() {
    stop();
    // The worker owns shared state, so destruction in its callback is safe as well.
    if (thread_.joinable()) thread_.detach();
  }
  timer(const timer&) = delete;
  timer& operator=(const timer&) = delete;

  void start_periodic(milliseconds interval, std::function<void()> callback) {
    if (interval.count() <= 0) throw std::invalid_argument("Timer interval must be positive");
    start(interval, std::move(callback), true);
  }
  void start_once(milliseconds delay, std::function<void()> callback) {
    if (delay.count() < 0) throw std::invalid_argument("Timer delay must not be negative");
    start(delay, std::move(callback), false);
  }
  // External stop waits for an active callback; callback stop only requests cancellation.
  void stop() {
    if (active_timer_ == this) {
      request_stop();
      return;
    }
    std::lock_guard lock(lifecycle_mutex_);
    stop_and_join();
  }
  [[nodiscard]] bool is_running() const {
    std::lock_guard lock(state_->mutex);
    return state_->running;
  }
  [[nodiscard]] std::exception_ptr exception() const {
    std::lock_guard lock(state_->mutex);
    return state_->exception;
  }
  void rethrow_if_failed() const {
    if (auto error = exception()) std::rethrow_exception(error);
  }

 private:
  struct state {
    std::mutex mutex;
    std::condition_variable cv;
    bool running = false;
    std::exception_ptr exception;
  };
  inline static thread_local const timer* active_timer_ = nullptr;
  std::shared_ptr<state> state_ = std::make_shared<state>();
  std::mutex lifecycle_mutex_;
  std::thread thread_;
  void request_stop() {
    {
      std::lock_guard lock(state_->mutex);
      state_->running = false;
    }
    state_->cv.notify_all();
  }
  void stop_and_join() {
    request_stop();
    if (thread_.joinable()) thread_.join();
  }
  void start(milliseconds interval, std::function<void()> callback, bool periodic) {
    if (active_timer_ == this) throw std::logic_error("Cannot restart a timer from its callback");
    if (!callback) throw std::invalid_argument("Timer callback is empty");
    if (interval > duration_cast<milliseconds>(steady_clock::duration::max()) / 2)
      throw std::out_of_range("Timer interval is not representable");
    std::unique_lock lifecycle(lifecycle_mutex_);
    stop_and_join();
    auto shared = state_;
    std::unique_lock state_lock(shared->mutex);
    shared->running = true;
    shared->exception = nullptr;
    try {
#ifdef CPPX_FAULT_INJECTION
      testing::thread_creation();
#endif
      thread_ = std::thread(
          [state = shared, owner = this, interval, callback = std::move(callback), periodic] {
            active_timer_ = owner;
            std::unique_lock lock(state->mutex);
            if (!periodic) state->cv.wait_for(lock, interval, [&] { return !state->running; });
            while (state->running) {
              auto start = steady_clock::now();
              lock.unlock();
              try {
                callback();
              } catch (...) {
                lock.lock();
                state->exception = std::current_exception();
                state->running = false;
                break;
              }
              lock.lock();
              if (!periodic) {
                state->running = false;
                break;
              }
              state->cv.wait_until(lock, start + interval, [&] { return !state->running; });
            }
            state->cv.notify_all();
            // Keep thread identity through destruction of the callback's captures.
            // A last shared owner of this timer may be released there.
          });
    } catch (...) {
      shared->running = false;
      throw;
    }
    // Publish the thread handle and release the object's lock before the callback can run.
    // The remaining local lock owns shared state, allowing callback-side destruction.
    lifecycle.unlock();
  }
};

// ============================================================================
// Sleep utilities
// ============================================================================

// Sleep for duration
export template <typename Rep, typename Period>
void sleep_for(std::chrono::duration<Rep, Period> duration) {
  std::this_thread::sleep_for(duration);
}

// Sleep until time point
export void sleep_until(const system_clock::time_point& time_point) {
  std::this_thread::sleep_until(time_point);
}

// Sleep for milliseconds (convenience)
export void sleep_ms(long long milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Sleep for seconds (convenience)
export void sleep_sec(long long seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// ============================================================================
// Stopwatch class
// ============================================================================

export class stopwatch {
 public:
  stopwatch() : running_(false) {}

  void start() {
    start_ = steady_clock::now();
    running_ = true;
  }

  void stop() {
    if (running_) {
      end_ = steady_clock::now();
      running_ = false;
    }
  }

  void reset() {
    start_ = steady_clock::time_point{};
    end_ = steady_clock::time_point{};
    running_ = false;
  }

  void restart() {
    start_ = steady_clock::now();
    running_ = true;
  }

  [[nodiscard]] milliseconds elapsed_ms() const {
    if (running_) {
      return duration_cast<milliseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<milliseconds>(end_ - start_);
    }
  }

  [[nodiscard]] microseconds elapsed_us() const {
    if (running_) {
      return duration_cast<microseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<microseconds>(end_ - start_);
    }
  }

  [[nodiscard]] nanoseconds elapsed_ns() const {
    if (running_) {
      return duration_cast<nanoseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<nanoseconds>(end_ - start_);
    }
  }

  [[nodiscard]] bool is_running() const { return running_; }

 private:
  steady_clock::time_point start_;
  steady_clock::time_point end_;
  bool running_;
};

// ============================================================================
// Timestamp utilities
// ============================================================================

// Get current Unix timestamp (seconds since epoch)
export long long timestamp_sec() {
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

// Get current Unix timestamp (milliseconds since epoch)
export long long timestamp_ms() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Get current Unix timestamp (microseconds since epoch)
export long long timestamp_us() {
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

namespace time_detail {
template <typename Duration>
system_clock::time_point from_timestamp(long long value) {
  using clock_duration = system_clock::duration;
  using ratio = std::ratio_divide<typename Duration::period, typename clock_duration::period>;
  if constexpr (ratio::den == 1) {
    if (value > std::numeric_limits<typename clock_duration::rep>::max() / ratio::num ||
        value < std::numeric_limits<typename clock_duration::rep>::min() / ratio::num)
      throw std::out_of_range("Timestamp exceeds system clock range");
  }
  return system_clock::time_point(duration_cast<clock_duration>(Duration(value)));
}
}  // namespace time_detail

export system_clock::time_point from_timestamp_sec(long long timestamp) {
  return time_detail::from_timestamp<seconds>(timestamp);
}
export system_clock::time_point from_timestamp_ms(long long timestamp) {
  return time_detail::from_timestamp<milliseconds>(timestamp);
}
export system_clock::time_point from_timestamp_us(long long timestamp) {
  return time_detail::from_timestamp<microseconds>(timestamp);
}

}  // namespace cppx
