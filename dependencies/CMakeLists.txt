# CMake packages
# provides fmt, assert
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-meta-library.git"
        GIT_TAG 1.3.0
        GIT_SHALLOW TRUE)
cpmaddpackage(
        NAME serious-execution-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-execution-library.git"
        GIT_TAG 2.0.4
        GIT_SHALLOW TRUE)
# provides spdlog
cpmaddpackage(
        NAME serious-runtime-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-runtime-library.git"
        GIT_TAG 0.0.3
        GIT_SHALLOW TRUE)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-graphics-library.git"
        GIT_TAG 0.2.2
        GIT_SHALLOW TRUE)

cpmaddpackage(
        NAME EnTT
        URL https://github.com/skypjack/entt/archive/refs/tags/v3.13.2.tar.gz)
cpmaddpackage(
        NAME fastgltf
        URL https://github.com/spnda/fastgltf/archive/refs/tags/v0.9.0.tar.gz
        OPTIONS
        "FASTGLTF_USE_CUSTOM_SMALLVECTOR OFF"
        "FASTGLTF_COMPILE_AS_CPP20 ON")

sl_target_link_system_libraries(${PROJECT_NAME}
        PUBLIC
        # serious libraries
        sl::meta
        sl::rt
        sl::exec
        sl::gfx

        # utils
        EnTT::EnTT
        fastgltf::fastgltf
)
