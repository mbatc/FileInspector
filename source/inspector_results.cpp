#include "inspector_results.h"
#include "fileinspector.h"
#include "log.h"

#include <regex>
#include <fstream>

namespace fi
{
  uint64_t hash_combine(uint64_t h, uint64_t k)
  {
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's
    // from hashing to 0.
    h += 0xe6546b64;

    return h;
  }

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
    m_contentsInspected = 0;
    m_directoriesToInspect = 0;
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

  int64_t inspector_results::num_file_contents_inspected() const
  {
    return m_contentsInspected;
  }

  const std::map<std::string, file_list>& inspector_results::get_name_collisions()
  {
    auto ul = get_lock();
    return m_nameCollisions;
  }

  const std::map<uint64_t, file_list>& inspector_results::get_data_collisions()
  {
    auto ul = get_lock();
    return m_contentCollisions;
  }

  inspector_options inspector_results::get_options() const
  {
    return m_opts;
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

    if (pResults->is_stopped())
    {
      --pResults->m_directoriesToInspect;
      return;
    }

    std::error_code err;
    for (auto &entry : std::filesystem::directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied, err))
    {
      if (entry.is_directory(err))
      {
        ++pResults->m_directoriesToInspect;
        pResults->m_ctx.enqueue_task(0,
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

    --pResults->m_directoriesToInspect;
  }

  void inspector_results::inspect_entry(std::shared_ptr<inspector_results> pResults, std::filesystem::directory_entry const &entry)
  {
    ++pResults->m_filesFound;

    std::string name = entry.path().filename().string();

    if (pResults->get_options().matchSimilarNames)
    { // Parse 'name'
      name = entry.path().stem().string();

      std::regex expr("(.+)\\s+-\\s*copy", std::regex_constants::icase);
      auto end = std::sregex_iterator();
      auto it = std::sregex_iterator(name.begin(), name.end(), expr);
      while (it != end)
      {
        auto match = *it;
        name = match[match.size() - 1];
        it = std::sregex_iterator(name.begin(), name.end(), expr);
      }

      size_t nameStart = name.find_first_not_of(' ');
      size_t nameEnd   = name.find_last_not_of(' ') + 1;
      name = name.substr(nameStart, nameEnd - nameStart);
      name = name + entry.path().extension().string();
    }

    if (pResults->get_options().matchContents)
    {
      pResults->m_ctx.enqueue_task(1,
        [=]()
        {
          if (pResults->is_stopped())
            return 0;

          std::ifstream stream(entry.path().string());
          
          if (!stream.is_open())
          {
            fi::log("warning", "Failed to open %s. Cannot compare contents", entry.path().string().c_str());
            return 0;
          }
          
          uint64_t hash = 0;
          while (stream.peek() != EOF)
            hash = hash_combine(hash, stream.get());
          
          pResults->m_lock.lock();
          auto &data = pResults->get_data_by_content_hash(hash);
          data.entries.push_back(entry);
          pResults->m_lock.unlock();
          ++pResults->m_contentsInspected;

          return 1;
        }
      );
    }

    pResults->m_lock.lock();
    auto &data = pResults->get_data_by_name(name);
    data.entries.push_back(entry);
    pResults->m_lock.unlock();
  }
}
