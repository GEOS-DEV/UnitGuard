
set( unitguard_headers
     ConstexprAlgorithms.hpp
     Unit.hpp
     UnitGuard.hpp
     Quantity.hpp
   )

set( unitguard_sources
   )

set( unitguard_dependencies
   )

blt_add_library( NAME             unitguard
                 HEADERS          ${unitguard_headers}
                 DEPENDS_ON       ${unitguard_dependencies}
                )

target_include_directories( unitguard
                            INTERFACE
                            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
                            $<INSTALL_INTERFACE:include> )

install( FILES ${unitguard_headers}
         DESTINATION include )


install( TARGETS unitguard
         EXPORT unitguard
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib )

install( EXPORT unitguard
         DESTINATION lib/cmake/unitguard )

unitguard_add_code_checks( PREFIX unitguard
                           EXCLUDES "blt/*" )

if( UNITGUARD_ENABLE_UNIT_TESTS )
  add_subdirectory( unitTests )
endif()