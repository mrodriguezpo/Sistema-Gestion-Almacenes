cmake_minimum_required(VERSION 3.16)
project(SmartWarehouse CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Incluir encabezados del proyecto y de Raylib
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/raylib/include
)

# 2. Ruta a la librería precompilada (.lib)
link_directories(
    ${CMAKE_SOURCE_DIR}/vendor/raylib/lib
)

# 3. Compilar los archivos fuente
# (Si tienes un archivo src/Pedido.cpp u otro .cpp, agrégalo también aquí)
add_executable(SmartWarehouse
    src/main.cpp
    src/WarehouseManager.cpp
)

# 4. Enlazar Raylib y librerías del sistema de Windows (incluyendo OpenGL)
target_link_libraries(SmartWarehouse PRIVATE
    raylib
    opengl32
    winmm
    gdi32
    user32
    shell32
)