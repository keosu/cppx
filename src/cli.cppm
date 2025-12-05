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

enum class arg_type {
    positional,
    option,
    flag
};

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
    
    argument(std::string n, std::string short_n, std::string desc, 
            arg_type t, bool req = false)
        : name(std::move(n))
        , short_name(std::move(short_n))
        , description(std::move(desc))
        , type(t)
        , required(req)
    {}
};

// ============================================================================
// Parsed result
// ============================================================================

class parse_result {
public:
    void set_value(const std::string& name, const std::string& value) {
        values_[name] = value;
    }
    
    void set_flag(const std::string& name, bool value) {
        flags_[name] = value;
    }
    
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
            return expected<int, error_info>::err(
                error_info("Argument not found: " + name)
            );
        }
        return parse_int(*value);
    }
    
    [[nodiscard]] expected<long, error_info> get_long(const std::string& name) const {
        auto value = get_string(name);
        if (!value) {
            return expected<long, error_info>::err(
                error_info("Argument not found: " + name)
            );
        }
        return parse_long(*value);
    }
    
    [[nodiscard]] expected<double, error_info> get_double(const std::string& name) const {
        auto value = get_string(name);
        if (!value) {
            return expected<double, error_info>::err(
                error_info("Argument not found: " + name)
            );
        }
        return parse_double(*value);
    }
    
    [[nodiscard]] bool get_flag(const std::string& name) const {
        if (auto it = flags_.find(name); it != flags_.end()) {
            return it->second;
        }
        return false;
    }
    
    [[nodiscard]] const std::map<std::string, std::string>& values() const {
        return values_;
    }
    
    [[nodiscard]] const std::map<std::string, bool>& flags() const {
        return flags_;
    }

private:
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
        : program_name_(std::move(program_name))
        , description_(std::move(description))
    {}
    
    // Add positional argument
    argument_parser& add_positional(
        const std::string& name,
        const std::string& description,
        bool required = true
    ) {
        arguments_.emplace_back(name, "", description, arg_type::positional, required);
        return *this;
    }
    
    // Add option argument (e.g., --output file.txt)
    argument_parser& add_option(
        const std::string& name,
        const std::string& short_name,
        const std::string& description,
        bool required = false,
        const std::string& default_value = ""
    ) {
        auto& arg = arguments_.emplace_back(
            name, short_name, description, arg_type::option, required
        );
        arg.default_value = default_value;
        return *this;
    }
    
    // Add flag argument (e.g., --verbose, -v)
    argument_parser& add_flag(
        const std::string& name,
        const std::string& short_name,
        const std::string& description
    ) {
        arguments_.emplace_back(name, short_name, description, arg_type::flag, false);
        return *this;
    }
    
    // Set choices for an argument
    argument_parser& set_choices(
        const std::string& name,
        const std::vector<std::string>& choices
    ) {
        for (auto& arg : arguments_) {
            if (arg.name == name) {
                arg.choices = choices;
                break;
            }
        }
        return *this;
    }
    
    // Add subcommand
    std::shared_ptr<argument_parser> add_subcommand(
        const std::string& name,
        const std::string& description
    ) {
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
        parse_result result;
        size_t positional_index = 0;
        
        // Check for help flag
        if (std::find(args.begin(), args.end(), "--help") != args.end() ||
            std::find(args.begin(), args.end(), "-h") != args.end()) {
            print_help();
            std::exit(0);
        }
        
        // Check for subcommand
        if (!args.empty() && !subcommands_.empty()) {
            const auto& first_arg = args[0];
            if (!starts_with(first_arg, "-")) {
                if (auto it = subcommands_.find(first_arg); it != subcommands_.end()) {
                    std::vector<std::string> subargs(args.begin() + 1, args.end());
                    return it->second->parse(subargs);
                }
            }
        }
        
        // Parse arguments
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            
            // Long option or flag (--name)
            if (starts_with(arg, "--")) {
                std::string name = arg.substr(2);
                
                // Check if it's a flag
                bool is_flag = false;
                for (const auto& def : arguments_) {
                    if (def.type == arg_type::flag && def.name == name) {
                        result.set_flag(name, true);
                        is_flag = true;
                        break;
                    }
                }
                
                if (!is_flag) {
                    // It's an option, need a value
                    if (i + 1 >= args.size()) {
                        return expected<parse_result, error_info>::err(
                            error_info("Option --" + name + " requires a value")
                        );
                    }
                    
                    std::string value = args[++i];
                    
                    // Validate choices
                    if (auto* def = find_argument(name)) {
                        if (!def->choices.empty()) {
                            if (std::find(def->choices.begin(), def->choices.end(), value) 
                                == def->choices.end()) {
                                return expected<parse_result, error_info>::err(
                                    error_info("Invalid value for --" + name + 
                                             ": must be one of " + join(def->choices, ", "))
                                );
                            }
                        }
                    }
                    
                    result.set_value(name, value);
                }
            }
            // Short option or flag (-n)
            else if (starts_with(arg, "-") && arg.length() > 1 && arg[1] != '-') {
                std::string short_name = arg.substr(1);
                
                // Find argument by short name
                const argument* def = nullptr;
                for (const auto& a : arguments_) {
                    if (a.short_name == short_name) {
                        def = &a;
                        break;
                    }
                }
                
                if (!def) {
                    return expected<parse_result, error_info>::err(
                        error_info("Unknown option: -" + short_name)
                    );
                }
                
                if (def->type == arg_type::flag) {
                    result.set_flag(def->name, true);
                } else {
                    if (i + 1 >= args.size()) {
                        return expected<parse_result, error_info>::err(
                            error_info("Option -" + short_name + " requires a value")
                        );
                    }
                    result.set_value(def->name, args[++i]);
                }
            }
            // Positional argument
            else {
                // Find next positional argument definition
                const argument* def = nullptr;
                size_t pos_count = 0;
                for (const auto& a : arguments_) {
                    if (a.type == arg_type::positional) {
                        if (pos_count == positional_index) {
                            def = &a;
                            break;
                        }
                        ++pos_count;
                    }
                }
                
                if (!def) {
                    return expected<parse_result, error_info>::err(
                        error_info("Unexpected positional argument: " + arg)
                    );
                }
                
                result.set_value(def->name, arg);
                ++positional_index;
            }
        }
        
        // Check required arguments and set defaults
        for (const auto& arg : arguments_) {
            if (arg.required && !result.has(arg.name)) {
                return expected<parse_result, error_info>::err(
                    error_info("Missing required argument: " + arg.name)
                );
            }
            
            if (!result.has(arg.name) && !arg.default_value.empty()) {
                result.set_value(arg.name, arg.default_value);
            }
            
            if (arg.type == arg_type::flag && !result.has(arg.name)) {
                result.set_flag(arg.name, false);
            }
        }
        
        return expected<parse_result, error_info>::ok(std::move(result));
    }
    
    // Print help message
    void print_help() const {
        std::cout << program_name_;
        if (!description_.empty()) {
            std::cout << " - " << description_;
        }
        std::cout << "\n\n";
        
        std::cout << "USAGE:\n";
        std::cout << "  " << program_name_;
        
        // Show positional arguments
        for (const auto& arg : arguments_) {
            if (arg.type == arg_type::positional) {
                if (arg.required) {
                    std::cout << " <" << arg.name << ">";
                } else {
                    std::cout << " [" << arg.name << "]";
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
            std::cout << " [OPTIONS]";
        }
        
        // Show subcommands
        if (!subcommands_.empty()) {
            std::cout << " <SUBCOMMAND>";
        }
        
        std::cout << "\n\n";
        
        // Positional arguments
        bool has_positional = false;
        for (const auto& arg : arguments_) {
            if (arg.type == arg_type::positional) {
                if (!has_positional) {
                    std::cout << "ARGUMENTS:\n";
                    has_positional = true;
                }
                std::cout << "  " << arg.name;
                if (arg.required) {
                    std::cout << " (required)";
                }
                std::cout << "\n";
                if (!arg.description.empty()) {
                    std::cout << "      " << arg.description << "\n";
                }
            }
        }
        if (has_positional) {
            std::cout << "\n";
        }
        
        // Options and flags
        if (has_options) {
            std::cout << "OPTIONS:\n";
            
            // Help option
            std::cout << "  -h, --help\n";
            std::cout << "      Show this help message\n";
            
            for (const auto& arg : arguments_) {
                if (arg.type != arg_type::positional) {
                    std::cout << "  ";
                    if (!arg.short_name.empty()) {
                        std::cout << "-" << arg.short_name << ", ";
                    }
                    std::cout << "--" << arg.name;
                    
                    if (arg.type == arg_type::option) {
                        std::cout << " <value>";
                    }
                    
                    if (arg.required) {
                        std::cout << " (required)";
                    }
                    
                    std::cout << "\n";
                    
                    if (!arg.description.empty()) {
                        std::cout << "      " << arg.description << "\n";
                    }
                    
                    if (!arg.default_value.empty()) {
                        std::cout << "      [default: " << arg.default_value << "]\n";
                    }
                    
                    if (!arg.choices.empty()) {
                        std::cout << "      [choices: " << join(arg.choices, ", ") << "]\n";
                    }
                }
            }
            std::cout << "\n";
        }
        
        // Subcommands
        if (!subcommands_.empty()) {
            std::cout << "SUBCOMMANDS:\n";
            for (const auto& [name, parser] : subcommands_) {
                std::cout << "  " << name;
                if (!parser->description_.empty()) {
                    std::cout << " - " << parser->description_;
                }
                std::cout << "\n";
            }
            std::cout << "\n";
            std::cout << "Use '" << program_name_ 
                     << " <SUBCOMMAND> --help' for more information\n";
        }
    }
    
    [[nodiscard]] const std::string& program_name() const {
        return program_name_;
    }
    
    [[nodiscard]] const std::string& description() const {
        return description_;
    }

private:
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

} // namespace cppx

