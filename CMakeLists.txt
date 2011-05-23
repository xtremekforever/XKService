cmake_minimum_required(VERSION 2.6)
project (xtremek-service)

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/ )

find_package ( PQXX )
find_package ( Boost )

include_directories( ./ common )

set ( common_SRCS
    common/database.cpp
    common/log.cpp
)

set ( xkservice_SRCS
    ${common_SRCS}
    service.cpp 
    main.cpp
)

ADD_EXECUTABLE( xtremek-service ${xkservice_SRCS} )

TARGET_LINK_LIBRARIES( xtremek-service ${PQXX_LIBRARY} ${Boost_LIBRARIES} )

install( TARGETS xtremek-service DESTINATION bin )

# Install the /etc/init.d script
install( PROGRAMS xtremek-service DESTINATION /etc/init.d/ )

