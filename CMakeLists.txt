cmake_minimum_required(VERSION 3.28)
project(lcr)

set(CMAKE_CXX_STANDARD 17)

add_executable(lcr src/main.cpp
        include/game.h
        include/player.h
        include/dice.h
        include/helpers.h
        include/result.h
        include/output.h
)
