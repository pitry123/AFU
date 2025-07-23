cmake_minimum_required(VERSION 3.10)
project(AFU)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Retrieve the Boost library path from the environment variable
if(DEFINED ENV{BOOST_1_80})
    set(BOOST_ROOT $ENV{BOOST_1_80})
    set(CMAKE_PREFIX_PATH ${BOOST_ROOT})  # Set prefix path for find_package
else()
    message(FATAL_ERROR "BOOST_1_80 environment variable not defined.")
endif()

find_package(Boost)

 
# Include directories
include_directories(Include)
include_directories(${Boost_INCLUDE_DIRS})




# Add each sample executable
add_subdirectory(Samples/UdpSample)
add_subdirectory(Samples/TcpSample)
add_subdirectory(Samples/BuidlerSample)
