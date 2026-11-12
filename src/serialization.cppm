export module cppx.serialization;

import std;

export namespace cppx {

// ============================================================================
// JSON 类型定义
// ============================================================================

struct json_parse_options {
  size_t max_input_bytes = 16 * 1024 * 1024;
  size_t max_depth = 128;
};

class json_parse_error : public std::runtime_error {
 public:
  json_parse_error(std::string_view message, size_t position)
      : std::runtime_error(std::format("{} at byte {}", message, position)), position_(position) {}
  [[nodiscard]] size_t position() const noexcept { return position_; }

 private:
  size_t position_;
};

namespace json_detail {
inline bool valid_utf8(std::string_view input) {
  for (size_t i = 0; i < input.size();) {
    auto c = static_cast<unsigned char>(input[i++]);
    if (c < 0x80) continue;
    unsigned count = 0;
    unsigned code = 0;
    unsigned minimum = 0;
    if (c >= 0xc2 && c <= 0xdf) {
      count = 1;
      code = c & 0x1f;
      minimum = 0x80;
    } else if (c >= 0xe0 && c <= 0xef) {
      count = 2;
      code = c & 0x0f;
      minimum = 0x800;
    } else if (c >= 0xf0 && c <= 0xf4) {
      count = 3;
      code = c & 7;
      minimum = 0x10000;
    } else
      return false;
    if (input.size() - i < count) return false;
    while (count--) {
      auto next = static_cast<unsigned char>(input[i++]);
      if ((next & 0xc0) != 0x80) return false;
      code = (code << 6) | (next & 0x3f);
    }
    if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) return false;
  }
  return true;
}
inline void append_utf8(std::string& out, unsigned code) {
  if (code < 0x80)
    out += static_cast<char>(code);
  else if (code < 0x800) {
    out += static_cast<char>(0xc0 | (code >> 6));
    out += static_cast<char>(0x80 | (code & 0x3f));
  } else if (code < 0x10000) {
    out += static_cast<char>(0xe0 | (code >> 12));
    out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    out += static_cast<char>(0x80 | (code & 0x3f));
  } else {
    out += static_cast<char>(0xf0 | (code >> 18));
    out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
    out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    out += static_cast<char>(0x80 | (code & 0x3f));
  }
}
}  // namespace json_detail

class json {
 public:
  enum class type { null, boolean, number, string, array, object };

  using array_type = std::vector<json>;
  using object_type = std::map<std::string, json>;

 private:
  using value_type = std::variant<std::nullptr_t, bool, double, std::string, array_type,
                                  object_type, std::int64_t, std::uint64_t>;

  value_type value_;

 public:
  // 构造函数
  json() : value_(nullptr) {}
  json(std::nullptr_t) : value_(nullptr) {}
  json(bool v) : value_(v) {}
  template <std::integral T>
    requires(!std::same_as<T, bool>)
  json(T v) {
    if constexpr (std::is_signed_v<T>)
      value_ = static_cast<std::int64_t>(v);
    else
      value_ = static_cast<std::uint64_t>(v);
  }
  json(float v) : json(static_cast<double>(v)) {}
  json(double v) : value_(v) {
    if (!std::isfinite(v)) throw std::invalid_argument("JSON numbers must be finite");
  }
  json(const char* v) : value_(nullptr) {
    if (!v) throw std::invalid_argument("JSON string pointer is null");
    value_ = std::string(v);
  }
  json(std::string v) : value_(std::move(v)) {}
  json(std::string_view v) : value_(std::string(v)) {}
  json(array_type v) : value_(std::move(v)) {}
  json(object_type v) : value_(std::move(v)) {}
  json(std::initializer_list<json> init) {
    // 如果是对象格式 {{"key", value}, ...}
    bool is_object = true;
    for (const auto& elem : init) {
      if (!elem.is_array() || elem.size() != 2 || !elem[0].is_string()) {
        is_object = false;
        break;
      }
    }

    if (is_object && init.size() > 0) {
      object_type obj;
      for (const auto& elem : init) {
        obj[elem[0].as_string()] = elem[1];
      }
      value_ = std::move(obj);
    } else {
      // 否则是数组
      value_ = array_type(init.begin(), init.end());
    }
  }

  // 静态工厂方法
  static json object() { return json(object_type{}); }

  static json array() { return json(array_type{}); }

  // 类型检查
  type get_type() const { return is_number() ? type::number : static_cast<type>(value_.index()); }

  bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
  bool is_bool() const { return std::holds_alternative<bool>(value_); }
  bool is_integer() const { return std::holds_alternative<std::int64_t>(value_); }
  bool is_unsigned() const { return std::holds_alternative<std::uint64_t>(value_); }
  bool is_number() const {
    return is_integer() || is_unsigned() || std::holds_alternative<double>(value_);
  }
  bool is_string() const { return std::holds_alternative<std::string>(value_); }
  bool is_array() const { return std::holds_alternative<array_type>(value_); }
  bool is_object() const { return std::holds_alternative<object_type>(value_); }

  // 值访问（带类型检查）
  bool as_bool() const {
    if (auto* v = std::get_if<bool>(&value_)) return *v;
    throw std::runtime_error("json value is not a boolean");
  }

  double as_number() const {
    if (auto* v = std::get_if<double>(&value_)) return *v;
    if (auto* v = std::get_if<std::int64_t>(&value_)) return static_cast<double>(*v);
    if (auto* v = std::get_if<std::uint64_t>(&value_)) return static_cast<double>(*v);
    throw std::runtime_error("json value is not a number");
  }

  template <std::integral T>
    requires(!std::same_as<T, bool>)
  T as_integer() const {
    if (auto* v = std::get_if<std::int64_t>(&value_)) {
      if constexpr (std::is_signed_v<T>) {
        if (*v >= static_cast<std::int64_t>(std::numeric_limits<T>::min()) &&
            *v <= static_cast<std::int64_t>(std::numeric_limits<T>::max()))
          return static_cast<T>(*v);
      } else {
        if (*v >= 0 && static_cast<std::uint64_t>(*v) <= std::numeric_limits<T>::max())
          return static_cast<T>(*v);
      }
    } else if (auto* v = std::get_if<std::uint64_t>(&value_)) {
      if (*v <= static_cast<std::uint64_t>(std::numeric_limits<T>::max()))
        return static_cast<T>(*v);
    } else {
      double number = as_number();
      double upper = std::ldexp(1.0, std::numeric_limits<T>::digits);
      double lower = std::is_signed_v<T> ? -upper : 0.0;
      if (std::isfinite(number) && std::trunc(number) == number && number >= lower &&
          number < upper)
        return static_cast<T>(number);
    }
    throw std::out_of_range("JSON integer conversion is fractional or out of range");
  }
  int as_int() const { return as_integer<int>(); }
  std::int64_t as_int64() const { return as_integer<std::int64_t>(); }
  std::uint64_t as_uint64() const { return as_integer<std::uint64_t>(); }

  const std::string& as_string() const {
    if (auto* v = std::get_if<std::string>(&value_)) return *v;
    throw std::runtime_error("json value is not a string");
  }

  const array_type& as_array() const {
    if (auto* v = std::get_if<array_type>(&value_)) return *v;
    throw std::runtime_error("json value is not an array");
  }

  array_type& as_array() {
    if (auto* v = std::get_if<array_type>(&value_)) return *v;
    throw std::runtime_error("json value is not an array");
  }

  const object_type& as_object() const {
    if (auto* v = std::get_if<object_type>(&value_)) return *v;
    throw std::runtime_error("json value is not an object");
  }

  object_type& as_object() {
    if (auto* v = std::get_if<object_type>(&value_)) return *v;
    throw std::runtime_error("json value is not an object");
  }

  // 通用模板访问
  template <typename T>
  T as() const {
    if constexpr (std::same_as<T, bool>)
      return as_bool();
    else if constexpr (std::integral<T>)
      return as_integer<T>();
    else if constexpr (std::floating_point<T>) {
      auto value = as_number();
      if (value < -std::numeric_limits<T>::max() || value > std::numeric_limits<T>::max())
        throw std::out_of_range("JSON floating conversion out of range");
      return static_cast<T>(value);
    } else if constexpr (std::same_as<T, std::string>)
      return as_string();
    else
      static_assert(std::is_arithmetic_v<T>, "Unsupported JSON value access type");
  }

  // 对象操作
  json& operator[](const std::string& key) {
    if (!is_object()) {
      value_ = object_type{};
    }
    return as_object()[key];
  }

  const json& operator[](const std::string& key) const {
    const auto& obj = as_object();
    auto it = obj.find(key);
    if (it == obj.end()) {
      throw std::runtime_error("key not found: " + key);
    }
    return it->second;
  }

  bool contains(const std::string& key) const {
    if (!is_object()) return false;
    return as_object().contains(key);
  }

  // 数组操作
  json& operator[](size_t index) {
    if (!is_array()) {
      throw std::runtime_error("json value is not an array");
    }
    return as_array().at(index);
  }

  const json& operator[](size_t index) const { return as_array().at(index); }

  size_t size() const {
    if (is_array()) return as_array().size();
    if (is_object()) return as_object().size();
    throw std::runtime_error("json value is not an array or object");
  }

  void push_back(json value) {
    if (!is_array()) {
      value_ = array_type{};
    }
    as_array().push_back(std::move(value));
  }

  // 序列化为字符串
  std::string dump(int indent = -1) const {
    if (indent < -1 || indent > 1024) throw std::invalid_argument("JSON indent must be -1..1024");
    return dump_impl(indent, 0);
  }

  // 从字符串解析
  static json parse(std::string_view str, json_parse_options options = {});

 private:
  std::string dump_impl(int indent, int current_depth) const {
    if (current_depth > 512) throw std::length_error("JSON output nesting limit exceeded");
    std::string result;

    if (is_null()) {
      result = "null";
    } else if (is_bool()) {
      result = as_bool() ? "true" : "false";
    } else if (is_number()) {
      if (is_integer())
        result = std::to_string(std::get<std::int64_t>(value_));
      else if (is_unsigned())
        result = std::to_string(std::get<std::uint64_t>(value_));
      else {
        result = std::format("{}", as_number());
        if (result.find_first_of(".eE") == std::string::npos) result += ".0";
      }
    } else if (is_string()) {
      result = "\"" + escape_string(as_string()) + "\"";
    } else if (is_array()) {
      const auto& arr = as_array();
      if (arr.empty()) {
        result = "[]";
      } else {
        result = "[";
        if (indent >= 0) {
          result += "\n";
          std::string indent_str(indent * (current_depth + 1), ' ');
          for (size_t i = 0; i < arr.size(); ++i) {
            result += indent_str + arr[i].dump_impl(indent, current_depth + 1);
            if (i < arr.size() - 1) result += ",";
            result += "\n";
          }
          result += std::string(indent * current_depth, ' ');
        } else {
          for (size_t i = 0; i < arr.size(); ++i) {
            result += arr[i].dump_impl(indent, current_depth + 1);
            if (i < arr.size() - 1) result += ",";
          }
        }
        result += "]";
      }
    } else if (is_object()) {
      const auto& obj = as_object();
      if (obj.empty()) {
        result = "{}";
      } else {
        result = "{";
        if (indent >= 0) {
          result += "\n";
          std::string indent_str(indent * (current_depth + 1), ' ');
          size_t i = 0;
          for (const auto& [key, value] : obj) {
            result += indent_str + "\"" + escape_string(key) + "\": ";
            result += value.dump_impl(indent, current_depth + 1);
            if (i < obj.size() - 1) result += ",";
            result += "\n";
            ++i;
          }
          result += std::string(indent * current_depth, ' ');
        } else {
          size_t i = 0;
          for (const auto& [key, value] : obj) {
            result += "\"" + escape_string(key) + "\":";
            result += value.dump_impl(indent, current_depth + 1);
            if (i < obj.size() - 1) result += ",";
            ++i;
          }
        }
        result += "}";
      }
    }

    return result;
  }

  static std::string escape_string(const std::string& str) {
    std::string result;
    if (!json_detail::valid_utf8(str)) throw std::invalid_argument("JSON string is not UTF-8");
    for (unsigned char c : str) {
      switch (c) {
        case '\"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
          if (c < 0x20) {
            result += std::format("\\u{:04x}", static_cast<unsigned char>(c));
          } else {
            result += c;
          }
      }
    }
    return result;
  }
};

// Strict parser: errors use zero-based byte positions and limits count containers.
class json_parser {
 public:
  explicit json_parser(std::string_view input, json_parse_options options = {})
      : input_(input), options_(options) {}
  json parse() {
    if (input_.size() > options_.max_input_bytes) fail("JSON input size limit exceeded");
    if (options_.max_depth > 512) throw std::invalid_argument("JSON depth limit must be <= 512");
    auto result = parse_value(0);
    whitespace();
    if (pos_ != input_.size()) fail("Unexpected trailing input");
    return result;
  }

 private:
  std::string_view input_;
  json_parse_options options_;
  size_t pos_ = 0;
  [[noreturn]] void fail(std::string_view message) const { throw json_parse_error(message, pos_); }
  char peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }
  char next() {
    if (pos_ == input_.size()) fail("Unexpected end of input");
    return input_[pos_++];
  }
  static bool digit(char c) { return c >= '0' && c <= '9'; }
  void whitespace() {
    while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r') ++pos_;
  }
  json literal(std::string_view token, json result) {
    if (input_.substr(pos_, token.size()) != token) fail("Invalid literal");
    pos_ += token.size();
    return result;
  }
  json parse_value(size_t depth) {
    whitespace();
    switch (peek()) {
      case 'n': return literal("null", json(nullptr));
      case 't': return literal("true", json(true));
      case 'f': return literal("false", json(false));
      case '"': return json(parse_string());
      case '[':
      case '{': {
        if (depth >= options_.max_depth) fail("JSON nesting limit exceeded");
        return peek() == '[' ? parse_array(depth + 1) : parse_object(depth + 1);
      }
      default:
        if (peek() == '-' || digit(peek())) return parse_number();
        fail("Expected JSON value");
    }
  }
  json parse_number() {
    auto start = pos_;
    bool negative = peek() == '-';
    if (negative) ++pos_;
    if (peek() == '0') {
      ++pos_;
      if (digit(peek())) fail("Leading zero in number");
    } else {
      if (!digit(peek())) fail("Expected digit");
      while (digit(peek())) ++pos_;
    }
    bool floating = false;
    if (peek() == '.') {
      floating = true;
      ++pos_;
      if (!digit(peek())) fail("Expected fraction digits");
      while (digit(peek())) ++pos_;
    }
    if (peek() == 'e' || peek() == 'E') {
      floating = true;
      ++pos_;
      if (peek() == '+' || peek() == '-') ++pos_;
      if (!digit(peek())) fail("Expected exponent digits");
      while (digit(peek())) ++pos_;
    }
    auto first = input_.data() + start;
    auto last = input_.data() + pos_;
    if (floating) {
      double value{};
      auto result = std::from_chars(first, last, value, std::chars_format::general);
      if (result.ec != std::errc{} || result.ptr != last || !std::isfinite(value))
        fail("Floating number out of range");
      return json(value);
    }
    if (negative) {
      std::int64_t value{};
      auto result = std::from_chars(first, last, value);
      if (result.ec != std::errc{}) fail("Signed integer out of range");
      return json(value);
    }
    std::uint64_t value{};
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{}) fail("Unsigned integer out of range");
    return json(value);
  }
  unsigned hex4() {
    unsigned value = 0;
    for (int i = 0; i < 4; ++i) {
      char c = next();
      unsigned digit_value;
      if (c >= '0' && c <= '9')
        digit_value = c - '0';
      else if (c >= 'a' && c <= 'f')
        digit_value = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
        digit_value = c - 'A' + 10;
      else
        fail("Invalid Unicode escape");
      value = (value << 4) | digit_value;
    }
    return value;
  }
  std::string parse_string() {
    if (next() != '"') fail("Expected string");
    std::string out;
    while (true) {
      auto c = static_cast<unsigned char>(next());
      if (c == '"') break;
      if (c < 0x20) fail("Unescaped control character");
      if (c != '\\') {
        out += static_cast<char>(c);
        continue;
      }
      switch (next()) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned code = hex4();
          if (code >= 0xd800 && code <= 0xdbff) {
            if (next() != '\\' || next() != 'u') fail("Expected low surrogate");
            auto low = hex4();
            if (low < 0xdc00 || low > 0xdfff) fail("Invalid low surrogate");
            code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
          } else if (code >= 0xdc00 && code <= 0xdfff)
            fail("Unpaired low surrogate");
          json_detail::append_utf8(out, code);
          break;
        }
        default: fail("Invalid string escape");
      }
    }
    if (!json_detail::valid_utf8(out)) fail("Invalid UTF-8 string");
    return out;
  }
  json parse_array(size_t depth) {
    ++pos_;
    json::array_type out;
    whitespace();
    if (peek() == ']') {
      ++pos_;
      return json(std::move(out));
    }
    while (true) {
      out.push_back(parse_value(depth));
      whitespace();
      char c = next();
      if (c == ']') return json(std::move(out));
      if (c != ',') fail("Expected comma or closing bracket");
    }
  }
  json parse_object(size_t depth) {
    ++pos_;
    json::object_type out;
    whitespace();
    if (peek() == '}') {
      ++pos_;
      return json(std::move(out));
    }
    while (true) {
      whitespace();
      auto key = parse_string();
      whitespace();
      if (next() != ':') fail("Expected colon");
      auto value = parse_value(depth);
      if (!out.emplace(std::move(key), std::move(value)).second) fail("Duplicate object key");
      whitespace();
      char c = next();
      if (c == '}') return json(std::move(out));
      if (c != ',') fail("Expected comma or closing brace");
    }
  }
};

inline json json::parse(std::string_view str, json_parse_options options) {
  return json_parser(str, options).parse();
}

// ============================================================================
// 序列化器接口
// ============================================================================

template <typename T>
struct serializer;

// 基础类型特化
template <>
struct serializer<bool> {
  static json to_json(const bool& value) { return json(value); }
  static bool from_json(const json& j) { return j.as_bool(); }
};

template <std::integral T>
  requires(!std::same_as<T, bool>)
struct serializer<T> {
  static json to_json(T value) { return json(value); }
  static T from_json(const json& j) { return j.as_integer<T>(); }
};

template <std::floating_point T>
struct serializer<T> {
  static json to_json(T value) {
    if (!std::isfinite(value) || value < -std::numeric_limits<double>::max() ||
        value > std::numeric_limits<double>::max())
      throw std::out_of_range("JSON floating number out of range");
    return json(static_cast<double>(value));
  }
  static T from_json(const json& j) { return j.as<T>(); }
};

template <>
struct serializer<std::string> {
  static json to_json(const std::string& value) { return json(value); }
  static std::string from_json(const json& j) { return j.as_string(); }
};

// 容器类型特化
template <typename T>
struct serializer<std::vector<T>> {
  static json to_json(const std::vector<T>& value) {
    json::array_type arr;
    for (const auto& elem : value) {
      arr.push_back(serializer<T>::to_json(elem));
    }
    return json(arr);
  }

  static std::vector<T> from_json(const json& j) {
    const auto& arr = j.as_array();
    std::vector<T> result;
    result.reserve(arr.size());
    for (const auto& elem : arr) {
      result.push_back(serializer<T>::from_json(elem));
    }
    return result;
  }
};

template <typename K, typename V>
struct serializer<std::map<K, V>> {
  static json to_json(const std::map<K, V>& value) {
    json::object_type obj;
    for (const auto& [key, val] : value) {
      // 将 key 转换为字符串
      std::string key_str;
      if constexpr (std::is_same_v<K, std::string>) {
        key_str = key;
      } else {
        key_str = std::to_string(key);
      }
      obj[key_str] = serializer<V>::to_json(val);
    }
    return json(obj);
  }

  static std::map<K, V> from_json(const json& j) {
    const auto& obj = j.as_object();
    std::map<K, V> result;
    for (const auto& [key_str, val] : obj) {
      K key;
      if constexpr (std::is_same_v<K, std::string>) {
        key = key_str;
      } else {
        static_assert(std::integral<K> && !std::same_as<K, bool>,
                      "JSON map keys must be strings or integers");
        auto parsed = std::from_chars(key_str.data(), key_str.data() + key_str.size(), key);
        if (parsed.ec != std::errc{} || parsed.ptr != key_str.data() + key_str.size())
          throw std::invalid_argument("Invalid JSON integer map key");
      }
      if (!result.emplace(key, serializer<V>::from_json(val)).second)
        throw std::invalid_argument("Duplicate converted JSON map key");
    }
    return result;
  }
};

template <typename T>
struct serializer<std::optional<T>> {
  static json to_json(const std::optional<T>& value) {
    if (value.has_value()) {
      return serializer<T>::to_json(*value);
    }
    return json(nullptr);
  }

  static std::optional<T> from_json(const json& j) {
    if (j.is_null()) {
      return std::nullopt;
    }
    return serializer<T>::from_json(j);
  }
};

// ============================================================================
// 便捷函数
// ============================================================================

template <typename T>
std::string to_json_string(const T& value, int indent = -1) {
  auto j = serializer<T>::to_json(value);
  return j.dump(indent);
}

template <typename T>
T from_json_string(std::string_view json_str) {
  auto j = json::parse(json_str);
  return serializer<T>::from_json(j);
}

// 文件 I/O
// Keep serialization helpers out of the public file-system overload set.
namespace json_detail {
inline std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to open file: " + path.string());
  }

  std::ostringstream ss;
  char buffer[8192];
  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) ss.write(buffer, file.gcount());
  if (file.bad() || !file.eof()) throw std::runtime_error("failed to read file: " + path.string());
  return ss.str();
}

inline void write_file(const std::filesystem::path& path, std::string_view content) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to create file: " + path.string());
  }

  file << content;
  file.flush();
  if (!file) throw std::runtime_error("failed to write file: " + path.string());
  file.close();
  if (!file) throw std::runtime_error("failed to close file: " + path.string());
}
}  // namespace json_detail

template <typename T>
void save_json(const std::filesystem::path& path, const T& value, int indent = 2) {
  auto json_str = to_json_string(value, indent);
  json_detail::write_file(path, json_str);
}

template <typename T>
T load_json(const std::filesystem::path& path) {
  auto json_str = json_detail::read_file(path);
  return from_json_string<T>(json_str);
}

}  // namespace cppx
