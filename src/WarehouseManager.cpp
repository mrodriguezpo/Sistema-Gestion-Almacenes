#include "../include/WarehouseManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cstdio>
#include <limits>

WarehouseManager::WarehouseManager(int cantidadRobots, int capacidadMaximaRack, int capMaxRobot) {
    turnoRobotActual = 0;
    reconfigurarParametros(cantidadRobots, capacidadMaximaRack, capMaxRobot);
}

/**
 * @brief Escribe un evento en la bitácora de auditoría data/log_operaciones.txt con marca de tiempo.
 */
void WarehouseManager::registrarLog(const std::string& mensaje) const {
    std::ofstream archivoLog("data/log_operaciones.txt", std::ios::app);
    if (archivoLog.is_open()) {
        auto ahora = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string marcaTiempo = std::ctime(&ahora);
        if (!marcaTiempo.empty() && marcaTiempo.back() == '\n') {
            marcaTiempo.pop_back();
        }
        archivoLog << "[" << marcaTiempo << "] " << mensaje << "\n";
        archivoLog.close();
    }
}

/**
 * @brief Búsqueda instantánea O(1) en la tabla Hash de IDs.
 */
bool WarehouseManager::existeID(int id) const {
    return idsExistentes.find(id) != idsExistentes.end();
}

/**
 * @brief Ajusta el vector de robots preservando el estado de los existentes.
 */
void WarehouseManager::actualizarFlotaRobots(int nuevaCantidad, int nuevaCapacidadRobot) {
    totalRobots = nuevaCantidad;
    capacidadRobotDefault = nuevaCapacidadRobot;

    std::vector<Robot> nuevaFlota;
    for (int i = 1; i <= totalRobots; ++i) {
        std::string nombreRobot = "Robot-" + std::to_string(i);
        bool encontrado = false;

        for (const auto& r : flotaRobots) {
            if (r.nombre == nombreRobot) {
                Robot copia = r;
                copia.capacidadMaxima = nuevaCapacidadRobot;
                nuevaFlota.push_back(copia);
                encontrado = true;
                break;
            }
        }
        if (!encontrado) {
            nuevaFlota.push_back({ nombreRobot, 0, nuevaCapacidadRobot, "Disponible" });
        }
    }
    flotaRobots = nuevaFlota;
    if (turnoRobotActual >= totalRobots) {
        turnoRobotActual = 0;
    }
}

void WarehouseManager::reconfigurarParametros(int nuevosRobots, int nuevaCapacidadRack, int nuevaCapacidadRobot) {
    if (nuevosRobots > 0 && nuevaCapacidadRobot > 0) {
        actualizarFlotaRobots(nuevosRobots, nuevaCapacidadRobot);
    }
    if (nuevaCapacidadRack > 0) {
        if (rackAlmacenamiento.size() > static_cast<size_t>(nuevaCapacidadRack)) {
            std::cout << "[!] ADVERTENCIA: La nueva capacidad del rack (" << nuevaCapacidadRack
                << ") es menor que los paquetes actuales ("
                << rackAlmacenamiento.size() << "). Liberar espacio antes de reducir.\n";
        }
        else {
            capacidadRack = nuevaCapacidadRack;
        }
    }
    std::cout << "-> Configuración actualizada: " << totalRobots << " Robots (Capacidad: "
        << capacidadRobotDefault << " envios/robot) | Capacidad Rack: " << capacidadRack << " unidades.\n";
    registrarLog("CONFIGURACION: " + std::to_string(totalRobots) + " Robots, Cap. Robot: "
        + std::to_string(capacidadRobotDefault) + ", Cap. Rack: " + std::to_string(capacidadRack));
}

/**
 * @brief Recarga selectiva: Solamente restablece a los robots en estado 'En Recarga'.
 */
void WarehouseManager::recargarRobotsFueraDeServicio() {
    int recargados = 0;
    for (auto& r : flotaRobots) {
        if (r.estado == "En Recarga") {
            r.entregasActuales = 0;
            r.estado = "Disponible";
            recargados++;
        }
    }

    if (recargados > 0) {
        std::cout << "-> ¡Recarga completada! Se han rehabilitado " << recargados << " robot(s) agotado(s).\n";
        registrarLog("RECARGA: " + std::to_string(recargados) + " robot(s) restablecido(s) a 'Disponible'.");
    }
    else {
        std::cout << "-> No hay robots fuera de servicio en este momento.\n";
    }
}

/**
 * @brief Registra un nuevo pedido ingresándolo en la Cola de Prioridad y la Tabla Hash O(1).
 */
void WarehouseManager::registrarPedido(int id, const std::string& descripcion, int prioridad) {
    Pedido nuevoPedido = { id, descripcion, prioridad, "En Espera", "N/A" };
    colaRecepcion.push(nuevoPedido);
    catalogoGlobal.push_back(nuevoPedido);
    idsExistentes.insert(id); // Registra en la tabla Hash para validación O(1)

    std::cout << "-> Pedido #" << id << " registrado exitosamente en recepcion.\n";
    registrarLog("REGISTRO: Pedido #" + std::to_string(id) + " (" + descripcion + ") Prio: " + std::to_string(prioridad));
}

/**
 * @brief Traslada el paquete de mayor prioridad desde la Cola de Recepción hacia la Pila del Rack.
 */
void WarehouseManager::procesarAlmacenamiento() {
    if (colaRecepcion.empty()) {
        std::cout << "-> No hay pedidos en espera para almacenar.\n";
        return;
    }

    if (rackAlmacenamiento.size() >= static_cast<size_t>(capacidadRack)) {
        std::cout << "-> ATENCION: El rack esta lleno. Capacidad maxima: " << capacidadRack << ".\n";
        return;
    }

    Pedido paqueteEntrante = colaRecepcion.top();

    // Verificación de prioridad para prevenir enterrar paquetes urgentes
    if (!rackAlmacenamiento.empty()) {
        Pedido paqueteEnCima = rackAlmacenamiento.top();
        if (paqueteEnCima.prioridad > paqueteEntrante.prioridad) {
            std::cout << "\n[!] ALERTA INTELIGENTE [!]\n";
            std::cout << "Estas a punto de enterrar el pedido #" << paqueteEnCima.id
                << " (Prioridad " << paqueteEnCima.prioridad
                << ") debajo de un paquete de menor prioridad (" << paqueteEntrante.prioridad << ").\n";
            std::cout << "¿Deseas continuar de todos modos? (s/n): ";

            char confirmacion;
            std::cin >> confirmacion;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (confirmacion == 'n' || confirmacion == 'N') {
                std::cout << "-> Operación cancelada por el operador.\n";
                return;
            }
        }
    }

    colaRecepcion.pop();
    paqueteEntrante.estado = "Almacenado";
    paqueteEntrante.robotAsignado = "N/A";
    rackAlmacenamiento.push(paqueteEntrante);
    historialUndo.push(paqueteEntrante);

    for (auto& p : catalogoGlobal) {
        if (p.id == paqueteEntrante.id) {
            p.estado = "Almacenado";
            p.robotAsignado = "N/A";
            break;
        }
    }

    std::cout << "-> Pedido #" << paqueteEntrante.id << " trasladado al rack de almacenamiento.\n";
    registrarLog("ALMACENAMIENTO: Pedido #" + std::to_string(paqueteEntrante.id) + " movido al Rack.");
}

/**
 * @brief Extrae el paquete superior del Rack y lo asigna al Robot en turno mediante la Cola Circular (Round-Robin).
 */
void WarehouseManager::despacharARobot() {
    if (rackAlmacenamiento.empty()) {
        std::cout << "-> El rack esta vacio. No hay paquetes para despachar.\n";
        return;
    }

    // Búsqueda del siguiente robot disponible con aritmética modular (Cola Circular)
    int intentos = 0;
    while (intentos < totalRobots && flotaRobots[turnoRobotActual].estado == "En Recarga") {
        turnoRobotActual = (turnoRobotActual + 1) % totalRobots;
        intentos++;
    }

    if (intentos == totalRobots) {
        std::cout << "\n[!] ALERTA DE FLOTA: Toda la flota (" << totalRobots << "/" << totalRobots
            << " robots) esta 'En Recarga'.\n";
        std::cout << "    Seleccione la opcion [11] para recargar los robots agotados.\n";
        return;
    }

    Robot& robotSeleccionado = flotaRobots[turnoRobotActual];
    Pedido paqueteListo = rackAlmacenamiento.top();
    rackAlmacenamiento.pop();

    robotSeleccionado.entregasActuales++;
    paqueteListo.estado = "Despachado";
    paqueteListo.robotAsignado = robotSeleccionado.nombre;

    // Verificación de límite de capacidad por robot
    if (robotSeleccionado.entregasActuales >= robotSeleccionado.capacidadMaxima) {
        robotSeleccionado.estado = "En Recarga";
        std::cout << "-> Pedido #" << paqueteListo.id << " despachado con " << robotSeleccionado.nombre
            << " [¡Alcanzo su limite de " << robotSeleccionado.capacidadMaxima
            << " envios y paso a 'En Recarga'!]\n";
    }
    else {
        std::cout << "-> Pedido #" << paqueteListo.id << " despachado con " << robotSeleccionado.nombre
            << " (Carga actual: " << robotSeleccionado.entregasActuales << "/"
            << robotSeleccionado.capacidadMaxima << ").\n";
    }

    for (auto& p : catalogoGlobal) {
        if (p.id == paqueteListo.id) {
            p.estado = "Despachado";
            p.robotAsignado = robotSeleccionado.nombre;
            break;
        }
    }

    // Rotación circular al siguiente robot
    turnoRobotActual = (turnoRobotActual + 1) % totalRobots;
    registrarLog("DESPACHO: Pedido #" + std::to_string(paqueteListo.id) + " entregado por " + robotSeleccionado.nombre);
}

/**
 * @brief Revierte la última operación de almacenamiento devolviendo el paquete a la cola de recepción.
 */
void WarehouseManager::deshacerAccion() {
    if (historialUndo.empty()) {
        std::cout << "-> No hay acciones recientes para revertir.\n";
        return;
    }

    Pedido ultimoMovimiento = historialUndo.top();
    historialUndo.pop();

    if (!rackAlmacenamiento.empty() && rackAlmacenamiento.top().id == ultimoMovimiento.id) {
        rackAlmacenamiento.pop();
    }

    ultimoMovimiento.estado = "En Espera";
    ultimoMovimiento.robotAsignado = "N/A";
    colaRecepcion.push(ultimoMovimiento);

    for (auto& p : catalogoGlobal) {
        if (p.id == ultimoMovimiento.id) {
            p.estado = "En Espera";
            p.robotAsignado = "N/A";
            break;
        }
    }

    std::cout << "-> Accion revertida: El pedido #" << ultimoMovimiento.id << " ha sido devuelto a recepcion.\n";
    registrarLog("REVERSION (UNDO): Pedido #" + std::to_string(ultimoMovimiento.id) + " devuelto a recepcion.");
}

void WarehouseManager::rastrearPedido(int id) const {
    if (!existeID(id)) {
        std::cout << "-> Error: No se encontro ningun pedido registrado con el ID #" << id << ".\n";
        return;
    }

    for (const auto& p : catalogoGlobal) {
        if (p.id == id) {
            std::cout << "\n===========================================================\n";
            std::cout << "             RASTREO DE PEDIDO #" << id << "\n";
            std::cout << "===========================================================\n";
            std::cout << "Descripcion   : " << p.descripcion << "\n";
            std::cout << "Prioridad     : " << p.prioridad << "\n";
            std::cout << "Estado Actual : " << p.estado << "\n";

            if (p.estado == "Despachado") {
                std::cout << "Asignacion    : " << p.robotAsignado << "\n";
            }
            else if (p.estado == "Almacenado") {
                std::stack<Pedido> copiaRack = rackAlmacenamiento;
                int posicion = 1;
                while (!copiaRack.empty()) {
                    if (copiaRack.top().id == id) {
                        std::cout << "Ubicacion Rack: Posicion " << posicion << " desde la cima (LIFO)\n";
                        break;
                    }
                    copiaRack.pop();
                    posicion++;
                }
            }
            std::cout << "===========================================================\n";
            return;
        }
    }
}

void WarehouseManager::generarCargaSimulada(int cantidad) {
    if (cantidad <= 0) return;
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    int generados = 0;

    while (generados < cantidad) {
        int randomId = 1000 + (std::rand() % 9000);
        if (!existeID(randomId)) { // Verificación en O(1)
            int randomPrio = 1 + (std::rand() % 3);
            std::string desc = "Paquete_Simulado_" + std::to_string(generados + 1);
            registrarPedido(randomId, desc, randomPrio);
            generados++;
        }
    }
    std::cout << "-> Generados " << cantidad << " pedidos aleatorios exitosamente.\n";
    registrarLog("SIMULACION: Carga automatica de " + std::to_string(cantidad) + " pedidos.");
}

void WarehouseManager::mostrarDashboardKPI() const {
    std::cout << "\n===========================================================\n";
    std::cout << "             DASHBOARD DE METRICAS Y KPIS                  \n";
    std::cout << "===========================================================\n";

    int total = catalogoGlobal.size();
    int enEspera = 0, almacenados = 0, despachados = 0;
    int prio1 = 0, prio2 = 0, prio3 = 0;

    for (const auto& p : catalogoGlobal) {
        if (p.estado == "En Espera") enEspera++;
        else if (p.estado == "Almacenado") almacenados++;
        else if (p.estado == "Despachado") despachados++;

        if (p.prioridad == 1) prio1++;
        else if (p.prioridad == 2) prio2++;
        else if (p.prioridad == 3) prio3++;
    }

    double ocupacionRack = (capacidadRack > 0) ? (static_cast<double>(almacenados) / capacidadRack) * 100.0 : 0.0;

    std::cout << "1. RESUMEN DE INVENTARIO:\n";
    std::cout << "   - Total Pedidos Registrados : " << total << "\n";
    std::cout << "   - En Fila de Recepcion      : " << enEspera << "\n";
    std::cout << "   - En Rack de Almacenes      : " << almacenados << " / " << capacidadRack << " (" << ocupacionRack << "% Ocupacion)\n";
    std::cout << "   - Despachados con Exito     : " << despachados << "\n\n";

    std::cout << "2. DISTRIBUCION POR PRIORIDAD:\n";
    std::cout << "   - Baja (1)     : " << prio1 << "\n";
    std::cout << "   - Media (2)    : " << prio2 << "\n";
    std::cout << "   - Critica (3)  : " << prio3 << "\n\n";

    std::cout << "3. ESTADO Y CAPACIDAD DE LA FLOTA DE ROBOTS:\n";
    for (const auto& r : flotaRobots) {
        int conteoHistorico = 0;
        for (const auto& p : catalogoGlobal) {
            if (p.estado == "Despachado" && p.robotAsignado == r.nombre) conteoHistorico++;
        }
        std::cout << "   - " << r.nombre << ": " << r.entregasActuales << "/" << r.capacidadMaxima
            << " cargas en turno [" << r.estado << "] | Total entregas: " << conteoHistorico << "\n";
    }
    std::cout << "===========================================================\n";
}

void WarehouseManager::despacharPorLotes(int cantidad) {
    if (cantidad <= 0) {
        std::cout << "-> Cantidad invalida.\n";
        return;
    }
    if (rackAlmacenamiento.empty()) {
        std::cout << "-> El rack esta vacio. No hay paquetes para despachar.\n";
        return;
    }

    std::cout << "--- INICIANDO DESPACHO POR LOTES (" << cantidad << " unidades) ---\n";
    int despachados = 0;
    while (despachados < cantidad && !rackAlmacenamiento.empty()) {
        int intentAntes = despachados;
        despacharARobot();
        if (intentAntes == despachados && !rackAlmacenamiento.empty() && flotaRobots[turnoRobotActual].estado == "En Recarga") {
            break;
        }
        despachados++;
    }
    std::cout << "-> Lote finalizado. Total procesados: " << despachados << " paquete(s).\n";
    registrarLog("DESPACHO POR LOTES: Procesados " + std::to_string(despachados) + " pedidos.");
}

void WarehouseManager::mostrarInventario() const {
    std::cout << "\n===========================================================\n";
    std::cout << "             CATALOGO GLOBAL DEL ALMACEN                   \n";
    std::cout << "===========================================================\n";
    std::cout << "Configuracion Actual: " << totalRobots << " Robots | Cap. Robot: " << capacidadRobotDefault
        << " envios | Cap. Rack: " << capacidadRack << "\n";
    std::cout << "-----------------------------------------------------------\n";

    if (catalogoGlobal.empty()) {
        std::cout << "El inventario esta completamente vacio.\n";
    }
    else {
        for (const auto& p : catalogoGlobal) {
            std::cout << "ID: " << p.id
                << " | Desc: " << p.descripcion
                << " | Prio: " << p.prioridad
                << " | Estado: " << p.estado;

            if (p.estado == "Despachado") std::cout << " [" << p.robotAsignado << "]";
            std::cout << "\n";
        }
    }
    std::cout << "===========================================================\n";
}

void WarehouseManager::guardarEnCSV(const std::string& rutaArchivo) {
    std::ofstream archivo(rutaArchivo);
    if (!archivo.is_open()) {
        std::cout << "-> Error: No se pudo abrir el archivo '" << rutaArchivo << "'.\n";
        return;
    }

    archivo << "CONFIG," << totalRobots << "," << capacidadRack << "," << capacidadRobotDefault << "\n";
    archivo << "ID,Descripcion,Prioridad,Estado,RobotAsignado\n";

    for (const auto& p : catalogoGlobal) {
        archivo << p.id << "," << p.descripcion << "," << p.prioridad << "," << p.estado << "," << p.robotAsignado << "\n";
    }

    archivo.close();
    std::cout << "-> Datos guardados exitosamente en '" << rutaArchivo << "'.\n";
    registrarLog("PERSISTENCIA: Datos guardados en " + rutaArchivo);
}

bool WarehouseManager::cargarDesdeCSV(const std::string& rutaArchivo) {
    std::ifstream archivo(rutaArchivo);
    if (!archivo.is_open()) return false;

    std::string linea;
    catalogoGlobal.clear();
    idsExistentes.clear();

    if (std::getline(archivo, linea)) {
        if (linea.rfind("CONFIG,", 0) == 0) {
            std::stringstream ss(linea);
            std::string tag, numRobotsStr, capRackStr, capRobotStr;
            std::getline(ss, tag, ',');
            std::getline(ss, numRobotsStr, ',');
            std::getline(ss, capRackStr, ',');

            int capRobot = 3;
            if (std::getline(ss, capRobotStr, ',')) {
                capRobot = std::stoi(capRobotStr);
            }

            reconfigurarParametros(std::stoi(numRobotsStr), std::stoi(capRackStr), capRobot);
            std::getline(archivo, linea);
        }
    }

    while (std::getline(archivo, linea)) {
        if (linea.empty()) continue;

        std::stringstream ss(linea);
        std::string idStr, desc, prioStr, estado, robot;

        if (std::getline(ss, idStr, ',') &&
            std::getline(ss, desc, ',') &&
            std::getline(ss, prioStr, ',') &&
            std::getline(ss, estado, ',')) {

            if (!std::getline(ss, robot, ',')) robot = "N/A";

            int idNum = std::stoi(idStr);
            Pedido p = { idNum, desc, std::stoi(prioStr), estado, robot };
            catalogoGlobal.push_back(p);
            idsExistentes.insert(idNum);

            if (p.estado == "En Espera") colaRecepcion.push(p);
            else if (p.estado == "Almacenado") rackAlmacenamiento.push(p);
        }
    }

    archivo.close();
    std::cout << "\n-> ¡Historial cargado con exito! Pedidos importados: " << catalogoGlobal.size() << "\n";
    mostrarInventario();
    registrarLog("PERSISTENCIA: Historial cargado desde " + rutaArchivo);
    return true;
}

bool WarehouseManager::borrarArchivoCSV(const std::string& rutaArchivo) {
    if (std::remove(rutaArchivo.c_str()) == 0) {
        while (!colaRecepcion.empty()) colaRecepcion.pop();
        while (!rackAlmacenamiento.empty()) rackAlmacenamiento.pop();
        while (!historialUndo.empty()) historialUndo.pop();
        catalogoGlobal.clear();
        idsExistentes.clear();

        for (auto& r : flotaRobots) {
            r.entregasActuales = 0;
            r.estado = "Disponible";
        }
        turnoRobotActual = 0;

        std::cout << "-> El archivo '" << rutaArchivo << "' ha sido eliminado y el inventario reiniciado a cero.\n";
        registrarLog("PERSISTENCIA: Archivo '" + rutaArchivo + "' eliminado y sistema reiniciado.");
        return true;
    }
    else {
        std::cout << "-> No se pudo eliminar el archivo '" << rutaArchivo << "'.\n";
        return false;
    }
}