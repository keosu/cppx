// cppx.math_utils module demo

import std;
import cppx.math_utils;

import cppx.logging;

using namespace cppx;

int main() {
  auto logger = get_logger("math_utils_demo");
  std::cout << std::fixed << std::setprecision(2);
  logger->info("=== cppx.math_utils Demo ===\n");

  // Example 1: Statistical functions
  logger->info("1. Statistical functions:");
  std::vector<int> data = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

  logger->info("   Data: ");
  for (auto v : data) std::cout << v << " ";
  logger->info("\n");

  logger->info("   Mean: {}", mean(data));
  logger->info("   Median: {}", median(data));
  logger->info("   Std Dev: {}", std_dev(data));
  logger->info("   Min: {}", min_value(data));
  logger->info("   Max: {}", max_value(data));
  logger->info("   Range: {}", range(data));
  logger->info("   Sum: {}", sum(data));

  // Example 2: Clamping and normalization
  logger->info("2. Clamping and normalization:");
  logger->info("   clamp(15, 0, 10) = {}", clamp(15, 0, 10));
  logger->info("   clamp(-5, 0, 10) = {}", clamp(-5, 0, 10));
  logger->info("   normalize(5, 0, 10) = {}", normalize(5, 0, 10));
  logger->info("   normalize(75, 0, 100) = {}", normalize(75, 0, 100));

  // Example 3: Interpolation
  logger->info("3. Interpolation:");
  logger->info("   lerp(0, 100, 0.25) = {}", lerp(0.0, 100.0, 0.25));
  logger->info("   lerp(0, 100, 0.50) = {}", lerp(0.0, 100.0, 0.50));
  logger->info("   lerp(0, 100, 0.75) = {}", lerp(0.0, 100.0, 0.75));
  logger->info("   smoothstep(0.5) = {}", smoothstep(0.5));

  // Example 4: Random numbers
  logger->info("4. Random numbers:");
  logger->info("   Random int [1-10]: {}", random_int(1, 10));
  logger->info("   Random int [1-10]: {}", random_int(1, 10));
  logger->info("   Random real [0-1]: {}", random_real(0.0, 1.0));
  logger->info("   Random bool: {}", (random_bool() ? "true" : "false"));

  // Example 5: Random choice and shuffle
  logger->info("5. Random choice and shuffle:");
  std::vector<std::string> items = {"apple", "banana", "cherry", "date", "elderberry"};
  logger->info("   Items: ");
  for (const auto& item : items) std::cout << item << " ";
  logger->info("\n");

  logger->info("   Random choice: {}", random_choice(items));
  logger->info("   Random choice: {}", random_choice(items));

  auto sample = random_sample(items, 3);
  logger->info("   Random sample (3): ");
  for (const auto& item : sample) std::cout << item << " ";
  logger->info("\n");

  shuffle(items);
  logger->info("   After shuffle: ");
  for (const auto& item : items) std::cout << item << " ";
  logger->info("\n");

  // Example 6: Range generation
  logger->info("6. Range generation:");
  auto r1 = range(10);
  logger->info("   range(10): ");
  for (auto v : r1) std::cout << v << " ";
  logger->info("\n");

  auto r2 = range(5, 15);
  logger->info("   range(5, 15): ");
  for (auto v : r2) std::cout << v << " ";
  logger->info("\n");

  auto r3 = range(0, 20, 3);
  logger->info("   range(0, 20, 3): ");
  for (auto v : r3) std::cout << v << " ";
  logger->info("\n");

  // Example 7: Vector operations
  logger->info("7. Vector operations:");
  std::vector<int> v1 = {1, 2, 3, 4};
  std::vector<int> v2 = {5, 6, 7, 8};

  logger->info("   v1: ");
  for (auto v : v1) std::cout << v << " ";
  logger->info("\n");

  logger->info("   v2: ");
  for (auto v : v2) std::cout << v << " ";
  logger->info("\n");

  auto v_add = add(v1, v2);
  logger->info("   v1 + v2: ");
  for (auto v : v_add) std::cout << v << " ";
  logger->info("\n");

  auto v_scaled = scale(v1, 3);
  logger->info("   v1 * 3: ");
  for (auto v : v_scaled) std::cout << v << " ";
  logger->info("\n");

  auto dot = dot_product(v1, v2);
  logger->info("   dot(v1, v2): {}", dot);

  auto mag = magnitude(v1);
  logger->info("   magnitude(v1): {}", mag);

  // Example 8: Angle conversions
  logger->info("8. Angle conversions:");
  logger->info("   180° = {} rad (≈π)", deg_to_rad(180.0));
  logger->info("   π rad = {}°", rad_to_deg(PI));
  logger->info("   90° = {} rad (≈π/2)", deg_to_rad(90.0));

  logger->info("\n=== Demo completed ===");
  return 0;
}
