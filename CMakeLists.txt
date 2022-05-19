cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
#set(CMAKE_CXX_FLAGS "-g -Wno-conversion-null -Wno-error=sign-conversion")
set(FMT_MASTER_PROJECT OFF)

project(chatroom)

find_package(fmt QUIET)
if(NOT ${fmt_FOUND})
	MESSAGE(STATUS "fmt was not found. Cloning from fmt git repo.")
	include(FetchContent)
	FetchContent_Declare(fmt
		GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		GIT_TAG master
	)
	FetchContent_MakeAvailable(fmt)
endif()

add_executable(chatroom 
	src/main.cpp
	src/MainWindow.cpp	
	src/util.cpp	
	src/RichEditWrapper.cpp
	src/LoginWindow.cpp
	src/ChatClient.cpp
	src/ChatServer.cpp
	src/ServerWindow.cpp
	src/NetworkMessages.cpp
)

target_include_directories(chatroom PUBLIC headers)
target_link_libraries(chatroom ws2_32 gdi32 fmt::fmt)
