#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

#include <boost/asio.hpp>

struct ThreadSafeQueueException : std::runtime_error {
  ThreadSafeQueueException(std::string const& what) : std::runtime_error(what) {};
};

// Wraps a Boost ASIO timer in a struct with a boolean indicating
// whether the operation should be canceled.  The boost timer's
// cancel() method cannot be used, because if the timer is expired at
// the time of the cancel() call, it will be run regardless.
class PushDelayTimer {
  std::mutex mtx;
  bool cancelled = false;
  boost::asio::steady_timer timer;

public:
  PushDelayTimer(boost::asio::io_context &ctx)
    : timer(ctx) {}

  boost::asio::steady_timer &boost_timer() {
    return timer;
  }

  void cancel() {
    std::lock_guard guard(mtx);
    cancelled = true;
  }
  bool is_cancelled() {
    std::lock_guard guard(mtx);
    return cancelled;
  }
};


// Cancels a push_delay timer when destructed.  Use
// TimerLifetime::create to create instances.
class TimerLifetime {
  std::shared_ptr<PushDelayTimer> timer;

public:
  // Cancel timer in destructor
  ~TimerLifetime() {
    if (timer) {
      timer->cancel();
    }
  }

  static std::unique_ptr<TimerLifetime>
  create(std::shared_ptr<PushDelayTimer> timer) {
    auto timer_lifetime = std::make_unique<TimerLifetime>();
    timer_lifetime->timer = std::move(timer);
    return timer_lifetime;
  }
};


template<typename T>
class ThreadSafeQueue {
  std::mutex mtx; // Locks the queue
  std::queue<T> queue;

  // Only one work item should be run at a time (e.g. call poll_one(),
  // do not call poll()), so that the handler for one work item can
  // easily cancel queued work items.
  boost::asio::io_context ctx;

  // Informs the context there is more work coming.  (So that
  // ctx.run_one() does not return immediately.)
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;

  void lock_and_push_value(T t) {
    std::lock_guard guard(mtx);
    queue.push(std::move(t));
  }
  std::optional<T> lock_and_pop_value() {
    std::lock_guard guard(mtx);
    if (!queue.size()) {
      return {};
    }
    auto t = std::move(queue.front());
    queue.pop();
    return t;
  }
public:
  ThreadSafeQueue()
    : ctx()
    , work(boost::asio::make_work_guard(ctx)) {}

  // Push a value onto the queue.
  void push(T t) {
    // mutex is not required here because the io_context is already
    // thread-safe
    boost::asio::post(ctx, [this, t=std::move(t)]() mutable {
      // mark as `mutable` for move-only types
      lock_and_push_value(std::move(t));
    });
  }

  // Push a value onto the queue after a set delay.
  template<typename Ref, typename Period>
  std::shared_ptr<PushDelayTimer> push_delay(T t, std::chrono::duration<Ref, Period> delay) {
    auto timer = std::make_shared<PushDelayTimer>(ctx);
    timer->boost_timer().expires_after(delay);

    // By capturing a shared_ptr to the timer in this lambda
    // expression, the timer is kept alive until the expression runs.
    // Then the timer is destroyed.  (This is a fairly common pattern
    // in ASIO, unfortunately).
    timer->boost_timer().async_wait([this, timer, t=std::move(t)](auto error_code) mutable {
      if (error_code == boost::asio::error::operation_aborted) {
        return;
      }
      else if (error_code) {
        // This should never be reached, according to
        // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/reference/basic_waitable_timer/async_wait.html
        throw ThreadSafeQueueException(error_code.message());
      }

      if (timer->is_cancelled()) {
        return;
      }

      lock_and_push_value(std::move(t));
    });

    return timer;
  }

  // Block until a value is ready, and then return it.
  T pop() {
    // Wait until an item is pushed to the queue.
    auto result = lock_and_pop_value();
    while (!result) {
      ctx.run_one();
      result = lock_and_pop_value();
    }
    // Since while loop has exited, result must hold a value
    return std::move(*result);
  }

  // Block until a value is ready or until a specified timeout expries.
  template<typename Ref, typename Period>
  std::optional<T> pop_timeout(std::chrono::duration<Ref, Period> timeout) {
    // Must handle the zero-timeout (non-blocking) case and timeout
    // case separately in Boost
    auto result = std::optional<T> {};

    // If timeout is zero, run all work that is immediately available
    // and return a value from the queue (if any)
    if (timeout == std::chrono::duration<Ref, Period>::zero()) {
      while (!result) {
        // poll_one returns the number of work items executed.  If zero,
        // there are no more items that can be executed immediately, so
        // break from the loop and return.
        if (!ctx.poll_one()) {
          break;
        }
        result = lock_and_pop_value();
      }
      return result;
    }

    // If timeout is nonzero, run work until there is a value on the
    // queue or until the timeout has expired
    auto end_time_point = std::chrono::steady_clock::now() + timeout;
    while (!result) {
      // same as with poll_one above, but if zero is returned, the
      // timeout has expired, so break from the loop and return.
      if (!ctx.run_one_until(end_time_point)) {
        break;
      }
      result = lock_and_pop_value();
    }
    return result;
  }
};

