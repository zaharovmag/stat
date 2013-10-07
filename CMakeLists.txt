cmake_minimum_required(VERSION 2.8)
project(stat)
INCLUDE (CheckIncludeFiles)
SET (CMAKE_C_FLAGS "-Wall -O3 -std=c99")


include_directories("./include/")

CHECK_INCLUDE_FILES("nvml.h" HAVE_NVML)

IF(NOT HAVE_NVML)
  message( FATAL_ERROR "nvml.h is not found" )
ELSE()
  SET(nvstat src/nvidia.c)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_NVML")
  SET(DEP_LIB "${DEP_LIB} nvidia-ml")
ENDIF()




SET (STAT_SOURCES
  ${nvstat}
)
ADD_LIBRARY(stat SHARED ${STAT_SOURCES})
TARGET_LINK_LIBRARIES(stat ${DEP_LIB})



