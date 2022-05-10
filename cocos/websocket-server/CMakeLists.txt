cmake_minimum_required(VERSION 3.8)

project(ws-server CXX)


add_library(ws-server
    WebSocketServer.h
    WebSocketServer.cpp
)


target_link_libraries(ws-server ccbase uv websockets)

