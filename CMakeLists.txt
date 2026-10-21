# Versión mínima requerida de CMake
cmake_minimum_required(VERSION 3.8)

# Nombre del proyecto
project(SistemaGestionAlmacenes)

# Estandar C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Busqueda de arcivos .hpp
include_directories(include)

# Aechivos a complicar para crear el ejecutable
add_executable(SmartWarehouse src/main.cpp src/WarehouseManager.cpp)