
# cppx API Documentation

Complete API reference for all modules in the cppx library.

## Table of Contents

1. [cppx.error](#cppxerror)
2. [cppx.string_utils](#cppxstring_utils)
3. [cppx.file_system](#cppxfile_system)
4. [cppx.logging](#cppxlogging)
5. [cppx.time_utils](#cppxtime_utils)
6. [cppx.concurrent](#cppxconcurrent)
7. [cppx.math_utils](#cppxmath_utils)
8. [cppx.cli](#cppxcli)

---

## cppx.error

Modern error handling with `expected<T, E>` type and error utilities.

### expected<T, E>

Result type for operations that may fail (similar to Rust's `Result<T, E>`).

```cpp
import cppx.error;

// Create success value
auto result = expected<int, std::string>::ok(42);

// Create error value
auto error = expected<int, std::string>::err("something went wrong");
```

#### Methods

- `static expected ok(T value)` - Create successful result
- `static expected err(E error)` - Create error result
- `bool is_ok() const` - Check if result is success
- `bool is_err() const` - Check if result is error
- `T& value()` - Get value (throws if error)
- `E& error()` - Get error (throws if ok)
- `T value_or(T default_value)` - Get value or default
- `expected<U, E> map(F func)` - Transform the value
- `expected<T, F> map_err(F func)` - Transform the error
- `expected<U, E> and_then(F func)` - Chain operations

### error_info

Rich error information with context.

```cpp
error_info err("File not found");
err.add_context("read_config");
err.add_context("init_system");
// Result: "init_system -> read_config: File not found"
```

#### Methods

- `error_info(std::string message)` - Constructor
- `const std::string& message() const` - Get error message
- `const std::string& context() const` - Get context
- `void add_context(std::string_view ctx)` - Add context layer
- `std::string to_string() const` - Get formatted error string

### Utility Functions

- `auto ok(T value)` - Create ok result
- `auto err(E error)` - Create error result
- `T unwrap_or_throw(expected<T, E> result)` - Convert to exception
- `expected<R, error_info> try_catch(F func)` - Wrap function in try-catch

---

## cppx.string_utils

String manipulation and utilities.

### Splitting and Joining

```cpp
import cppx.string_utils;

// Split string
auto parts = split("a,b,c", ",");  // {"a", "b", "c"}

// Join strings
auto joined = join(parts, "-");  // "a-b-c"

// Split by any delimiter
auto tokens = split_any("a:b;c", ":;");  // {"a", "b", "c"}

// Split lines
auto lines = split_lines("line1\nline2\nline3");
```

### Trimming

```cpp
// Trim whitespace
auto trimmed = trim("  hello  ");        // "hello"
auto left = trim_left("  hello");        // "hello"
auto right = trim_right("hello  ");      // "hello"

// Trim specific characters
auto cleaned = trim_chars("..hello..", ".");  // "hello"
```

### Case Conversion

```cpp
auto upper = to_upper("hello");      // "HELLO"
auto lower = to_lower("WORLD");      // "world"
auto cap = capitalize("hello world"); // "Hello world"
```

### String Predicates

```cpp
bool starts = starts_with("hello world", "hello");  // true
bool ends = ends_with("hello world", "world");      // true
bool has = contains("hello world", "lo wo");        // true
bool equal = equals_ignore_case("Hello", "HELLO");  // true
```

### Replacement

```cpp
auto once = replace_first("hello hello", "hello", "hi");  // "hi hello"
auto all = replace_all("hello hello", "hello", "hi");     // "hi hi"
```

### Padding

```cpp
auto left = pad_left("5", 3, '0');      // "005"
auto right = pad_right("5", 3, '0');    // "500"
auto centered = center("hi", 6, '-');   // "--hi--"
```

### Regex

```cpp
// Match pattern
bool matches = regex_match("abc123", "[a-z]+\\d+");

// Search for pattern
bool found = regex_search("hello123world", "\\d+");

// Replace with regex
auto result = regex_replace("hello123world", "\\d+", "XXX");
// result.value() == "helloXXXworld"

// Find all matches
auto matches = regex_find_all("abc123def456", "\\d+");
// matches.value() == {"123", "456"}
```

### Parsing

```cpp
auto i = parse_int("42");         // expected<int, error_info>
auto l = parse_long("1000000");   // expected<long, error_info>
auto d = parse_double("3.14");    // expected<double, error_info>
```

---

## cppx.file_system

File I/O and filesystem operations.

### File Reading

```cpp
import cppx.file_system;

// Read entire file as text
auto content = read_file_text("data.txt");
if (content.is_ok()) {
    std::cout << content.value();
}

// Read as binary
auto binary = read_file_binary("image.png");

// Read lines
auto lines = read_lines("data.txt");
```

### File Writing

```cpp
// Write text
write_file("output.txt", "Hello, World!");

// Append
write_file("log.txt", "New entry\n", true);

// Write binary
std::vector<std::byte> data = {...};
write_file_binary("output.bin", data);

// Write lines
std::vector<std::string> lines = {"line1", "line2", "line3"};
write_lines("output.txt", lines);
```

### Path Operations

```cpp
// Join paths
auto path = path_join("/home", "user", "file.txt");

// Ensure directory exists
ensure_dir("/tmp/myapp/data");

// Check existence
bool exists = file_exists("file.txt");
bool is_dir = dir_exists("/tmp");

// File size
auto size = file_size("file.txt");

// Copy/move/remove
copy_file("src.txt", "dst.txt", true);  // overwrite
move_file("old.txt", "new.txt");
remove_file("temp.txt");
remove_dir("/tmp/mydir");

// List directory
auto entries = list_dir("/tmp");
```

### Temporary Files

```cpp
// Temporary file (RAII - auto-deleted)
{
    auto temp = temp_file("prefix");
    write_file(temp.path(), "temporary data");
    // File deleted when temp goes out of scope
}

// Keep temp file
auto temp = temp_file();
temp.keep();  // Won't be deleted

// Temporary directory
{
    auto dir = temp_dir("tmpdir");
    auto file = path_join(dir.path(), "file.txt");
    write_file(file, "data");
    // Directory and contents deleted when dir goes out of scope
}
```

---

## cppx.logging

Lightweight logging system with colors and rotation.

### Log Levels

```cpp
import cppx.logging;

enum class log_level {
    debug,  // Detailed information
    info,   // General information
    warn,   // Warning messages
    error,  // Error messages
    off     // Disable logging
};
```

### Logger Creation

```cpp
// Get or create logger
auto logger = get_logger("myapp");

// Create new logger
auto logger = create_logger("mylogger");

// Set global log level
set_global_log_level(log_level::info);
```

### Logging

```cpp
auto logger = get_logger("app");

// Set level
logger->set_level(log_level::debug);

// Log messages
logger->debug("Debug info");
logger->info("Application started");
logger->warn("Low memory");
logger->error("Failed to open file");

// Formatted logging
logger->info("User {} logged in", username);
logger->debug("Processing {} items", count);
```

### Sinks

```cpp
// Add console sink (with colors)
logger->add_console_sink(true);

// Add file sink (with rotation)
logger->add_file_sink("/var/log/app.log", 10 * 1024 * 1024);  // 10 MB

// Remove sinks
logger->remove_console_sink();
logger->remove_file_sink();
```

### Log Scope

```cpp
// Automatic timing of operations
{
    log_scope scope(logger, "expensive_operation");
    // Do work...
    // Logs entry, exit, and duration automatically
}
```

---

## cppx.time_utils

Time formatting, parsing, and utilities.

### Time Formatting

```cpp
import cppx.time_utils;

// Current time
auto iso = now_iso8601();     // "2025-12-05T10:30:45.123Z"
auto rfc = now_rfc3339();     // Same as ISO8601
auto custom = now_formatted("%Y-%m-%d %H:%M:%S");

// Format time point
auto tp = system_clock::now();
auto formatted = format_iso8601(tp);
auto custom = format_time(tp, "%Y/%m/%d");
```

### Duration Formatting

```cpp
auto ms = format_duration(milliseconds(1500));  // "1.500s"
auto min = format_duration(milliseconds(65000)); // "1m 5s"
auto hr = format_duration(milliseconds(3661000)); // "1h 1m 1s"
```

### Time Parsing

```cpp
auto tp = parse_iso8601("2025-12-05T10:30:45");
auto custom = parse_time("2025/12/05 10:30", "%Y/%m/%d %H:%M");
```

### Stopwatch

```cpp
stopwatch sw;
sw.start();
// Do work...
sw.stop();

auto ms = sw.elapsed_ms();
auto us = sw.elapsed_us();
auto ns = sw.elapsed_ns();

// Restart
sw.restart();
```

### Scope Timer

```cpp
// Automatic timing with callback
{
    scope_timer timer("operation", [](milliseconds dur) {
        std::cout << "Took " << dur.count() << "ms\n";
    });
    // Do work...
}

// Check elapsed without stopping
auto elapsed = timer.elapsed();
timer.reset();  // Reset timer
```

### Timer

```cpp
timer t;

// Periodic execution
t.start_periodic(milliseconds(1000), []() {
    std::cout << "Tick\n";
});

// One-shot execution
t.start_once(milliseconds(5000), []() {
    std::cout << "Fired after 5 seconds\n";
});

t.stop();
```

### Timestamps

```cpp
auto sec = timestamp_sec();  // Unix timestamp in seconds
auto ms = timestamp_ms();    // Milliseconds since epoch
auto us = timestamp_us();    // Microseconds since epoch

// Convert back
auto tp = from_timestamp_ms(ms);
```

### Sleep Utilities

```cpp
sleep_ms(100);     // Sleep for 100 milliseconds
sleep_sec(2);      // Sleep for 2 seconds
sleep_for(milliseconds(500));
sleep_until(timepoint);
```

---

## cppx.concurrent

Concurrency utilities including thread pool and channels.

### Thread Pool

```cpp
import cppx.concurrent;

// Create thread pool
thread_pool pool(4);  // 4 worker threads

// Submit task
auto future = pool.submit([]() {
    return 42;
});

int result = future.get();

// Submit with arguments
auto future2 = pool.submit([](int a, int b) {
    return a + b;
}, 10, 20);
```

### Parallel For

```cpp
// Parallel iteration
parallel_for(0, 1000, [](int i) {
    // Process item i in parallel
});

// With custom thread count
parallel_for(0, 1000, [](int i) {
    // Work...
}, 8);  // Use 8 threads

// Parallel for each
std::vector<int> data = {1, 2, 3, 4, 5};
parallel_for_each(data, [](int& value) {
    value *= 2;
});
```

### Channel

```cpp
// Create channel (Go-style)
channel<int> ch(10);  // Capacity of 10

// Send (blocks if full)
ch.send(42);

// Try send (non-blocking)
bool sent = ch.try_send(43);

// Receive (blocks if empty)
auto value = ch.receive();  // optional<int>

// Try receive (non-blocking)
auto value = ch.try_receive();

// Close channel
ch.close();
```

### Blocking Queue

```cpp
blocking_queue<std::string> queue;

// Push (never blocks)
queue.push("item1");
queue.push("item2");

// Pop (blocks until available)
std::string item = queue.pop();

// Try pop (non-blocking)
std::string item;
if (queue.try_pop(item)) {
    // Got item
}
```

### Atomic Counter

```cpp
atomic_counter counter(0);

counter.increment();  // Returns new value
counter.decrement();  // Returns new value
counter.add(10);      // Add delta
auto val = counter.get();
counter.set(100);

// Compare and exchange
bool swapped = counter.compare_exchange(100, 200);
```

### Spinlock

```cpp
spinlock lock;

lock.lock();
// Critical section
lock.unlock();

// Or use with RAII
{
    std::lock_guard<spinlock> guard(lock);
    // Critical section
}
```

### Utilities

```cpp
// Async execution
auto future = async([]() { return 42; });
int result = future.get();

// Parallel invoke (run multiple functions simultaneously)
parallel_invoke(
    []() { task1(); },
    []() { task2(); },
    []() { task3(); }
);

// Call once
call_once once;
once([]() {
    // Initialize something (called only once across all threads)
});
```

---

## cppx.math_utils

Mathematical utilities and statistics.

### Statistics

```cpp
import cppx.math_utils;

std::vector<int> data = {1, 2, 3, 4, 5};

auto avg = mean(data);          // 3.0
auto med = median(data);        // 3.0
auto var = variance(data);      // Variance
auto sd = std_dev(data);        // Standard deviation
auto min = min_value(data);     // 1
auto max = max_value(data);     // 5
auto rng = range(data);         // 4 (max - min)
auto s = sum(data);             // 15
auto p = product(data);         // 120
```

### Clamping and Normalization

```cpp
auto c = clamp(15, 0, 10);       // 10
auto n = normalize(50, 0, 100);  // 0.5 (normalized to [0, 1])
auto n2 = normalize(50, 0, 100, 0.0, 10.0);  // 5.0 (normalized to [0, 10])
auto d = denormalize(0.5, 0, 100);  // 50
```

### Interpolation

```cpp
auto l = lerp(0.0, 100.0, 0.5);     // 50.0
auto inv = inverse_lerp(0.0, 100.0, 25.0);  // 0.25
auto smooth = smoothstep(0.5);       // Smooth interpolation
auto smoother = smootherstep(0.5);   // Even smoother
```

### Random Numbers

```cpp
// Random integer
auto i = random_int(1, 10);  // [1, 10]

// Random float/double
auto r = random_real(0.0, 1.0);  // [0.0, 1.0)
auto r2 = random();              // [0.0, 1.0)

// Random boolean
auto b = random_bool(0.7);  // 70% chance of true

// Random choice
std::vector<std::string> items = {"a", "b", "c"};
auto choice = random_choice(items);

// Shuffle
shuffle(items);

// Random sample
auto sample = random_sample(items, 2);  // Get 2 random items

// Normal distribution
auto normal = random_normal(0.0, 1.0);  // Mean 0, stddev 1

// Set seed
set_random_seed(12345);
```

### Range Generation

```cpp
auto r1 = range(10);        // {0, 1, 2, ..., 9}
auto r2 = range(5, 15);     // {5, 6, 7, ..., 14}
auto r3 = range(0, 20, 3);  // {0, 3, 6, 9, 12, 15, 18}
```

### Vector Operations

```cpp
std::vector<int> a = {1, 2, 3};
std::vector<int> b = {4, 5, 6};

auto sum = add(a, b);           // {5, 7, 9}
auto diff = subtract(a, b);     // {-3, -3, -3}
auto mult = multiply(a, b);     // {4, 10, 18}
auto scaled = scale(a, 2);      // {2, 4, 6}
auto dot = dot_product(a, b);   // 32
auto mag = magnitude(a);        // √14
auto norm = normalize_vector(a); // Unit vector
```

### Angle Conversion

```cpp
constexpr double PI = 3.14159265358979323846;

auto rad = deg_to_rad(180.0);  // π radians
auto deg = rad_to_deg(PI);     // 180 degrees
```

### Common Operations

```cpp
bool eq = approx_equal(3.14, 3.14001, 0.001);  // true
int s = sign(-5);          // -1
auto a = abs(-10);         // 10
auto p = pow(2, 10);       // 1024
auto sq = sqrt(16);        // 4.0
auto rounded = round_to(3.14159, 2);  // 3.14
```

---

## cppx.cli

Command-line argument parsing with subcommands.

### Basic Usage

```cpp
import cppx.cli;

argument_parser parser("myapp", "My application");

// Add positional argument
parser.add_positional("input", "Input file", true);

// Add option
parser.add_option("output", "o", "Output file", false, "output.txt");

// Add flag
parser.add_flag("verbose", "v", "Verbose output");

// Parse
auto result = parser.parse(argc, argv);
if (result.is_ok()) {
    auto& args = result.value();
    auto input = args.get_string("input");
    bool verbose = args.get_flag("verbose");
}
```

### Argument Types

```cpp
// Positional (required by default)
parser.add_positional("filename", "File to process");

// Option with short and long form
parser.add_option("threads", "t", "Thread count", false, "4");

// Flag (boolean)
parser.add_flag("quiet", "q", "Quiet mode");

// With choices
parser.add_option("format", "f", "Output format");
parser.set_choices("format", {"json", "xml", "csv"});
```

### Parsing Results

```cpp
auto result = parser.parse(args);
if (result.is_ok()) {
    auto& parsed = result.value();
    
    // Get string
    auto str = parsed.get_string("name");  // optional<string>
    auto str2 = parsed.get_string_or("name", "default");
    
    // Get typed values
    auto i = parsed.get_int("count");      // expected<int, error_info>
    auto l = parsed.get_long("size");      // expected<long, error_info>
    auto d = parsed.get_double("rate");    // expected<double, error_info>
    
    // Get flag
    bool verbose = parsed.get_flag("verbose");
    
    // Check existence
    if (parsed.has("output")) { ... }
}
```

### Subcommands

```cpp
argument_parser parser("git", "Git version control");

// Add subcommand
auto commit_cmd = parser.add_subcommand("commit", "Commit changes");
commit_cmd->add_option("message", "m", "Commit message", true);
commit_cmd->add_flag("amend", "a", "Amend previous commit");

auto push_cmd = parser.add_subcommand("push", "Push changes");
push_cmd->add_positional("remote", "Remote name");

// Usage:
// ./git commit -m "message" --amend
// ./git push origin
```

### Help Generation

Help is automatically generated. Use `--help` or `-h` to display it:

```
myapp - My application

USAGE:
  myapp <input> [OPTIONS]

ARGUMENTS:
  input (required)
      Input file to process

OPTIONS:
  -h, --help
      Show this help message
  -o, --output <value>
      Output file
      [default: output.txt]
  -v, --verbose
      Enable verbose output
```

---

## Best Practices

### Error Handling

Prefer `expected<T, E>` over exceptions for recoverable errors:

```cpp
auto result = read_file("config.json");
if (result.is_err()) {
    // Handle error gracefully
    logger->error("Config error: {}", result.error().message());
    return false;
}
// Use result.value()
```

### Logging

Use appropriate log levels and structured logging:

```cpp
auto logger = get_logger("module_name");
logger->set_level(log_level::debug);  // In debug builds
logger->info("Operation {} completed: {} items", op_name, count);
```

### Concurrency

Use thread pool instead of creating threads manually:

```cpp
thread_pool pool;  // Uses hardware_concurrency by default

// Submit multiple tasks
std::vector<std::future<int>> futures;
for (auto& task : tasks) {
    futures.push_back(pool.submit(process_task, task));
}

// Collect results
for (auto& f : futures) {
    results.push_back(f.get());
}
```

### File Operations

Use RAII temporary files to avoid cleanup issues:

```cpp
{
    auto temp = temp_file();
    write_file(temp.path(), data);
    process_file(temp.path());
    // Automatically cleaned up
}
```

### String Processing

Use string_view for read-only operations to avoid copies:

```cpp
void process(std::string_view text) {
    if (starts_with(text, "prefix")) {
        auto parts = split(text, ",");
        // ...
    }
}
```

---

## Examples

See the [examples](../examples/) directory for complete, runnable examples of each module.

## License

MIT License - see [LICENSE](../LICENSE) file for details.

