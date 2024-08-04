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
        NAME libassert
        GIT_REPOSITORY "git@github.com:jeremy-rifkin/libassert.git"
        GIT_TAG v2.0.1)

cpmaddpackage(
        NAME EnTT
        GIT_REPOSITORY "git@github.com:skypjack/entt.git"
        GIT_TAG v3.13.2)

include("${CMAKE_CURRENT_SOURCE_DIR}/boost.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/spdlog.cmake")

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
)
