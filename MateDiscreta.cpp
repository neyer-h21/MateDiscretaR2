#include "pch.h"
#include <iostream>
#include <vector>
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace std;

// Función matemática vital (igual que antes)
Vector2 ConvertirAPantalla(float mathX, float mathY, int anchoPantalla, int altoPantalla) {
    Vector2 puntoPantalla;
    puntoPantalla.x = mathX + (anchoPantalla / 2.0f);
    puntoPantalla.y = (altoPantalla / 2.0f) - mathY;
    return puntoPantalla;
}

int main() {
    // Hacemos la ventana más ancha para que quepa el panel de control
    const int screenWidth = 1000;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Dashboard - Transformaciones Lineales");
    SetTargetFPS(60);

    // ==========================================
    // VARIABLES DEL DASHBOARD (UI)
    // ==========================================
    int transformacionSeleccionada = 0; // 0: Rotacion, 1: Homotecia, 2: Reflexion
    bool modoEdicionDropdown = false;   // Controla si el menú está desplegado

    float valorAngulo = 0.0f;           // Para el slider de rotación
    float valorEscala = 1.0f;           // Para la homotecia

    // Figura de prueba (Un cuadrado centrado)
    vector<Vector2> puntosMatematicos = { {-50, 50}, {50, 50}, {50, -50}, {-50, -50} };

    // Estilo general de la UI (hacer los controles un poco más grandes)
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    while (!WindowShouldClose()) {

        // ==========================================
        // LÓGICA DE DIBUJO Y UI
        // ==========================================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // --- ZONA 1: EL LIENZO (Plano Cartesiano a la izquierda) ---
        // El centro matemático ahora está desplazado para no chocar con el panel
        int centroLienzoX = (screenWidth - 300) / 2;
        int centroLienzoY = screenHeight / 2;

        DrawLine(0, centroLienzoY, screenWidth - 300, centroLienzoY, LIGHTGRAY); // Eje X
        DrawLine(centroLienzoX, 0, centroLienzoX, screenHeight, LIGHTGRAY);      // Eje Y
        DrawCircle(centroLienzoX, centroLienzoY, 4, RED);                        // Origen

        // Dibujar figura de prueba (usando tu lógica previa o triangulación)
        for (int i = 0; i < puntosMatematicos.size(); i++) {
            Vector2 p1 = { puntosMatematicos[i].x + centroLienzoX, centroLienzoY - puntosMatematicos[i].y };
            int sig = (i + 1) % puntosMatematicos.size();
            Vector2 p2 = { puntosMatematicos[sig].x + centroLienzoX, centroLienzoY - puntosMatematicos[sig].y };
            DrawLineEx(p1, p2, 3.0f, DARKBLUE);
        }

        // --- ZONA 2: PANEL DE CONTROL (Derecha) ---
        // Fondo del panel
        DrawRectangle(screenWidth - 300, 0, 300, screenHeight, Fade(LIGHTGRAY, 0.5f));
        DrawLine(screenWidth - 300, 0, screenWidth - 300, screenHeight, DARKGRAY);
        DrawText("PANEL DE CONTROL", screenWidth - 260, 20, 20, DARKBLUE);

        // Controles interactivos (Deben dibujarse de abajo hacia arriba si hay menús desplegables)

        // 1. Botón de Acción Principal
        if (GuiButton((Rectangle) { screenWidth - 250, 450, 200, 40 }, "APLICAR TRANSFORMACIÓN")) {
            // AQUI IRÁ LA MULTIPLICACIÓN DE MATRICES
            cout << "Boton presionado. Aplicar: " << transformacionSeleccionada << endl;
        }

        // 2. Parámetros dinámicos según lo seleccionado
        if (transformacionSeleccionada == 0) { // Rotación
            DrawText("Ángulo de Rotación (Grados):", screenWidth - 280, 140, 16, DARKGRAY);
            // Un slider para elegir el ángulo interactivamente
            GuiSlider((Rectangle) { screenWidth - 250, 170, 160, 20 }, "0", "360", & valorAngulo, 0.0f, 360.0f);
        }
        else if (transformacionSeleccionada == 1) { // Homotecia
            DrawText("Factor de Escala (k):", screenWidth - 280, 140, 16, DARKGRAY);
            GuiSlider((Rectangle) { screenWidth - 250, 170, 160, 20 }, "0.1", "3.0", & valorEscala, 0.1f, 3.0f);
        }
        else if (transformacionSeleccionada == 2) { // Reflexión
            DrawText("Eje de Reflexión:", screenWidth - 280, 140, 16, DARKGRAY);
            // Botones simples como opciones
            if (GuiButton((Rectangle) { screenWidth - 280, 170, 90, 30 }, "EJE X")) { /* Lógica matriz X */ }
            if (GuiButton((Rectangle) { screenWidth - 180, 170, 90, 30 }, "EJE Y")) { /* Lógica matriz Y */ }
        }

        // 3. Menú Desplegable (Combobox) - SIEMPRE DEBE DIBUJARSE AL FINAL
        // para que la lista se despliegue "por encima" de los demás controles
        DrawText("Elegir Transformación:", screenWidth - 280, 70, 16, DARKGRAY);
        if (GuiDropdownBox((Rectangle) { screenWidth - 280, 95, 260, 30 }, "Rotacion;Homotecia;Reflexion", & transformacionSeleccionada, modoEdicionDropdown)) {
            modoEdicionDropdown = !modoEdicionDropdown; // Abre o cierra el menú al hacer clic
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}