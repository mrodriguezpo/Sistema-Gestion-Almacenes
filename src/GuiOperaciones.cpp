#include "GuiOperaciones.hpp"
#include "imgui.h"
#include <string>
#include <vector>

void renderizarModuloOperaciones(WarehouseManager& manager) {
    // 1. Panel de Control y Formulario
    ImGui::Begin("Panel de Operaciones - Almacen");

    ImGui::Text("Módulo de Gestión de Pedidos");
    ImGui::Separator();

    static int inputID = 100;
    static char inputDesc[128] = "";
    static int inputPrioridad = 1;

    ImGui::InputInt("ID Pedido", &inputID);
    ImGui::InputText("Descripción", inputDesc, IM_ARRAYSIZE(inputDesc));
    ImGui::SliderInt("Prioridad (1-3)", &inputPrioridad, 1, 3);

    if (ImGui::Button("Registrar / Encolar Pedido")) {
        if (inputID > 0 && strlen(inputDesc) > 0) {
            manager.registrarPedido(inputID, std::string(inputDesc), inputPrioridad);
            inputID++;
            inputDesc[0] = '\0';
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Mover a Rack (Procesar)")) {
        manager.procesarAlmacenamiento();
    }

    ImGui::SameLine();
    if (ImGui::Button("Despachar a Robot")) {
        manager.despacharARobot();
    }

    ImGui::SameLine();
    if (ImGui::Button("Deshacer (Undo)")) {
        manager.deshacerAccion();
    }

    ImGui::End();

    // 2. Visualizador de la Max-Heap (Cola de Recepción)
    ImGui::Begin("Cola de Recepcion (Max-Heap)");
    ImGui::Text("Cantidad en Recepcion: %d", manager.getCantidadEnRecepcion());
    ImGui::Separator();

    std::vector<Pedido> cola = manager.getColaRecepcionVector();
    for (size_t i = 0; i < cola.size(); i++) {
        ImGui::Text("[%d] ID: %d | Desc: %s | Prio: %d",
            (int)i + 1, cola[i].id, cola[i].descripcion.c_str(), cola[i].prioridad);
    }
    ImGui::End();

    // 3. Visualizador del Rack (Pila LIFO)
    ImGui::Begin("Rack de Almacenamiento (Pila)");
    ImGui::Text("Cantidad en Rack: %d", manager.getCantidadEnRack());
    ImGui::Separator();

    std::vector<Pedido> rack = manager.getRackVector();
    for (size_t i = 0; i < rack.size(); i++) {
        if (i == 0) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "[CIMA/TOP] ID: %d | %s", rack[i].id, rack[i].descripcion.c_str());
        }
        else {
            ImGui::Text("[%d] ID: %d | %s", (int)i + 1, rack[i].id, rack[i].descripcion.c_str());
        }
    }
    ImGui::End();
}