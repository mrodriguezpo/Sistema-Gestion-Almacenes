#ifndef WAREHOUSEMANAGER_HPP
#define WAREHOUSEMANAGER_HPP

#include "Pedido.hpp"
#include <queue>
#include <stack>
#include <vector>
#include <string>
#include <unordered_set>

/**
 * @struct Robot
 * @brief Representa un agente físico de transporte con capacidad y autonomía finita.
 */
struct Robot {
    std::string nombre;                 // Nombre identificador (Ej: "Robot-1")
    int entregasActuales = 0;          // Cantidad de envíos realizados en la tanda actual
    int capacidadMaxima = 3;           // Límite máximo de envíos antes de requerir mantenimiento
    std::string estado = "Disponible";  // Estado operativo: "Disponible" o "En Recarga"
};

/**
 * @class WarehouseManager
 * @brief Controlador principal del sistema Smart Warehouse.
 * @details Administra las estructuras de datos, la lógica de negocio, la flota de robots y la persistencia en disco.
 */
class WarehouseManager {
private:
    // --- ESTRUCTURAS DE DATOS PRINCIPALES ---
    std::priority_queue<Pedido> colaRecepcion;  // Max-Heap: Ordena pedidos por prioridad (3: Crítica > 1: Baja)
    std::stack<Pedido> rackAlmacenamiento;      // Pila LIFO: Simula el rack de almacenamiento vertical
    std::stack<Pedido> historialUndo;            // Pila LIFO: Guarda el historial para la reversión de acciones
    std::vector<Pedido> catalogoGlobal;          // Vector dinámico: Almacena la totalidad de pedidos registrados
    std::vector<Robot> flotaRobots;              // Vector dinámico: Mantiene el estado de cada robot

    // --- TABLA HASH PARA BÚSQUEDA RÁPIDA O(1) ---
    std::unordered_set<int> idsExistentes;      // Conjunto Hash: Permite verificar la existencia de IDs en tiempo O(1)

    // --- PARÁMETROS DEL SISTEMA ---
    int turnoRobotActual;       // Índice del robot en turno para la distribución circular (Round-Robin)
    int totalRobots;            // Cantidad total de robots configurados
    int capacidadRack;          // Límite físico máximo de espacio en el rack
    int capacidadRobotDefault;   // Límite de entregas predeterminado por robot

    /**
     * @brief Actualiza la flota de robots ante cambios de configuración manteniendo el estado previo.
     */
    void actualizarFlotaRobots(int nuevaCantidad, int nuevaCapacidadRobot);

public:
    /**
     * @brief Constructor con valores de configuración iniciales por defecto.
     */
    WarehouseManager(int cantidadRobots = 3, int capacidadMaximaRack = 10, int capMaxRobot = 3);

    /**
     * @brief Verifica si un ID ya existe en el sistema.
     * @return Complejidad temporal O(1) gracias a std::unordered_set.
     */
    bool existeID(int id) const;

    // --- OPERACIONES PRINCIPALES DEL ALMACÉN ---
    void registrarPedido(int id, const std::string& descripcion, int prioridad);
    void procesarAlmacenamiento();
    void despacharARobot();
    void deshacerAccion();
    void reconfigurarParametros(int nuevosRobots, int nuevaCapacidadRack, int nuevaCapacidadRobot);

    // --- MANTENIMIENTO DE FLOTA ---
    void recargarRobotsFueraDeServicio();

    // --- REPORTES Y SIMULACIÓN ---
    void rastrearPedido(int id) const;
    void generarCargaSimulada(int cantidad);
    void mostrarDashboardKPI() const;
    void despacharPorLotes(int cantidad);
    void mostrarInventario() const;
    void registrarLog(const std::string& mensaje) const;

    // --- GETTERS DE CONFIGURACIÓN ---
    int getTotalRobots() const { return totalRobots; }
    int getCapacidadRack() const { return capacidadRack; }
    int getCapacidadRobotDefault() const { return capacidadRobotDefault; }

    // --- MÓDULO DE PERSISTENCIA (CSV) ---
    void guardarEnCSV(const std::string& rutaArchivo);
    bool cargarDesdeCSV(const std::string& rutaArchivo);
    bool borrarArchivoCSV(const std::string& rutaArchivo);
};

#endif // WAREHOUSEMANAGER_HPP