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

set(PROJECT_NAME "map-service")
set(PROJECT_VERSION "1.0")
set(PROJECT_PRETTY_NAME "SE")
set(PROJECT_DESCRIPTION "Sample application for homescreen and windowmanager")
set(PROJECT_URL "")
set(CMAKE_CXX_FLAGS "-Wall -fpermissive")

pkg_check_modules(AFB REQUIRED json-c libafbwsc libsystemd)

#source directory
aux_source_directory(src DIR_SRCS)

#head file path
include_directories(include)

#set extern libraries
SET(LIBRARIES
    libEGL.so
    libGLESv2.so
    libm.so
    libwayland-egl.so
    libwayland-client.so
    libjson-c.so
    librt.so
    libpthread.so
)

#add executable file
add_executable(simple-egl ${DIR_SRCS})

#add link library
TARGET_LINK_LIBRARIES(simple-egl ${LIBRARIES} ${AFB_LIBRARIES})

add_custom_command(TARGET simple-egl POST_BUILD
   COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/bin
   COMMAND cp -f ${PROJECT_BINARY_DIR}/map-service/ui/simple-egl ${PROJECT_BINARY_DIR}/package/root/bin
   )
