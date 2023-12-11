cmake_minimum_required(VERSION 3.20)
project(embededOS C)

set(CMAKE_C_STANDARD 11)

add_executable(embededOS buffer.c
        DataStructureLibrary/hashtable.c
        DataStructureLibrary/hashtable.h
        DataStructureLibrary/linkedlist.c
        DataStructureLibrary/linkedlist.h
)

target_link_libraries(embededOS m)