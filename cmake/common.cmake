# Common utilities for libiohub CMake configuration

# Get all sources (common + platform-specific) for a platform
function(get_iohub_sources ROOT_DIR PLATFORM_NAME OUT_VAR)
    # Collect common sources (exclude platform directories)
    file(GLOB_RECURSE COMMON_SRC ${ROOT_DIR}/src/*.c)
    list(FILTER COMMON_SRC EXCLUDE REGEX ".*/platform/.*")
    
    # Add platform-specific sources
    string(TOLOWER "${PLATFORM_NAME}" PLATFORM_LOWER)
    file(GLOB_RECURSE PLATFORM_SRC 
        ${ROOT_DIR}/src/platform/${PLATFORM_LOWER}/*.c
    )
    
    set(${OUT_VAR} ${COMMON_SRC} ${PLATFORM_SRC} PARENT_SCOPE)
endfunction()

# Configure target with platform settings
function(configure_iohub_target TARGET_NAME PLATFORM_NAME)
    string(TOUPPER "${PLATFORM_NAME}" PLATFORM_UPPER)
    
    # Check if this is an ESP-IDF component (interface library)
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        target_compile_definitions(${TARGET_NAME}
            INTERFACE IOHUB_PLATFORM_${PLATFORM_UPPER}=1
        )
    else()
        target_compile_definitions(${TARGET_NAME}
            PUBLIC IOHUB_PLATFORM_${PLATFORM_UPPER}=1
        )
        set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD 11)
        set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD_REQUIRED ON)
    endif()
endfunction()