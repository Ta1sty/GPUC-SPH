#pragma once
#include <string>

#if defined(WORKING_DIR)
inline std::string workingDir = std::string(WORKING_DIR) + "/";
#else
inline std::string workingDir = std::string("./");
#endif

const static std::string shaderDir = workingDir + "build/shaders/";
inline std::string shaderPath(const char *file) {
    return shaderDir + file + ".spv";
}
