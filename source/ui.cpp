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

          ImGui::Text("Analyzed %d files in %d directories", pResults->num_files_discovered(), pResults->num_directories_discovered());
        }
      }

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
        if (m_options.matchContents)
          ImGui::Checkbox("Match similar contents", &m_options.matchSimilarContents);
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

    void results_page::update(view & parent, inspector & ctx)
    {
      parent.get<status_bar>()->pResults = m_pResults;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

      // Fill the entire screen
      if (ImGui::Begin("Results"))
      {
        if (!m_pResults->empty())
        {
          auto nameCols = m_pResults->get_name_collisions();
          ImGui::PushMultiItemsWidths(2, ImGui::GetWindowWidth());
          ImGui::BeginChild("Frame_NameCollisions", ImVec2(ImGui::CalcItemWidth(), 0));

          ImGui::CollapsingHeader("Duplicates Found", ImGuiTreeNodeFlags_Leaf);
          ImGui::Separator();

          ImGui::BeginChild("Frame_NameCollisionsList", ImVec2(0, 0));

          for (auto &entry : nameCols)
          {
            if (entry.second.entries.size() > 1)
              if (ImGui::Selectable(entry.first.c_str(), entry.first == m_selectedDuplicate))
                m_selectedDuplicate = entry.first;
          }

          ImGui::EndChild();
          ImGui::EndChild();
          ImGui::PopItemWidth();

          ImGui::SameLine();

          ImGui::BeginChild("Frame_Selected", ImVec2(ImGui::CalcItemWidth(), 0));

          ImGui::CollapsingHeader("Duplicates Found", ImGuiTreeNodeFlags_Leaf);
          ImGui::Separator();

          ImGui::BeginChild("Frame_SelectedList", ImVec2(0, 0));


          auto entryIt = nameCols.find(m_selectedDuplicate);
          if (entryIt != nameCols.end())
          {
            static std::string actionItem;

            bool openDefaultAction = false;
            bool openInExplorer = false;
            bool deleteFile = false;

            for (auto &file : entryIt->second.entries)
            {
              ImGui::Selectable(file.path().string().c_str());

              if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
              {
                openDefaultAction = true;
                actionItem = file.path().string();
              }

              if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
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
          ImGui::EndChild();
          ImGui::EndChild();
          ImGui::PopItemWidth();

        }
      }

      ImGui::PopStyleVar();
      ImGui::End();
    }
  }
}

