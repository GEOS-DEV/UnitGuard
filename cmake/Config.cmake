if( ENABLE_ADDR2LINE )
    if ( NOT DEFINED ADDR2LINE_EXEC )
        set( ADDR2LINE_EXEC /usr/bin/addr2line CACHE PATH "" )
    endif()

    if ( NOT EXISTS ${ADDR2LINE_EXEC} )
        message( FATAL_ERROR "The addr2line executable does not exist: ${ADDR2LINE_EXEC}" )
    endif()

    set( UNITGUARD_ADDR2LINE_EXEC ${ADDR2LINE_EXEC} )
endif()


configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/src/UnitGuardConfig.hpp.in
                ${CMAKE_BINARY_DIR}/include/UnitGuardConfig.hpp )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/src/UnitGuardConfig.hpp.in
                ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/UnitGuardConfig.hpp )

# Install the generated header.
install( FILES ${CMAKE_BINARY_DIR}/include/UnitGuardConfig.hpp
         DESTINATION include )

set(UNITGUARD_INSTALL_INCLUDE_DIR "include" CACHE STRING "")
set(UNITGUARD_INSTALL_CONFIG_DIR "lib" CACHE STRING "")
set(UNITGUARD_INSTALL_LIB_DIR "lib" CACHE STRING "")
set(UNITGUARD_INSTALL_BIN_DIR "bin" CACHE STRING "")
set(UNITGUARD_INSTALL_CMAKE_MODULE_DIR "${UNITGUARD_INSTALL_CONFIG_DIR}/cmake" CACHE STRING "")
set(UNITGUARD_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "" FORCE)


include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/unitguard-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/unitguard-config.cmake
  INSTALL_DESTINATION
    ${UNITGUARD_INSTALL_CONFIG_DIR}
  PATH_VARS
    UNITGUARD_INSTALL_INCLUDE_DIR
    UNITGUARD_INSTALL_LIB_DIR
    UNITGUARD_INSTALL_BIN_DIR
    UNITGUARD_INSTALL_CMAKE_MODULE_DIR
  )


install( FILES ${CMAKE_CURRENT_BINARY_DIR}/unitguard-config.cmake
         DESTINATION share/unitguard/cmake/)