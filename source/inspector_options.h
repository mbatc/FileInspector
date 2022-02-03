#pragma once

#include <filesystem>

namespace fi
{
  struct inspector_options
  {
    std::filesystem::path rootPath;

    bool matchSimilarNames = false;
    bool matchContents = false;
    bool matchSimilarContents = false;
  };
}
