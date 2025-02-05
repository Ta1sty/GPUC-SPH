#pragma once
#include <optional>
#include <string>

#if defined(WORKING_DIR)
inline std::string workingDir = std::string(WORKING_DIR) + "/";
#else
inline std::string workingDir = std::string("./");
#endif

enum class SceneType : uint32_t {
    SPH_BOX_2D,
    SPH_BOX_3D
};

const static std::string shaderDir = workingDir + "build/shaders/";
inline std::string shaderPath(const char *file, std::optional<SceneType> type) {
    if (!type.has_value()) {
        return shaderDir + file + ".spv";
    }

    switch (type.value()) {
        case SceneType::SPH_BOX_2D:
            return shaderDir + file + ".2D.spv";
        case SceneType::SPH_BOX_3D:
            return shaderDir + file + ".3D.spv";
        default:
            throw std::runtime_error("unknown case");
    }
}

// comment out this define to stop everything from pausing all the time •`_´•
#define PAUSE_ON_ERROR
#ifdef PAUSE_ON_ERROR
#include <iostream>
#endif
inline void pauseExecution() {
#ifdef PAUSE_ON_ERROR
    std::cout << "PRESS SPACE TO CONTINUE" << std::endl;
    std::cin.get();
#endif
}
