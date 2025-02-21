# CMake packages
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-meta-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-graphics-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-runtime-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-runtime-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-execution-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-execution-library.git"
        GIT_TAG main)

cpmaddpackage(
        NAME fmt
        GIT_REPOSITORY "git@github.com:fmtlib/fmt.git"
        GIT_TAG 10.1.1)
cpmaddpackage(
        NAME spdlog
        GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
        GIT_TAG v1.12.0
        OPTIONS
        "SPDLOG_FMT_EXTERNAL ON"
        )
cpmaddpackage(
        NAME libassert
        GIT_REPOSITORY "git@github.com:jeremy-rifkin/libassert.git"
        GIT_TAG v2.0.1)
cpmaddpackage(
        NAME EnTT
        GIT_REPOSITORY "git@github.com:skypjack/entt.git"
        GIT_TAG v3.13.2)
cpmaddpackage(
        NAME nlohmann_json
        GIT_REPOSITORY "git@github.com:nlohmann/json.git"
        GIT_TAG v3.11.3)
cpmaddpackage(
        NAME fx-gltf
        GIT_REPOSITORY "git@github.com:jessey-git/fx-gltf.git"
        GIT_TAG v2.0.0
        OPTIONS "FX_GLTF_BUILD_TESTS OFF" "FX_GLTF_INSTALL OFF"
        )

include("${CMAKE_CURRENT_SOURCE_DIR}/boost.cmake")

sl_target_link_system_libraries(${PROJECT_NAME}
        PUBLIC
        # serious libraries
        sl::meta
        sl::gfx
        sl::rt
        sl::exec

        # fmt/log
        fmt::fmt
        spdlog::spdlog

        # utils
        libassert::assert
        Boost::container
        EnTT::EnTT
        nlohmann_json::nlohmann_json
        fx-gltf::fx-gltf
)
