#pragma once

#include <map>
#include <string>
#include <typeindex>
#include <memory>

#include "inspector_options.h"

namespace fi
{
  class inspector;
  class inspector_results;

  namespace ui
  {
    class view;

    class menu
    {
    public:
      virtual void update(view &parent, inspector &ctx) = 0;
    };

    class view
    {
    public:
      void update(inspector &ctx)
      {
        for (auto &[type, pMenu] : m_toAdd)
          if (!m_menus.try_emplace(type, pMenu).second)
            delete pMenu;
        m_toAdd.clear();

        for (auto &[type, pMenu] : m_menus)
          pMenu->update(*this, ctx);

        for (auto *pMenu : m_toRemove)
        {
          for (auto it = m_menus.begin(); it != m_menus.end(); ++it)
          {
            if (it->second == pMenu)
            {
              delete it->second;
              m_menus.erase(it);
              break;
            }
          }
        }
        m_toRemove.clear();
      }

      template<typename T, typename... Args>
      void add(Args&&... args) { m_toAdd.emplace_back(typeid(T), new T(args...)); }
      void remove(menu *pMenu) { m_toRemove.push_back(pMenu); }

      template<typename T>
      T* get()
      {
        auto it = m_menus.find(typeid(T));
        return it == m_menus.end() ? nullptr : (T*)it->second;
      }

    private:
      std::map<std::type_index, menu*> m_menus;

      std::vector<menu*> m_toRemove;
      std::vector<std::pair<std::type_index, menu*>> m_toAdd;
    };

    class status_bar : public menu
    {
    public:
      virtual void update(view &parent, inspector &ctx) override;

      std::shared_ptr<inspector_results> pResults;
    };
    
    class main_menu : public menu
    {
    public:
      virtual void update(view &parent, inspector &ctx) override;
    };

    class options_menu : public menu
    {
    public:
      options_menu(std::filesystem::path rootPath);

      virtual void update(view &parent, inspector &ctx) override;

      inspector_options m_options;
    };

    class results_page : public menu
    {
    public:
      results_page(std::shared_ptr<inspector_results> pResults);

      virtual void update(view &parent, inspector &ctx) override;

    private:
      std::shared_ptr<inspector_results> m_pResults;
      std::string m_selectedDuplicate;
    };
  }
}
