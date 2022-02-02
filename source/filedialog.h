
#include <shobjidl_core.h>
#include <windows.h>
#include <string>
#include <codecvt>
#include <locale>
#include <vector>
#include <filesystem>

#include "log.h"

namespace fi
{
  inline std::wstring StringToWide(std::string const &str)
  {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(str);
  }

  inline std::string WideToString(std::wstring const &wstr)
  {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(wstr);
  }

  class open_file_dialog
  {
  public:
    open_file_dialog(bool openFolder, bool multiselect);

    bool show(std::filesystem::path const & initialPath);

    void set_filter(std::vector<std::string> const & extensions, std::vector<std::string> const &descriptions);

    std::vector<std::string> const & get_selected() const;

  private:
    IFileOpenDialog *m_pSystemDialog = nullptr;

    std::vector<std::string> m_selected;
  };

  class save_file_dialog
  {
  public:
    save_file_dialog(bool openFolder);

    bool show(std::filesystem::path const & initialPath);

    void set_filter(std::vector<std::string> const &extensions, std::vector<std::string> const &descriptions);

    std::string const & get_selected() const;

  private:
    IFileSaveDialog *m_pSystemDialog = nullptr;

    std::string m_selected;
  };
}
