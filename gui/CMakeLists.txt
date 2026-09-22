cmake_minimum_required(VERSION 3.21)
project(MultieffectGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# PortAudio jest zainstalowane przez vcpkg (tak samo jak w głównym projekcie
# .vcxproj) — dokładamy jego prefiks do ścieżki wyszukiwania, niezależnie od
# tego skąd CMake bierze Qt (to ustawia Kit w Qt Creatorze).
list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
find_package(portaudio CONFIG REQUIRED)

qt_standard_project_setup()

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(EFFECTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../effects)

qt_add_executable(MultieffectGui
    main.cpp
    ChainModel.h
    ChainModel.cpp
    ${CORE_DIR}/Effect.h
    ${CORE_DIR}/Effect.cpp
    ${CORE_DIR}/EffectChain.h
    ${CORE_DIR}/EffectChain.cpp
    ${CORE_DIR}/AudioEngine.h
    ${CORE_DIR}/AudioEngine.cpp
    ${CORE_DIR}/WavWriter.h
    ${CORE_DIR}/WavWriter.cpp
    ${EFFECTS_DIR}/Fuzz.h ${EFFECTS_DIR}/Fuzz.cpp
    ${EFFECTS_DIR}/Tremolo.h ${EFFECTS_DIR}/Tremolo.cpp
    ${EFFECTS_DIR}/Tuner.h ${EFFECTS_DIR}/Tuner.cpp
    ${EFFECTS_DIR}/NoiseGate.h ${EFFECTS_DIR}/NoiseGate.cpp
    ${EFFECTS_DIR}/Rat.h ${EFFECTS_DIR}/Rat.cpp
    ${EFFECTS_DIR}/TubeScreamer.h ${EFFECTS_DIR}/TubeScreamer.cpp
    ${EFFECTS_DIR}/Delay.h ${EFFECTS_DIR}/Delay.cpp
    ${EFFECTS_DIR}/Compressor.h ${EFFECTS_DIR}/Compressor.cpp
    ${EFFECTS_DIR}/Vibrato.h ${EFFECTS_DIR}/Vibrato.cpp
    ${EFFECTS_DIR}/Flanger.h ${EFFECTS_DIR}/Flanger.cpp
    ${EFFECTS_DIR}/Phaser.h ${EFFECTS_DIR}/Phaser.cpp
    ${EFFECTS_DIR}/AutoWah.h ${EFFECTS_DIR}/AutoWah.cpp
    ${EFFECTS_DIR}/Equalizer.h ${EFFECTS_DIR}/Equalizer.cpp
    ${EFFECTS_DIR}/Boost.h ${EFFECTS_DIR}/Boost.cpp
)

target_include_directories(MultieffectGui PRIVATE ${CORE_DIR} ${EFFECTS_DIR})

qt_add_qml_module(MultieffectGui
    URI MultieffectGui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(MultieffectGui PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    portaudio
)

# portaudio jest zbudowane jako DLL — dokładamy je obok .exe, żeby program
# w ogóle wystartował (bez tego: "portaudio.dll not found" przy uruchomieniu)
add_custom_command(TARGET MultieffectGui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/vcpkg/installed/x64-windows/bin/portaudio.dll"
        "$<TARGET_FILE_DIR:MultieffectGui>"
)
