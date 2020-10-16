cmake_minimum_required(VERSION 3.14)
project(ServerPDS)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "--coverage")
set(CMAKE_PREFIX_PATH ${BOOST_LIBRARYDIR}\\cmake)
set(BOOST_ROOT "C:/boost_1_72_0")
set(Boost_DIR "C:/boost/lib/cmake/Boost-1.72.0")

find_package(Boost CONFIG REQUIRED COMPONENTS regex)
find_package(Boost CONFIG REQUIRED COMPONENTS thread)
find_package(Boost CONFIG REQUIRED COMPONENTS fileSystem)
find_package(Boost CONFIG REQUIRED COMPONENTS random)
find_package(Qt5Sql REQUIRED)

add_executable(ServerPDS main.cpp SocketManager.cpp Headers/SocketManager.h HandleRequest.cpp Headers/HandleRequest.h ManagementDB.cpp Headers/ManagementDB.h Symbol.cpp Headers/Symbol.h Message.cpp Headers/Message.h Server.cpp Headers/Server.h Participant.cpp Headers/Participant.h MessageSymbol.cpp Headers/MessageSymbol.h Libs/md5.cpp Libs/md5.h Headers/Style.h Style.cpp)

#target_link_libraries(DBMSProject ${Boost_LIBRARIES})  # that didn't work
list(APPEND LIBRARIES
        Qt5::Sql)
list(REMOVE_DUPLICATES LIBRARIES)
target_link_libraries(ServerPDS ${LIBRARIES})
target_link_libraries(ServerPDS Boost::thread)
target_link_libraries(ServerPDS Boost::regex)
target_link_libraries(ServerPDS Boost::filesystem)
target_link_libraries(ServerPDS Boost::random)

if(MSVC OR MINGW)
    target_link_libraries(ServerPDS ws2_32 mswsock)
endif()
