// cppx.math_utils - Mathematical utilities
// Provides statistical functions, interpolation, and random number utilities

export module cppx.math_utils;

import std;
import cppx.error;

namespace cppx {

// ============================================================================
// Statistical functions
// ============================================================================

// Calculate mean (average)
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
double mean(const std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot calculate mean of empty vector");
  }

  double sum = std::accumulate(values.begin(), values.end(), 0.0);
  return sum / values.size();
}

// Calculate median
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
double median(std::vector<T> values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot calculate median of empty vector");
  }

  std::sort(values.begin(), values.end());
  size_t n = values.size();

  if (n % 2 == 0) {
    return (values[n / 2 - 1] + values[n / 2]) / 2.0;
  } else {
    return values[n / 2];
  }
}

// Calculate variance
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
double variance(const std::vector<T>& values, bool sample = true) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot calculate variance of empty vector");
  }

  if (values.size() == 1 && sample) {
    throw std::invalid_argument("Cannot calculate sample variance with single value");
  }

  double m = mean(values);
  double sum_sq_diff = 0.0;

  for (const auto& value : values) {
    double diff = value - m;
    sum_sq_diff += diff * diff;
  }

  size_t divisor = sample ? (values.size() - 1) : values.size();
  return sum_sq_diff / divisor;
}

// Calculate standard deviation
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
double std_dev(const std::vector<T>& values, bool sample = true) {
  return std::sqrt(variance(values, sample));
}

// Calculate minimum
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T min_value(const std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot find min of empty vector");
  }
  return *std::min_element(values.begin(), values.end());
}

// Calculate maximum
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T max_value(const std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot find max of empty vector");
  }
  return *std::max_element(values.begin(), values.end());
}

// Calculate sum
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T sum(const std::vector<T>& values) {
  return std::accumulate(values.begin(), values.end(), T{0});
}

// Calculate product
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T product(const std::vector<T>& values) {
  if (values.empty()) {
    return T{0};
  }
  return std::accumulate(values.begin(), values.end(), T{1}, std::multiplies<T>());
}

// Calculate range (max - min)
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T range(const std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot find range of empty vector");
  }
  return max_value(values) - min_value(values);
}

// ============================================================================
// Clamping and normalization
// ============================================================================

// Clamp value between min and max
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr T clamp(T value, T min, T max) {
  return std::clamp(value, min, max);
}

// Normalize value from [in_min, in_max] to [out_min, out_max]
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr double normalize(T value, T in_min, T in_max, double out_min = 0.0,
                           double out_max = 1.0) {
  if (in_min == in_max) {
    return out_min;
  }

  double normalized = static_cast<double>(value - in_min) / (in_max - in_min);
  return out_min + normalized * (out_max - out_min);
}

// Map value from [0, 1] to [min, max]
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr T denormalize(double normalized, T min, T max) {
  return static_cast<T>(min + normalized * (max - min));
}

// ============================================================================
// Interpolation
// ============================================================================

// Linear interpolation
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr double lerp(T a, T b, double t) {
  return a + t * (b - a);
}

// Inverse linear interpolation (get t from value)
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr double inverse_lerp(T a, T b, T value) {
  if (a == b) {
    return 0.0;
  }
  return static_cast<double>(value - a) / (b - a);
}

// Smooth interpolation (smoothstep)
export constexpr double smoothstep(double t) {
  t = clamp(t, 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

// Smoother interpolation (smootherstep)
export constexpr double smootherstep(double t) {
  t = clamp(t, 0.0, 1.0);
  return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// ============================================================================
// Random number generation
// ============================================================================

// Thread-local random engine
inline std::mt19937& get_random_engine() {
  thread_local std::random_device rd;
  thread_local std::mt19937 engine(rd());
  return engine;
}

// Generate random integer in range [min, max]
export template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
T random_int(T min, T max) {
  std::uniform_int_distribution<T> dist(min, max);
  return dist(get_random_engine());
}

// Generate random real number in range [min, max)
export template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
T random_real(T min, T max) {
  std::uniform_real_distribution<T> dist(min, max);
  return dist(get_random_engine());
}

// Generate random double in [0, 1)
export double random() { return random_real(0.0, 1.0); }

// Generate random boolean with given probability of true
export bool random_bool(double probability = 0.5) {
  std::bernoulli_distribution dist(probability);
  return dist(get_random_engine());
}

// Choose random element from vector
export template <typename T>
const T& random_choice(const std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot choose from empty vector");
  }

  size_t index = random_int<size_t>(0, values.size() - 1);
  return values[index];
}

export template <typename T>
T& random_choice(std::vector<T>& values) {
  if (values.empty()) {
    throw std::invalid_argument("Cannot choose from empty vector");
  }

  size_t index = random_int<size_t>(0, values.size() - 1);
  return values[index];
}

// Shuffle vector
export template <typename T>
void shuffle(std::vector<T>& values) {
  std::shuffle(values.begin(), values.end(), get_random_engine());
}

// Generate random sample from vector
export template <typename T>
std::vector<T> random_sample(const std::vector<T>& values, size_t count) {
  if (count > values.size()) {
    throw std::invalid_argument("Sample size cannot exceed vector size");
  }

  std::vector<T> sample = values;
  shuffle(sample);
  sample.resize(count);
  return sample;
}

// Generate normal distribution random number
export double random_normal(double mean = 0.0, double stddev = 1.0) {
  std::normal_distribution<double> dist(mean, stddev);
  return dist(get_random_engine());
}

// Set random seed
export void set_random_seed(unsigned int seed) { get_random_engine().seed(seed); }

// ============================================================================
// Range generation (Python-like)
// ============================================================================

// Generate range of integers [start, end)
export std::vector<int> range(int start, int end, int step = 1) {
  if (step == 0) {
    throw std::invalid_argument("Step cannot be zero");
  }

  std::vector<int> result;

  if (step > 0) {
    for (int i = start; i < end; i += step) {
      result.push_back(i);
    }
  } else {
    for (int i = start; i > end; i += step) {
      result.push_back(i);
    }
  }

  return result;
}

// Generate range [0, end)
export std::vector<int> range(int end) { return range(0, end, 1); }

// ============================================================================
// Common math operations
// ============================================================================

// Check if value is approximately equal (within epsilon)
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr bool approx_equal(T a, T b, T epsilon = T{0.0001}) {
  return std::abs(a - b) < epsilon;
}

// Sign function (-1, 0, or 1)
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr int sign(T value) {
  return (T{0} < value) - (value < T{0});
}

// Absolute value
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr T abs(T value) {
  return std::abs(value);
}

// Power function
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr double pow(T base, T exponent) {
  return std::pow(base, exponent);
}

// Square root
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr double sqrt(T value) {
  return std::sqrt(value);
}

// Round to n decimal places
export double round_to(double value, int decimal_places) {
  double multiplier = std::pow(10.0, decimal_places);
  return std::round(value * multiplier) / multiplier;
}

// ============================================================================
// Vector operations
// ============================================================================

// Element-wise addition
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<T> add(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match");
  }

  std::vector<T> result(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

// Element-wise subtraction
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<T> subtract(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match");
  }

  std::vector<T> result(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    result[i] = a[i] - b[i];
  }
  return result;
}

// Element-wise multiplication
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<T> multiply(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match");
  }

  std::vector<T> result(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    result[i] = a[i] * b[i];
  }
  return result;
}

// Scalar multiplication
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<T> scale(const std::vector<T>& vec, T scalar) {
  std::vector<T> result(vec.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    result[i] = vec[i] * scalar;
  }
  return result;
}

// Dot product
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T dot_product(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match");
  }

  T result = T{0};
  for (size_t i = 0; i < a.size(); ++i) {
    result += a[i] * b[i];
  }
  return result;
}

// Vector magnitude
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
double magnitude(const std::vector<T>& vec) {
  T sum_sq = T{0};
  for (const auto& value : vec) {
    sum_sq += value * value;
  }
  return std::sqrt(sum_sq);
}

// Normalize vector to unit length
export template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<double> normalize_vector(const std::vector<T>& vec) {
  double mag = magnitude(vec);
  if (mag == 0.0) {
    throw std::invalid_argument("Cannot normalize zero vector");
  }

  std::vector<double> result(vec.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    result[i] = vec[i] / mag;
  }
  return result;
}

// ============================================================================
// Angle conversions
// ============================================================================

export constexpr double PI = 3.14159265358979323846;

// Degrees to radians
export constexpr double deg_to_rad(double degrees) { return degrees * PI / 180.0; }

// Radians to degrees
export constexpr double rad_to_deg(double radians) { return radians * 180.0 / PI; }

}  // namespace cppx
