set(TARGET soft-pkcs)
project(${TARGET})

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DDEBUG)
endif()
    add_definitions(-DDEBUG)

add_definitions("-std=c++11 -fvisibility=hidden")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED COMPONENTS
    system
    filesystem
)
include_directories(${Boost_INCLUDE_DIRS})

#add_subdirectory(3rdparty//qtwebdav)

set(HEADERS
    types.h
    object.h
    soft_token.h
    storage.h
    tools.h
    log.h
    attribute.h
)

set(SOURCES 
    main.cpp
    tools.cpp
    storage.cpp
    soft_token.cpp
    object.cpp
    log.cpp
    attribute.cpp
)


set(DEPS
	${Boost_LIBRARIES}
)

add_library(${TARGET} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${TARGET} ${DEPS})



