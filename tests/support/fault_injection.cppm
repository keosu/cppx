// Only compiled by cppx_fault_injection; never distributed in installed libraries.
export module cppx.fault_injection;
import std;

export namespace cppx::testing {
enum class fault {
  thread_create,
  file_read,
  file_write,
  file_flush,
  file_close,
  replace,
  log_write,
  log_flush
};
inline thread_local std::optional<fault> armed;
inline thread_local size_t remaining = 0;
inline void fail_after(fault point, size_t successful_operations = 0) {
  armed = point;
  remaining = successful_operations;
}
inline void clear_fault() { armed.reset(); }
inline bool hit(fault point) {
  if (armed != point) return false;
  if (remaining > 0) {
    --remaining;
    return false;
  }
  armed.reset();
  return true;
}
inline void thread_creation() {
  if (hit(fault::thread_create))
    throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
}
}  // namespace cppx::testing
