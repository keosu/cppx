// Test for cppx.math_utils module

import std;
import cppx.math_utils;
import cppx.logging;

#define assert(x)                                                                               \
  if (!(x)) {                                                                                   \
    std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    std::abort();                                                                               \
  }

using namespace cppx;

void test_statistics() {
  std::vector<int> values = {1, 2, 3, 4, 5};

  assert(mean(values) == 3.0);
  assert(median(values) == 3.0);
  assert(min_value(values) == 1);
  assert(max_value(values) == 5);
  assert(sum(values) == 15);

  auto logger = get_logger("test_math_utils");
  logger->info("✓ statistics test passed");
}

void test_clamping() {
  assert(clamp(5, 0, 10) == 5);
  assert(clamp(-5, 0, 10) == 0);
  assert(clamp(15, 0, 10) == 10);
  auto logger = get_logger("test_math_utils");
  logger->info("✓ clamping test passed");
}

void test_interpolation() {
  assert(lerp(0.0, 10.0, 0.5) == 5.0);
  assert(lerp(0.0, 10.0, 0.0) == 0.0);
  assert(lerp(0.0, 10.0, 1.0) == 10.0);
  auto logger = get_logger("test_math_utils");
  logger->info("✓ interpolation test passed");
}

void test_random() {
  auto val = random_int(1, 10);
  assert(val >= 1 && val <= 10);

  auto real = random_real(0.0, 1.0);
  assert(real >= 0.0 && real < 1.0);

  auto logger = get_logger("test_math_utils");
  logger->info("✓ random test passed");
}

void test_range() {
  auto r = range(5);
  assert(r.size() == 5);
  assert(r[0] == 0);
  assert(r[4] == 4);

  auto r2 = range(2, 6);
  assert(r2.size() == 4);
  assert(r2[0] == 2);
  assert(r2[3] == 5);

  auto logger = get_logger("test_math_utils");
  logger->info("✓ range test passed");
}

void test_vector_operations() {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {4, 5, 6};

  auto sum = add(a, b);
  assert(sum[0] == 5 && sum[1] == 7 && sum[2] == 9);

  auto scaled = scale(a, 2);
  assert(scaled[0] == 2 && scaled[1] == 4 && scaled[2] == 6);

  auto dot = dot_product(a, b);
  assert(dot == 32);  // 1*4 + 2*5 + 3*6

  auto logger = get_logger("test_math_utils");
  logger->info("✓ vector operations test passed");
}

void test_angles() {
  auto rad = deg_to_rad(180.0);
  assert(std::abs(rad - PI) < 0.001);

  auto deg = rad_to_deg(PI);
  assert(std::abs(deg - 180.0) < 0.001);

  auto logger = get_logger("test_math_utils");
  logger->info("✓ angle conversion test passed");
}

int main() {
  auto logger = get_logger("test_math_utils");
  logger->info("=== Math Utils Test Suite ===\n");

  test_statistics();
  test_clamping();
  test_interpolation();
  test_random();
  test_range();
  test_vector_operations();
  test_angles();

  logger->info("\n✓ All math_utils tests passed!");
  return 0;
}
