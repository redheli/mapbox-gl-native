set(CMAKE_PREFIX_PATH $ENV{CUSTOM_QT_CMAKE_PATH} ${CMAKE_PREFIX_PATH})

find_package(Qt5Core     REQUIRED)
find_package(Qt5Gui      REQUIRED)
find_package(Qt5Network  REQUIRED)
find_package(Qt5OpenGL   REQUIRED)
find_package(Qt5Widgets  REQUIRED)
find_package(Qt5Sql      REQUIRED)

set(EXT_ERROR_MESSAGE " +++ Please specify env var CUSTOM_QT_CMAKE_PATH or upgrade the system to the latest Qt")
if (Qt5Core_FOUND)
    message(STATUS " +++ Qt Version ${Qt5Core_VERSION} detected")
    if (Qt5Core_VERSION VERSION_LESS 5.10.1)
        message(FATAL_ERROR " +++ Minimum supported Qt5 version is 5.10! \n${EXT_ERROR_MESSAGE}")
    endif()
else()
    message(FATAL_ERROR " +++ The Qt5Core library could not be found! \n${EXT_ERROR_MESSAGE}")
endif()

# Qt5 always build OpenGL ES2 which is the compatibility
# mode. Qt5 will take care of translating the desktop
# version of OpenGL to ES2.
add_definitions("-DMBGL_USE_GLES2")

message(STATUS " +++ Qt Version ${Qt5Core_VERSION} detected")

set(MBGL_QT_CORE_LIBRARIES
    PUBLIC Qt5::Core
    PUBLIC Qt5::Gui
    PUBLIC Qt5::OpenGL
)

set(MBGL_QT_FILESOURCE_LIBRARIES
    PUBLIC Qt5::Network
    PUBLIC Qt5::Sql
)

target_link_libraries(mbgl-qt
    PRIVATE Qt5::Widgets
)
