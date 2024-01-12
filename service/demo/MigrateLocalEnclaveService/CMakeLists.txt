cmake_minimum_required(VERSION 3.16.3)

project("MigrateLocalEnclaveService" VERSION 1 LANGUAGES CXX)

message(STATUS "Configuring MigrateLocalEnclaveService...")
#  Register interface here
if (NOT DEFINED ENCLAVE)
	set(ENCLAVE "PL")
        set(CC_PL ON)
        set(PENGLAI_ENGINE penglai_0)
endif()

if(${ENCLAVE} STREQUAL "PL")
        set(CC_PL ON)
        set(PENGLAI_ENGINE penglai_0)
endif()

if(${ENCLAVE} STREQUAL "other_enclave")
        set(CC_PL ON)
        set(OTHER_ENGINE other_enclave_0)
endif()


file(GLOB MIGRATELOCALENCLAVESERVICE_SOURCES "../../../communication/src/*.cpp" "../../communication_machanism/DDS/src/*.cpp" "../../enclave/interface/src/*.cpp" "../../enclave/tee_framework/penglai/enclave_demo/src/*.cpp" "../../enclave/tee_framework/penglai/src/*.cpp" "./*.cpp")
include_directories(../../../communication/include ../../communication_machanism/DDS/include ../../enclave/interface ../../enclave/tee_framework/penglai/include ../../enclave/tee_framework/penglai/include/enclave_demo)

add_executable(MigrateLocalEnclaveService ${MIGRATELOCALENCLAVESERVICE_SOURCES})
target_compile_features(MigrateLocalEnclaveService PRIVATE cxx_std_14)
target_compile_definitions(MigrateLocalEnclaveService PRIVATE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
target_link_libraries(MigrateLocalEnclaveService fastrtps fastcdr foonathan_memory)

install(TARGETS MigrateLocalEnclaveService
        RUNTIME DESTINATION examples/cpp/dds/TEE-Capability/RemoteEnclave/${BIN_INSTALL_DIR}
        )
