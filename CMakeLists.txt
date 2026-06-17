cmake_minimum_required(VERSION 3.11)
project(PowerGridSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Raylib
find_package(raylib QUIET)

add_executable(${PROJECT_NAME} src/main.cpp)

if(raylib_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
else()
    # Try finding via pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(RAYLIB raylib)
        if(RAYLIB_FOUND)
            target_include_directories(${PROJECT_NAME} PUBLIC ${RAYLIB_INCLUDE_DIRS})
            target_link_directories(${PROJECT_NAME} PUBLIC ${RAYLIB_LIBRARY_DIRS})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARIES})
        else()
            # Direct link fallback
            target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11)
        endif()
    else()
        # Direct link fallback
        target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11)
    endif()
endif()
