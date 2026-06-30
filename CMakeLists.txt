# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "components/device" "components/Service" "main/Application" "main/taskManager") # ESP-IDF 的 CMake 默认只扫描 components/ 的第一层子文件夹
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SmartLedSendSensorToMQTTexternC)
