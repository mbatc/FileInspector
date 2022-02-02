#include "fileinspector.h"

namespace fi
{
  void fi::inspector::update()
  {
  }

  void fi::inspector::draw_ui()
  {
    m_ui.mainMenu.update(*this);
  }

  void fi::inspector::start_inspecting(std::filesystem::path const &folder)
  {
    clear_results();
    m_stop = false;
    ++m_directoriesToInspect;
    enqueue_task([=]() { inspect_directory(folder); return 1; });
  }

  void inspector::stop_inspecting()
  {
    m_stop = true;
  }

  bool inspector::is_inspecting() const
  {
    return m_directoriesToInspect > 0;
  }

  bool fi::inspector::has_results() const
  {
    return m_filesFound > 0;
  }

  void inspector::clear_results()
  {
    auto ul = get_lock();
    m_directoriesFound = 0;
    m_filesFound = 0;
    m_contentCollisions.clear();
    m_nameCollisions.clear();
  }

  int64_t inspector::num_files_discovered() const
  {
    return m_filesFound;
  }

  int64_t inspector::num_directories_discovered() const
  {
    return m_directoriesFound;
  }

  std::map<std::string, inspector::file_data> inspector::get_name_collisions()
  {
    auto ul = get_lock();
    return m_nameCollisions;
  }

  std::map<uint64_t, inspector::file_data> inspector::get_data_collisions()
  {
    auto ul = get_lock();
    return m_contentCollisions;
  }

  thread_pool &fi::inspector::get_thread_pool()
  {
    static thread_pool pool;
    return pool;
  }

  void fi::inspector::inspect_directory(std::filesystem::path const &folder)
  {
    ++m_directoriesFound;

    if (!m_stop)
    {
      for (auto &entry : std::filesystem::directory_iterator(folder))
      {
        if (entry.is_directory())
        {
          ++m_directoriesToInspect;
          enqueue_task([=]() { inspect_directory(entry.path()); return 1; });
        }
        else
        {
          inspect_entry(entry);
        }
      }
    }
    --m_directoriesToInspect;
  }

  void fi::inspector::inspect_entry(std::filesystem::directory_entry const &entry)
  {
    ++m_filesFound;

    m_lock.lock();
    auto &data = get_data_by_name(entry.path().filename().string());
    data.entries.push_back(entry);
    m_lock.unlock();
  }

  inspector::file_data &fi::inspector::get_data_by_name(std::string const &name)
  {
    return m_nameCollisions[name];
  }

  inspector::file_data &fi::inspector::get_data_by_content_hash(uint64_t const &contentHash)
  {
    return m_contentCollisions[contentHash];
  }

  std::unique_lock<std::mutex> fi::inspector::get_lock()
  {
    return std::unique_lock<std::mutex>(m_lock);
  }
}
