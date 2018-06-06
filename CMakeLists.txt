# NextCash Base

cmake_minimum_required(VERSION 3.4.1)


include_directories( src/base
                     src/io
                     src/crypto
                     src/dev
                     . )

add_library( nextcash STATIC SHARED
             src/base/distributed_vector.cpp
             src/base/hash.cpp
             src/base/hash_data_set.cpp
             src/base/log.cpp
             src/base/mutex.cpp
             src/base/string.cpp
             src/base/thread.cpp
             src/crypto/digest.cpp
             src/crypto/encrypt.cpp
             src/dev/profiler.cpp
             src/io/buffer.cpp
             src/io/email.cpp
             src/io/network.cpp
             src/io/stream.cpp
             nextcash_test.cpp )

# Link Android Log Library
find_library( log-lib log )
target_link_libraries( nextcash ${log-lib} )

set_property( TARGET nextcash PROPERTY CXX_STANDARD 11 )
set_property( TARGET nextcash PROPERTY CXX_STANDARD_REQUIRED ON )
