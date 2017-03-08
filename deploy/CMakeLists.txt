if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(Clara_arch x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(Clara_arch x32)
else()
    message(FATAL_ERROR "Unknown architecture! (Neither 32 nor 64 bits)")
endif()

set(Clara_version ${PROJECT_NAME}-${PROJECT_VERSION})
set(Clara_package_name ${Clara_version}-${CMAKE_SYSTEM_NAME}-${Clara_arch}.zip)
set(Clara_tar_output "${CMAKE_BINARY_DIR}/${Clara_package_name}")

# Packaging Clara in a zip file for distribution.
add_custom_target(ClaraPackage
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/RemovePythonCaches.cmake"
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${Clara_tar_output}" --format=zip Clara
    DEPENDS ClaraInstall
    WORKING_DIRECTORY "${currentSublimePackagesFolder}"
    COMMENT "Packaging Clara")

# Deploys the ClaraPackage.
add_custom_target(ClaraDeploy
    COMMAND
        ${CMAKE_COMMAND} 
            -D VERSION=${PROJECT_VERSION} 
            -D SUBLIME_PLATFORM_EXT=${SUBLIME_PLATFORM_EXT} 
            -D ZIPFILE="${Clara_tar_output}" 
            -P "${CMAKE_CURRENT_SOURCE_DIR}/Deploy.cmake"
    DEPENDS
        ClaraPackage
    COMMENT
        "Deploying Clara"
    )
