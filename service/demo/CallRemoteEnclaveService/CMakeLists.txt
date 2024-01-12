cmake_minimum_required(VERSION 3.16.3)

project("CallRemoteEnclaveService" VERSION 1 LANGUAGES CXX)

message(STATUS "Configuring CallRemoteEnclaveService...")

file(GLOB CALLREMOTEENCLAVESERVICE_SOURCES "../../../communication/src/*.cpp" "../../communication_machanism/DDS/src/*.cpp" "../../enclave/interface/src/*.cpp" "../../enclave/tee_framework/penglai/enclave_demo/src/*.cpp" "../../enclave/tee_framework/penglai/src/*.cpp" "./*.cpp")
include_directories(../../../communication/include ../../communication_machanism/DDS/include ../../enclave/interface ../../enclave/tee_framework/penglai/include ../../enclave/tee_framework/penglai/include/enclave_demo)

add_executable(CallRemoteEnclaveService ${CALLREMOTEENCLAVESERVICE_SOURCES})
target_compile_features(CallRemoteEnclaveService PRIVATE cxx_std_14)
target_compile_definitions(CallRemoteEnclaveService PRIVATE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
target_link_libraries(CallRemoteEnclaveService fastrtps fastcdr foonathan_memory)

install(TARGETS CallRemoteEnclaveService
        RUNTIME DESTINATION examples/cpp/dds/TEE-Capability/RemoteLocalEnclave/${BIN_INSTALL_DIR}
        )
