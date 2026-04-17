#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace std;

// ==========================================
// 1. MATEMÁTICAS PARA LA TRIANGULACIÓN
// ==========================================
float ProductoCruz(Vector2 a, Vector2 b, Vector2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool PuntoEnTriangulo(Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
    float cp1 = ProductoCruz(a, b, p);
    float cp2 = ProductoCruz(b, c, p);
    float cp3 = ProductoCruz(c, a, p);
    bool tieneNegativo = (cp1 < 0) || (cp2 < 0) || (cp3 < 0);
    bool tienePositivo = (cp1 > 0) || (cp2 > 0) || (cp3 > 0);
    return !(tieneNegativo && tienePositivo);
}

vector<Vector2> TriangularPoligono(vector<Vector2> vertices) {
    vector<Vector2> triangulos;
    if (vertices.size() < 3) return triangulos;
    vector<Vector2> v = vertices;

    float area = 0;
    for (int i = 0; i < v.size(); i++) {
        int j = (i + 1) % v.size();
        area += (v[i].x * v[j].y) - (v[j].x * v[i].y);
    }
    if (area > 0) reverse(v.begin(), v.end());

    int intentos = 0;
    while (v.size() > 3 && intentos < 1000) {
        bool orejaCortada = false;
        int n = v.size();
        for (int i = 0; i < n; i++) {
            int prev = (i - 1 + n) % n;
            int next = (i + 1) % n;
            Vector2 A = v[prev], B = v[i], C = v[next];

            if (ProductoCruz(A, B, C) >= 0) continue;

            bool esOreja = true;
            for (int j = 0; j < n; j++) {
                if (j == prev || j == i || j == next) continue;
                if (PuntoEnTriangulo(v[j], A, B, C)) {
                    esOreja = false; break;
                }
            }
            if (esOreja) {
                triangulos.push_back(A); triangulos.push_back(B); triangulos.push_back(C);
                v.erase(v.begin() + i);
                orejaCortada = true; break;
            }
        }
        intentos++;
        if (!orejaCortada) break;
    }
    if (v.size() == 3) {
        triangulos.push_back(v[0]); triangulos.push_back(v[1]); triangulos.push_back(v[2]);
    }
    return triangulos;
}

// ==========================================
// 2. MATEMÁTICAS DE TRANSFORMACIONES
// ==========================================
Vector2 ConvertirAPantalla(Vector2 mathP, int origenX, int origenY) {
    return { mathP.x + origenX, origenY - mathP.y };
}

Vector2 Lerp(Vector2 a, Vector2 b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 650;

    InitWindow(screenWidth, screenHeight, "VectraLab");
    SetTargetFPS(60);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    int origenX = (screenWidth - 350) / 2;
    int origenY = screenHeight / 2;

    // --- VARIABLES DE ESTADO Y UI ---
    vector<Vector2> figuraOriginal = { {50, 50}, {150, 50}, {150, 150}, {50, 150} };
    vector<Vector2> figuraObjetivo = figuraOriginal;
    vector<Vector2> figuraAnimada = figuraOriginal;

    char inputX[16] = "";
    char inputY[16] = "";
    bool editX = false;
    bool editY = false;

    // Dos variables independientes para los dos menús desplegables
    bool dropdownTransformacionEdit = false;
    bool dropdownReflexionEdit = false;

    int transformacionSeleccionada = 0;
    float valorAngulo = 90.0f;
    float valorEscala = 1.5f;
    int ejeReflexion = 0; // 0: X, 1: Y, 2: Origen, 3: Y=X, 4: Y=-X

    float animacionT = 1.0f;

    while (!WindowShouldClose()) {

        // --- LÓGICA DE ANIMACIÓN PROGRESIVA ---
        if (animacionT < 1.0f) {
            animacionT += 0.015f;
            if (animacionT > 1.0f) animacionT = 1.0f;

            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                figuraAnimada[i] = Lerp(figuraOriginal[i], figuraObjetivo[i], animacionT);
            }
        }

        // ==========================================
        // DIBUJO DEL LIENZO
        // ==========================================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawLine(0, origenY, screenWidth - 350, origenY, LIGHTGRAY);
        DrawLine(origenX, 0, origenX, screenHeight, LIGHTGRAY);
        DrawCircle(origenX, origenY, 4, RED);

        vector<Vector2> puntosPantalla;
        for (auto p : figuraAnimada) {
            puntosPantalla.push_back(ConvertirAPantalla(p, origenX, origenY));
        }

        if (puntosPantalla.size() >= 3) {
            vector<Vector2> triangulos = TriangularPoligono(puntosPantalla);

            Color colorRelleno = { 0, 121, 241, 150 };
            for (size_t i = 0; i + 2 < triangulos.size(); i += 3) {
                DrawTriangle(triangulos[i], triangulos[i + 1], triangulos[i + 2], colorRelleno);
            }

            for (size_t i = 0; i < puntosPantalla.size(); i++) {
                int sig = (i + 1) % puntosPantalla.size();
                DrawLineEx(puntosPantalla[i], puntosPantalla[sig], 3.0f, DARKBLUE);
            }
        }
        else if (puntosPantalla.size() == 2) {
            DrawLineEx(puntosPantalla[0], puntosPantalla[1], 3.0f, DARKBLUE);
        }

        for (size_t i = 0; i < puntosPantalla.size(); i++) {
            DrawCircleV(puntosPantalla[i], 5.0f, MAROON);
        }

        // ==========================================
        // DIBUJO DEL PANEL DE CONTROL (RAYGUI)
        // ==========================================
        float panelX = screenWidth - 350.0f;
        DrawRectangle((int)panelX, 0, 350, screenHeight, Fade(LIGHTGRAY, 0.4f));
        DrawLine((int)panelX, 0, (int)panelX, screenHeight, GRAY);

        DrawText("DASHBOARD DE MATRICES", (int)panelX + 50, 20, 20, DARKBLUE);

        // --- SECCIÓN 1: Ingreso de Puntos ---
        GuiGroupBox(Rectangle{ panelX + 20.0f, 60.0f, 310.0f, 140.0f }, "1. GESTION DE PUNTOS");

        GuiLabel(Rectangle{ panelX + 40.0f, 80.0f, 20.0f, 30.0f }, "X:");
        if (GuiTextBox(Rectangle{ panelX + 60.0f, 80.0f, 80.0f, 30.0f }, inputX, 16, editX)) editX = !editX;

        GuiLabel(Rectangle{ panelX + 160.0f, 80.0f, 20.0f, 30.0f }, "Y:");
        if (GuiTextBox(Rectangle{ panelX + 180.0f, 80.0f, 80.0f, 30.0f }, inputY, 16, editY)) editY = !editY;

        if (GuiButton(Rectangle{ panelX + 40.0f, 120.0f, 130.0f, 30.0f }, "Añadir Punto")) {
            try {
                float px = stof(inputX);
                float py = stof(inputY);
                figuraOriginal.push_back({ px, py });
                figuraObjetivo = figuraOriginal;
                figuraAnimada = figuraOriginal;
                inputX[0] = '\0'; inputY[0] = '\0';
            }
            catch (...) {}
        }

        if (GuiButton(Rectangle{ panelX + 180.0f, 120.0f, 130.0f, 30.0f }, "Limpiar Figura")) {
            figuraOriginal.clear(); figuraObjetivo.clear(); figuraAnimada.clear();
        }

        DrawText(TextFormat("Vertices actuales: %d", figuraOriginal.size()), (int)panelX + 40, 165, 14, DARKGRAY);

        // --- SECCIÓN 2: Parámetros Matemáticos ---
        GuiGroupBox(Rectangle{ panelX + 20.0f, 220.0f, 310.0f, 220.0f }, "2. PARÁMETROS MATEMÁTICOS");

        // Dibujamos primero los controles secundarios (Sliders)
        if (transformacionSeleccionada == 0) {
            DrawText(TextFormat("Angulo (Grados): %.1f", valorAngulo), (int)panelX + 40, 350, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 380.0f, 270.0f, 20.0f }, "-360", "360", &valorAngulo, -360.0f, 360.0f);
        }
        else if (transformacionSeleccionada == 1) {
            DrawText(TextFormat("Factor Escala (k): %.2f", valorEscala), (int)panelX + 40, 350, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 380.0f, 270.0f, 20.0f }, "0.1", "3.0", &valorEscala, 0.1f, 3.0f);
        }

        // --- SECCIÓN 3: Botón Aplicar ---
        // Protegemos el botón para que no se presione por accidente si un menú está abierto
        if (dropdownTransformacionEdit || dropdownReflexionEdit) GuiDisable();

        if (GuiButton(Rectangle{ panelX + 20.0f, 460.0f, 310.0f, 50.0f }, "APLICAR TRANSFORMACIÓN (Animar)")) {
            figuraOriginal = figuraObjetivo;
            animacionT = 0.0f;

            for (size_t i = 0; i < figuraObjetivo.size(); i++) {
                float px = figuraOriginal[i].x;
                float py = figuraOriginal[i].y;

                if (transformacionSeleccionada == 0) { // ROTACION
                    float rad = valorAngulo * (PI / 180.0f);
                    figuraObjetivo[i].x = px * cos(rad) - py * sin(rad);
                    figuraObjetivo[i].y = px * sin(rad) + py * cos(rad);
                }
                else if (transformacionSeleccionada == 1) { // HOMOTECIA
                    figuraObjetivo[i].x = px * valorEscala;
                    figuraObjetivo[i].y = py * valorEscala;
                }
                else if (transformacionSeleccionada == 2) { // REFLEXION
                    if (ejeReflexion == 0) { figuraObjetivo[i].x = px; figuraObjetivo[i].y = -py; } // Eje X
                    if (ejeReflexion == 1) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = py; } // Eje Y
                    if (ejeReflexion == 2) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = -py; } // Origen
                    if (ejeReflexion == 3) { figuraObjetivo[i].x = py; figuraObjetivo[i].y = px; } // Y = X
                    if (ejeReflexion == 4) { figuraObjetivo[i].x = -py; figuraObjetivo[i].y = -px; } // Y = -X
                }
            }
        }

        GuiEnable(); // Volvemos a habilitar la interfaz

        // --- DIBUJAR LOS MENÚS DESPLEGABLES AL FINAL ---
        // Se dibujan al final para que queden visualmente "encima" de todo lo demás

        // Menú Secundario: Tipos de Reflexión (Solo aparece si seleccionamos Reflexión)
        if (transformacionSeleccionada == 2) {
            DrawText("Seleccione la Reflexion:", (int)panelX + 40, 350, 16, BLACK);
            if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 375.0f, 270.0f, 30.0f }, "Eje X;Eje Y;Origen;Recta Y = X;Recta Y = -X", &ejeReflexion, dropdownReflexionEdit)) {
                dropdownReflexionEdit = !dropdownReflexionEdit;
            }
        }

        // Menú Principal: Tipo de Transformación (Arriba de todo)
        GuiLabel(Rectangle{ panelX + 40.0f, 240.0f, 200.0f, 20.0f }, "Tipo de Transformacion:");
        if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 265.0f, 270.0f, 30.0f }, "ROTACION;HOMOTECIA;REFLEXION", &transformacionSeleccionada, dropdownTransformacionEdit)) {
            dropdownTransformacionEdit = !dropdownTransformacionEdit;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}