#pragma once
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <unordered_set>
#include "Pedido.hpp"

struct Robot {
    std::string nombre;
    int entregasActuales = 0;
    int capacidadMaxima = 3;
    int entregasTotalesAcumuladas = 0; // KPI acumulativo
    std::string estado = "Disponible";  // "Disponible" o "En Recarga"
};

class WarehouseManager {
private:
    std::priority_queue<Pedido> colaRecepcion;
    std::stack<Pedido> rackAlmacenamiento;
    std::stack<Pedido> historialUndo;
    std::vector<Pedido> catalogoGlobal;
    std::vector<Robot> flotaRobots;
    std::unordered_set<int> idsExistentes;

    int turnoRobotActual;
    int totalRobots;
    int capacidadRack;
    int capacidadRobotDefault;

public:
    WarehouseManager(int cantidadRobots = 3, int capacidadMaximaRack = 10, int capMaxRobot = 3);

    // [1] Registrar Pedido
    bool existeID(int id) const;
    bool registrarPedido(int id, const std::string& descripcion, int prioridad);

    // [2] Almacenamiento + Alerta Inteligente
    bool requiereAlertaInteligente(int& prioEntrante, int& prioCimaRack) const;
    bool procesarAlmacenamiento(bool forzar = false);

    // [3] Despacho Individual & [4] Despacho por Lotes
    bool despacharARobot();
    int despacharPorLotes(int cantidad);

    // [5] Reversión (Undo)
    bool deshacerAccion();

    // [6] Tracking por ID
    bool rastrearPedido(int id, Pedido& pedidoEncontrado, int& profundidadRack) const;

    // [8] Simulación Aleatoria
    void generarCargaSimulada(int cantidad);

    // [9] Métricas KPI
    float getPorcentajeOcupacionRack() const;
    void getDistribucionPrioridades(int& prio1, int& prio2, int& prio3) const;

    // [10] Configuración
    void reconfigurarParametros(int nuevosRobots, int nuevaCapMaxRobot, int nuevaCapMaxRack);

    // [11] Mantenimiento
    int recargarRobotsFueraDeServicio();

    // [12] Eliminar Historial
    bool eliminarHistorialYCSV(const std::string& rutaArchivo = "data/inventario.csv");

    // [13] Persistencia CSV
    void guardarEnCSV(const std::string& rutaArchivo = "data/inventario.csv");
    bool cargarDesdeCSV(const std::string& rutaArchivo = "data/inventario.csv");

    // Getters auxiliares
    int getTotalRobots() const { return totalRobots; }
    int getCapacidadRack() const { return capacidadRack; }
    int getCapacidadRobotDefault() const { return capacidadRobotDefault; }
    std::vector<Pedido> getColaRecepcionVector() const;
    std::vector<Pedido> getRackVector() const;
    const std::vector<Pedido>& getCatalogoGlobal() const { return catalogoGlobal; }
    const std::vector<Robot>& getFlotaRobots() const { return flotaRobots; }
    int getCantidadEnRecepcion() const { return static_cast<int>(colaRecepcion.size()); }
    int getCantidadEnRack() const { return static_cast<int>(rackAlmacenamiento.size()); }
};