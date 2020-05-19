cmake_minimum_required(VERSION 3.14)
project(ServerPDS)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "--coverage")
set(CMAKE_PREFIX_PATH ${BOOST_LIBRARYDIR}\\cmake)
set(BOOST_ROOT "C:/boost_1_72_0")

set(Boost_DIR "C:/boost/lib/cmake/Boost-1.72.0")

find_package(Boost CONFIG REQUIRED COMPONENTS regex)
find_package(Boost CONFIG REQUIRED COMPONENTS thread)
find_package(Qt5Sql REQUIRED)
add_executable(ServerPDS main.cpp Server.cpp Server.h HandleRequest.cpp HandleRequest.h ManagementDB.cpp ManagementDB.h Symbol.cpp Symbol.h message.cpp message.h Room.cpp Room.h Participant.cpp Participant.h MessageSymbol.cpp MessageSymbol.h)

#target_link_libraries(DBMSProject ${Boost_LIBRARIES})  # that didn't work
list(APPEND LIBRARIES
        Qt5::Sql)
list(REMOVE_DUPLICATES LIBRARIES)
target_link_libraries(ServerPDS ${LIBRARIES})
target_link_libraries(ServerPDS Boost::thread)
target_link_libraries(ServerPDS Boost::regex)
if(MSVC OR MINGW)
    target_link_libraries(ServerPDS ws2_32 mswsock)
endif()
