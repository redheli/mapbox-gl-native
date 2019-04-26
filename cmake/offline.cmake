add_executable(mbgl-offline
    bin/offline.cpp
)

target_sources(mbgl-offline
    PRIVATE platform/default/mbgl/util/default_styles.hpp
)

target_include_directories(mbgl-offline
    PRIVATE platform/default
)

target_link_libraries(mbgl-offline
    PRIVATE mbgl-core
)

target_add_mason_package(mbgl-offline PRIVATE boost)
target_add_mason_package(mbgl-offline PRIVATE args)

mbgl_platform_offline()

create_source_groups(mbgl-offline)

initialize_xcode_cxx_build_settings(mbgl-offline)

set_target_properties(mbgl-offline PROPERTIES FOLDER "Executables")

xcode_create_scheme(
    TARGET mbgl-offline
    OPTIONAL_ARGS
        "--style=file.json"
        "--north=37.2"
        "--west=-122.8"
        "--south=38.1"
        "--east=-121.7"
        "--minZoom=0.0"
        "--maxZoom=15.0"
        "--pixelRatio=1.0"
        "--token="
        "--output=offline.db"
)


#
add_executable(test-actor-model
    bin/test-actor-model.cpp
)
target_sources(test-actor-model
    PRIVATE platform/default/mbgl/util/default_styles.hpp
)

target_link_libraries(test-actor-model
    PUBLIC mbgl-core
    PRIVATE mbgl-filesource
    PRIVATE mbgl-loop-uv
)

#
add_executable(test-render
    bin/test-render.cpp
    test/src/mbgl/test/util.cpp
)
target_sources(test-render
    PRIVATE platform/default/mbgl/util/default_styles.hpp
)
target_include_directories(test-render
    PRIVATE src
    PRIVATE test/include
    PRIVATE test/src
    PRIVATE platform/default
    PRIVATE platform/linux
)
target_link_libraries(test-render
    PUBLIC mbgl-core
    PRIVATE mbgl-filesource
    PRIVATE mbgl-loop-uv
    PRIVATE gtest
)
target_compile_definitions(test-render PRIVATE -DRESOURCES="${CMAKE_CURRENT_SOURCE_DIR}")
target_add_mason_package(test-render PRIVATE geometry)
target_add_mason_package(test-render PRIVATE variant)
target_add_mason_package(test-render PRIVATE unique_resource)
target_add_mason_package(test-render PRIVATE rapidjson)
target_add_mason_package(test-render PRIVATE pixelmatch)
target_add_mason_package(test-render PRIVATE boost)
target_add_mason_package(test-render PRIVATE geojson)
target_add_mason_package(test-render PRIVATE geojsonvt)
target_add_mason_package(test-render PRIVATE shelf-pack)
