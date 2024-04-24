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
        NAME fmt
        GIT_REPOSITORY "git@github.com:fmtlib/fmt.git"
        GIT_TAG 10.1.1)
cpmaddpackage(
        NAME libassert
        GIT_REPOSITORY "git@github.com:jeremy-rifkin/libassert.git"
        GIT_TAG main)
cpmaddpackage(
        NAME function2
        GIT_REPOSITORY "git@github.com:Naios/function2.git"
        GIT_TAG 4.2.4)
cpmaddpackage(
        NAME range-v3
        GIT_REPOSITORY "git@github.com:ericniebler/range-v3.git"
        GIT_TAG 0.12.0)

include("${CMAKE_CURRENT_SOURCE_DIR}/tl.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/boost.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/spdlog.cmake")

sl_target_link_system_libraries(${PROJECT_NAME}
        INTERFACE
        # fmt/log
        fmt::fmt
        spdlog::spdlog

        # fp components
        serious-meta-library
        tl::optional
        tl::expected
        function2::function2
        range-v3::range-v3

        # utils
        libassert::assert
        Boost::container

        # graphics
        serious-graphics-library
)
