// cppx.error - Modern error handling utilities
// Provides expected<T, E> type and error utilities

export module cppx.error;

import std;

namespace cppx {

// ============================================================================
// expected<T, E> - Result type for operations that may fail
// ============================================================================

export template<typename T, typename E>
class expected {
public:
    // Constructors
    expected() = delete;
    
    // Create a successful result
    static expected ok(const T& value) {
        return expected(ok_tag{}, value);
    }
    
    static expected ok(T&& value) {
        return expected(ok_tag{}, std::move(value));
    }
    
    // Create an error result
    static expected err(const E& error) {
        return expected(err_tag{}, error);
    }
    
    static expected err(E&& error) {
        return expected(err_tag{}, std::move(error));
    }
    
    // Copy and move constructors
    expected(const expected& other) = default;
    expected(expected&& other) noexcept = default;
    expected& operator=(const expected& other) = default;
    expected& operator=(expected&& other) noexcept = default;
    
    // Check if result is ok or error
    [[nodiscard]] bool is_ok() const noexcept {
        return std::holds_alternative<T>(storage_);
    }
    
    [[nodiscard]] bool is_err() const noexcept {
        return std::holds_alternative<E>(storage_);
    }
    
    // Get value (throws if error)
    [[nodiscard]] const T& value() const& {
        if (is_err()) {
            throw std::runtime_error("Called value() on error result");
        }
        return std::get<T>(storage_);
    }
    
    [[nodiscard]] T& value() & {
        if (is_err()) {
            throw std::runtime_error("Called value() on error result");
        }
        return std::get<T>(storage_);
    }
    
    [[nodiscard]] T&& value() && {
        if (is_err()) {
            throw std::runtime_error("Called value() on error result");
        }
        return std::move(std::get<T>(storage_));
    }
    
    // Get error (throws if ok)
    [[nodiscard]] const E& error() const& {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return std::get<E>(storage_);
    }
    
    [[nodiscard]] E& error() & {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return std::get<E>(storage_);
    }
    
    [[nodiscard]] E&& error() && {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return std::move(std::get<E>(storage_));
    }
    
    // Get value or default
    [[nodiscard]] T value_or(const T& default_value) const& {
        return is_ok() ? value() : default_value;
    }
    
    [[nodiscard]] T value_or(T&& default_value) && {
        return is_ok() ? std::move(value()) : std::move(default_value);
    }
    
    // Map operations
    template<typename F>
    auto map(F&& f) const& -> expected<std::invoke_result_t<F, const T&>, E> {
        if (is_ok()) {
            return expected<std::invoke_result_t<F, const T&>, E>::ok(f(value()));
        } else {
            return expected<std::invoke_result_t<F, const T&>, E>::err(error());
        }
    }
    
    template<typename F>
    auto map(F&& f) && -> expected<std::invoke_result_t<F, T&&>, E> {
        if (is_ok()) {
            return expected<std::invoke_result_t<F, T&&>, E>::ok(f(std::move(value())));
        } else {
            return expected<std::invoke_result_t<F, T&&>, E>::err(std::move(error()));
        }
    }
    
    template<typename F>
    auto map_err(F&& f) const& -> expected<T, std::invoke_result_t<F, const E&>> {
        if (is_err()) {
            return expected<T, std::invoke_result_t<F, const E&>>::err(f(error()));
        } else {
            return expected<T, std::invoke_result_t<F, const E&>>::ok(value());
        }
    }
    
    template<typename F>
    auto map_err(F&& f) && -> expected<T, std::invoke_result_t<F, E&&>> {
        if (is_err()) {
            return expected<T, std::invoke_result_t<F, E&&>>::err(f(std::move(error())));
        } else {
            return expected<T, std::invoke_result_t<F, E&&>>::ok(std::move(value()));
        }
    }
    
    // And then (flatMap/bind)
    template<typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&> {
        if (is_ok()) {
            return f(value());
        } else {
            using result_type = std::invoke_result_t<F, const T&>;
            return result_type::err(error());
        }
    }
    
    template<typename F>
    auto and_then(F&& f) && -> std::invoke_result_t<F, T&&> {
        if (is_ok()) {
            return f(std::move(value()));
        } else {
            using result_type = std::invoke_result_t<F, T&&>;
            return result_type::err(std::move(error()));
        }
    }
    
    // Boolean operators
    explicit operator bool() const noexcept {
        return is_ok();
    }

private:
    struct ok_tag {};
    struct err_tag {};
    
    expected(ok_tag, const T& value) : storage_(value) {}
    expected(ok_tag, T&& value) : storage_(std::move(value)) {}
    expected(err_tag, const E& error) : storage_(error) {}
    expected(err_tag, E&& error) : storage_(std::move(error)) {}
    
    std::variant<T, E> storage_;
};

// Specialization for void type
export template<typename E>
class expected<void, E> {
public:
    expected() = delete;
    
    static expected ok() {
        return expected(ok_tag{});
    }
    
    static expected err(const E& error) {
        return expected(err_tag{}, error);
    }
    
    static expected err(E&& error) {
        return expected(err_tag{}, std::move(error));
    }
    
    expected(const expected& other) = default;
    expected(expected&& other) noexcept = default;
    expected& operator=(const expected& other) = default;
    expected& operator=(expected&& other) noexcept = default;
    
    [[nodiscard]] bool is_ok() const noexcept {
        return !has_error_;
    }
    
    [[nodiscard]] bool is_err() const noexcept {
        return has_error_;
    }
    
    [[nodiscard]] const E& error() const& {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return error_;
    }
    
    [[nodiscard]] E& error() & {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return error_;
    }
    
    [[nodiscard]] E&& error() && {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok result");
        }
        return std::move(error_);
    }
    
    explicit operator bool() const noexcept {
        return is_ok();
    }

private:
    struct ok_tag {};
    struct err_tag {};
    
    expected(ok_tag) : has_error_(false), error_() {}
    expected(err_tag, const E& error) : has_error_(true), error_(error) {}
    expected(err_tag, E&& error) : has_error_(true), error_(std::move(error)) {}
    
    bool has_error_;
    E error_;
};

// ============================================================================
// error_info - Rich error information
// ============================================================================

export class error_info {
public:
    error_info() = default;
    
    explicit error_info(std::string message)
        : message_(std::move(message)) {}
    
    error_info(std::string message, std::string context)
        : message_(std::move(message)), context_(std::move(context)) {}
    
    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }
    
    [[nodiscard]] const std::string& context() const noexcept {
        return context_;
    }
    
    void set_message(std::string message) {
        message_ = std::move(message);
    }
    
    void set_context(std::string context) {
        context_ = std::move(context);
    }
    
    void add_context(std::string_view additional_context) {
        if (!context_.empty()) {
            context_ += " -> ";
        }
        context_ += additional_context;
    }
    
    [[nodiscard]] std::string to_string() const {
        if (context_.empty()) {
            return message_;
        }
        return context_ + ": " + message_;
    }

private:
    std::string message_;
    std::string context_;
};

// ============================================================================
// Utility functions
// ============================================================================

// Create ok result
export template<typename T>
auto ok(T&& value) {
    using value_type = std::decay_t<T>;
    return expected<value_type, error_info>::ok(std::forward<T>(value));
}

export inline auto ok() {
    return expected<void, error_info>::ok();
}

// Create error result
export template<typename E>
auto err(E&& error) {
    using error_type = std::decay_t<E>;
    return expected<void, error_type>::err(std::forward<E>(error));
}

// Create error from string
export inline auto err(std::string_view message) {
    return expected<void, error_info>::err(error_info(std::string(message)));
}

// Exception wrapper
export class exception_wrapper : public std::exception {
public:
    explicit exception_wrapper(error_info info)
        : info_(std::move(info)) {}
    
    [[nodiscard]] const char* what() const noexcept override {
        return info_.message().c_str();
    }
    
    [[nodiscard]] const error_info& info() const noexcept {
        return info_;
    }

private:
    error_info info_;
};

// Convert expected to exception
export template<typename T, typename E>
T unwrap_or_throw(expected<T, E>&& result) {
    if (result.is_ok()) {
        return std::move(result).value();
    }
    throw std::runtime_error("Unwrap failed");
}

export template<typename E>
void unwrap_or_throw(expected<void, E>&& result) {
    if (result.is_err()) {
        throw std::runtime_error("Unwrap failed");
    }
}

// Specialized for error_info
export template<typename T>
T unwrap_or_throw(expected<T, error_info>&& result) {
    if (result.is_ok()) {
        return std::move(result).value();
    }
    throw exception_wrapper(std::move(result).error());
}

export inline void unwrap_or_throw(expected<void, error_info>&& result) {
    if (result.is_err()) {
        throw exception_wrapper(std::move(result).error());
    }
}

// Try catch to expected
export template<typename F>
auto try_catch(F&& f) -> expected<std::invoke_result_t<F>, error_info> {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
            f();
            return expected<void, error_info>::ok();
        } else {
            return expected<std::invoke_result_t<F>, error_info>::ok(f());
        }
    } catch (const exception_wrapper& e) {
        return expected<std::invoke_result_t<F>, error_info>::err(e.info());
    } catch (const std::exception& e) {
        return expected<std::invoke_result_t<F>, error_info>::err(
            error_info(e.what())
        );
    } catch (...) {
        return expected<std::invoke_result_t<F>, error_info>::err(
            error_info("Unknown exception")
        );
    }
}

} // namespace cppx

