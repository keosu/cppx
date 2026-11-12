// cppx.cli - Command-line argument parsing
// Provides argument parser with subcommands and auto-generated help

export module cppx.cli;

import std;
import cppx.error;
import cppx.string_utils;

namespace cppx {

// ============================================================================
// Argument types
// ============================================================================

enum class arg_type { positional, option, flag };

// ============================================================================
// Argument definition
// ============================================================================

struct argument {
  std::string name;
  std::string short_name;
  std::string description;
  arg_type type;
  bool required;
  std::string default_value;
  std::vector<std::string> choices;

  argument(std::string n, std::string short_n, std::string desc, arg_type t, bool req = false)
      : name(std::move(n)),
        short_name(std::move(short_n)),
        description(std::move(desc)),
        type(t),
        required(req) {}
};

// ============================================================================
// Parsed result
// ============================================================================

export class parse_result {
 public:
  void set_value(const std::string& name, const std::string& value) { values_[name] = value; }

  void set_flag(const std::string& name, bool value) { flags_[name] = value; }

  [[nodiscard]] bool has(const std::string& name) const {
    return values_.contains(name) || flags_.contains(name);
  }

  [[nodiscard]] std::optional<std::string> get_string(const std::string& name) const {
    if (auto it = values_.find(name); it != values_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] std::string get_string_or(const std::string& name,
                                          const std::string& default_val) const {
    return get_string(name).value_or(default_val);
  }

  [[nodiscard]] expected<int, error_info> get_int(const std::string& name) const {
    auto value = get_string(name);
    if (!value) {
      return expected<int, error_info>::err(error_info("Argument not found: " + name));
    }
    return parse_int(*value);
  }

  [[nodiscard]] expected<long, error_info> get_long(const std::string& name) const {
    auto value = get_string(name);
    if (!value) {
      return expected<long, error_info>::err(error_info("Argument not found: " + name));
    }
    return parse_long(*value);
  }

  [[nodiscard]] expected<double, error_info> get_double(const std::string& name) const {
    auto value = get_string(name);
    if (!value) {
      return expected<double, error_info>::err(error_info("Argument not found: " + name));
    }
    return parse_double(*value);
  }

  [[nodiscard]] bool get_flag(const std::string& name) const {
    if (auto it = flags_.find(name); it != flags_.end()) {
      return it->second;
    }
    return false;
  }

  [[nodiscard]] const std::map<std::string, std::string>& values() const { return values_; }

  [[nodiscard]] const std::map<std::string, bool>& flags() const { return flags_; }

  [[nodiscard]] bool help_requested() const noexcept { return help_requested_; }
  [[nodiscard]] const std::string& help_text() const noexcept { return help_text_; }
  void request_help(std::string text) {
    help_requested_ = true;
    help_text_ = std::move(text);
  }

 private:
  bool help_requested_ = false;
  std::string help_text_;
  std::map<std::string, std::string> values_;
  std::map<std::string, bool> flags_;
};

// ============================================================================
// Subcommand
// ============================================================================

export class subcommand;

export class argument_parser {
 public:
  explicit argument_parser(std::string program_name, std::string description = "")
      : program_name_(std::move(program_name)), description_(std::move(description)) {}

  // Add positional argument
  argument_parser& add_positional(const std::string& name, const std::string& description,
                                  bool required = true) {
    validate_argument(name, "");
    arguments_.emplace_back(name, "", description, arg_type::positional, required);
    return *this;
  }

  // Add option argument (e.g., --output file.txt)
  argument_parser& add_option(const std::string& name, const std::string& short_name,
                              const std::string& description, bool required = false,
                              const std::string& default_value = "") {
    validate_argument(name, short_name);
    auto& arg = arguments_.emplace_back(name, short_name, description, arg_type::option, required);
    arg.default_value = default_value;
    return *this;
  }

  // Add flag argument (e.g., --verbose, -v)
  argument_parser& add_flag(const std::string& name, const std::string& short_name,
                            const std::string& description) {
    validate_argument(name, short_name);
    arguments_.emplace_back(name, short_name, description, arg_type::flag, false);
    return *this;
  }

  // Set choices for an argument
  argument_parser& set_choices(const std::string& name, const std::vector<std::string>& choices) {
    for (auto& arg : arguments_) {
      if (arg.name == name) {
        arg.choices = choices;
        return *this;
      }
    }
    throw std::invalid_argument("Unknown argument for choices: " + name);
  }

  // Add subcommand
  std::shared_ptr<argument_parser> add_subcommand(const std::string& name,
                                                  const std::string& description) {
    auto subcmd = std::make_shared<argument_parser>(name, description);
    subcommands_[name] = subcmd;
    return subcmd;
  }

  // Parse command-line arguments
  expected<parse_result, error_info> parse(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }
    return parse(args);
  }

  expected<parse_result, error_info> parse(const std::vector<std::string>& args) {
    using R = expected<parse_result, error_info>;
    parse_result result;
    size_t positional_index = 0;
    bool options = true;
    if (!args.empty()) {
      if (auto it = subcommands_.find(args.front()); it != subcommands_.end())
        return it->second->parse(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    for (size_t i = 0; i < args.size(); ++i) {
      const auto& token = args[i];
      if (options && token == "--") {
        options = false;
        continue;
      }
      if (options && (token == "--help" || token == "-h")) {
        result.request_help(help_text());
        return R::ok(std::move(result));
      }
      if (options && token.size() > 1 && token.front() == '-') {
        bool long_option = token.starts_with("--");
        auto equals = long_option ? token.find('=') : std::string::npos;
        auto name =
            token.substr(long_option ? 2 : 1, equals == std::string::npos ? equals : equals - 2);
        const argument* def = nullptr;
        for (const auto& candidate : arguments_) {
          if (candidate.type != arg_type::positional &&
              (long_option ? candidate.name == name : candidate.short_name == name)) {
            def = &candidate;
            break;
          }
        }
        if (!def) return R::err(error_info("Unknown option: " + token));
        if (def->type == arg_type::flag) {
          if (equals != std::string::npos)
            return R::err(error_info("Flag does not take a value: " + token));
          result.set_flag(def->name, true);
        } else {
          if (equals != std::string::npos)
            result.set_value(def->name, token.substr(equals + 1));
          else {
            if (i + 1 == args.size() || args[i + 1].starts_with("--") || args[i + 1] == "-h")
              return R::err(error_info("Option requires a value: " + token));
            for (const auto& candidate : arguments_) {
              if (!candidate.short_name.empty() && args[i + 1] == "-" + candidate.short_name)
                return R::err(error_info("Option requires a value: " + token));
            }
            result.set_value(def->name, args[++i]);
          }
        }
      } else {
        const argument* def = nullptr;
        size_t index = 0;
        for (const auto& candidate : arguments_) {
          if (candidate.type == arg_type::positional && index++ == positional_index) {
            def = &candidate;
            break;
          }
        }
        if (!def) return R::err(error_info("Unexpected positional argument: " + token));
        result.set_value(def->name, token);
        ++positional_index;
      }
    }
    for (const auto& arg : arguments_) {
      if (!result.has(arg.name) && !arg.default_value.empty())
        result.set_value(arg.name, arg.default_value);
      if (arg.required && !result.has(arg.name))
        return R::err(error_info("Missing required argument: " + arg.name));
      if (auto value = result.get_string(arg.name);
          value && !arg.choices.empty() &&
          std::find(arg.choices.begin(), arg.choices.end(), *value) == arg.choices.end())
        return R::err(error_info("Invalid value for " + arg.name + ": must be one of " +
                                 join(arg.choices, ", ")));
      if (arg.type == arg_type::flag && !result.has(arg.name)) result.set_flag(arg.name, false);
    }
    return R::ok(std::move(result));
  }

  [[nodiscard]] std::string help_text() const {
    std::ostringstream out;
    print_help(out);
    return out.str();
  }

  // Print help message
  void print_help(std::ostream& out = std::cout) const {
    out << program_name_;
    if (!description_.empty()) {
      out << " - " << description_;
    }
    out << "\n\n";

    out << "USAGE:\n";
    out << "  " << program_name_;

    // Show positional arguments
    for (const auto& arg : arguments_) {
      if (arg.type == arg_type::positional) {
        if (arg.required) {
          out << " <" << arg.name << ">";
        } else {
          out << " [" << arg.name << "]";
        }
      }
    }

    // Show options
    bool has_options = false;
    for (const auto& arg : arguments_) {
      if (arg.type != arg_type::positional) {
        has_options = true;
        break;
      }
    }
    if (has_options) {
      out << " [OPTIONS]";
    }

    // Show subcommands
    if (!subcommands_.empty()) {
      out << " <SUBCOMMAND>";
    }

    out << "\n\n";

    // Positional arguments
    bool has_positional = false;
    for (const auto& arg : arguments_) {
      if (arg.type == arg_type::positional) {
        if (!has_positional) {
          out << "ARGUMENTS:\n";
          has_positional = true;
        }
        out << "  " << arg.name;
        if (arg.required) {
          out << " (required)";
        }
        out << "\n";
        if (!arg.description.empty()) {
          out << "      " << arg.description << "\n";
        }
      }
    }
    if (has_positional) {
      out << "\n";
    }

    // Options and flags
    {
      out << "OPTIONS:\n";

      // Help option
      out << "  -h, --help\n";
      out << "      Show this help message\n";

      for (const auto& arg : arguments_) {
        if (arg.type != arg_type::positional) {
          out << "  ";
          if (!arg.short_name.empty()) {
            out << "-" << arg.short_name << ", ";
          }
          out << "--" << arg.name;

          if (arg.type == arg_type::option) {
            out << " <value>";
          }

          if (arg.required) {
            out << " (required)";
          }

          out << "\n";

          if (!arg.description.empty()) {
            out << "      " << arg.description << "\n";
          }

          if (!arg.default_value.empty()) {
            out << "      [default: " << arg.default_value << "]\n";
          }

          if (!arg.choices.empty()) {
            out << "      [choices: " << join(arg.choices, ", ") << "]\n";
          }
        }
      }
      out << "\n";
    }

    // Subcommands
    if (!subcommands_.empty()) {
      out << "SUBCOMMANDS:\n";
      for (const auto& [name, parser] : subcommands_) {
        out << "  " << name;
        if (!parser->description_.empty()) {
          out << " - " << parser->description_;
        }
        out << "\n";
      }
      out << "\n";
      out << "Use '" << program_name_ << " <SUBCOMMAND> --help' for more information\n";
    }
  }

  [[nodiscard]] const std::string& program_name() const { return program_name_; }

  [[nodiscard]] const std::string& description() const { return description_; }

 private:
  void validate_argument(const std::string& name, const std::string& short_name) const {
    if (name.empty() || name.front() == '-' || name.find_first_of("= ") != std::string::npos ||
        name == "help" || short_name == "h" || (!short_name.empty() && short_name.front() == '-'))
      throw std::invalid_argument("Invalid or reserved argument name");
    for (const auto& arg : arguments_)
      if (arg.name == name || (!short_name.empty() && arg.short_name == short_name))
        throw std::invalid_argument("Duplicate argument name or alias");
  }
  const argument* find_argument(const std::string& name) const {
    for (const auto& arg : arguments_) {
      if (arg.name == name) {
        return &arg;
      }
    }
    return nullptr;
  }

  std::string program_name_;
  std::string description_;
  std::vector<argument> arguments_;
  std::map<std::string, std::shared_ptr<argument_parser>> subcommands_;
};

}  // namespace cppx
