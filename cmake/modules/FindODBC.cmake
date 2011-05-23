
find_library ( ODBC_LIBRARY
    NAMES odbc
    PATHS
        /usr/local/lib
        /usr/lib/
        /usr/lib64/
    DOC "Location of odbc library"
    NO_DEFAULT_PATH
)

IF ( ODBC_LIBRARY )
    SET ( ODBC_FOUND TRUE )
ENDIF ( ODBC_LIBRARY)

IF ( ODBC_FOUND )
    IF ( NOT ODBC_FIND_QUIETLY )      
        MESSAGE ( STATUS "Found ODBC library: ${ODBC_LIBRARY}" )
    ENDIF ( NOT ODBC_FIND_QUIETLY)
ELSE ( ODBC_FOUND )
   IF ( ODBC_FIND_REQUIRED )
      MESSAGE ( FATAL_ERROR "Could not find ODBC library" )
   ENDIF ( ODBC_FIND_REQUIRED )
ENDIF ( ODBC_FOUND )
