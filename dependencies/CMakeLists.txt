# CMake packages
# provides fmt, assert
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-meta-library.git"
        GIT_TAG 1.0.2)
cpmaddpackage(
        NAME serious-execution-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-execution-library.git"
        GIT_TAG 1.0.4)
# provides spdlog
cpmaddpackage(
        NAME serious-runtime-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-runtime-library.git"
        GIT_TAG 0.0.0)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-graphics-library.git"
        GIT_TAG 0.0.3)

include("${CMAKE_CURRENT_SOURCE_DIR}/boost.cmake")

cpmaddpackage(
        NAME EnTT
        GIT_REPOSITORY "git@github.com:skypjack/entt.git"
        GIT_TAG v3.13.2)
cpmaddpackage(
        NAME fastgltf
        GIT_REPOSITORY "git@github.com:spnda/fastgltf.git"
        GIT_TAG v0.9.0
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
        Boost::container
        EnTT::EnTT
        fastgltf::fastgltf
)
