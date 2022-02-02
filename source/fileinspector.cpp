#include "fileinspector.h"
#include "log.h"

namespace fi
{
  inspector::inspector()
  {
    m_ui.add<ui::main_menu>();
    m_ui.add<ui::status_bar>();
  }

  void fi::inspector::update()
  {
  }

  void fi::inspector::draw_ui()
  {
    m_ui.update(*this);
  }

  std::shared_ptr<inspector_results> inspector::start_inspecting(inspector_options opts)
  {
    auto pResults = std::make_shared<inspector_results>(*this, opts);
    enqueue_task([=]() { inspector_results::inspect_directory(pResults, opts.rootPath); return 1; });
    return pResults;
  }

  thread_pool &fi::inspector::get_thread_pool()
  {
    static thread_pool pool;
    return pool;
  }

  bool inspector::is_inspecting() const
  {
    for (auto &pInspection : m_inspections)
      if (pInspection->directories_to_inspect() > 0)
        return true;
    return false;
  }
}
