cmake_minimum_required(VERSION 3.16)
project(WukongTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent)

# QXlsx 子模块
set(QXLSX_LIB_TYPE STATIC)
add_subdirectory(../QXlsx/QXlsx ../QXlsx_build)

set(TEST_SOURCES
    test_real_data.cpp
    ../src/Calculation/FreightCalculator.cpp
    ../src/Calculation/SimdCalculator.cpp
    ../src/Calculation/RuleManager.cpp
    ../src/Calculation/ThreadPool.cpp
    ../src/DataModel/OrderData.cpp
    ../src/DataModel/PriceTable.cpp
    ../src/DataModel/CalculationRule.cpp
    ../src/DataModel/MappingTemplate.cpp
    ../src/Excel/ExcelEngine.cpp
    ../src/Excel/ExcelImporter.cpp
    ../src/Excel/ExcelExporter.cpp
    ../src/Utils/ProvinceUtils.cpp
    ../src/Utils/Logger.cpp
)

add_executable(test_real_data ${TEST_SOURCES})

target_link_libraries(test_real_data
    Qt6::Core
    Qt6::Concurrent
    QXlsx::QXlsx
)

target_include_directories(test_real_data
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
