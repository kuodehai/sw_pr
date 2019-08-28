
####################################################
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

#head file path
INCLUDE_DIRECTORIES( .)


#source directories
AUX_SOURCE_DIRECTORY(. DIR_SRCS)

#add library file
#ADD_LIBRARY(ConfigUtils SHARED "${DIR_SRCS}" "${THREADING_SRCS}" "${WATCHER_SRCS}" "${I2C_SRCS}" "${LEDS_SRCS}" "${SHOW_SRCS}")
#TARGET_LINK_LIBRARIES(ConfigUtils json-c ev ugpio)

#INSTALL(TARGETS ConfigUtils DESTINATION lib)
#INSTALL(DIRECTORY include DESTINATION .)


########################################################


set(INCLUDE_PATH 
     ${GOOGLETEST_INCLUDE_DIR}  .)


set(LINK_PATH gtest gmock)


if(BUILD_TESTING)
    set(testSourceFiles
	    "${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationNodeTest.cpp")
    # file(GLOB_RECURSE testSourceFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*Test.cpp")
    foreach (testSourceFile IN LISTS testSourceFiles)
        get_filename_component(testName ${testSourceFile} NAME_WE)
	add_executable(${testName} ${testSourceFile}   ConfigurationNode.cpp)
        target_include_directories(${testName} PUBLIC "${INCLUDE_PATH}")
        target_link_libraries(${testName} "${LINK_PATH}")
    endforeach()
endif()
