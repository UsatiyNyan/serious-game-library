# CMake packages
# provides fmt, assert
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-meta-library.git"
        GIT_TAG main)
# provides spdlog
cpmaddpackage(
        NAME serious-runtime-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-runtime-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-execution-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-execution-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-graphics-library.git"
        GIT_TAG main)

include("${CMAKE_CURRENT_SOURCE_DIR}/boost.cmake")

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
        nlohmann_json::nlohmann_json
        fx-gltf::fx-gltf
)
