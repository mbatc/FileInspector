#include "ui.h"
#include "filedialog.h"
#include "fileinspector.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace fi
{
  namespace ui
  {
    void main_menu::update(inspector &ctx)
    {
      // Fill the entire screen
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::SetNextWindowPos(ImVec2(0, 0));

      if (ImGui::Begin("Main Menu"))
      {
        if (ctx.is_inspecting())
          ImGui::Text("[ Working... ]");
        else
          ImGui::Text("[ Stopped... ]");

        if (ctx.has_results())
        {
          ImGui::SameLine();
          ImGui::Text("Analyzed %d files in %d directories", ctx.num_files_discovered(), ctx.num_directories_discovered());
        }

        ImGui::Separator();

        if (ImGui::Button("Inspect Folder"))
        {
          open_file_dialog dialog(true, false);
          if (dialog.show(""))
            ctx.start_inspecting(dialog.get_selected()[0]);
        }

        if (ctx.is_inspecting())
        {
          ImGui::SameLine();
          if (ImGui::Button("Stop"))
            ctx.stop_inspecting();
        }

        ImGui::Separator();

        if (ctx.has_results())
        {
          auto nameCols = ctx.get_name_collisions();
          ImGui::PushMultiItemsWidths(2, ImGui::GetWindowWidth());
          ImGui::BeginChild("Frame_NameCollisions", ImVec2(ImGui::CalcItemWidth(), 0), true);
          ImGui::CollapsingHeader("Duplicates Found", ImGuiTreeNodeFlags_Leaf);

          ImGui::BeginChild("Frame_NameCollisionsList", ImVec2(0, 0), true);

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

          ImGui::BeginChild("Frame_Selected", ImVec2(ImGui::CalcItemWidth(), 0), true);
          ImGui::CollapsingHeader("Duplicates Found", ImGuiTreeNodeFlags_Leaf);
          ImGui::BeginChild("Frame_SelectedList", ImVec2(0, 0), true);


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

      ImGui::End();
    }
  }
}

