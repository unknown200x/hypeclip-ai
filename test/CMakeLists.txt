# Host unit tests — compiled WITHOUT libobs/Qt. The production sources guard all
# OBS includes behind HYPECLIP_HAVE_OBS, so they compile and run on any platform.
add_executable(hypeclip_tests
  test_main.cpp
  ${CMAKE_SOURCE_DIR}/src/core/EventBus.cpp
  ${CMAKE_SOURCE_DIR}/src/core/Config.cpp
  ${CMAKE_SOURCE_DIR}/src/audio/AudioFeatures.cpp
  ${CMAKE_SOURCE_DIR}/src/audio/VoiceExcitementDetector.cpp
  ${CMAKE_SOURCE_DIR}/src/audio/GameAudioDetector.cpp
  ${CMAKE_SOURCE_DIR}/src/scoring/MomentumTracker.cpp
  ${CMAKE_SOURCE_DIR}/src/scoring/ConfidenceEngine.cpp
  ${CMAKE_SOURCE_DIR}/src/scoring/ScoreEngine.cpp
  ${CMAKE_SOURCE_DIR}/src/metadata/AutoTitler.cpp
  ${CMAKE_SOURCE_DIR}/src/metadata/AutoTagger.cpp
)
target_include_directories(hypeclip_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(hypeclip_tests PRIVATE cxx_std_20)
add_test(NAME hypeclip_tests COMMAND hypeclip_tests)
