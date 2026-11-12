// cppx.concurrent - Concurrency utilities
// Provides thread pool, parallel execution, and channels

export module cppx.concurrent;

import std;
import cppx.error;
#ifdef CPPX_FAULT_INJECTION
import cppx.fault_injection;
#endif

namespace cppx {

// ============================================================================
// Thread pool
// ============================================================================

export class thread_pool {
 public:
  explicit thread_pool(size_t num_threads = std::thread::hardware_concurrency(),
                       size_t capacity = 0)
      : capacity_(capacity) {
    if (num_threads == 0) num_threads = 1;
    workers_.reserve(num_threads);
    try {
      for (size_t i = 0; i < num_threads; ++i) {
#ifdef CPPX_FAULT_INJECTION
        testing::thread_creation();
#endif
        workers_.emplace_back([this] { worker_thread(); });
      }
    } catch (...) {
      close();
      for (auto& worker : workers_) worker.join();
      throw;
    }
  }
  // Lifetime must be owned outside the pool's workers.
  ~thread_pool() { shutdown(); }
  thread_pool(const thread_pool&) = delete;
  thread_pool& operator=(const thread_pool&) = delete;
  thread_pool(thread_pool&&) = delete;
  thread_pool& operator=(thread_pool&&) = delete;

  template <typename F, typename... Args>
  auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    return submit_for(std::chrono::milliseconds::zero(), std::forward<F>(f),
                      std::forward<Args>(args)...);
  }
  // A full bounded queue rejects on timeout; capacity zero means unbounded.
  template <typename Rep, typename Period, typename F, typename... Args>
  auto submit_for(std::chrono::duration<Rep, Period> timeout, F&& f,
                  Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<R()>>(
        [fn = std::forward<F>(f),
         arguments = std::make_tuple(std::forward<Args>(args)...)]() mutable -> R {
          return std::apply(std::move(fn), std::move(arguments));
        });
    auto future = task->get_future();
    std::unique_lock lock(queue_mutex_);
    if (stop_) throw std::runtime_error("Cannot submit to a closed thread pool");
    // Reentrant submissions run inline, allowing a single worker to wait on a child future.
    if (active_pool_ == this) {
      lock.unlock();
      (*task)();
      return future;
    }
    if (!space_.wait_for(lock, timeout,
                         [&] { return stop_ || capacity_ == 0 || tasks_.size() < capacity_; }))
      throw std::runtime_error("Thread pool queue is full (submission timed out)");
    if (stop_) throw std::runtime_error("Cannot submit to a closed thread pool");
    tasks_.emplace([task] { (*task)(); });
    lock.unlock();
    condition_.notify_one();
    return future;
  }
  // Reject new work and drain previously accepted work. Callable from a worker.
  void close() {
    {
      std::lock_guard lock(queue_mutex_);
      stop_ = true;
    }
    condition_.notify_all();
    space_.notify_all();
  }
  void shutdown() {
    if (active_pool_ == this)
      throw std::logic_error("A pool worker cannot join its own pool; use close()");
    std::lock_guard lock(join_mutex_);
    close();
    for (auto& worker : workers_)
      if (worker.joinable()) worker.join();
  }
  [[nodiscard]] bool is_closed() const {
    std::lock_guard lock(queue_mutex_);
    return stop_;
  }
  [[nodiscard]] size_t size() const { return workers_.size(); }
  [[nodiscard]] size_t pending_tasks() const {
    std::lock_guard lock(queue_mutex_);
    return tasks_.size();
  }

 private:
  void worker_thread() {
    active_pool_ = this;
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock lock(queue_mutex_);
        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty()) break;
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      space_.notify_one();
      task();  // packaged_task stores user exceptions in the future
    }
    active_pool_ = nullptr;
  }
  inline static thread_local thread_pool* active_pool_ = nullptr;
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  mutable std::mutex queue_mutex_;
  std::mutex join_mutex_;
  std::condition_variable condition_;
  std::condition_variable space_;
  size_t capacity_;
  bool stop_ = false;
};

namespace concurrent_detail {
class parallel_group {
 public:
  ~parallel_group() { join(); }
  template <typename F>
  void run(F&& f) {
#ifdef CPPX_FAULT_INJECTION
    testing::thread_creation();
#endif
    workers_.emplace_back([this, fn = std::forward<F>(f)]() mutable {
      try {
        std::invoke(fn);
      } catch (...) {
        std::lock_guard lock(mutex_);
        if (!exception_) exception_ = std::current_exception();
      }
    });
  }
  void finish() {
    join();
    if (exception_) std::rethrow_exception(exception_);
  }

 private:
  void join() {
    for (auto& worker : workers_)
      if (worker.joinable()) worker.join();
  }
  // Destruction joins before any state captured by the workers is destroyed.
  std::vector<std::thread> workers_;
  std::mutex mutex_;
  std::exception_ptr exception_;
};
}  // namespace concurrent_detail

export template <std::integral IndexType, typename Function>
void parallel_for(IndexType start, IndexType end, Function&& func,
                  size_t num_threads = std::thread::hardware_concurrency()) {
  if (start >= end) return;
  using U = std::make_unsigned_t<IndexType>;
  U total = static_cast<U>(end) - static_cast<U>(start);
  size_t count =
      static_cast<size_t>(std::min<std::uintmax_t>(total, std::max<size_t>(1, num_threads)));
  U chunk = total / static_cast<U>(count);
  U extra = total % static_cast<U>(count);
  U offset = 0;
  concurrent_detail::parallel_group group;
  for (size_t t = 0; t < count; ++t) {
    U length = chunk + (t < extra ? 1 : 0);
    IndexType first = static_cast<IndexType>(static_cast<U>(start) + offset);
    group.run([first, length, &func] {
      IndexType current = first;
      for (U n = 0; n < length; ++n) {
        std::invoke(func, current);
        if (n + 1 < length) ++current;
      }
    });
    offset += length;
  }
  group.finish();
}

export template <typename Container, typename Function>
void parallel_for_each(Container& container, Function&& func,
                       size_t num_threads = std::thread::hardware_concurrency()) {
  if (container.empty()) return;
  size_t count = std::min(container.size(), std::max<size_t>(1, num_threads));
  size_t chunk = container.size() / count, extra = container.size() % count;
  auto it = container.begin();
  concurrent_detail::parallel_group group;
  for (size_t t = 0; t < count; ++t) {
    auto first = it;
    std::advance(it, chunk + (t < extra ? 1 : 0));
    group.run([first, last = it, &func] {
      for (auto current = first; current != last; ++current) std::invoke(func, *current);
    });
  }
  group.finish();
}

// ============================================================================
// Channel (thread-safe queue with blocking)
// ============================================================================

export template <typename T>
class channel {
 public:
  explicit channel(size_t capacity = 0) : capacity_(capacity), closed_(false) {}

  // Send value to channel (blocks if full)
  bool send(const T& value) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (closed_) {
      return false;
    }

    if (capacity_ > 0) {
      send_cv_.wait(lock, [this] { return queue_.size() < capacity_ || closed_; });

      if (closed_) {
        return false;
      }
    }

    queue_.push(value);
    recv_cv_.notify_one();
    return true;
  }

  bool send(T&& value) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (closed_) {
      return false;
    }

    if (capacity_ > 0) {
      send_cv_.wait(lock, [this] { return queue_.size() < capacity_ || closed_; });

      if (closed_) {
        return false;
      }
    }

    queue_.push(std::move(value));
    recv_cv_.notify_one();
    return true;
  }

  // Try to send without blocking
  bool try_send(const T& value) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (closed_) {
      return false;
    }

    if (capacity_ > 0 && queue_.size() >= capacity_) {
      return false;
    }

    queue_.push(value);
    recv_cv_.notify_one();
    return true;
  }

  bool try_send(T&& value) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (closed_) {
      return false;
    }

    if (capacity_ > 0 && queue_.size() >= capacity_) {
      return false;
    }

    queue_.push(std::move(value));
    recv_cv_.notify_one();
    return true;
  }

  // Receive value from channel (blocks if empty)
  std::optional<T> receive() {
    std::unique_lock<std::mutex> lock(mutex_);

    recv_cv_.wait(lock, [this] { return !queue_.empty() || closed_; });

    if (queue_.empty()) {
      return std::nullopt;
    }

    T value = std::move(queue_.front());
    queue_.pop();

    if (capacity_ > 0) {
      send_cv_.notify_one();
    }

    return value;
  }

  // Try to receive without blocking
  std::optional<T> try_receive() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (queue_.empty()) {
      return std::nullopt;
    }

    T value = std::move(queue_.front());
    queue_.pop();

    if (capacity_ > 0) {
      send_cv_.notify_one();
    }

    return value;
  }

  template <typename Rep, typename Period>
  std::optional<T> receive_for(std::chrono::duration<Rep, Period> timeout) {
    std::unique_lock lock(mutex_);
    if (!recv_cv_.wait_for(lock, timeout, [&] { return closed_ || !queue_.empty(); }) ||
        queue_.empty())
      return std::nullopt;
    T value = std::move(queue_.front());
    queue_.pop();
    send_cv_.notify_one();
    return value;
  }

  template <typename U, typename Rep, typename Period>
  bool send_for(U&& value, std::chrono::duration<Rep, Period> timeout) {
    std::unique_lock lock(mutex_);
    if (!send_cv_.wait_for(
            lock, timeout,
            [&] { return closed_ || capacity_ == 0 || queue_.size() < capacity_; }) ||
        closed_)
      return false;
    queue_.push(std::forward<U>(value));
    recv_cv_.notify_one();
    return true;
  }

  // Close the channel
  void close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    recv_cv_.notify_all();
    send_cv_.notify_all();
  }

  [[nodiscard]] bool is_closed() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return closed_;
  }

  [[nodiscard]] size_t size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  [[nodiscard]] bool empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

 private:
  std::queue<T> queue_;
  size_t capacity_;
  bool closed_;

  mutable std::mutex mutex_;
  std::condition_variable recv_cv_;
  std::condition_variable send_cv_;
};

// ============================================================================
// Blocking queue (unbounded thread-safe queue)
// ============================================================================

export template <typename T>
class blocking_queue {
 public:
  bool push(const T& value) { return channel_.send(value); }
  bool push(T&& value) { return channel_.send(std::move(value)); }
  // Legacy pop throws at end-of-stream; wait_pop returns nullopt after close and drain.
  T pop() {
    auto value = channel_.receive();
    if (!value) throw std::runtime_error("Blocking queue is closed and empty");
    return std::move(*value);
  }
  std::optional<T> wait_pop() { return channel_.receive(); }
  template <typename Rep, typename Period>
  std::optional<T> pop_for(std::chrono::duration<Rep, Period> timeout) {
    return channel_.receive_for(timeout);
  }
  bool try_pop(T& value) {
    auto next = channel_.try_receive();
    if (!next) return false;
    value = std::move(*next);
    return true;
  }
  void close() { channel_.close(); }
  [[nodiscard]] bool is_closed() const { return channel_.is_closed(); }
  [[nodiscard]] bool empty() const { return channel_.empty(); }
  [[nodiscard]] size_t size() const { return channel_.size(); }

 private:
  channel<T> channel_;
};

// ============================================================================
// Async utilities
// ============================================================================

// Run function asynchronously and return future
export template <typename F, typename... Args>
auto async(F&& f, Args&&... args) {
  return std::async(std::launch::async, std::forward<F>(f), std::forward<Args>(args)...);
}

// Run multiple functions in parallel and wait for all to complete
export template <typename... Functions>
void parallel_invoke(Functions&&... functions) {
  concurrent_detail::parallel_group group;
  (group.run(std::forward<Functions>(functions)), ...);
  group.finish();
}

// ============================================================================
// Atomic counter
// ============================================================================

export class atomic_counter {
 public:
  explicit atomic_counter(long long initial = 0) : value_(initial) {}

  long long increment() { return add(1); }

  long long decrement() { return add(-1); }

  long long add(long long delta) {
    return static_cast<long long>(static_cast<unsigned long long>(value_.fetch_add(delta)) +
                                  static_cast<unsigned long long>(delta));
  }

  long long get() const { return value_.load(); }

  void set(long long value) { value_.store(value); }

  long long exchange(long long new_value) { return value_.exchange(new_value); }

  bool compare_exchange(long long expected, long long desired) {
    return value_.compare_exchange_strong(expected, desired);
  }

 private:
  std::atomic<long long> value_;
};

// ============================================================================
// Spinlock (for very short critical sections)
// ============================================================================

export class spinlock {
 public:
  void lock() {
    while (flag_.test_and_set(std::memory_order_acquire)) {
      // Busy wait
    }
  }

  void unlock() { flag_.clear(std::memory_order_release); }

  bool try_lock() { return !flag_.test_and_set(std::memory_order_acquire); }

 private:
  std::atomic_flag flag_;
};

// ============================================================================
// Once flag wrapper
// ============================================================================

export class call_once {
 public:
  template <typename F, typename... Args>
  void operator()(F&& f, Args&&... args) {
    std::call_once(flag_, std::forward<F>(f), std::forward<Args>(args)...);
  }

 private:
  std::once_flag flag_;
};

}  // namespace cppx
