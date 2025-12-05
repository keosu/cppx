// cppx.cli module demo

import std;
import cppx.cli;

import cppx.logging;

using namespace cppx;

int main(int argc, char** argv) {
  auto logger = get_logger("processor");
  logger->info("=== cppx.cli Demo ===\n");

  // Create argument parser
  argument_parser parser("cli_demo", "Demo of command-line argument parsing");

  // Add positional arguments
  parser.add_positional("input", "Input file path");

  // Add options
  parser.add_option("output", "o", "Output file path", false, "output.txt");
  parser.add_option("format", "f", "Output format", false, "json");
  parser.add_option("threads", "t", "Number of threads", false, "4");

  // Add flags
  parser.add_flag("verbose", "v", "Enable verbose output");
  parser.add_flag("debug", "d", "Enable debug mode");
  parser.add_flag("quiet", "q", "Suppress output");

  // Set choices for format
  parser.set_choices("format", {"json", "xml", "csv"});

  logger->info("Command-line parser created with:");
  logger->info("  - Positional: input");
  logger->info("  - Options: output (-o), format (-f), threads (-t)");
  logger->info("  - Flags: verbose (-v), debug (-d), quiet (-q)\n");

  // If no arguments provided, show examples
  if (argc == 1) {
    logger->info("No arguments provided. Here are some examples:\n");

    logger->info("Example 1: Basic usage");
    logger->info("  $ ./cli_demo input.txt\n");

    logger->info("Example 2: With options");
    logger->info("  $ ./cli_demo input.txt --output result.txt --format xml\n");

    logger->info("Example 3: With short options");
    logger->info("  $ ./cli_demo input.txt -o result.txt -f csv\n");

    logger->info("Example 4: With flags");
    logger->info("  $ ./cli_demo input.txt --verbose --debug\n");

    logger->info("Example 5: Show help");
    logger->info("  $ ./cli_demo --help\n");

    logger->info("Parsing test arguments: input.txt -o output.json --verbose\n");

    // Demonstrate parsing with test arguments
    std::vector<std::string> test_args = {"input.txt", "-o", "output.json", "--verbose"};

    auto result = parser.parse(test_args);

    if (result.is_ok()) {
      logger->info("✓ Parsing successful!\n");

      auto& parsed = result.value();

      logger->info("Parsed values:");
      logger->info("  input: {}", parsed.get_string("input").value_or("N/A"));
      logger->info("  output: {}", parsed.get_string("output").value_or("N/A"));
      logger->info("  format: {}", parsed.get_string("format").value_or("N/A"));
      logger->info("  threads: {}", parsed.get_string("threads").value_or("N/A"));
      logger->info("  verbose: {}", (parsed.get_flag("verbose") ? "true" : "false"));
      logger->info("  debug: {}", (parsed.get_flag("debug") ? "true" : "false"));
      logger->info("  quiet: {}", (parsed.get_flag("quiet") ? "true" : "false"));

      // Type conversion example
      auto threads_result = parsed.get_int("threads");
      if (threads_result.is_ok()) {
        logger->info("Threads as int: {}", threads_result.value());
      }
    } else {
      logger->info("✗ Parsing failed: {}", result.error().message());
    }
  } else {
    // Parse actual command-line arguments
    auto result = parser.parse(argc, argv);

    if (result.is_ok()) {
      logger->info("✓ Arguments parsed successfully!\n");

      auto& parsed = result.value();

      logger->info("Configuration:");
      logger->info("  Input file: {}", parsed.get_string("input").value_or("N/A"));
      logger->info("  Output file: {}", parsed.get_string("output").value_or("N/A"));
      logger->info("  Format: {}", parsed.get_string("format").value_or("N/A"));

      auto threads_result = parsed.get_int("threads");
      if (threads_result.is_ok()) {
        logger->info("  Threads: {}", threads_result.value());
      }

      logger->info("\nFlags:");
      logger->info("  Verbose: {}", (parsed.get_flag("verbose") ? "enabled" : "disabled"));
      logger->info("  Debug: {}", (parsed.get_flag("debug") ? "enabled" : "disabled"));
      logger->info("  Quiet: {}", (parsed.get_flag("quiet") ? "enabled" : "disabled"));

      logger->info("\n[Application would process the file here]");
    } else {
      logger->info("✗ Error: {}", result.error().message());
      logger->info("\nUse --help to see usage information");
      return 1;
    }
  }

  logger->info("\n=== Demo completed ===");
  return 0;
}
