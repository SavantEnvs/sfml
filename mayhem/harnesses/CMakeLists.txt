# Builds SFML (the checkout two levels up) with the fuzz flags from CMAKE_<LANG>_FLAGS, then each
# harness twice: a libFuzzer binary and a run-once `-standalone` reproducer (STANDALONE_MAIN).
cmake_minimum_required(VERSION 3.28)
project(sfml_mayhem_harnesses LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. sfml EXCLUDE_FROM_ALL)

# stb_image_write's JPEG encoder (compiled into Image.cpp) left-shifts negative values on almost
# every image it encodes (stb_image_write.h, stbiw__jpg_writeBits): relax only UBSan's shift-base
# check, only for that file. Everything else in ASan/UBSan stays on and halting.
if("${CMAKE_CXX_FLAGS}" MATCHES "fsanitize=[^ ]*undefined")
    get_filename_component(sfml_image_cpp ${CMAKE_CURRENT_SOURCE_DIR}/../../src/SFML/Graphics/Image.cpp ABSOLUTE)
    set_source_files_properties(${sfml_image_cpp}
        TARGET_DIRECTORY sfml-graphics PROPERTIES COMPILE_OPTIONS -fno-sanitize=shift-base)
endif()

if(NOT STANDALONE_MAIN)
    message(FATAL_ERROR "pass -DSTANDALONE_MAIN=<path to StandaloneFuzzTargetMain.c>")
endif()
if(NOT LSAN_OFF_SOURCE)
    message(FATAL_ERROR "pass -DLSAN_OFF_SOURCE=<path to mayhem/lsan_off.cc>")
endif()
add_library(standalone_main OBJECT ${STANDALONE_MAIN})
add_library(lsan_off OBJECT ${LSAN_OFF_SOURCE})

function(sfml_harness name)
    foreach(variant fuzz standalone)
        if(variant STREQUAL fuzz)
            set(exe ${name})
        else()
            set(exe ${name}-standalone)
        endif()
        add_executable(${exe} ${name}.cpp $<TARGET_OBJECTS:lsan_off>)
        target_link_libraries(${exe} PRIVATE ${ARGN})
        if(variant STREQUAL fuzz)
            target_link_options(${exe} PRIVATE ${LIB_FUZZING_ENGINE})
        else()
            target_sources(${exe} PRIVATE $<TARGET_OBJECTS:standalone_main>)
        endif()
    endforeach()
endfunction()

separate_arguments(LIB_FUZZING_ENGINE UNIX_COMMAND "${LIB_FUZZING_ENGINE}")

sfml_harness(packet_fuzzer SFML::Network)
sfml_harness(ipaddress_fuzzer SFML::Network)
sfml_harness(http_fuzzer SFML::Network)
sfml_harness(ftp_fuzzer SFML::Network)
sfml_harness(string_fuzzer SFML::System)
sfml_harness(soundfile_fuzzer SFML::Audio)
sfml_harness(image_fuzzer SFML::Graphics)
sfml_harness(font_fuzzer SFML::Graphics)
