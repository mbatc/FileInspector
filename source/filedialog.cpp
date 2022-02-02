#include "filedialog.h"

namespace fi
{
  open_file_dialog::open_file_dialog(bool openFolder, bool multiselect)
  {
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pSystemDialog));
    if (FAILED(result))
    {
      log("FileDialog", "Failed to create Open File Dialog");
      return;
    }

    FILEOPENDIALOGOPTIONS opts = 0;
    m_pSystemDialog->GetOptions(&opts);
    opts |= (openFolder * FOS_PICKFOLDERS) | FOS_FILEMUSTEXIST | (multiselect * FOS_ALLOWMULTISELECT);
    m_pSystemDialog->SetOptions(opts);
  }

  bool open_file_dialog::show(std::filesystem::path const &initialPath)
  {
    initialPath;

    if (m_pSystemDialog == nullptr)
      return false;

    if (FAILED(m_pSystemDialog->Show(nullptr)))
      return false;

    IShellItemArray *pItemArray;
    if (SUCCEEDED(m_pSystemDialog->GetResults(&pItemArray)))
    {
      DWORD count;
      if (SUCCEEDED(pItemArray->GetCount(&count)))
      {
        for (int64_t i = 0; i < count; ++i)
        {
          IShellItem *pItem;
          if (SUCCEEDED(pItemArray->GetItemAt((DWORD)i, &pItem)))
          {
            LPWSTR displayName;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName)))
              m_selected.push_back(WideToString(displayName).c_str());
          }
        }
      }
    }

    return true;
  }

  void open_file_dialog::set_filter(std::vector<std::string> const &extensions, std::vector<std::string> const &descriptions)
  {
    std::vector<COMDLG_FILTERSPEC> filterSpecs;
    filterSpecs.resize(extensions.size());

    std::vector<std::wstring> names(filterSpecs.size(), L""); // Store the strings somewhere
    std::vector<std::wstring> patterns(filterSpecs.size(), L""); // Store the strings somewhere

    for (size_t i = 0; i < extensions.size(); ++i)
    {
      // Store strings
      names[i] = StringToWide(descriptions[i]);
      patterns[i] = StringToWide(std::string("*.") + extensions[i]);

      // Pass filter to WinAPI structure
      filterSpecs[i].pszName = names[i].c_str();
      filterSpecs[i].pszSpec = patterns[i].c_str();
    }

    m_pSystemDialog->SetFileTypes((uint32_t)filterSpecs.size(), filterSpecs.data());
  }

  std::vector<std::string> const & open_file_dialog::get_selected() const
  {
    return m_selected;
  }

  save_file_dialog::save_file_dialog(bool openFolder)
  {
    HRESULT result = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pSystemDialog));
    if (FAILED(result))
    {
      log("FileDialog", "Failed to create Open File Dialog");
      return;
    }

    FILEOPENDIALOGOPTIONS opts = 0;
    m_pSystemDialog->GetOptions(&opts);
    opts |= (openFolder * FOS_PICKFOLDERS);
    m_pSystemDialog->SetOptions(opts);
  }

  bool save_file_dialog::show(std::filesystem::path const & initialPath)
  {
    initialPath;

    if (m_pSystemDialog == nullptr)
      return false;

    if (FAILED(m_pSystemDialog->Show(nullptr)))
      return false;

    IShellItem *pItem;
    if (SUCCEEDED(m_pSystemDialog->GetResult(&pItem)))
    {
      LPWSTR displayName;
      if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName)))
        m_selected = WideToString(displayName);
    }

    return true;
  }

  void save_file_dialog::set_filter(std::vector<std::string> const &extensions, std::vector<std::string> const &descriptions)
  {
    std::vector<COMDLG_FILTERSPEC> filterSpecs;
    filterSpecs.resize(extensions.size());

    std::vector<std::wstring> names(filterSpecs.size(), L""); // Store the strings somewhere
    std::vector<std::wstring> patterns(filterSpecs.size(), L""); // Store the strings somewhere

    for (size_t i = 0; i < extensions.size(); ++i)
    {
      // Store strings
      names[i] = StringToWide(descriptions[i]);
      patterns[i] = StringToWide(std::string("*.") + extensions[i]);

      // Pass filter to WinAPI structure
      filterSpecs[i].pszName = names[i].c_str();
      filterSpecs[i].pszSpec = patterns[i].c_str();
    }

    m_pSystemDialog->SetFileTypes((uint32_t)filterSpecs.size(), filterSpecs.data());
  }

  std::string const &save_file_dialog::get_selected() const
  {
    return m_selected;
  }
}
