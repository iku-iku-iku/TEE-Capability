cmake_minimum_required(VERSION 3.16.3)

project(distributed_tee VERSION 1 LANGUAGES CXX)

message(STATUS "Configuring ${PROJECT_NAME}...")

file(GLOB CALLREMOTEENCLAVESERVICE_SOURCES "../../../communication/src/*.cpp" "../../communication_machanism/DDS/src/*.cpp" "../../enclave/interface/src/*.cpp" "../../enclave/tee_framework/penglai/enclave_demo/src/*.cpp" "../../enclave/tee_framework/penglai/src/*.cpp" "./*.cpp")
include_directories(../../../communication/include ../../communication_machanism/DDS/include ../../enclave/interface ../../enclave/tee_framework/penglai/include ../../enclave/tee_framework/penglai/include/enclave_demo)

add_library(${PROJECT_NAME} ${CALLREMOTEENCLAVESERVICE_SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
target_link_libraries(${PROJECT_NAME} fastrtps fastcdr foonathan_memory)

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/lib)
