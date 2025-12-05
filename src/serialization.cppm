export module cppx.serialization;

import std;

export namespace cppx {

// ============================================================================
// JSON 类型定义
// ============================================================================

class json {
public:
    enum class type {
        null,
        boolean,
        number,
        string,
        array,
        object
    };
    
    using array_type = std::vector<json>;
    using object_type = std::map<std::string, json>;
    
private:
    using value_type = std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        array_type,
        object_type
    >;
    
    value_type value_;
    
public:
    // 构造函数
    json() : value_(nullptr) {}
    json(std::nullptr_t) : value_(nullptr) {}
    json(bool v) : value_(v) {}
    json(int v) : value_(static_cast<double>(v)) {}
    json(long v) : value_(static_cast<double>(v)) {}
    json(long long v) : value_(static_cast<double>(v)) {}
    json(unsigned int v) : value_(static_cast<double>(v)) {}
    json(unsigned long v) : value_(static_cast<double>(v)) {}
    json(unsigned long long v) : value_(static_cast<double>(v)) {}
    json(float v) : value_(static_cast<double>(v)) {}
    json(double v) : value_(v) {}
    json(const char* v) : value_(std::string(v)) {}
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
    static json object() {
        return json(object_type{});
    }
    
    static json array() {
        return json(array_type{});
    }
    
    // 类型检查
    type get_type() const {
        return static_cast<type>(value_.index());
    }
    
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool is_bool() const { return std::holds_alternative<bool>(value_); }
    bool is_number() const { return std::holds_alternative<double>(value_); }
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
        throw std::runtime_error("json value is not a number");
    }
    
    int as_int() const {
        return static_cast<int>(as_number());
    }
    
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
    template<typename T>
    T as() const;
    
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
        return as_array()[index];
    }
    
    const json& operator[](size_t index) const {
        return as_array()[index];
    }
    
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
        return dump_impl(indent, 0);
    }
    
    // 从字符串解析
    static json parse(std::string_view str);
    
private:
    std::string dump_impl(int indent, int current_depth) const {
        std::string result;
        
        if (is_null()) {
            result = "null";
        } else if (is_bool()) {
            result = as_bool() ? "true" : "false";
        } else if (is_number()) {
            double num = as_number();
            // 检查是否为整数
            if (num == static_cast<long long>(num)) {
                result = std::to_string(static_cast<long long>(num));
            } else {
                result = std::format("{}", num);
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
        for (char c : str) {
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

// json::as<T> 特化
template<> inline bool json::as<bool>() const { return as_bool(); }
template<> inline int json::as<int>() const { return as_int(); }
template<> inline double json::as<double>() const { return as_number(); }
template<> inline std::string json::as<std::string>() const { return as_string(); }

// ============================================================================
// JSON 解析器
// ============================================================================

class json_parser {
private:
    std::string_view input_;
    size_t pos_ = 0;
    
public:
    explicit json_parser(std::string_view input) : input_(input) {}
    
    json parse() {
        skip_whitespace();
        auto result = parse_value();
        skip_whitespace();
        if (pos_ < input_.size()) {
            throw std::runtime_error("unexpected characters after JSON value");
        }
        return result;
    }
    
private:
    char peek() const {
        if (pos_ >= input_.size()) return '\0';
        return input_[pos_];
    }
    
    char next() {
        if (pos_ >= input_.size()) return '\0';
        return input_[pos_++];
    }
    
    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) {
            ++pos_;
        }
    }
    
    json parse_value() {
        skip_whitespace();
        char c = peek();
        
        if (c == '\0') throw std::runtime_error("unexpected end of JSON input");
        
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(c)) return parse_number();
        
        throw std::runtime_error(std::format("unexpected character: '{}'", c));
    }
    
    json parse_null() {
        if (input_.substr(pos_, 4) == "null") {
            pos_ += 4;
            return json(nullptr);
        }
        throw std::runtime_error("invalid null value");
    }
    
    json parse_bool() {
        if (input_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return json(true);
        }
        if (input_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return json(false);
        }
        throw std::runtime_error("invalid boolean value");
    }
    
    json parse_number() {
        size_t start = pos_;
        
        if (peek() == '-') next();
        
        if (!std::isdigit(peek())) {
            throw std::runtime_error("invalid number");
        }
        
        while (std::isdigit(peek())) next();
        
        if (peek() == '.') {
            next();
            if (!std::isdigit(peek())) {
                throw std::runtime_error("invalid number");
            }
            while (std::isdigit(peek())) next();
        }
        
        if (peek() == 'e' || peek() == 'E') {
            next();
            if (peek() == '+' || peek() == '-') next();
            if (!std::isdigit(peek())) {
                throw std::runtime_error("invalid number");
            }
            while (std::isdigit(peek())) next();
        }
        
        std::string num_str(input_.substr(start, pos_ - start));
        return json(std::stod(num_str));
    }
    
    json parse_string() {
        if (next() != '"') throw std::runtime_error("expected '\"'");
        
        std::string result;
        while (true) {
            char c = next();
            if (c == '\0') throw std::runtime_error("unterminated string");
            if (c == '"') break;
            
            if (c == '\\') {
                c = next();
                switch (c) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // Unicode escape \uXXXX
                        std::string hex;
                        for (int i = 0; i < 4; ++i) {
                            char h = next();
                            if (!std::isxdigit(h)) {
                                throw std::runtime_error("invalid unicode escape");
                            }
                            hex += h;
                        }
                        int codepoint = std::stoi(hex, nullptr, 16);
                        if (codepoint < 0x80) {
                            result += static_cast<char>(codepoint);
                        } else {
                            // 简化处理：只支持基本的 Unicode
                            result += '?';
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error(std::format("invalid escape sequence: \\{}", c));
                }
            } else {
                result += c;
            }
        }
        
        return json(result);
    }
    
    json parse_array() {
        if (next() != '[') throw std::runtime_error("expected '['");
        
        json::array_type arr;
        skip_whitespace();
        
        if (peek() == ']') {
            next();
            return json(arr);
        }
        
        while (true) {
            arr.push_back(parse_value());
            skip_whitespace();
            
            char c = next();
            if (c == ']') break;
            if (c != ',') throw std::runtime_error("expected ',' or ']'");
            skip_whitespace();
        }
        
        return json(arr);
    }
    
    json parse_object() {
        if (next() != '{') throw std::runtime_error("expected '{'");
        
        json::object_type obj;
        skip_whitespace();
        
        if (peek() == '}') {
            next();
            return json(obj);
        }
        
        while (true) {
            skip_whitespace();
            if (peek() != '"') throw std::runtime_error("expected string key");
            
            std::string key = parse_string().as_string();
            skip_whitespace();
            
            if (next() != ':') throw std::runtime_error("expected ':'");
            
            obj[key] = parse_value();
            skip_whitespace();
            
            char c = next();
            if (c == '}') break;
            if (c != ',') throw std::runtime_error("expected ',' or '}'");
        }
        
        return json(obj);
    }
};

inline json json::parse(std::string_view str) {
    json_parser parser(str);
    return parser.parse();
}

// ============================================================================
// 序列化器接口
// ============================================================================

template<typename T>
struct serializer;

// 基础类型特化
template<> struct serializer<bool> {
    static json to_json(const bool& value) { return json(value); }
    static bool from_json(const json& j) { return j.as_bool(); }
};

template<> struct serializer<int> {
    static json to_json(const int& value) { return json(value); }
    static int from_json(const json& j) { return j.as_int(); }
};

template<> struct serializer<long> {
    static json to_json(const long& value) { return json(value); }
    static long from_json(const json& j) { return static_cast<long>(j.as_number()); }
};

template<> struct serializer<long long> {
    static json to_json(const long long& value) { return json(value); }
    static long long from_json(const json& j) { return static_cast<long long>(j.as_number()); }
};

template<> struct serializer<unsigned int> {
    static json to_json(const unsigned int& value) { return json(value); }
    static unsigned int from_json(const json& j) { return static_cast<unsigned int>(j.as_number()); }
};

template<> struct serializer<unsigned long> {
    static json to_json(const unsigned long& value) { return json(value); }
    static unsigned long from_json(const json& j) { return static_cast<unsigned long>(j.as_number()); }
};

template<> struct serializer<unsigned long long> {
    static json to_json(const unsigned long long& value) { return json(value); }
    static unsigned long long from_json(const json& j) { return static_cast<unsigned long long>(j.as_number()); }
};

template<> struct serializer<float> {
    static json to_json(const float& value) { return json(value); }
    static float from_json(const json& j) { return static_cast<float>(j.as_number()); }
};

template<> struct serializer<double> {
    static json to_json(const double& value) { return json(value); }
    static double from_json(const json& j) { return j.as_number(); }
};

template<> struct serializer<std::string> {
    static json to_json(const std::string& value) { return json(value); }
    static std::string from_json(const json& j) { return j.as_string(); }
};

// 容器类型特化
template<typename T>
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

template<typename K, typename V>
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
            } else if constexpr (std::is_same_v<K, int>) {
                key = std::stoi(key_str);
            } else {
                // 其他类型需要额外处理
                key = static_cast<K>(std::stoll(key_str));
            }
            result[key] = serializer<V>::from_json(val);
        }
        return result;
    }
};

template<typename T>
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

template<typename T>
std::string to_json_string(const T& value, int indent = -1) {
    auto j = serializer<T>::to_json(value);
    return j.dump(indent);
}

template<typename T>
T from_json_string(std::string_view json_str) {
    auto j = json::parse(json_str);
    return serializer<T>::from_json(j);
}

// 文件 I/O
inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file: " + path.string());
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline void write_file(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to create file: " + path.string());
    }
    
    file << content;
}

template<typename T>
void save_json(const std::filesystem::path& path, const T& value, int indent = 2) {
    auto json_str = to_json_string(value, indent);
    write_file(path, json_str);
}

template<typename T>
T load_json(const std::filesystem::path& path) {
    auto json_str = read_file(path);
    return from_json_string<T>(json_str);
}

} // namespace cppx

