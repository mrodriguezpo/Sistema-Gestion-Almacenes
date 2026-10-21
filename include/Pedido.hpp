#ifndef PEDIDO_HPP
#define PEDIDO_HPP

#include <string>

/**
 * @struct Pedido
 * @brief Representa la entidad básica de un paquete dentro de la cadena logística del almacén.
 */
struct Pedido {
    int id;                     // Identificador único numérico del paquete
    std::string descripcion;    // Breve detalle del contenido de la carga
    int prioridad;              // Nivel de urgencia: 1 (Baja), 2 (Media), 3 (Crítica)
    std::string estado;         // Estado operativo: "En Espera", "Almacenado", "Despachado"
    std::string robotAsignado;  // Nombre del robot de transporte asignado ("Robot-X" o "N/A")

    /**
     * @brief Sobrecarga del operador de comparación '<' para la Cola de Prioridad (std::priority_queue).
     * @details Define que un pedido tiene 'menor prioridad' que otro si su valor entero es menor.
     *          Esto permite que la Max-Heap ubique automáticamente en la cima el pedido de mayor urgencia (3 > 2 > 1).
     */
    bool operator<(const Pedido& otro) const {
        return prioridad < otro.prioridad;
    }
};

#endif // PEDIDO_HPP