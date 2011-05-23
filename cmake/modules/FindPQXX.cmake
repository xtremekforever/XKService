
find_library ( PQXX_LIBRARY
    NAMES libpqxx pqxx
    PATHS
        /usr/local/pgsql/lib
        /usr/local/lib
        /usr/lib/
        /usr/lib64/
    DOC "Location of libpqxx library"
    NO_DEFAULT_PATH
)

IF ( PQXX_LIBRARY )
    SET ( PQXX_FOUND TRUE )
ENDIF ( PQXX_LIBRARY)

IF ( PQXX_FOUND ) 
    IF ( NOT PQXX_FIND_QUIETLY )
        MESSAGE ( STATUS "Found PQXX library: ${PQXX_LIBRARY}" )
    ENDIF ( NOT PQXX_FIND_QUIETLY )
ELSE ( PQXX_FOUND )
   IF ( PQXX_FIND_REQUIRED )
      MESSAGE ( FATAL_ERROR "Could not find PQXX library" )
   ENDIF ( PQXX_FIND_REQUIRED )
ENDIF ( PQXX_FOUND )
