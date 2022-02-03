#pragma once

#include <filesystem>
#include <map>
#include <atomic>
#include <mutex>

#include "inspector_options.h"

namespace fi
{
  class inspector;

  struct file_list
  {
    std::vector<std::filesystem::directory_entry> entries;
  };

  class inspector_results
  {
  public:
    inspector_results(inspector &ctx, inspector_options opts);

    std::unique_lock<std::mutex> get_lock();

    bool empty() const;
    void clear();
    void stop();

    bool is_stopped() const;

    int64_t num_files_discovered() const;
    int64_t num_directories_discovered() const;
    int64_t num_file_contents_inspected() const;
    int64_t directories_to_inspect() const;

    const std::map<std::string, file_list>& get_name_collisions();
    const std::map<uint64_t, file_list>&    get_data_collisions();

    inspector_options get_options() const;

    static void inspect_directory(std::shared_ptr<inspector_results> pResults, std::filesystem::path const &folder);
    static void inspect_entry(std::shared_ptr<inspector_results> pResults, std::filesystem::directory_entry const &entry);

  private:
    file_list& get_data_by_name(std::string const &name);
    file_list& get_data_by_content_hash(uint64_t const &contentHash);

    const inspector_options m_opts;
    bool m_stopped = false;

    std::mutex m_lock;
    std::atomic_uint64_t m_directoriesFound;
    std::atomic_uint64_t m_filesFound;
    std::atomic_uint64_t m_contentsInspected;
    std::atomic_uint64_t m_directoriesToInspect;

    // Mapping from path to matched name
    std::map<std::string, std::string> m_pathToName;

    // Mapping from path to content hash
    std::map<std::string, uint64_t>    m_pathToContent;

    std::map<std::string, file_list> m_nameCollisions;
    std::map<uint64_t, file_list> m_contentCollisions;

    inspector &m_ctx;
  };
}