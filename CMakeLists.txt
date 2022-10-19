cmake_minimum_required(VERSION 3.10)
project(uefi-key-rpmb VERSION 1.0)

add_executable(poc poc.c)

target_include_directories(poc PUBLIC /usr/include/efi/aarch64)
target_link_libraries(poc teec)