#pragma once

#include <filesystem>

namespace fi
{
  struct inspector_options
  {
    std::filesystem::path rootPath;

    bool matchSimilarNames;
    bool matchContents;
    bool matchSimilarContents;
  };
}
