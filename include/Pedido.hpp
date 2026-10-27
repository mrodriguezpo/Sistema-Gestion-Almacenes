#pragma once
#include <string>

struct Pedido {
    int id;
    std::string descripcion;
    int prioridad;          // 1: Baja, 2: Media, 3: VIP/Crítica
    std::string estado;        // "En Espera", "Almacenado", "Despachado"
    std::string robotAsignado; // "Robot-1", "Robot-2", "N/A"

    bool operator<(const Pedido& otro) const {
        return prioridad < otro.prioridad; // Criterio Max-Heap
    }
};