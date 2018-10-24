#
# Copyright (c) 2017 TOYOTA MOTOR CORPORATION
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set(TARGET_PUB_LIB map-service-binding)
set(TARGET_LOCAL_LIB map-local-binding)

pkg_check_modules(AFB REQUIRED afb-daemon)
pkg_check_modules(SD REQUIRED libsystemd>=222)

### Create public binding

add_library(${TARGET_PUB_LIB} MODULE
    map-public-binding.cpp)

target_include_directories(${TARGET_PUB_LIB}
    PRIVATE
    ${AFB_INCLUDE_DIRS}
    ${SD_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_PUB_LIB}
    PRIVATE
        ${AFB_LIBRARIES}
        ${SD_LIBRARIES}
)

target_compile_definitions(${TARGET_PUB_LIB}
    PRIVATE
        _GNU_SOURCE
)

target_compile_options(${TARGET_PUB_LIB}
    PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-comment)

set_target_properties(${TARGET_PUB_LIB}
    PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON

        C_EXTENSIONS OFF
        C_STANDARD 99
        C_STANDARD_REQUIRED ON

        LINK_FLAGS "-Wl,-rpath,'$ORIGIN'"
)

### Create public binding

add_library(${TARGET_LOCAL_LIB} MODULE
    map-public-binding.cpp)

target_include_directories(${TARGET_LOCAL_LIB}
    PRIVATE
    ${AFB_INCLUDE_DIRS}
    ${SD_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_LOCAL_LIB}
    PRIVATE
        ${AFB_LIBRARIES}
        ${SD_LIBRARIES}
)

target_compile_definitions(${TARGET_LOCAL_LIB}
    PRIVATE
        _GNU_SOURCE
)

target_compile_options(${TARGET_LOCAL_LIB}
    PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-comment)

set_target_properties(${TARGET_LOCAL_LIB}
    PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON

        C_EXTENSIONS OFF
        C_STANDARD 99
        C_STANDARD_REQUIRED ON

        LINK_FLAGS "-Wl,-rpath,'$ORIGIN'"
)

add_custom_command(TARGET ${TARGET_PUB_LIB} POST_BUILD
   COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/lib
   COMMAND cp -f ${PROJECT_BINARY_DIR}/map-service/binding/lib${TARGET_PUB_LIB}.so ${PROJECT_BINARY_DIR}/package/root/lib
   )

add_custom_command(TARGET ${TARGET_LOCAL_LIB} POST_BUILD
   COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/lib
   COMMAND cp -f ${PROJECT_BINARY_DIR}/map-service/binding/lib${TARGET_LOCAL_LIB}.so ${PROJECT_BINARY_DIR}/package/root/lib
   )