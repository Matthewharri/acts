add_library(
  ActsExamplesExperimentalMuon SHARED
  src/Muon_Detector.cpp)
target_include_directories(
  ActsExamplesExperimentalMuon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ActsExamplesExperimentalMuon
  PUBLIC
    ActsCore ActsPluginIdentification
    ActsExamplesFramework ActsPluginJson
    nlohmann_json::nlohmann_json)

install(
  TARGETS ActsExamplesExperimentalMuon
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})