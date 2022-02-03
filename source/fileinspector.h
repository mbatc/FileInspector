#pragma once

#include "ui.h"
#include "thread_pool.h"
#include "inspector_options.h"
#include "inspector_results.h"

#include <filesystem>
#include <map>

namespace fi
{
  static constexpr int64_t NumThreadPools = 2;

  class inspector
  {
  public:
    inspector();

    void update();
    void draw_ui();

    std::shared_ptr<inspector_results> start_inspecting(inspector_options opts);

    bool is_inspecting() const;

    static thread_pool &get_thread_pool(uint64_t poolID);

    template<typename Functor, typename... Args>
    static auto enqueue_task(uint64_t poolID, Functor &&func, Args... args)
    {
      return get_thread_pool(poolID).enqueue(func, std::forward<Args>(args)...);
    }

  private:
    std::vector<std::shared_ptr<inspector_results>> m_inspections;
    ui::view m_ui;
  };
}
