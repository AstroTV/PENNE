SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-pthread)
add_executable(penne_ecu
        ecu.c
        helpers.c
        can.c
        crypto.c
        main.c)


# We need pthread_create
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(penne_ecu PRIVATE Threads::Threads)


message(STATUS "Looking for librt")
find_library(LIBRT rt)

if(LIBRT)
        message(STATUS "librt found")
        target_link_libraries(penne_ecu PRIVATE ${LIBRT} m crypto pthread)
else()
        message(SEND_ERROR "librt not found")
endif()

