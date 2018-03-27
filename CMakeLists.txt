# NextCash Base

cmake_minimum_required(VERSION 3.4.1)

add_definitions( -DANDROID -DJAVA )


include_directories( src/base
                     src/io
                     src/crypto
                     src/dev )

add_library( nextcash STATIC SHARED
             src/base/distributed_vector.cpp
             src/base/hash.cpp
             src/base/hash_data_set.cpp
             src/base/log.cpp
             src/base/mutex.cpp
             src/base/string.cpp
             src/base/thread.cpp
             src/crypto/digest.cpp
             src/dev/profiler.cpp
             src/io/buffer.cpp
             src/io/email.cpp
             src/io/network.cpp
             src/io/stream.cpp )