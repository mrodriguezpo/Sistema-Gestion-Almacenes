#include "../include/WarehouseManager.hpp"
#include <iostream>
#include <string>
#include <limits>

// FUNCIONES AUXILIARES ROBUSTAS PARA SANITIZACIÓN DE ENTRADA POR TECLADO (cin)

/**
 * @brief Lee un entero de forma segura evitando bucles infinitos por entradas inválidas (e.g. letras).
 */
int leerEntero(const std::string& prompt, int minVal = std::numeric_limits<int>::min(), int maxVal = std::numeric_limits<int>::max()) {
    int valor;
    while (true) {
        std::cout << prompt;
        if (std::cin >> valor) {
            if (valor >= minVal && valor <= maxVal) {
                // Limpieza del búfer de caracteres restantes
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return valor;
            }
            else {
                std::cout << "[!] Error: El numero debe estar entre " << minVal << " y " << maxVal << ".\n";
            }
        }
        else {
            std::cout << "[!] Error: Entrada invalida. Debe ingresar un numero entero.\n";
            std::cin.clear(); // Limpia el estado de error de cin
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Descarta el texto incorrecto
        }
    }
}

/**
 * @brief Lee una cadena de texto sin dejar residuos en el búfer.
 */
std::string leerTexto(const std::string& prompt) {
    std::string texto;
    std::cout << prompt;
    std::getline(std::cin, texto);
    return texto;
}

/**
 * @brief Solicita confirmación de tipo 's' o 'n'.
 */
bool leerConfirmacion(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        char c;
        std::cin >> c;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (c == 's' || c == 'S') return true;
        if (c == 'n' || c == 'N') return false;
        std::cout << "[!] Respuesta invalida. Ingrese 's' para si o 'n' para no.\n";
    }
}

// ============================================================================
// PUNTO DE ENTRADA PRINCIPAL (TUI)
// ============================================================================

int main() {
    WarehouseManager almacen(3, 10, 3);

    std::cout << "=== SISTEMA SMART WAREHOUSE C++ ===\n";

    if (!almacen.cargarDesdeCSV("data/inventario.csv")) {
        std::cout << "-> No se encontro historial previo. Iniciando configuracion manual:\n";
        int numRobots = leerEntero("Ingrese la cantidad de robots en la flota (1-50): ", 1, 50);
        int capRobot = leerEntero("Ingrese la capacidad de envios por robot antes de recargar (1-100): ", 1, 100);
        int capRack = leerEntero("Ingrese la capacidad maxima del rack (1-1000): ", 1, 1000);
        almacen.reconfigurarParametros(numRobots, capRack, capRobot);
    }

    int opcion;
    do {
        std::cout << "\n=========================================\n";
        std::cout << "        SMART WAREHOUSE C++ TUI          \n";
        std::cout << "=========================================\n";
        std::cout << "[1]  Recepcion: Registrar pedido individual\n";
        std::cout << "[2]  Almacenamiento: Mover al Rack (LIFO)\n";
        std::cout << "[3]  Despacho: Asignar a Robot (1 pedido)\n";
        std::cout << "[4]  Despacho por Lotes (Batch)\n";
        std::cout << "[5]  Reversion: Deshacer ultima accion (Undo)\n";
        std::cout << "[6]  Rastrear Pedido (Tracking por ID)\n";
        std::cout << "[7]  Inventario: Ver Catalogo Global\n";
        std::cout << "[8]  Simulacion: Cargar N pedidos aleatorios\n";
        std::cout << "[9]  Dashboard: Ver KPIs y Estado de Robots\n";
        std::cout << "[10] Configuracion: Ajustar Flota y Rack\n";
        std::cout << "[11] Mantenimiento: Recargar Robots Agotados\n";
        std::cout << "[12] Eliminar Historial: Borrar datos persistentes (CSV)\n";
        std::cout << "[13] Salir y Guardar\n";
        std::cout << "=========================================\n";

        opcion = leerEntero("Seleccione una opcion [1-13]: ", 1, 13);

        switch (opcion) {
        case 1: {
            int id;
            do {
                id = leerEntero("ID del pedido (1 - 99999): ", 1, 99999);
                if (almacen.existeID(id)) {
                    std::cout << "[!] Error: Ya existe un pedido con el ID #" << id << ".\n";
                }
            } while (almacen.existeID(id));

            std::string desc = leerTexto("Descripcion del paquete: ");
            int prioridad = leerEntero("Prioridad (1-Baja, 2-Media, 3-Critica): ", 1, 3);

            almacen.registrarPedido(id, desc, prioridad);
            break;
        }
        case 2:
            almacen.procesarAlmacenamiento();
            break;
        case 3:
            almacen.despacharARobot();
            break;
        case 4: {
            int cantidad = leerEntero("Ingrese la cantidad de paquetes a despachar en lote: ", 1, 1000);
            almacen.despacharPorLotes(cantidad);
            break;
        }
        case 5:
            almacen.deshacerAccion();
            break;
        case 6: {
            int id = leerEntero("Ingrese el ID del pedido a rastrear: ", 1, 99999);
            almacen.rastrearPedido(id);
            break;
        }
        case 7:
            almacen.mostrarInventario();
            break;
        case 8: {
            int cantidad = leerEntero("¿Cuantos pedidos aleatorios deseas generar?: ", 1, 500);
            almacen.generarCargaSimulada(cantidad);
            break;
        }
        case 9:
            almacen.mostrarDashboardKPI();
            break;
        case 10: {
            std::cout << "\n--- RECONFIGURACION DEL SISTEMA ---\n";
            int nuevosRobots = leerEntero("Nueva cantidad de robots en la flota: ", 1, 50);
            int nuevaCapRobot = leerEntero("Nueva capacidad de envios por robot: ", 1, 100);
            int nuevaCapacidadRack = leerEntero("Nueva capacidad del rack: ", 1, 1000);
            almacen.reconfigurarParametros(nuevosRobots, nuevaCapacidadRack, nuevaCapRobot);
            break;
        }
        case 11:
            almacen.recargarRobotsFueraDeServicio();
            break;
        case 12: {
            if (leerConfirmacion("\n[!] ATENCION: ¿Esta seguro que desea eliminar 'data/inventario.csv'? (s/n): ")) {
                almacen.borrarArchivoCSV("data/inventario.csv");
            }
            else {
                std::cout << "-> Operacion cancelada. Los datos permanecen intactos.\n";
            }
            break;
        }
        case 13:
            std::cout << "Guardando datos y configuracion...\n";
            almacen.guardarEnCSV("data/inventario.csv");
            std::cout << "Sistema cerrado correctamente.\n";
            break;
        }
    } while (opcion != 13);

    return 0;
}