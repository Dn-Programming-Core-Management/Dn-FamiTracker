/*
 * fair_mutex.hpp
 *
 * MIT License
 *
 * Copyright (c) 2017 yohhoy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef YAMC_FAIR_MUTEX_HPP_
#define YAMC_FAIR_MUTEX_HPP_

#include <condition_variable>
#include <mutex>


namespace yamc {

/*
 * fairness (FIFO locking) mutex
 *
 * - yamc::fair::mutex
 * - yamc::fair::recursive_mutex
 * - yamc::fair::timed_mutex
 * - yamc::fair::recursive_timed_mutex
 */
namespace fair {

class mutex {
  std::size_t next_ = 0;
  std::size_t curr_ = 0;
  std::condition_variable cv_;
  std::mutex mtx_;

public:
  mutex() = default;
  ~mutex() = default;

  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  void lock()
  {
    std::unique_lock<decltype(mtx_)> lk(mtx_);
    const std::size_t request = next_++;
    while (request != curr_) {
      cv_.wait(lk);
    }
  }

  bool try_lock()
  {
    std::lock_guard<decltype(mtx_)> lk(mtx_);
    if (next_ != curr_)
      return false;
    ++next_;
    return true;
  }

  void unlock()
  {
    std::lock_guard<decltype(mtx_)> lk(mtx_);
    ++curr_;
    cv_.notify_all();
  }
};

} // namespace fair
} // namespace yamc

#endif
