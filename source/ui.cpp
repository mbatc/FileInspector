#include "ui.h"
#include "filedialog.h"
#include "fileinspector.h"
#include "inspector_results.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace fi
{
  namespace ui
  {
    void status_bar::update(view & parent, inspector & ctx)
    {
      ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y), 0, ImVec2(0, 1));
      ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetIO().DisplaySize.x, 0), ImVec2(ImGui::GetIO().DisplaySize.x, FLT_MAX));

      if (ImGui::Begin("Status"))
      {
        if (pResults && pResults->directories_to_inspect() > 0)
          ImGui::Text("[ Working... ]");
        else
          ImGui::Text("[ Stopped... ]");

        if (pResults && !pResults->empty())
        {
          if (pResults->directories_to_inspect() > 0)
          {
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
              pResults->stop();
          }

          ImGui::SameLine();
          ImGui::Text("Found %d files in %d directories.", pResults->num_files_discovered(), pResults->num_directories_discovered());
          ImGui::SameLine();
          ImGui::Text("Compared %d files contents.", pResults->num_file_contents_inspected());
        }
      }
      
      height = ImGui::GetWindowHeight();

      ImGui::End();
    }

    void main_menu::update(view & parent, inspector & ctx)
    {
      ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2, 0, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize / 2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 16.0f));

      if (ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
      {
        ImGui::SetCursorPos(ImGui::GetWindowSize() / 4);
        if (ImGui::Button("Start", ImVec2(ImGui::GetWindowSize() / 2)))
        {
          open_file_dialog dialog(true, false);
          if (dialog.show(""))
          {
            parent.remove(this);
            parent.add<options_menu>(dialog.get_selected()[0]);
          }
        }
      }

      ImGui::PopStyleVar(3);
      ImGui::End();
    }

    options_menu::options_menu(std::filesystem::path rootPath)
    {
      m_options.rootPath = rootPath;
    }

    void options_menu::update(view & parent, inspector & ctx)
    {
      ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2, 0, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 0));

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 16.0f));

      if (ImGui::Begin("Options", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
      {
        ImGui::Text("Inspecting %s...", m_options.rootPath.string().c_str());
        ImGui::Separator();
        ImGui::NewLine();
        ImGui::Checkbox("Match similar names", &m_options.matchSimilarNames);
        ImGui::Checkbox("Match file contents", &m_options.matchContents);
        // if (m_options.matchContents)
        //   ImGui::Checkbox("Match similar contents", &m_options.matchSimilarContents);
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::PushMultiItemsWidths(2, ImGui::GetWindowContentRegionWidth());
        if (ImGui::Button("Back", { ImGui::CalcItemWidth(), ImGui::GetTextLineHeightWithSpacing() * 2 }))
        {
          parent.remove(this);
          parent.add<main_menu>();
        }

        ImGui::PopItemWidth();
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::Button("Go!", { ImGui::CalcItemWidth(), ImGui::GetTextLineHeightWithSpacing() * 2 }))
        {
          parent.remove(this);
          parent.add<results_page>(ctx.start_inspecting(m_options));
        }

        ImGui::PopItemWidth();
      }

      ImGui::PopStyleVar(3);

      ImGui::End();
    }

    results_page::results_page(std::shared_ptr<inspector_results> pResults)
    {
      m_pResults = pResults;
    }


    auto results_page::draw_name_collisions_list(std::map<std::string, file_list> const & nameCols)
    {
      for (auto &entry : nameCols)
      {
        if (entry.second.entries.size() > 1)
          if (ImGui::Selectable(entry.first.c_str(), entry.first == m_selectedDuplicate))
            m_selectedDuplicate = entry.first;
      }

      return nameCols.find(m_selectedDuplicate);
    }

    auto results_page::draw_data_collisions_list(std::map<uint64_t, file_list> const & dataCols)
    {
      for (auto &entry : dataCols)
      {
        if (entry.second.entries.size() > 1)
        {
          int *pIDs = (int*)&m_selectedContentHash;
          ImGui::PushID(pIDs[0]);
          ImGui::PushID(pIDs[1]);

          std::string name = entry.second.entries[0].path().filename().string();
          if (ImGui::Selectable(name.c_str(), entry.first == m_selectedContentHash))
            m_selectedContentHash = entry.first;

          ImGui::PopID();
          ImGui::PopID();
        }
      }

      return dataCols.find(m_selectedContentHash);
    }

    void results_page::update(view & parent, inspector & ctx)
    {
      parent.get<status_bar>()->pResults = m_pResults;
      float height = ImGui::GetIO().DisplaySize.y - parent.get<status_bar>()->height;
      float width  = ImGui::GetIO().DisplaySize.x;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 16.0f));
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(ImVec2(width, height));

      ActivePage page = AP_Names;

      // Fill the entire screen
      if (ImGui::Begin("Results", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
      {
        if (!m_pResults->empty())
        {
          ImGui::PushMultiItemsWidths(3, ImGui::GetWindowWidth());
          ImGui::BeginChild("Frame_NameCollisions", ImVec2(ImGui::CalcItemWidth(), 0));

          if (ImGui::BeginTabBar("Frame_ColTabs"))
          {
            if (ImGui::BeginTabItem("Name Matches"))
            {
              page = AP_Names;
              ImGui::EndTabItem();
            }
          
            if (m_pResults->get_options().matchContents)
            {
              if (ImGui::BeginTabItem("Content Matches"))
              {
                page = AP_Contents;
                ImGui::EndTabItem();
              }
            }

            ImGui::EndTabBar();
          }

          auto& nameCols = m_pResults->get_name_collisions();
          auto& contentCols = m_pResults->get_data_collisions();

          ImGui::Separator();

          ImGui::BeginChild("Frame_NameCollisionsList", ImVec2(0, 0));

          file_list files;

          if (page == AP_Names)
          {
            auto it = draw_name_collisions_list(nameCols);
            if (it != nameCols.end())
              files = it->second;
          }
          else if (page == AP_Contents)
          {
            auto it = draw_data_collisions_list(contentCols);
            if (it != contentCols.end())
              files = it->second;
          }

          ImGui::EndChild();
          ImGui::EndChild();
          ImGui::PopItemWidth();

          ImGui::SameLine();

          ImGui::BeginChild("Frame_Selected", ImVec2(ImGui::CalcItemWidth() * 2, 0));

          ImGui::CollapsingHeader("Duplicates Found", ImGuiTreeNodeFlags_Leaf);
          ImGui::Separator();

          ImGui::BeginChild("Frame_SelectedList", ImVec2(0, 0));

          if (files.entries.size() > 0)
            draw_selected_content(files);

          ImGui::EndChild();
          ImGui::EndChild();

          ImGui::PopItemWidth();
          ImGui::PopItemWidth();
        }
      }

      ImGui::PopStyleVar(3);
      ImGui::End();
    }

    void results_page::draw_selected_content(file_list const & collisions)
    {
      static std::string actionItem;

      bool openDefaultAction = false;
      bool openInExplorer = false;
      bool deleteFile = false;

      for (auto &file : collisions.entries)
      {
        ImGui::Selectable(file.path().string().c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
          openDefaultAction = true;
          actionItem = file.path().string();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
          ImGui::OpenPopup("context_menu");
          actionItem = file.path().string();
        }
      }

      if (ImGui::BeginPopup("context_menu"))
      {
        if (ImGui::Selectable("Show in Explorer"))
          openInExplorer = true;
        if (ImGui::Selectable("Open File"))
          openDefaultAction = true;
        if (ImGui::Selectable("Delete"))
          deleteFile = true;

        ImGui::EndPopup();
      }

      if (openInExplorer)
      {
        ShellExecuteA(0, "open", "explorer.exe", ("/select,\"" + actionItem + "\"").c_str(), 0, SW_NORMAL);
        fi::log("OP", "show in explorer: %s", actionItem.c_str());
      }

      if (openDefaultAction)
      {
        ShellExecuteA(NULL, "", actionItem.c_str(), 0, 0, SW_SHOWDEFAULT);
        fi::log("OP", "open in default program: %s", actionItem.c_str());
      }

      if (deleteFile)
      {
        std::vector<char> name;
        for (char c : std::filesystem::canonical(actionItem).string())
          name.push_back(c);
        name.push_back('\0');

        SHFILEOPSTRUCTA opt = { 0 };
        opt.fFlags = FOF_ALLOWUNDO;
        opt.wFunc = FO_DELETE;
        opt.pFrom = name.data();
        SHFileOperationA(&opt);

        fi::log("OP", "deleting file: %s", name.data());
      }
    }
  }
}

