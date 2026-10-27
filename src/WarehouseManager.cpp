/**
 * @file WarehouseManager.cpp
 * @brief Implementación de la clase WarehouseManager para la gestión de logística de almacén.
 * @details Administra las estructuras de datos principales: Max-Heap (cola de recepción),
 *          Pila LIFO (rack de almacenamiento), Flota de Robots (Round-Robin) y la
 *          persistencia en disco (CSV) mediante C++17.
 */

#include "WarehouseManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem> // C++17: Para verificar y crear directorios automáticamente

namespace fs = std::filesystem;

/**
 * @brief Constructor de la clase WarehouseManager.
 * @param cantidadRobots Número inicial de robots en la flota.
 * @param capacidadMaximaRack Capacidad límite de almacenamiento del Rack.
 * @param capMaxRobot Capacidad máxima de entregas por tanda de cada robot.
 */
WarehouseManager::WarehouseManager(int cantidadRobots, int capacidadMaximaRack, int capMaxRobot)
    : turnoRobotActual(0), totalRobots(cantidadRobots), capacidadRack(capacidadMaximaRack), capacidadRobotDefault(capMaxRobot) {

    // Inicialización e inserción de la flota robótica
    for (int i = 1; i <= totalRobots; i++) {
        Robot r;
        r.nombre = "Robot-" + std::to_string(i);
        r.capacidadMaxima = capacidadRobotDefault;
        r.entregasActuales = 0;
        r.entregasTotalesAcumuladas = 0;
        r.estado = "Disponible";
        flotaRobots.push_back(r);
    }
}

/**
 * @brief Verifica si un identificador numérico de pedido ya existe en el sistema.
 * @param id Identificador a consultar.
 * @return true si el ID ya fue registrado previamente; false en caso contrario.
 */
bool WarehouseManager::existeID(int id) const {
    return idsExistentes.find(id) != idsExistentes.end();
}

/**
 * @brief Registra un nuevo pedido en la zona de recepción.
 * @param id Identificador único del pedido.
 * @param descripcion Detalle del contenido del paquete.
 * @param prioridad Nivel de urgencia (1: Baja, 2: Media, 3: VIP/Crítica).
 * @return true si la inserción fue exitosa; false si el ID ya existía.
 */
bool WarehouseManager::registrarPedido(int id, const std::string& descripcion, int prioridad) {
    if (existeID(id)) return false;

    // Creación de la estructura Pedido e inserción en recepción, catálogo e índice
    Pedido p{ id, descripcion, prioridad, "En Espera", "N/A" };
    colaRecepcion.push(p);
    catalogoGlobal.push_back(p);
    idsExistentes.insert(id);

    guardarEnCSV(); // Guarda inmediatamente en disco
    return true;
}

/**
 * @brief Evalúa si el almacenamiento de un paquete requiere confirmación por conflicto de prioridad.
 * @param[out] prioEntrante Prioridad del paquete en la cima de la recepción.
 * @param[out] prioCimaRack Prioridad del paquete en la cima del Rack.
 * @return true si la prioridad del paquete entrante es menor que la del paquete en el Rack.
 */
bool WarehouseManager::requiereAlertaInteligente(int& prioEntrante, int& prioCimaRack) const {
    if (colaRecepcion.empty() || rackAlmacenamiento.empty()) return false;

    prioEntrante = colaRecepcion.top().prioridad;
    prioCimaRack = rackAlmacenamiento.top().prioridad;

    return (prioEntrante < prioCimaRack);
}

/**
 * @brief Traslada el paquete más urgente desde la recepción hacia el Rack LIFO.
 * @param forzar Omite la advertencia de prioridad si se establece en true.
 * @return true si la operación se completó con éxito; false si falla o se cancela.
 */
bool WarehouseManager::procesarAlmacenamiento(bool forzar) {
    if (colaRecepcion.empty()) return false;
    if (static_cast<int>(rackAlmacenamiento.size()) >= capacidadRack) return false;

    int pEntrante = 0, pCima = 0;
    if (!forzar && requiereAlertaInteligente(pEntrante, pCima)) {
        return false;
    }

    // Extracción de la cola de recepción
    Pedido p = colaRecepcion.top();
    colaRecepcion.pop();

    // Actualización de estado y apilamiento en el Rack
    p.estado = "Almacenado";
    rackAlmacenamiento.push(p);
    historialUndo.push(p);

    // Sincronización con el catálogo global
    for (auto& item : catalogoGlobal) {
        if (item.id == p.id) {
            item.estado = p.estado;
            break;
        }
    }

    guardarEnCSV();
    return true;
}

/**
 * @brief Despacha el paquete en la cima del Rack hacia un robot disponible (Round-Robin).
 * @return true si el paquete fue despachado exitosamente; false si no hay paquetes o robots libres.
 */
bool WarehouseManager::despacharARobot() {
    if (rackAlmacenamiento.empty() || flotaRobots.empty()) return false;

    // Búsqueda del siguiente robot disponible en el ciclo Round-Robin
    int intentos = 0;
    while (intentos < totalRobots && flotaRobots[turnoRobotActual].estado == "En Recarga") {
        turnoRobotActual = (turnoRobotActual + 1) % totalRobots;
        intentos++;
    }

    if (flotaRobots[turnoRobotActual].estado == "En Recarga") return false;

    // Extracción del paquete de la cima del Rack
    Pedido p = rackAlmacenamiento.top();
    rackAlmacenamiento.pop();

    // Asignación de carga y actualización del contador del robot
    Robot& bot = flotaRobots[turnoRobotActual];
    p.estado = "Despachado";
    p.robotAsignado = bot.nombre;

    bot.entregasActuales++;
    bot.entregasTotalesAcumuladas++;
    if (bot.entregasActuales >= bot.capacidadMaxima) {
        bot.estado = "En Recarga";
    }

    // Actualización de la entrada equivalente en el catálogo global
    for (auto& item : catalogoGlobal) {
        if (item.id == p.id) {
            item.estado = p.estado;
            item.robotAsignado = p.robotAsignado;
            break;
        }
    }

    // Rotación de turno al siguiente robot
    turnoRobotActual = (turnoRobotActual + 1) % totalRobots;
    guardarEnCSV();
    return true;
}

/**
 * @brief Ejecuta múltiples despachos consecutivos en una sola llamada.
 * @param cantidad Número de paquetes a despachar.
 * @return Número total de paquetes efectivamente procesados.
 */
int WarehouseManager::despacharPorLotes(int cantidad) {
    int procesados = 0;
    for (int i = 0; i < cantidad; i++) {
        if (despacharARobot()) procesados++;
        else break;
    }
    return procesados;
}

/**
 * @brief Deshace la última transferencia hacia el Rack, retornando el paquete a recepción.
 * @return true si se logró deshacer la acción; false si la pila de undo está vacía.
 */
bool WarehouseManager::deshacerAccion() {
    if (historialUndo.empty()) return false;
    Pedido p = historialUndo.top();
    historialUndo.pop();

    // Verificación de coherencia en la cima del Rack
    if (!rackAlmacenamiento.empty() && rackAlmacenamiento.top().id == p.id) {
        rackAlmacenamiento.pop();
        p.estado = "En Espera";
        colaRecepcion.push(p);

        // Restauración en el catálogo global
        for (auto& item : catalogoGlobal) {
            if (item.id == p.id) {
                item.estado = "En Espera";
                item.robotAsignado = "N/A";
                break;
            }
        }
        guardarEnCSV();
        return true;
    }
    return false;
}

/**
 * @brief Consulta el expediente de un pedido y calcula su nivel de profundidad en el Rack.
 * @param id Identificador del pedido a rastrear.
 * @param[out] pedidoEncontrado Estructura con la información copiada del pedido.
 * @param[out] profundidadRack Distancia desde la cima del Rack (1 si está en la cima).
 * @return true si el paquete existe; false en caso contrario.
 */
bool WarehouseManager::rastrearPedido(int id, Pedido& pedidoEncontrado, int& profundidadRack) const {
    profundidadRack = -1;
    bool encontrado = false;

    // Búsqueda del expediente en el catálogo global
    for (const auto& item : catalogoGlobal) {
        if (item.id == id) {
            pedidoEncontrado = item;
            encontrado = true;
            break;
        }
    }

    if (!encontrado) return false;

    // Si el pedido está almacenado, recorre una copia temporal de la pila para hallar la profundidad
    if (pedidoEncontrado.estado == "Almacenado") {
        std::stack<Pedido> temp = rackAlmacenamiento;
        int nivel = 1;
        while (!temp.empty()) {
            if (temp.top().id == id) {
                profundidadRack = nivel;
                break;
            }
            temp.pop();
            nivel++;
        }
    }

    return true;
}

/**
 * @brief Genera una cantidad de paquetes de prueba con IDs aleatorios de 4 dígitos.
 * @param cantidad Número de paquetes simulados a crear.
 */
void WarehouseManager::generarCargaSimulada(int cantidad) {
    int creados = 0;
    while (creados < cantidad) {
        int idAleatorio = 1000 + (rand() % 9000);
        if (!existeID(idAleatorio)) {
            int prio = (rand() % 3) + 1;
            registrarPedido(idAleatorio, "Paquete Simulado #" + std::to_string(idAleatorio), prio);
            creados++;
        }
    }
}

/**
 * @brief Calcula el porcentaje de ocupación actual del Rack respecto a su capacidad máxima.
 * @return Flotante representando el porcentaje (0.0f a 100.0f).
 */
float WarehouseManager::getPorcentajeOcupacionRack() const {
    if (capacidadRack == 0) return 0.0f;
    return (static_cast<float>(rackAlmacenamiento.size()) / capacidadRack) * 100.0f;
}

/**
 * @brief Obtiene el desglose de paquetes por nivel de prioridad en el catálogo global.
 * @param[out] prio1 Conteo de pedidos con prioridad Baja (1).
 * @param[out] prio2 Conteo de pedidos con prioridad Media (2).
 * @param[out] prio3 Conteo de pedidos con prioridad VIP (3).
 */
void WarehouseManager::getDistribucionPrioridades(int& prio1, int& prio2, int& prio3) const {
    prio1 = 0; prio2 = 0; prio3 = 0;
    for (const auto& p : catalogoGlobal) {
        if (p.prioridad == 1) prio1++;
        else if (p.prioridad == 2) prio2++;
        else if (p.prioridad == 3) prio3++;
    }
}

/**
 * @brief Reconfigura las restricciones físicas del sistema (robots y almacenamiento).
 * @param nuevosRobots Nueva cantidad total de unidades en la flota.
 * @param nuevaCapMaxRobot Nuevo límite de entregas por tanda por robot.
 * @param nuevaCapMaxRack Nueva capacidad tope de la pila del Rack.
 */
void WarehouseManager::reconfigurarParametros(int nuevosRobots, int nuevaCapMaxRobot, int nuevaCapMaxRack) {
    totalRobots = nuevosRobots;
    capacidadRobotDefault = nuevaCapMaxRobot;
    capacidadRack = nuevaCapMaxRack;

    // Reestructuración completa de la flota robótica
    flotaRobots.clear();
    for (int i = 1; i <= totalRobots; i++) {
        Robot r;
        r.nombre = "Robot-" + std::to_string(i);
        r.capacidadMaxima = capacidadRobotDefault;
        r.entregasActuales = 0;
        r.entregasTotalesAcumuladas = 0;
        r.estado = "Disponible";
        flotaRobots.push_back(r);
    }
    turnoRobotActual = 0;
    guardarEnCSV();
}

/**
 * @brief Restablece los robots agotados ("En Recarga") a su estado "Disponible".
 * @return Cantidad de robots que fueron recargados.
 */
int WarehouseManager::recargarRobotsFueraDeServicio() {
    int recargados = 0;
    for (auto& bot : flotaRobots) {
        if (bot.estado == "En Recarga") {
            bot.entregasActuales = 0;
            bot.estado = "Disponible";
            recargados++;
        }
    }
    return recargados;
}

/**
 * @brief Limpia la memoria activa del almacén y elimina el archivo CSV de datos.
 * @param rutaArchivo Ruta del archivo plano a eliminar de disco.
 * @return true indicando que la memoria se vació e intentó eliminar el archivo.
 */
bool WarehouseManager::eliminarHistorialYCSV(const std::string& rutaArchivo) {
    while (!colaRecepcion.empty()) colaRecepcion.pop();
    while (!rackAlmacenamiento.empty()) rackAlmacenamiento.pop();
    while (!historialUndo.empty()) historialUndo.pop();
    catalogoGlobal.clear();
    idsExistentes.clear();

    for (auto& bot : flotaRobots) {
        bot.entregasActuales = 0;
        bot.entregasTotalesAcumuladas = 0;
        bot.estado = "Disponible";
    }

    std::remove(rutaArchivo.c_str());
    return true;
}

/**
 * @brief Serializa y escribe el catálogo de inventario y configuración en un archivo CSV.
 * @details Crea de manera automática la carpeta 'data/' si no existe previa en el disco.
 * @param rutaArchivo Ruta de destino del archivo CSV.
 */
void WarehouseManager::guardarEnCSV(const std::string& rutaArchivo) {
    try {
        fs::path pathObj(rutaArchivo);
        if (pathObj.has_parent_path()) {
            fs::create_directories(pathObj.parent_path()); // Crea la carpeta 'data/' si no existe
        }
    }
    catch (...) {}

    std::ofstream archivo(rutaArchivo);
    if (!archivo.is_open()) return;

    // Escritura de la cabecera de configuración y metadatos
    archivo << "#CONFIG," << totalRobots << "," << capacidadRobotDefault << "," << capacidadRack << "\n";
    archivo << "id,descripcion,prioridad,estado,robotAsignado\n";

    // Escritura secuencial de cada registro en el catálogo
    for (const auto& p : catalogoGlobal) {
        archivo << p.id << "," << p.descripcion << "," << p.prioridad << "," << p.estado << "," << p.robotAsignado << "\n";
    }
    archivo.close();
}

/**
 * @brief Carga y deserializa el inventario y configuración desde un archivo CSV.
 * @param rutaArchivo Ruta del archivo CSV a leer.
 * @return true si el archivo existe y pudo procesarse; false si no se encontró.
 */
bool WarehouseManager::cargarDesdeCSV(const std::string& rutaArchivo) {
    std::ifstream archivo(rutaArchivo);
    if (!archivo.is_open()) return false;

    std::string linea;
    while (!colaRecepcion.empty()) colaRecepcion.pop();
    while (!rackAlmacenamiento.empty()) rackAlmacenamiento.pop();
    while (!historialUndo.empty()) historialUndo.pop();
    catalogoGlobal.clear();
    idsExistentes.clear();

    while (std::getline(archivo, linea)) {
        if (linea.empty()) continue;

        // Procesamiento de metadatos de configuración
        if (linea.rfind("#CONFIG", 0) == 0) {
            std::stringstream ss(linea);
            std::string tag, sRobots, sCapBot, sCapRack;
            std::getline(ss, tag, ',');
            std::getline(ss, sRobots, ',');
            std::getline(ss, sCapBot, ',');
            std::getline(ss, sCapRack, ',');

            if (!sRobots.empty()) totalRobots = std::stoi(sRobots);
            if (!sCapBot.empty()) capacidadRobotDefault = std::stoi(sCapBot);
            if (!sCapRack.empty()) capacidadRack = std::stoi(sCapRack);

            flotaRobots.clear();
            for (int i = 1; i <= totalRobots; i++) {
                Robot r;
                r.nombre = "Robot-" + std::to_string(i);
                r.capacidadMaxima = capacidadRobotDefault;
                flotaRobots.push_back(r);
            }
            continue;
        }

        if (linea.rfind("id,", 0) == 0) continue;

        // Extracción de cada campo del pedido mediante comas
        std::stringstream ss(linea);
        std::string sId, desc, sPrio, estado, robot;
        std::getline(ss, sId, ',');
        std::getline(ss, desc, ',');
        std::getline(ss, sPrio, ',');
        std::getline(ss, estado, ',');
        std::getline(ss, robot, ',');

        if (sId.empty()) continue;

        Pedido p;
        p.id = std::stoi(sId);
        p.descripcion = desc;
        p.prioridad = std::stoi(sPrio);
        p.estado = estado;
        p.robotAsignado = robot;

        catalogoGlobal.push_back(p);
        idsExistentes.insert(p.id);

        // Reconstrucción de la cola Max-Heap y Pila LIFO según su estado guardado
        if (estado == "En Espera") colaRecepcion.push(p);
        else if (estado == "Almacenado") rackAlmacenamiento.push(p);
    }
    archivo.close();
    return true;
}

/**
 * @brief Exporta una copia de la cola de recepción ordenada en formato vector.
 * @return Vector conteniendo los pedidos en recepción en orden de prioridad.
 */
std::vector<Pedido> WarehouseManager::getColaRecepcionVector() const {
    std::priority_queue<Pedido> temp = colaRecepcion;
    std::vector<Pedido> res;
    while (!temp.empty()) {
        res.push_back(temp.top());
        temp.pop();
    }
    return res;
}

/**
 * @brief Exporta una copia del Rack de almacenamiento en formato vector.
 * @return Vector conteniendo los pedidos ordenados desde la cima hacia el fondo.
 */
std::vector<Pedido> WarehouseManager::getRackVector() const {
    std::stack<Pedido> temp = rackAlmacenamiento;
    std::vector<Pedido> res;
    while (!temp.empty()) {
        res.push_back(temp.top());
        temp.pop();
    }
    return res;
}