
find_library ( ODBCXX_LIBRARY
    NAMES libodbc++ odbc++
    PATHS
        /usr/local/lib
        /usr/lib/
        /usr/lib64/
    DOC "Location of odbc library"
    NO_DEFAULT_PATH
)

IF ( ODBCXX_LIBRARY )
    SET ( ODBCXX_FOUND TRUE )
ENDIF ( ODBCXX_LIBRARY)

IF ( ODBCXX_FOUND )
    IF ( NOT ODBCXX_FIND_QUIETLY )      
        MESSAGE ( STATUS "Found ODBC++ library: ${ODBCXX_LIBRARY}" )
    ENDIF ( NOT ODBCXX_FIND_QUIETLY)
ELSE ( ODBCXX_FOUND )
   IF ( ODBCXX_FIND_REQUIRED )
      MESSAGE ( FATAL_ERROR "Could not find ODBC++ library" )
   ENDIF ( ODBCXX_FIND_REQUIRED )
ENDIF ( ODBCXX_FOUND )
