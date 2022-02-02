#pragma once

#include <thread>
#include <vector>
#include <future>
#include <functional>
#include <optional>
#include <deque>

class thread_pool
{
public:
  template<typename T>
  class handle
  {
    friend thread_pool;
  public:
    template<typename Functor, typename... Args>
    handle(Functor &&func, Args&&... args)
    {
      m_task = std::bind(func, std::forward<Args>(args)...);
    }

    std::optional<T> get()
    {
      return m_result.get();
    }

    bool aborted() const { return m_aborted; }
    bool done() const { return m_done; }
    bool running() const { return m_running; }

  private:
    void run()
    {
      if (!m_aborted)
      {
        if (!m_running)
        {
          m_running = true;
          m_aborted = false;
          m_promise.set_value(m_task());
          m_running = false;
          m_done = true;
        }
      }
      else
      {
        m_promise.set_value({});
      }
    }

    volatile bool m_aborted = false;
    volatile bool m_done = false;
    volatile bool m_running = false;

    std::function<T()> m_task;
    std::promise<std::optional<T>> m_promise;
    std::future<std::optional<T>> m_result;
  };

  thread_pool(size_t threadCount = std::thread::hardware_concurrency())
  {
    for (size_t i = 0; i < std::max<size_t>(1, threadCount); ++i)
      m_workers.emplace_back(&thread_pool::worker_thread, this);
  }

  ~thread_pool()
  {
    {
      auto ul = std::unique_lock(m_lock);
      m_running = false;
    }

    m_cv.notify_all();
    for (size_t i = 0; i < m_workers.size(); ++i)
      m_workers[i].join();
    m_tasks.clear();
  }

  thread_pool(thread_pool &&o) = delete;
  thread_pool(thread_pool const &o) = delete;

  template<typename Functor, typename... Args>
  auto enqueue(Functor&& functor, Args&&... args)
  {
    using ReturnType = decltype(functor(args...));

    auto funcHandle = std::make_shared<handle<ReturnType>>(functor, std::forward<Args>(args)...);


    m_lock.lock();
    m_tasks.push_back([funcHandle]() {
      funcHandle->run();
    });
    m_lock.unlock();

    m_cv.notify_one();

    return funcHandle;
  }

private:
  void worker_thread()
  {
    while (m_running)
    {
      // Wait for a task to be available
      std::unique_lock ul(m_lock);
      m_cv.wait(ul, [=]() { return !m_running || m_tasks.size() > 0; });

      if (!m_running)
        return;

      // Get the next task
      auto task = m_tasks.front();
      m_tasks.pop_front();
      ul.unlock();

      // Execute the task
      task();
    }
  }

  std::mutex m_lock;
  std::condition_variable m_cv;
  std::deque<std::function<void()>> m_tasks;
  std::vector<std::thread> m_workers;
  bool m_running = true;
};
