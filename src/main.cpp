/**
 * @file main.cpp
 * @brief Punto de entrada principal y capa de presentación gráfica (GUI) del Smart Warehouse System.
 * @details Este archivo implementa la interfaz gráfica de usuario utilizando la librería Raylib.
 *          Gestiona la renderización en tiempo real, la detección de eventos de mouse y teclado,
 *          el escalado dinámico de pantalla y las ventanas modales para las 13 funcionalidades.
 * @author Tu Nombre / Proyecto Estructuras de Datos
 * @date 2026
 */

#define NOGDI
#define NOUSER

#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include "WarehouseManager.hpp"

 // Prevención de conflictos de macros entre Windows API y Raylib
#if defined(DrawText)
#undef DrawText
#endif

#include "raylib.h"

#if defined(DrawText)
#undef DrawText
#endif

/**
 * @brief Función auxiliar para dibujar texto nítido usando fuentes personalizadas en Raylib.
 * @param font Fuente cargada (Font) a utilizar.
 * @param text Cadena de caracteres a renderizar en pantalla.
 * @param position Vector2 con las coordenadas (X, Y) de origen del texto.
 * @param fontSize Tamaño de la tipografía en píxeles.
 * @param color Color del texto en formato Raylib Color.
 * @param spacing Espaciado entre caracteres (por defecto 1.0f).
 */
void DrawTextClean(Font font, const char* text, Vector2 position, float fontSize, Color color, float spacing = 1.0f) {
    DrawTextEx(font, text, position, fontSize, spacing, color);
}

/**
 * @enum ModalActivo
 * @brief Identificadores de estado para controlar la ventana emergente (modal) visible en pantalla.
 */
enum ModalActivo {
    NINGUNO,             /**< No hay ninguna ventana modal abierta (pantalla principal activa). */
    MODAL_REGISTRO,      /**< [1] Formulario de registro individual de pedidos. */
    MODAL_ALERTA_RACK,   /**< [2] Advertencia de prioridad al almacenar en Rack. */
    MODAL_BATCH,         /**< [4] Ventana de despacho por lotes. */
    MODAL_TRACKING,      /**< [6] Búsqueda y rastreo de expedientes por ID. */
    MODAL_INVENTARIO,    /**< [7] Catálogo global con paginación y parámetros activos. */
    MODAL_SIMULACION,    /**< [8] Generador masivo de datos aleatorios. */
    MODAL_DASHBOARD,     /**< [9] Métricas KPIs y estado de flota de robots. */
    MODAL_CONFIG,        /**< [10] Reconfiguración de parámetros del sistema. */
    MODAL_BORRAR_CSV,    /**< [12] Confirmación de borrado de historial persistente. */
    MODAL_GUIA_AYUDA     /**< [?] Guía completa de comandos y accesos rápidos. */
};

/**
 * @brief Función principal que ejecuta el ciclo de vida del programa y la interfaz gráfica.
 * @return int Estado de salida del programa (0 para ejecución exitosa).
 */
int main() {
    // Permite que la ventana sea redimensionable por el usuario
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Smart Warehouse - Sistema de Gestion de Almacen");
    SetTargetFPS(60);

    // Anula la tecla ESC para evitar que cierre la aplicación accidentalmente
    SetExitKey(KEY_NULL);

    // ------------------- CARGA Y CONFIGURACIÓN DE FUENTE -------------------
    /**
     * Carga de la fuente tipográfica Arial desde la carpeta assets/ con soporte multi-ruta.
     * Se genera en tamaño HD (48px) y se aplica filtro bilineal para suavizado de bordes.
     */
    Font customFont;
    if (FileExists("assets/arial.ttf")) {
        customFont = LoadFontEx("assets/arial.ttf", 48, NULL, 0);
    }
    else if (FileExists("assets/arial")) {
        customFont = LoadFontEx("assets/arial", 48, NULL, 0);
    }
    else if (FileExists("../assets/arial.ttf")) {
        customFont = LoadFontEx("../assets/arial.ttf", 48, NULL, 0);
    }
    else {
        customFont = GetFontDefault(); // Fuente de respaldo en caso de ausencia
    }

    if (customFont.texture.id > 0) {
        SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    // ------------------- INICIALIZACIÓN DEL BACKEND -------------------
    // Instancia del gestor (3 robots iniciales, capacidad 10 en Rack, capacidad 3 por robot)
    WarehouseManager manager(3, 10, 3);
    manager.cargarDesdeCSV("data/inventario.csv"); // Carga previa de datos si el archivo existe

    // Variables de control de interfaz
    ModalActivo modal = NINGUNO;
    bool salirDelPrograma = false;

    // Buffers de texto para captura de entradas en formularios
    char bufID[16] = "", bufDesc[64] = "", bufBatch[16] = "", bufTrack[16] = "", bufSim[16] = "";
    char bufRobots[16] = "", bufCapBot[16] = "", bufCapRack[16] = "";
    int prioSel = 2, campoActivo = 0;
    int paginaCat = 0;    // Índice de página para el Catálogo Global [7]
    int paginaRobots = 0; // Índice de página para la lista de Robots [9]

    std::string msgEstado = "Sistema listo. Presione [7] para Inventario o [9] para Dashboard KPIs.";
    Color colEstado = GetColor(0x90A4AEFF);
    float timerCursor = 0.0f; // Temporizador para parpadeo de cursor de texto
    std::string msgModalErr = "";

    // Estructuras de almacenamiento temporal para el módulo de tracking
    Pedido trackingPedido;
    int trackingProfundidad = -1;
    bool trackingEncontrado = false;

    // ------------------- PALETA DE COLORES INDUSTRIAL DARK -------------------
    Color bgDark = GetColor(0x181A1FFF);
    Color headerBg = GetColor(0x21252BFF);
    Color panelBg = GetColor(0x1E222BFF);
    Color cardBg = GetColor(0x282C34FF);
    Color borderColor = GetColor(0x3E4451FF);
    Color textPrimary = GetColor(0xABB2BFFF);
    Color textMuted = GetColor(0x7F848EFF);
    Color accentBlue = GetColor(0x4C7D9AFF);
    Color accentGreen = GetColor(0x3B7A57FF);
    Color accentAmber = GetColor(0xB07D30FF);
    Color accentRed = GetColor(0x9E3B3BFF);

    // ------------------- BUCLE PRINCIPAL DE RENDERIZADO -------------------
    while (!WindowShouldClose() && !salirDelPrograma) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        timerCursor += GetFrameTime();

        // Cálculo de escala proporcional para fuentes y contenedores
        float scaleY = (float)screenHeight / 720.0f;
        float scaleX = (float)screenWidth / 1280.0f;
        float scale = (scaleY < scaleX) ? scaleY : scaleX;
        if (scale < 0.85f) scale = 0.85f;

        float fontTitle = 20.0f * scale;
        float fontSubtitle = 14.0f * scale;
        float fontBody = 13.0f * scale;
        float fontSmall = 11.0f * scale;

        // Botones superiores de la barra de título
        float btnHeight = 34.0f * scale;
        Rectangle recBtnGuia = { (float)screenWidth - (220.0f * scale), 12.0f * scale, 105.0f * scale, btnHeight };
        Rectangle recBtnSalir = { (float)screenWidth - (105.0f * scale), 12.0f * scale, 90.0f * scale, btnHeight };

        Vector2 mousePos = GetMousePosition();

        // Procesamiento de clics en la barra superior
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mousePos, recBtnGuia)) modal = MODAL_GUIA_AYUDA;
            if (CheckCollisionPointRec(mousePos, recBtnSalir)) salirDelPrograma = true;
        }

        // ------------------- DETECCIÓN DE TECLAS DE ACCESO RÁPIDO -------------------
        if (modal == NINGUNO) {
            if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_H)) modal = MODAL_GUIA_AYUDA;
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) { modal = MODAL_REGISTRO; campoActivo = 0; msgModalErr = ""; }
            if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
                int pE = 0, pC = 0;
                if (manager.requiereAlertaInteligente(pE, pC)) {
                    modal = MODAL_ALERTA_RACK;
                }
                else {
                    if (manager.procesarAlmacenamiento()) msgEstado = "[2] Paquete movido al Rack de Almacenamiento.";
                    else msgEstado = "[!] Error: Recepcion vacia o Rack lleno.";
                }
            }
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
                if (manager.despacharARobot()) msgEstado = "[3] Pedido despachado a Robot.";
                else msgEstado = "[!] Error: Rack vacio o Robots en recarga.";
            }
            if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) { modal = MODAL_BATCH; bufBatch[0] = '\0'; msgModalErr = ""; }
            if (IsKeyPressed(KEY_FIVE) || IsKeyPressed(KEY_KP_5)) {
                if (manager.deshacerAccion()) msgEstado = "[5] Ultima accion del Rack revertida.";
                else msgEstado = "[!] Nada que deshacer.";
            }
            if (IsKeyPressed(KEY_SIX) || IsKeyPressed(KEY_KP_6)) { modal = MODAL_TRACKING; trackingEncontrado = false; bufTrack[0] = '\0'; msgModalErr = ""; }
            if (IsKeyPressed(KEY_SEVEN) || IsKeyPressed(KEY_KP_7)) { modal = MODAL_INVENTARIO; paginaCat = 0; }
            if (IsKeyPressed(KEY_EIGHT) || IsKeyPressed(KEY_KP_8)) { modal = MODAL_SIMULACION; bufSim[0] = '\0'; msgModalErr = ""; }
            if (IsKeyPressed(KEY_NINE) || IsKeyPressed(KEY_KP_9)) { modal = MODAL_DASHBOARD; paginaRobots = 0; }
            if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) {
                modal = MODAL_CONFIG;
                campoActivo = 0;
                msgModalErr = "";
                snprintf(bufRobots, sizeof(bufRobots), "%d", manager.getTotalRobots());
                snprintf(bufCapBot, sizeof(bufCapBot), "%d", manager.getCapacidadRobotDefault());
                snprintf(bufCapRack, sizeof(bufCapRack), "%d", manager.getCapacidadRack());
            }
            if (IsKeyPressed(KEY_M)) {
                int r = manager.recargarRobotsFueraDeServicio();
                msgEstado = "[11] Mantenimiento: " + std::to_string(r) + " robots recargados a Disponibles.";
            }
            if (IsKeyPressed(KEY_X)) modal = MODAL_BORRAR_CSV;
        }

        // ------------------- DIBUJO DE PANTALLA PRINCIPAL -------------------
        BeginDrawing();
        ClearBackground(bgDark);

        // Renderizado del encabezado
        float headerH = 58.0f * scale;
        DrawRectangle(0, 0, screenWidth, (int)headerH, headerBg);
        DrawRectangle(0, (int)headerH - 2, screenWidth, 2, borderColor);

        DrawTextClean(customFont, "SMART WAREHOUSE", { 20.0f * scale, 12.0f * scale }, fontTitle, textPrimary, 1.2f);
        DrawTextClean(customFont, msgEstado.c_str(), { 20.0f * scale, 36.0f * scale }, fontSmall, colEstado);

        // Botón [?] Guía
        DrawRectangleRec(recBtnGuia, cardBg);
        DrawRectangleLinesEx(recBtnGuia, 1.0f * scale, accentBlue);
        DrawTextClean(customFont, "[?] Guia", { recBtnGuia.x + 14.0f * scale, recBtnGuia.y + 9.0f * scale }, fontSmall, textPrimary);

        // Botón [X] Salir
        DrawRectangleRec(recBtnSalir, cardBg);
        DrawRectangleLinesEx(recBtnSalir, 1.0f * scale, accentRed);
        DrawTextClean(customFont, "[X] Salir", { recBtnSalir.x + 14.0f * scale, recBtnSalir.y + 9.0f * scale }, fontSmall, textPrimary);

        // Cálculo de dimensiones de la cuadrícula de paneles
        int margin = (int)(15.0f * scale);
        int topY = (int)(headerH + margin);
        int topH = (int)((screenHeight - topY - margin * 2) * 0.58f);
        int botY = topY + topH + margin;
        int botH = screenHeight - botY - margin;
        int colW = (screenWidth - margin * 4) / 3;

        // PANEL 1: RECEPCIÓN (Cola Max-Heap)
        DrawRectangleLines(margin, topY, colW, topH, borderColor);
        DrawRectangle(margin, topY, colW, (int)(32 * scale), headerBg);
        DrawTextClean(customFont, "1. RECEPCION (Max-Heap)", { (float)margin + 10 * scale, (float)topY + 8 * scale }, fontSubtitle, accentAmber);
        auto cola = manager.getColaRecepcionVector();
        for (size_t i = 0; i < cola.size() && i < 6; i++) {
            int posY = topY + (int)(38 * scale) + (int)i * (int)(44 * scale);
            DrawRectangle(margin + 10, posY, colW - 20, (int)(38 * scale), cardBg);
            DrawRectangleLines(margin + 10, posY, colW - 20, (int)(38 * scale), borderColor);
            DrawTextClean(customFont, TextFormat("#%d | %s | Prio:%d", cola[i].id, cola[i].descripcion.c_str(), cola[i].prioridad), { (float)margin + 15 * scale, (float)posY + 10 * scale }, fontBody, textPrimary);
        }

        // PANEL 2: RACK VERTICAL (Pila LIFO)
        int col2X = margin * 2 + colW;
        DrawRectangleLines(col2X, topY, colW, topH, borderColor);
        DrawRectangle(col2X, topY, colW, (int)(32 * scale), headerBg);
        DrawTextClean(customFont, "2. RACK (Pila LIFO)", { (float)col2X + 10 * scale, (float)topY + 8 * scale }, fontSubtitle, accentBlue);
        auto rack = manager.getRackVector();
        for (size_t i = 0; i < rack.size() && i < 6; i++) {
            int posY = topY + (int)(38 * scale) + (int)i * (int)(44 * scale);
            Color rColor = (i == 0) ? GetColor(0x284236FF) : cardBg;
            DrawRectangle(col2X + 10, posY, colW - 20, (int)(38 * scale), rColor);
            DrawRectangleLines(col2X + 10, posY, colW - 20, (int)(38 * scale), (i == 0 ? accentGreen : borderColor));
            DrawTextClean(customFont, TextFormat("#%d | %s", rack[i].id, rack[i].descripcion.c_str()), { (float)col2X + 15 * scale, (float)posY + 10 * scale }, fontBody, textPrimary);
        }

        // PANEL 3: FLOTA DE ROBOTS (Round-Robin)
        int col3X = margin * 3 + colW * 2;
        Rectangle recPanelRobots = { (float)col3X, (float)topY, (float)colW, (float)topH };
        if (modal == NINGUNO && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recPanelRobots)) {
            modal = MODAL_DASHBOARD;
            paginaRobots = 0;
        }

        DrawRectangleLines(col3X, topY, colW, topH, borderColor);
        DrawRectangle(col3X, topY, colW, (int)(32 * scale), headerBg);
        DrawTextClean(customFont, "3. FLOTA ROBOTICA [9 Click]", { (float)col3X + 10 * scale, (float)topY + 8 * scale }, fontSubtitle, accentAmber);

        auto robots = manager.getFlotaRobots();
        size_t totalBots = robots.size();
        size_t maxBotsVisibles = 5;
        size_t botsAMostrar = (totalBots < maxBotsVisibles) ? totalBots : maxBotsVisibles;

        float spaceAvail = topH - (42.0f * scale);
        float cardGap = 5.0f * scale;
        float cardH = (botsAMostrar > 0) ? (spaceAvail - (botsAMostrar - 1) * cardGap) / botsAMostrar : 60.0f * scale;
        if (cardH > 68.0f * scale) cardH = 68.0f * scale;

        for (size_t i = 0; i < botsAMostrar; i++) {
            int posY = topY + (int)(36 * scale) + (int)i * (int)(cardH + cardGap);
            Color stCol = (robots[i].estado == "Disponible") ? accentGreen : accentRed;
            DrawRectangle(col3X + 10, posY, colW - 20, (int)cardH, cardBg);
            DrawRectangleLines(col3X + 10, posY, colW - 20, (int)cardH, stCol);

            if (cardH >= 50.0f * scale) {
                DrawTextClean(customFont, robots[i].nombre.c_str(), { (float)col3X + 18 * scale, (float)posY + 4 * scale }, fontSubtitle, textPrimary);
                DrawTextClean(customFont, TextFormat("Tanda: %d/%d | Acum: %d", robots[i].entregasActuales, robots[i].capacidadMaxima, robots[i].entregasTotalesAcumuladas), { (float)col3X + 18 * scale, (float)posY + 22 * scale }, fontSmall, textMuted);
                DrawTextClean(customFont, TextFormat("Estado: %s", robots[i].estado.c_str()), { (float)col3X + 18 * scale, (float)posY + 38 * scale }, fontSmall, stCol);
            }
            else {
                DrawTextClean(customFont, TextFormat("%s | %d/%d | %s", robots[i].nombre.c_str(), robots[i].entregasActuales, robots[i].capacidadMaxima, robots[i].estado.c_str()), { (float)col3X + 15 * scale, (float)posY + (cardH - 12 * scale) / 2.0f }, fontSmall, stCol);
            }
        }

        if (totalBots > maxBotsVisibles) {
            DrawTextClean(customFont, TextFormat("+ %d robots mas (Presione [9])", (int)(totalBots - maxBotsVisibles)), { (float)col3X + 18 * scale, (float)(topY + topH - 18 * scale) }, fontSmall, accentAmber);
        }

        // PANEL 4: CATÁLOGO GLOBAL Y KPIS
        int fullW = screenWidth - margin * 2;
        Rectangle recPanelCat = { (float)margin, (float)botY, (float)fullW, (float)botH };
        if (modal == NINGUNO && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recPanelCat)) {
            modal = MODAL_INVENTARIO; paginaCat = 0;
        }

        DrawRectangleLines(margin, botY, fullW, botH, borderColor);
        DrawRectangle(margin, botY, fullW, (int)(30 * scale), headerBg);
        DrawTextClean(customFont, "4. DASHBOARD KPIs METRICAS & INVENTARIO GLOBAL [7/9 Click]", { (float)margin + 10 * scale, (float)botY + 6 * scale }, fontSubtitle, textPrimary);

        int p1 = 0, p2 = 0, p3 = 0;
        manager.getDistribucionPrioridades(p1, p2, p3);
        DrawTextClean(customFont, TextFormat("Ocupacion Rack: %.1f%% (%d/%d)", manager.getPorcentajeOcupacionRack(), manager.getCantidadEnRack(), manager.getCapacidadRack()), { (float)margin + 15 * scale, (float)botY + 34 * scale }, fontBody, accentBlue);
        DrawTextClean(customFont, TextFormat("Prioridades -> Baja(1): %d | Media(2): %d | VIP(3): %d", p1, p2, p3), { (float)margin + 350 * scale, (float)botY + 34 * scale }, fontBody, accentAmber);

        auto cat = manager.getCatalogoGlobal();
        for (size_t i = 0; i < cat.size() && i < 4; i++) {
            int posY = botY + (int)(56 * scale) + (int)i * (int)(22 * scale);
            DrawTextClean(customFont, TextFormat("#%d | %s | Prio:%d | Estado:%s | Robot:%s", cat[i].id, cat[i].descripcion.c_str(), cat[i].prioridad, cat[i].estado.c_str(), cat[i].robotAsignado.c_str()), { (float)margin + 15 * scale, (float)posY }, fontSmall, textMuted);
        }

        // ------------------- RENDERIZADO DE VENTANAS MODALES EMERGENTES -------------------
        if (modal != NINGUNO) {
            // Fondo semitransparente para oscurecer el dashboard
            DrawRectangle(0, 0, screenWidth, screenHeight, GetColor(0x000000DD));

            float mW = 680.0f * scale;
            float mH = 520.0f * scale;
            float mX = (screenWidth - mW) / 2.0f;
            float mY = (screenHeight - mH) / 2.0f;

            DrawRectangleRec({ mX, mY, mW, mH }, panelBg);
            DrawRectangleLinesEx({ mX, mY, mW, mH }, 1.5f * scale, borderColor);

            bool mCursor = ((int)(timerCursor * 2.5f) % 2 == 0);

            // [?] GUÍA DETALLADA DE COMANDOS
            if (modal == MODAL_GUIA_AYUDA) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "GUIA DETALLADA DEL MENU Y COMANDOS DE ACCESO [1 - 13]", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);

                const char* guiaLineas[] = {
                    "[Tecla 1]  [1] Recepcion: Registrar pedido manual (ID unico, Desc, Prioridad 1-3).",
                    "[Tecla 2]  [2] Almacenamiento: Mover paquete al Rack (LIFO) + Alerta Inteligente.",
                    "[Tecla 3]  [3] Despacho: Extraer cima del Rack y asignarla al robot en turno.",
                    "[Tecla 4]  [4] Despacho por lotes (Batch): Procesa N despachos consecutivos.",
                    "[Tecla 5]  [5] Reversion (Undo): Cancela la ultima transferencia realizada al Rack.",
                    "[Tecla 6]  [6] Rastrear pedido (Tracking): Expediente e ID + profundidad en Rack.",
                    "[Tecla 7]  [7] Inventario: Ver ventana completa con catalogo y configuracion activa.",
                    "[Tecla 8]  [8] Simulacion: Genera N paquetes aleatorios con IDs de 4 digitos.",
                    "[Tecla 9]  [9] Dashboard KPIs: Ver ocupacion de rack %, prioridades y estado de flota.",
                    "[Tecla 0]  [10] Configuracion: Reconfigurar robots, cap. por robot y cap. rack.",
                    "[Tecla M]  [11] Mantenimiento: Recargar robots agotados ('En Recarga').",
                    "[Tecla X]  [12] Eliminar historial: Borra inventario.csv y vacia la memoria.",
                    "[Boton X]  [13] Salir y guardar: Escribe inventario.csv y finaliza la ejecucion."
                };

                for (int i = 0; i < 13; i++) {
                    DrawTextClean(customFont, guiaLineas[i], { mX + 18 * scale, mY + (48 + i * 24) * scale }, fontSmall, textPrimary);
                }

                Rectangle recCerrarGuia = { mX + (mW - 160 * scale) / 2, mY + mH - 42 * scale, 160 * scale, 32 * scale };
                DrawRectangleRec(recCerrarGuia, cardBg);
                DrawRectangleLinesEx(recCerrarGuia, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cerrar Guia (ESC)", { recCerrarGuia.x + 25 * scale, recCerrarGuia.y + 8 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCerrarGuia)) modal = NINGUNO;
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) modal = NINGUNO;
            }

            // [7] INVENTARIO COMPLETO (PAGINADO)
            else if (modal == MODAL_INVENTARIO) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[7] INVENTARIO: CATALOGO GLOBAL Y CONFIGURACION ACTIVA", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);

                DrawRectangle((int)(mX + 15 * scale), (int)(mY + 48 * scale), (int)(mW - 30 * scale), (int)(45 * scale), cardBg);
                DrawRectangleLinesEx({ mX + 15 * scale, mY + 48 * scale, mW - 30 * scale, 45 * scale }, 1.0f * scale, borderColor);

                DrawTextClean(customFont, "CONFIGURACION ACTIVA DEL SISTEMA:", { mX + 25 * scale, mY + 54 * scale }, fontSmall, accentAmber);
                DrawTextClean(customFont, TextFormat("Flota: %d Robots  |  Cap. Robot: %d envios  |  Cap. Rack: %d espacios", manager.getTotalRobots(), manager.getCapacidadRobotDefault(), manager.getCapacidadRack()), { mX + 25 * scale, mY + 72 * scale }, fontSmall, textPrimary);

                DrawTextClean(customFont, "LISTADO COMPLETO DE PEDIDOS REGISTRADOS:", { mX + 15 * scale, mY + 102 * scale }, fontSmall, textPrimary);

                auto catalogoCompleto = manager.getCatalogoGlobal();
                int itemsPorPagina = 8;
                int totalPaginas = (catalogoCompleto.empty()) ? 1 : (int)ceil((double)catalogoCompleto.size() / itemsPorPagina);

                if (paginaCat < 0) paginaCat = 0;
                if (paginaCat >= totalPaginas) paginaCat = totalPaginas - 1;

                int inicio = paginaCat * itemsPorPagina;
                int fin = (inicio + itemsPorPagina < (int)catalogoCompleto.size()) ? inicio + itemsPorPagina : (int)catalogoCompleto.size();

                if (catalogoCompleto.empty()) {
                    DrawTextClean(customFont, "No hay pedidos registrados en el catalogo global.", { mX + 25 * scale, mY + 135 * scale }, fontBody, textMuted);
                }
                else {
                    for (int i = inicio; i < fin; i++) {
                        int indexEnPagina = i - inicio;
                        float posY = mY + (125.0f + indexEnPagina * 36.0f) * scale;

                        DrawRectangle((int)(mX + 15 * scale), (int)posY, (int)(mW - 30 * scale), (int)(32 * scale), cardBg);
                        DrawRectangleLinesEx({ mX + 15 * scale, posY, mW - 30 * scale, 32 * scale }, 1.0f * scale, borderColor);

                        const auto& p = catalogoCompleto[i];
                        DrawTextClean(customFont, TextFormat("#%d  |  %s", p.id, p.descripcion.c_str()), { mX + 25 * scale, posY + 8 * scale }, fontSmall, textPrimary);
                        DrawTextClean(customFont, TextFormat("Prio: %d  |  Estado: %s  |  Robot: %s", p.prioridad, p.estado.c_str(), p.robotAsignado.c_str()), { mX + mW - 310 * scale, posY + 8 * scale }, fontSmall, (p.estado == "Despachado" ? accentGreen : p.estado == "Almacenado" ? accentBlue : accentAmber));
                    }
                }

                Rectangle recPrev = { mX + 20 * scale, mY + mH - 50 * scale, 120 * scale, 35 * scale };
                Rectangle recNext = { mX + 150 * scale, mY + mH - 50 * scale, 120 * scale, 35 * scale };
                Rectangle recCloseCat = { mX + mW - 160 * scale, mY + mH - 50 * scale, 140 * scale, 35 * scale };

                DrawRectangleRec(recPrev, (paginaCat > 0 ? cardBg : headerBg));
                DrawRectangleLinesEx(recPrev, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "< Anterior", { recPrev.x + 20 * scale, recPrev.y + 9 * scale }, fontSmall, (paginaCat > 0 ? textPrimary : textMuted));

                DrawRectangleRec(recNext, (paginaCat < totalPaginas - 1 ? cardBg : headerBg));
                DrawRectangleLinesEx(recNext, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Siguiente >", { recNext.x + 20 * scale, recNext.y + 9 * scale }, fontSmall, (paginaCat < totalPaginas - 1 ? textPrimary : textMuted));

                DrawTextClean(customFont, TextFormat("Pagina %d / %d  (Total: %d)", paginaCat + 1, totalPaginas, (int)catalogoCompleto.size()), { mX + 290 * scale, mY + mH - 40 * scale }, fontSmall, textMuted);

                DrawRectangleRec(recCloseCat, cardBg);
                DrawRectangleLinesEx(recCloseCat, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cerrar (ESC)", { recCloseCat.x + 25 * scale, recCloseCat.y + 9 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (CheckCollisionPointRec(mousePos, recPrev) && paginaCat > 0) paginaCat--;
                    if (CheckCollisionPointRec(mousePos, recNext) && paginaCat < totalPaginas - 1) paginaCat++;
                    if (CheckCollisionPointRec(mousePos, recCloseCat)) modal = NINGUNO;
                }

                if (IsKeyPressed(KEY_LEFT) && paginaCat > 0) paginaCat--;
                if (IsKeyPressed(KEY_RIGHT) && paginaCat < totalPaginas - 1) paginaCat++;
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [9] DASHBOARD KPIS Y ROBOTS (PAGINADO)
            else if (modal == MODAL_DASHBOARD) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[9] DASHBOARD: KPIs Y ESTADO DETALLADO DE ROBOTS", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);

                DrawRectangle((int)(mX + 15 * scale), (int)(mY + 45 * scale), (int)(mW - 30 * scale), (int)(55 * scale), cardBg);
                DrawRectangleLinesEx({ mX + 15 * scale, mY + 45 * scale, mW - 30 * scale, 55 * scale }, 1.0f * scale, borderColor);

                float ocupacionPerc = manager.getPorcentajeOcupacionRack();
                DrawTextClean(customFont, "1. OCUPACION ACTUAL DEL RACK:", { mX + 25 * scale, mY + 50 * scale }, fontSmall, accentBlue);
                DrawTextClean(customFont, TextFormat("%.1f%%  (%d de %d posiciones ocupadas)", ocupacionPerc, manager.getCantidadEnRack(), manager.getCapacidadRack()), { mX + 25 * scale, mY + 66 * scale }, fontSubtitle, textPrimary);

                Rectangle recBarBg = { mX + 25 * scale, mY + 86 * scale, mW - 50 * scale, 8 * scale };
                Rectangle recBarFill = { mX + 25 * scale, mY + 86 * scale, (mW - 50 * scale) * (ocupacionPerc / 100.0f), 8 * scale };
                DrawRectangleRec(recBarBg, headerBg);
                DrawRectangleRec(recBarFill, (ocupacionPerc > 80.0f ? accentRed : accentBlue));
                DrawRectangleLinesEx(recBarBg, 1.0f * scale, borderColor);

                int p1 = 0, p2 = 0, p3 = 0;
                manager.getDistribucionPrioridades(p1, p2, p3);
                int totalPed = p1 + p2 + p3;

                DrawRectangle((int)(mX + 15 * scale), (int)(mY + 108 * scale), (int)(mW - 30 * scale), (int)(48 * scale), cardBg);
                DrawRectangleLinesEx({ mX + 15 * scale, mY + 108 * scale, mW - 30 * scale, 48 * scale }, 1.0f * scale, borderColor);

                DrawTextClean(customFont, "2. DISTRIBUCION DE PEDIDOS POR PRIORIDAD:", { mX + 25 * scale, mY + 112 * scale }, fontSmall, accentAmber);
                DrawTextClean(customFont, TextFormat("Baja [1]: %d (%.0f%%)   |   Media [2]: %d (%.0f%%)   |   VIP/Critica [3]: %d (%.0f%%)",
                    p1, (totalPed > 0 ? (p1 * 100.0f / totalPed) : 0.0f),
                    p2, (totalPed > 0 ? (p2 * 100.0f / totalPed) : 0.0f),
                    p3, (totalPed > 0 ? (p3 * 100.0f / totalPed) : 0.0f)),
                    { mX + 25 * scale, mY + 132 * scale }, fontSmall, textPrimary);

                DrawTextClean(customFont, "3. ESTADO DETALLADO DE CADA ROBOT EN LA FLOTA:", { mX + 15 * scale, mY + 162 * scale }, fontSmall, textPrimary);

                auto flota = manager.getFlotaRobots();
                int robotsPorPagina = 4;
                int totalPaginasRobots = (flota.empty()) ? 1 : (int)ceil((double)flota.size() / robotsPorPagina);

                if (paginaRobots < 0) paginaRobots = 0;
                if (paginaRobots >= totalPaginasRobots) paginaRobots = totalPaginasRobots - 1;

                int inicioR = paginaRobots * robotsPorPagina;
                int finR = (inicioR + robotsPorPagina < (int)flota.size()) ? inicioR + robotsPorPagina : (int)flota.size();

                for (int i = inicioR; i < finR; i++) {
                    int idxInPage = i - inicioR;
                    float posY = mY + (180.0f + idxInPage * 62.0f) * scale;
                    Color stCol = (flota[i].estado == "Disponible") ? accentGreen : accentRed;

                    DrawRectangle((int)(mX + 15 * scale), (int)posY, (int)(mW - 30 * scale), (int)(56 * scale), cardBg);
                    DrawRectangleLinesEx({ mX + 15 * scale, posY, mW - 30 * scale, 56 * scale }, 1.0f * scale, stCol);

                    DrawTextClean(customFont, flota[i].nombre.c_str(), { mX + 25 * scale, posY + 6 * scale }, fontSubtitle, textPrimary);
                    DrawTextClean(customFont, TextFormat("Envios tanda actual: %d / %d", flota[i].entregasActuales, flota[i].capacidadMaxima), { mX + 160 * scale, posY + 6 * scale }, fontSmall, textMuted);
                    DrawTextClean(customFont, TextFormat("Entregas acumuladas: %d paquetes", flota[i].entregasTotalesAcumuladas), { mX + 160 * scale, posY + 28 * scale }, fontSmall, textPrimary);

                    DrawTextClean(customFont, TextFormat("Estado: %s", flota[i].estado.c_str()), { mX + mW - 170 * scale, posY + 16 * scale }, fontBody, stCol);
                }

                Rectangle recPrevR = { mX + 20 * scale, mY + mH - 48 * scale, 120 * scale, 34 * scale };
                Rectangle recNextR = { mX + 150 * scale, mY + mH - 48 * scale, 120 * scale, 34 * scale };
                Rectangle recCloseDash = { mX + mW - 180 * scale, mY + mH - 48 * scale, 160 * scale, 34 * scale };

                DrawRectangleRec(recPrevR, (paginaRobots > 0 ? cardBg : headerBg));
                DrawRectangleLinesEx(recPrevR, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "< Anterior", { recPrevR.x + 20 * scale, recPrevR.y + 8 * scale }, fontSmall, (paginaRobots > 0 ? textPrimary : textMuted));

                DrawRectangleRec(recNextR, (paginaRobots < totalPaginasRobots - 1 ? cardBg : headerBg));
                DrawRectangleLinesEx(recNextR, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Siguiente >", { recNextR.x + 20 * scale, recNextR.y + 8 * scale }, fontSmall, (paginaRobots < totalPaginasRobots - 1 ? textPrimary : textMuted));

                DrawTextClean(customFont, TextFormat("Pag %d / %d", paginaRobots + 1, totalPaginasRobots), { mX + 285 * scale, mY + mH - 38 * scale }, fontSmall, textMuted);

                DrawRectangleRec(recCloseDash, cardBg);
                DrawRectangleLinesEx(recCloseDash, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cerrar Dashboard (ESC)", { recCloseDash.x + 10 * scale, recCloseDash.y + 8 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (CheckCollisionPointRec(mousePos, recPrevR) && paginaRobots > 0) paginaRobots--;
                    if (CheckCollisionPointRec(mousePos, recNextR) && paginaRobots < totalPaginasRobots - 1) paginaRobots++;
                    if (CheckCollisionPointRec(mousePos, recCloseDash)) modal = NINGUNO;
                }

                if (IsKeyPressed(KEY_LEFT) && paginaRobots > 0) paginaRobots--;
                if (IsKeyPressed(KEY_RIGHT) && paginaRobots < totalPaginasRobots - 1) paginaRobots++;
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [1] REGISTRO INDIVIDUAL DE PEDIDOS
            else if (modal == MODAL_REGISTRO) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[1] REGISTRAR PEDIDO INDIVIDUAL", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);

                DrawTextClean(customFont, "1. ID Numerico:", { mX + 25 * scale, mY + 52 * scale }, fontBody, textPrimary);
                Rectangle recID = { mX + 25 * scale, mY + 72 * scale, mW - 50 * scale, 32 * scale };
                DrawRectangleRec(recID, (campoActivo == 0 ? cardBg : headerBg));
                DrawRectangleLinesEx(recID, 1.0f * scale, (campoActivo == 0 ? accentBlue : borderColor));
                std::string t1 = bufID; if (campoActivo == 0 && mCursor) t1 += "|";
                DrawTextClean(customFont, t1.c_str(), { recID.x + 10 * scale, recID.y + 8 * scale }, fontBody, textPrimary);

                DrawTextClean(customFont, "2. Descripcion del Producto:", { mX + 25 * scale, mY + 115 * scale }, fontBody, textPrimary);
                Rectangle recDesc = { mX + 25 * scale, mY + 135 * scale, mW - 50 * scale, 32 * scale };
                DrawRectangleRec(recDesc, (campoActivo == 1 ? cardBg : headerBg));
                DrawRectangleLinesEx(recDesc, 1.0f * scale, (campoActivo == 1 ? accentBlue : borderColor));
                std::string t2 = bufDesc; if (campoActivo == 1 && mCursor) t2 += "|";
                DrawTextClean(customFont, t2.c_str(), { recDesc.x + 10 * scale, recDesc.y + 8 * scale }, fontBody, textPrimary);

                DrawTextClean(customFont, "3. Nivel de Prioridad (Haga clic o presione 1, 2 o 3):", { mX + 25 * scale, mY + 180 * scale }, fontBody, textPrimary);

                float btnPrioW = (mW - 70 * scale) / 3.0f;
                Rectangle recPrio1 = { mX + 25 * scale, mY + 202 * scale, btnPrioW, 35 * scale };
                Rectangle recPrio2 = { mX + 35 * scale + btnPrioW, mY + 202 * scale, btnPrioW, 35 * scale };
                Rectangle recPrio3 = { mX + 45 * scale + btnPrioW * 2, mY + 202 * scale, btnPrioW, 35 * scale };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (CheckCollisionPointRec(mousePos, recID)) campoActivo = 0;
                    if (CheckCollisionPointRec(mousePos, recDesc)) campoActivo = 1;
                    if (CheckCollisionPointRec(mousePos, recPrio1)) prioSel = 1;
                    if (CheckCollisionPointRec(mousePos, recPrio2)) prioSel = 2;
                    if (CheckCollisionPointRec(mousePos, recPrio3)) prioSel = 3;
                }

                if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) prioSel = 1;
                if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) prioSel = 2;
                if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) prioSel = 3;

                DrawRectangleRec(recPrio1, (prioSel == 1 ? accentBlue : cardBg));
                DrawRectangleLinesEx(recPrio1, 1.0f * scale, (prioSel == 1 ? textPrimary : borderColor));
                DrawTextClean(customFont, "[1] Baja", { recPrio1.x + 25 * scale, recPrio1.y + 10 * scale }, fontBody, textPrimary);

                DrawRectangleRec(recPrio2, (prioSel == 2 ? accentAmber : cardBg));
                DrawRectangleLinesEx(recPrio2, 1.0f * scale, (prioSel == 2 ? textPrimary : borderColor));
                DrawTextClean(customFont, "[2] Media", { recPrio2.x + 20 * scale, recPrio2.y + 10 * scale }, fontBody, textPrimary);

                DrawRectangleRec(recPrio3, (prioSel == 3 ? accentRed : cardBg));
                DrawRectangleLinesEx(recPrio3, 1.0f * scale, (prioSel == 3 ? textPrimary : borderColor));
                DrawTextClean(customFont, "[3] VIP / Alta", { recPrio3.x + 15 * scale, recPrio3.y + 10 * scale }, fontBody, textPrimary);

                if (IsKeyPressed(KEY_TAB)) campoActivo = (campoActivo == 0) ? 1 : 0;

                int k = GetCharPressed();
                while (k > 0) {
                    if (campoActivo == 0 && strlen(bufID) < 10 && k >= '0' && k <= '9') { int l = strlen(bufID); bufID[l] = k; bufID[l + 1] = '\0'; }
                    else if (campoActivo == 1 && strlen(bufDesc) < 40 && k >= 32 && k <= 125) { int l = strlen(bufDesc); bufDesc[l] = k; bufDesc[l + 1] = '\0'; }
                    k = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (campoActivo == 0 && strlen(bufID) > 0) bufID[strlen(bufID) - 1] = '\0';
                    if (campoActivo == 1 && strlen(bufDesc) > 0) bufDesc[strlen(bufDesc) - 1] = '\0';
                }

                if (!msgModalErr.empty()) {
                    DrawTextClean(customFont, msgModalErr.c_str(), { mX + 25 * scale, mY + 250 * scale }, fontSmall, accentRed);
                }

                Rectangle recSave = { mX + 25 * scale, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };
                Rectangle recCanc = { mX + 35 * scale + (mW - 60 * scale) / 2, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };

                DrawRectangleRec(recSave, accentGreen);
                DrawTextClean(customFont, "[ENTER] Guardar Pedido", { recSave.x + 20 * scale, recSave.y + 10 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recCanc, cardBg);
                DrawRectangleLinesEx(recCanc, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cancelar (ESC)", { recCanc.x + 40 * scale, recCanc.y + 10 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCanc)) modal = NINGUNO;

                auto intentGuardar = [&]() {
                    if (strlen(bufID) == 0 || strlen(bufDesc) == 0) {
                        msgModalErr = "¡Error! Ingrese ID y Descripcion.";
                        return;
                    }
                    int idN = std::stoi(bufID);
                    if (manager.registrarPedido(idN, bufDesc, prioSel)) {
                        msgEstado = "[1] Pedido #" + std::to_string(idN) + " registrado correctamente.";
                        colEstado = accentGreen;
                        bufID[0] = '\0'; bufDesc[0] = '\0'; modal = NINGUNO;
                    }
                    else {
                        msgModalErr = "¡Error! El ID #" + std::to_string(idN) + " ya existe en el sistema.";
                    }
                    };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recSave)) {
                    intentGuardar();
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    if (campoActivo == 0 && strlen(bufID) > 0) campoActivo = 1;
                    else intentGuardar();
                }
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [2] ALERTA INTELIGENTE EN ALMACENAMIENTO
            else if (modal == MODAL_ALERTA_RACK) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "¡ALERTA INTELIGENTE DE ALMACENAMIENTO! [2]", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, accentRed);
                DrawTextClean(customFont, "El paquete entrante tiene MENOR prioridad que el que", { mX + 25 * scale, mY + 70 * scale }, fontBody, textPrimary);
                DrawTextClean(customFont, "esta en la cima del Rack. ¿Desea enterrar mercancia critica?", { mX + 25 * scale, mY + 95 * scale }, fontBody, accentAmber);

                Rectangle recConfirm = { mX + 40 * scale, mY + mH - 60 * scale, (mW - 100 * scale) / 2, 40 * scale };
                Rectangle recCancel = { mX + 60 * scale + (mW - 100 * scale) / 2, mY + mH - 60 * scale, (mW - 100 * scale) / 2, 40 * scale };

                DrawRectangleRec(recConfirm, accentRed);
                DrawTextClean(customFont, "[S] Confirmar/Forzar", { recConfirm.x + 20 * scale, recConfirm.y + 12 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recCancel, cardBg);
                DrawRectangleLinesEx(recCancel, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "[N] Cancelar", { recCancel.x + 40 * scale, recCancel.y + 12 * scale }, fontSmall, textPrimary);

                if (IsKeyPressed(KEY_S) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recConfirm))) {
                    manager.procesarAlmacenamiento(true); msgEstado = "[2] Forzado a Rack."; modal = NINGUNO;
                }
                if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCancel))) modal = NINGUNO;
            }

            // [4] DESPACHO POR LOTES
            else if (modal == MODAL_BATCH) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[4] DESPACHO POR LOTES (BATCH)", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);
                DrawTextClean(customFont, "Ingrese la cantidad de paquetes a despachar:", { mX + 25 * scale, mY + 70 * scale }, fontBody, textPrimary);
                Rectangle recB = { mX + 25 * scale, mY + 95 * scale, mW - 50 * scale, 35 * scale };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recB)) campoActivo = 0;

                DrawRectangleRec(recB, cardBg);
                DrawRectangleLinesEx(recB, 1.0f * scale, accentBlue);
                std::string tb = bufBatch; if (mCursor) tb += "|";
                DrawTextClean(customFont, tb.c_str(), { recB.x + 10 * scale, recB.y + 8 * scale }, fontBody, textPrimary);

                int k = GetCharPressed();
                while (k > 0) { if (strlen(bufBatch) < 5 && k >= '0' && k <= '9') { int l = strlen(bufBatch); bufBatch[l] = k; bufBatch[l + 1] = '\0'; } k = GetCharPressed(); }
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(bufBatch) > 0) bufBatch[strlen(bufBatch) - 1] = '\0';

                Rectangle recProc = { mX + 25 * scale, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };
                Rectangle recCancB = { mX + 35 * scale + (mW - 60 * scale) / 2, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };

                DrawRectangleRec(recProc, accentGreen);
                DrawTextClean(customFont, "[ENTER] Procesar Lote", { recProc.x + 20 * scale, recProc.y + 10 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recCancB, cardBg);
                DrawRectangleLinesEx(recCancB, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cancelar (ESC)", { recCancB.x + 40 * scale, recCancB.y + 10 * scale }, fontSmall, textPrimary);

                auto intentProc = [&]() {
                    if (strlen(bufBatch) > 0) {
                        int c = std::stoi(bufBatch);
                        int proc = manager.despacharPorLotes(c);
                        msgEstado = "[4] Lote procesado: " + std::to_string(proc) + " paquetes despachados.";
                        bufBatch[0] = '\0'; modal = NINGUNO;
                    }
                    };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recProc)) intentProc();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCancB)) modal = NINGUNO;
                if (IsKeyPressed(KEY_ENTER)) intentProc();
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [6] RASTREAR PEDIDO (TRACKING)
            else if (modal == MODAL_TRACKING) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[6] RASTREAR PEDIDO (TRACKING POR ID)", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);
                DrawTextClean(customFont, "Ingrese ID a buscar:", { mX + 25 * scale, mY + 55 * scale }, fontBody, textPrimary);
                Rectangle recT = { mX + 25 * scale, mY + 75 * scale, mW - 50 * scale, 32 * scale };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recT)) campoActivo = 0;

                DrawRectangleRec(recT, cardBg);
                DrawRectangleLinesEx(recT, 1.0f * scale, accentBlue);
                std::string tt = bufTrack; if (mCursor) tt += "|";
                DrawTextClean(customFont, tt.c_str(), { recT.x + 10 * scale, recT.y + 6 * scale }, fontBody, textPrimary);

                int k = GetCharPressed();
                while (k > 0) { if (strlen(bufTrack) < 8 && k >= '0' && k <= '9') { int l = strlen(bufTrack); bufTrack[l] = k; bufTrack[l + 1] = '\0'; } k = GetCharPressed(); }
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(bufTrack) > 0) bufTrack[strlen(bufTrack) - 1] = '\0';

                auto intentBuscar = [&]() {
                    if (strlen(bufTrack) > 0) {
                        trackingEncontrado = manager.rastrearPedido(std::stoi(bufTrack), trackingPedido, trackingProfundidad);
                    }
                    };

                if (IsKeyPressed(KEY_ENTER)) intentBuscar();

                if (trackingEncontrado) {
                    DrawRectangle((int)(mX + 25 * scale), (int)(mY + 120 * scale), (int)(mW - 50 * scale), (int)(220 * scale), cardBg);
                    DrawRectangleLinesEx({ mX + 25 * scale, mY + 120 * scale, mW - 50 * scale, 220 * scale }, 1.0f * scale, borderColor);

                    DrawTextClean(customFont, TextFormat("ID: #%d", trackingPedido.id), { mX + 40 * scale, mY + 135 * scale }, fontBody, accentAmber);
                    DrawTextClean(customFont, TextFormat("Descripcion: %s", trackingPedido.descripcion.c_str()), { mX + 40 * scale, mY + 165 * scale }, fontBody, textPrimary);
                    DrawTextClean(customFont, TextFormat("Prioridad: %d", trackingPedido.prioridad), { mX + 40 * scale, mY + 195 * scale }, fontBody, textPrimary);
                    DrawTextClean(customFont, TextFormat("Estado Actual: %s", trackingPedido.estado.c_str()), { mX + 40 * scale, mY + 225 * scale }, fontBody, accentGreen);

                    if (trackingPedido.estado == "Almacenado") {
                        DrawTextClean(customFont, TextFormat("Profundidad en Rack: Nivel %d desde la cima", trackingProfundidad), { mX + 40 * scale, mY + 255 * scale }, fontBody, accentBlue);
                    }
                    else if (trackingPedido.estado == "Despachado") {
                        DrawTextClean(customFont, TextFormat("Robot Asignado: %s", trackingPedido.robotAsignado.c_str()), { mX + 40 * scale, mY + 255 * scale }, fontBody, accentAmber);
                    }
                }
                else if (strlen(bufTrack) > 0) {
                    DrawTextClean(customFont, "Pedido no encontrado en el sistema.", { mX + 25 * scale, mY + 125 * scale }, fontBody, accentRed);
                }

                Rectangle recCloseTrack = { mX + (mW - 180 * scale) / 2, mY + mH - 45 * scale, 180 * scale, 35 * scale };
                DrawRectangleRec(recCloseTrack, cardBg);
                DrawRectangleLinesEx(recCloseTrack, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cerrar (ESC)", { recCloseTrack.x + 40 * scale, recCloseTrack.y + 10 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCloseTrack)) modal = NINGUNO;
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [8] SIMULACIÓN DE PAQUETES
            else if (modal == MODAL_SIMULACION) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[8] SIMULACION: CARGAR N ALEATORIOS", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);
                DrawTextClean(customFont, "Cantidad de paquetes de 4 digitos a generar:", { mX + 25 * scale, mY + 70 * scale }, fontBody, textPrimary);
                Rectangle recS = { mX + 25 * scale, mY + 95 * scale, mW - 50 * scale, 35 * scale };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recS)) campoActivo = 0;

                DrawRectangleRec(recS, cardBg);
                DrawRectangleLinesEx(recS, 1.0f * scale, accentBlue);
                std::string ts = bufSim; if (mCursor) ts += "|";
                DrawTextClean(customFont, ts.c_str(), { recS.x + 10 * scale, recS.y + 8 * scale }, fontBody, textPrimary);

                int k = GetCharPressed();
                while (k > 0) { if (strlen(bufSim) < 5 && k >= '0' && k <= '9') { int l = strlen(bufSim); bufSim[l] = k; bufSim[l + 1] = '\0'; } k = GetCharPressed(); }
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(bufSim) > 0) bufSim[strlen(bufSim) - 1] = '\0';

                Rectangle recGen = { mX + 25 * scale, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };
                Rectangle recCancS = { mX + 35 * scale + (mW - 60 * scale) / 2, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };

                DrawRectangleRec(recGen, accentGreen);
                DrawTextClean(customFont, "[ENTER] Generar", { recGen.x + 30 * scale, recGen.y + 10 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recCancS, cardBg);
                DrawRectangleLinesEx(recCancS, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cancelar (ESC)", { recCancS.x + 40 * scale, recCancS.y + 10 * scale }, fontSmall, textPrimary);

                auto intentSim = [&]() {
                    if (strlen(bufSim) > 0) {
                        int cant = std::stoi(bufSim);
                        manager.generarCargaSimulada(cant);
                        msgEstado = "[8] Generados " + std::to_string(cant) + " paquetes simulados.";
                        bufSim[0] = '\0'; modal = NINGUNO;
                    }
                    };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recGen)) intentSim();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCancS)) modal = NINGUNO;
                if (IsKeyPressed(KEY_ENTER)) intentSim();
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [10] RECONFIGURACIÓN DE PARÁMETROS
            else if (modal == MODAL_CONFIG) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[10] CONFIGURACION DE FLOTA Y RACK", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, textPrimary);

                DrawTextClean(customFont, "1. Nueva Cantidad de Robots:", { mX + 25 * scale, mY + 52 * scale }, fontSmall, textPrimary);
                Rectangle recC1 = { mX + 25 * scale, mY + 70 * scale, mW - 50 * scale, 30 * scale };
                DrawRectangleRec(recC1, (campoActivo == 0 ? cardBg : headerBg));
                DrawRectangleLinesEx(recC1, 1.0f * scale, (campoActivo == 0 ? accentBlue : borderColor));
                std::string cr = bufRobots; if (campoActivo == 0 && mCursor) cr += "|";
                DrawTextClean(customFont, cr.c_str(), { recC1.x + 10 * scale, recC1.y + 6 * scale }, fontSmall, textPrimary);

                DrawTextClean(customFont, "2. Capacidad Max. Envíos por Robot:", { mX + 25 * scale, mY + 110 * scale }, fontSmall, textPrimary);
                Rectangle recC2 = { mX + 25 * scale, mY + 128 * scale, mW - 50 * scale, 30 * scale };
                DrawRectangleRec(recC2, (campoActivo == 1 ? cardBg : headerBg));
                DrawRectangleLinesEx(recC2, 1.0f * scale, (campoActivo == 1 ? accentBlue : borderColor));
                std::string cb = bufCapBot; if (campoActivo == 1 && mCursor) cb += "|";
                DrawTextClean(customFont, cb.c_str(), { recC2.x + 10 * scale, recC2.y + 6 * scale }, fontSmall, textPrimary);

                DrawTextClean(customFont, "3. Capacidad Maxima del Rack:", { mX + 25 * scale, mY + 168 * scale }, fontSmall, textPrimary);
                Rectangle recC3 = { mX + 25 * scale, mY + 186 * scale, mW - 50 * scale, 30 * scale };
                DrawRectangleRec(recC3, (campoActivo == 2 ? cardBg : headerBg));
                DrawRectangleLinesEx(recC3, 1.0f * scale, (campoActivo == 2 ? accentBlue : borderColor));
                std::string ck = bufCapRack; if (campoActivo == 2 && mCursor) ck += "|";
                DrawTextClean(customFont, ck.c_str(), { recC3.x + 10 * scale, recC3.y + 6 * scale }, fontSmall, textPrimary);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (CheckCollisionPointRec(mousePos, recC1)) campoActivo = 0;
                    if (CheckCollisionPointRec(mousePos, recC2)) campoActivo = 1;
                    if (CheckCollisionPointRec(mousePos, recC3)) campoActivo = 2;
                }

                if (IsKeyPressed(KEY_TAB)) campoActivo = (campoActivo + 1) % 3;

                int k = GetCharPressed();
                while (k > 0) {
                    if (campoActivo == 0 && strlen(bufRobots) < 4 && k >= '0' && k <= '9') { int l = strlen(bufRobots); bufRobots[l] = k; bufRobots[l + 1] = '\0'; }
                    else if (campoActivo == 1 && strlen(bufCapBot) < 4 && k >= '0' && k <= '9') { int l = strlen(bufCapBot); bufCapBot[l] = k; bufCapBot[l + 1] = '\0'; }
                    else if (campoActivo == 2 && strlen(bufCapRack) < 4 && k >= '0' && k <= '9') { int l = strlen(bufCapRack); bufCapRack[l] = k; bufCapRack[l + 1] = '\0'; }
                    k = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (campoActivo == 0 && strlen(bufRobots) > 0) bufRobots[strlen(bufRobots) - 1] = '\0';
                    if (campoActivo == 1 && strlen(bufCapBot) > 0) bufCapBot[strlen(bufCapBot) - 1] = '\0';
                    if (campoActivo == 2 && strlen(bufCapRack) > 0) bufCapRack[strlen(bufCapRack) - 1] = '\0';
                }

                if (!msgModalErr.empty()) {
                    DrawTextClean(customFont, msgModalErr.c_str(), { mX + 25 * scale, mY + 230 * scale }, fontSmall, accentRed);
                }

                Rectangle recApply = { mX + 25 * scale, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };
                Rectangle recCancCfg = { mX + 35 * scale + (mW - 60 * scale) / 2, mY + mH - 50 * scale, (mW - 60 * scale) / 2, 38 * scale };

                DrawRectangleRec(recApply, accentGreen);
                DrawTextClean(customFont, "[ENTER] Aplicar Cambios", { recApply.x + 15 * scale, recApply.y + 10 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recCancCfg, cardBg);
                DrawRectangleLinesEx(recCancCfg, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "Cancelar (ESC)", { recCancCfg.x + 40 * scale, recCancCfg.y + 10 * scale }, fontSmall, textPrimary);

                auto intentAplicarConfig = [&]() {
                    if (strlen(bufRobots) == 0 || strlen(bufCapBot) == 0 || strlen(bufCapRack) == 0) {
                        msgModalErr = "¡Error! Ingrese valores en todos los campos.";
                        return;
                    }
                    int nRobots = std::stoi(bufRobots);
                    int nCapBot = std::stoi(bufCapBot);
                    int nCapRack = std::stoi(bufCapRack);

                    if (nRobots <= 0 || nCapBot <= 0 || nCapRack <= 0) {
                        msgModalErr = "¡Error! Los valores deben ser mayores a 0.";
                        return;
                    }

                    manager.reconfigurarParametros(nRobots, nCapBot, nCapRack);
                    msgEstado = "[10] Sistema reconfigurado: " + std::to_string(nRobots) + " Robots, Cap. Robot: " + std::to_string(nCapBot) + ", Cap. Rack: " + std::to_string(nCapRack);
                    colEstado = accentGreen;
                    modal = NINGUNO;
                    };

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recApply)) intentAplicarConfig();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recCancCfg)) modal = NINGUNO;

                if (IsKeyPressed(KEY_ENTER)) {
                    if (campoActivo == 0) campoActivo = 1;
                    else if (campoActivo == 1) campoActivo = 2;
                    else intentAplicarConfig();
                }
                if (IsKeyPressed(KEY_ESCAPE)) modal = NINGUNO;
            }

            // [12] ELIMINAR HISTORIAL Y BORRAR CSV
            else if (modal == MODAL_BORRAR_CSV) {
                DrawRectangle((int)mX, (int)mY, (int)mW, (int)(38 * scale), headerBg);
                DrawTextClean(customFont, "[12] ELIMINAR HISTORIAL PERSISTENTE", { mX + 15 * scale, mY + 10 * scale }, fontSubtitle, accentRed);
                DrawTextClean(customFont, "¿Esta seguro de borrar 'data/inventario.csv' y vaciar la memoria?", { mX + 25 * scale, mY + 70 * scale }, fontBody, textPrimary);

                Rectangle recDelete = { mX + 40 * scale, mY + mH - 60 * scale, (mW - 100 * scale) / 2, 40 * scale };
                Rectangle recKeep = { mX + 60 * scale + (mW - 100 * scale) / 2, mY + mH - 60 * scale, (mW - 100 * scale) / 2, 40 * scale };

                DrawRectangleRec(recDelete, accentRed);
                DrawTextClean(customFont, "[S] Confirmar Borrado", { recDelete.x + 20 * scale, recDelete.y + 12 * scale }, fontSmall, textPrimary);

                DrawRectangleRec(recKeep, cardBg);
                DrawRectangleLinesEx(recKeep, 1.0f * scale, borderColor);
                DrawTextClean(customFont, "[N] Cancelar", { recKeep.x + 40 * scale, recKeep.y + 12 * scale }, fontSmall, textPrimary);

                if (IsKeyPressed(KEY_S) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recDelete))) {
                    manager.eliminarHistorialYCSV();
                    msgEstado = "[12] Historial eliminado y memoria vaciada.";
                    modal = NINGUNO;
                }
                if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, recKeep))) modal = NINGUNO;
            }
        }

        EndDrawing();
    }

    // ------------------- FINALIZACIÓN Y GUARDADO PERSISTENTE -------------------
    manager.guardarEnCSV("data/inventario.csv");
    if (customFont.texture.id > 0) {
        UnloadFont(customFont);
    }
    CloseWindow();
    return 0;
}