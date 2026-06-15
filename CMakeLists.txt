cmake_minimum_required(VERSION 3.28...3.30)

# ---------------------------------------------------------------------------
# HypeClip AI — Automatic gaming highlight & instant-replay plugin for OBS
# "Never miss a moment."
#
# Build is based on the official obs-plugintemplate layout (obsproject).
# Target: OBS Studio 30.x / 31.x, Windows x64 (MSVC), also builds on Linux
# for CI/unit-tests (UI + vision GPU paths are Windows-only at runtime).
# ---------------------------------------------------------------------------

project(hypeclip-ai VERSION 1.0.0)

option(ENABLE_FRONTEND_API "Use the OBS frontend API (docks, replay buffer)" ON)
option(ENABLE_QT "Use Qt functionality (dock UI)" ON)
option(HYPECLIP_ENABLE_VISION "Build the optional GPU vision module" ON)
option(HYPECLIP_BUILD_TESTS "Build unit tests (no OBS runtime needed)" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake")

add_library(${CMAKE_PROJECT_NAME} MODULE)

# ---- Source files ---------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  # Entry point
  src/plugin-main.cpp

  # Core
  src/core/Config.cpp
  src/core/EventBus.cpp
  src/core/PipelineController.cpp

  # Audio
  src/audio/AudioCapture.cpp
  src/audio/AudioFeatures.cpp
  src/audio/VoiceExcitementDetector.cpp
  src/audio/GameAudioDetector.cpp

  # Scoring
  src/scoring/ScoreEngine.cpp
  src/scoring/MomentumTracker.cpp
  src/scoring/ConfidenceEngine.cpp

  # Replay / clips
  src/replay/ReplayBufferController.cpp
  src/replay/ClipManager.cpp
  src/replay/InstantReplayDirector.cpp

  # Metadata
  src/metadata/AutoTitler.cpp
  src/metadata/AutoTagger.cpp

  # Storage
  src/storage/StorageManager.cpp
)

# Optional GPU vision module
if(HYPECLIP_ENABLE_VISION)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    src/vision/VisionAnalyzer.cpp
    src/vision/GameProfile.cpp)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HYPECLIP_VISION=1)
endif()

# Qt dock UI
if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core Gui REQUIRED)
  # Enable Qt's meta-object compiler ON THE TARGET. (Setting the CMAKE_AUTOMOC
  # variable here would be too late, since the target was already created above —
  # so we set the target property directly. This generates the moc glue for every
  # Q_OBJECT class and fixes "unresolved external symbol metaObject/qt_metacall".)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    src/ui/HypeClipDock.cpp
    src/ui/DashboardTab.cpp
    src/ui/HighlightsTab.cpp
    src/ui/SettingsTab.cpp
    src/ui/HypeMeter.cpp)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HYPECLIP_QT=1)
endif()

# ---- OBS libraries --------------------------------------------------------
# Two ways to find OBS:
#  (A) CI "from source" mode: the cloud build compiles libobs + obs-frontend-api
#      from the OBS source tree and hands us their paths directly. Enabled when
#      HYPECLIP_OBS_SRC is defined. This avoids needing an OBS SDK install.
#  (B) Normal mode: a proper OBS dev package is present -> use find_package.
if(HYPECLIP_OBS_SRC)
  message(STATUS "HypeClip: using OBS from source tree at ${HYPECLIP_OBS_SRC}")

  add_library(OBS::libobs UNKNOWN IMPORTED)
  set_target_properties(OBS::libobs PROPERTIES
    IMPORTED_LOCATION "${OBS_LIBOBS_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${HYPECLIP_OBS_SRC}/libobs;${OBS_CONFIG_INCLUDE}")

  if(ENABLE_FRONTEND_API)
    add_library(OBS::obs-frontend-api UNKNOWN IMPORTED)
    set_target_properties(OBS::obs-frontend-api PROPERTIES
      IMPORTED_LOCATION "${OBS_FRONTEND_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${HYPECLIP_OBS_SRC}/UI/obs-frontend-api")
  endif()
else()
  find_package(libobs REQUIRED)
  if(ENABLE_FRONTEND_API)
    find_package(obs-frontend-api REQUIRED)
  endif()
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HYPECLIP_HAVE_OBS=1)

if(ENABLE_FRONTEND_API)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HYPECLIP_FRONTEND=1)
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_20)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME hypeclip-ai)

# ---- Unit tests (host build, no OBS) --------------------------------------
if(HYPECLIP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
