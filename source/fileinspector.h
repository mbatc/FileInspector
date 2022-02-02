#pragma once

#include "ui.h"
#include "thread_pool.h"

#include <filesystem>
#include <map>

namespace fi
{
  class inspector
  {
  public:
    struct file_data
    {
      std::vector<std::filesystem::directory_entry> entries;
    };

    void update();
    void draw_ui();

    void start_inspecting(std::filesystem::path const &folder);
    void stop_inspecting();
    bool is_inspecting() const;

    bool has_results() const;
    void clear_results();

    int64_t num_files_discovered() const;
    int64_t num_directories_discovered() const;

    std::map<std::string, file_data> get_name_collisions();
    std::map<uint64_t, file_data>    get_data_collisions();

    static thread_pool &get_thread_pool();

    template<typename Functor, typename... Args>
    static auto enqueue_task(Functor &&func, Args... args)
    {
      return get_thread_pool().enqueue(func, std::forward<Args>(args)...);
    }

  private:
    struct
    {
      ui::main_menu mainMenu;
    } m_ui;

    void inspect_directory(std::filesystem::path const &folder);
    void inspect_entry(std::filesystem::directory_entry const &entry);

    file_data& get_data_by_name(std::string const &name);
    file_data& get_data_by_content_hash(uint64_t const &contentHash);

    bool m_stop = false;

    std::unique_lock<std::mutex> get_lock();
    std::filesystem::path m_directory; // Directory to inspect

    std::mutex m_lock;
    std::atomic_uint64_t m_directoriesFound;
    std::atomic_uint64_t m_filesFound;
    std::atomic_uint64_t m_directoriesToInspect;

    std::map<std::string, file_data> m_nameCollisions;
    std::map<uint64_t, file_data> m_contentCollisions;
  };
}
