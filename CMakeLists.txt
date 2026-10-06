#============================================================================
# KaiAlloyShielding — Geant4 Radiation Transport for Prof. Kai's HEA Alloys
#
# A customized Geant4 framework to simulate the shielding effectiveness
# of newly developed high-entropy alloys and chemically complex
# intermetallic alloys for Gen-IV nuclear reactor applications.
#
# Author: Nghia
# Date:   2026
#============================================================================
cmake_minimum_required(VERSION 3.16)
project(KaiAlloyShielding VERSION 1.0.0 LANGUAGES CXX)

# --- Geant4 ---
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# --- ROOT (optional, for histogramming) ---
find_package(ROOT QUIET)
if(ROOT_FOUND)
  message(STATUS "ROOT found: ${ROOT_VERSION} — histogram output enabled")
  add_definitions(-DUSE_ROOT)
  include_directories(${ROOT_INCLUDE_DIRS})
else()
  message(STATUS "ROOT not found — CSV output will be used instead")
endif()

# --- Sources ---
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# --- Executable ---
add_executable(KaiShielding main.cc ${sources} ${headers})
target_include_directories(KaiShielding PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(KaiShielding ${Geant4_LIBRARIES})
if(ROOT_FOUND)
  target_link_libraries(KaiShielding ${ROOT_LIBRARIES})
endif()

# --- Copy macros to build directory ---
file(GLOB macros ${PROJECT_SOURCE_DIR}/macros/*.mac)
foreach(macro ${macros})
  get_filename_component(macro_name ${macro} NAME)
  configure_file(${macro} ${PROJECT_BINARY_DIR}/${macro_name} COPYONLY)
endforeach()

# --- Install ---
install(TARGETS KaiShielding DESTINATION bin)
