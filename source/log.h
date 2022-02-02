#pragma once

#include <string>

namespace fi
{
  template<typename... Args>
  inline void log(std::string const & category, std::string const &format, Args&&... args)
  {
    printf(("[" + category + "] " + format + "\n").c_str(), std::forward<Args>(args)...);
  }
}
