#include "inspector_results.h"
#include "fileinspector.h"
#include "log.h"

namespace fi
{
  inspector_results::inspector_results(inspector & ctx, inspector_options opts)
    : m_ctx(ctx)
    , m_opts(opts)
  {
    m_directoriesToInspect++;
  }

  void inspector_results::stop()
  {
    m_stopped = true;
  }

  bool inspector_results::is_stopped() const
  {
    return m_stopped;
  }

  bool inspector_results::empty() const
  {
    return m_filesFound == 0;
  }

  void inspector_results::clear()
  {
    auto ul = get_lock();
    m_directoriesFound = 0;
    m_filesFound = 0;
    m_contentCollisions.clear();
    m_nameCollisions.clear();
  }

  int64_t inspector_results::num_files_discovered() const
  {
    return m_filesFound;
  }

  int64_t inspector_results::num_directories_discovered() const
  {
    return m_directoriesFound;
  }

  int64_t inspector_results::directories_to_inspect() const
  {
    return m_directoriesToInspect;
  }

  std::map<std::string, file_list> inspector_results::get_name_collisions()
  {
    auto ul = get_lock();
    return m_nameCollisions;
  }

  std::map<uint64_t, file_list> inspector_results::get_data_collisions()
  {
    auto ul = get_lock();
    return m_contentCollisions;
  }

  inspector_options inspector_results::get_options() const
  {
    return inspector_options();
  }

  file_list &inspector_results::get_data_by_name(std::string const &name)
  {
    return m_nameCollisions[name];
  }

  file_list &inspector_results::get_data_by_content_hash(uint64_t const &contentHash)
  {
    return m_contentCollisions[contentHash];
  }

  std::unique_lock<std::mutex> inspector_results::get_lock()
  {
    return std::unique_lock<std::mutex>(m_lock);
  }

  void inspector_results::inspect_directory(std::shared_ptr<inspector_results> pResults, std::filesystem::path const &folder)
  {
    ++pResults->m_directoriesFound;

    if (!pResults->is_stopped())
    {
      std::error_code err;
      for (auto &entry : std::filesystem::directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied, err))
      {
        if (entry.is_directory(err))
        {
          ++pResults->m_directoriesToInspect;
          pResults->m_ctx.enqueue_task(
            [=]()
            {
              inspect_directory(pResults, entry.path());
              return 1;
            });
        }
        else if (!err)
        {
          inspect_entry(pResults, entry);
        }
        else
        {
          fi::log("error", "Unable to inspect %s: %s", entry.path().string().c_str(), err.message().c_str());
        }
      }

      if (err)
      {
        fi::log("error", "Unable to inspect %s: %s", folder.string().c_str(), err.message().c_str());
      }
    }

    --pResults->m_directoriesToInspect;
  }

  void inspector_results::inspect_entry(std::shared_ptr<inspector_results> pResults, std::filesystem::directory_entry const &entry)
  {
    ++pResults->m_filesFound;

    pResults->m_lock.lock();
    auto &data = pResults->get_data_by_name(entry.path().filename().string());
    data.entries.push_back(entry);
    pResults->m_lock.unlock();
  }

}
