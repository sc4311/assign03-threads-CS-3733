cmake_minimum_required(VERSION 3.28)
project(assign03_v5_sch_threads_sync_80 C)

set(CMAKE_C_STANDARD 11)

add_executable(assign03_v5_sch_threads_sync_80 main.c
        scheduler.c
        scheduler.h)
