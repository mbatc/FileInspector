#pragma once

#include <string>

namespace fi
{
  class inspector;

  namespace ui
  {
    class main_menu
    {
    public:
      void update(inspector &ctx);

    private:
      std::string m_selectedDuplicate;
    };
  }
}
