cmake_minimum_required(VERSION 3.16)
project(PALDemod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets)
find_package(OpenCV REQUIRED)
find_package(iir)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(PALModDemod main.cpp view/atvview.cpp view/atvview.h
    model/settings.h
    model/settings.cpp
    view/atvsettingsview.h
    view/atvsettingsview.cpp
    model/atvsampleshandler.h
    model/atvsampleshandler.cpp
    model/atvvideogenerator.h
    model/atvvideogenerator.cpp)

target_link_libraries(PALModDemod PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
    iir::iir
)
