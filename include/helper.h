#pragma once
#include <string>

#if defined(WORKING_DIR)
inline std::string workingDir = std::string(WORKING_DIR) + "/";
#else
inline std::string workingDir = std::string("./");
#endif
