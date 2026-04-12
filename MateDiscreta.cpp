#include "pch.h" // Si Visual Studio te da error aquí, puedes borrar esta línea.
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

// Interpolación Lineal para la animación progresiva
Vector2 Lerp(Vector2 a, Vector2 b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

int main() {
    const int screenWidth = 1000; // Más ancho para el panel
    const int screenHeight = 650;

    InitWindow(screenWidth, screenHeight, "Dashboard - Transformaciones Lineales 2D");
    SetTargetFPS(60);

    // Estilo de la Interfaz
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    // Centro del plano cartesiano (Desplazado a la izquierda para dejar espacio al panel)
    int origenX = (screenWidth - 350) / 2;
    int origenY = screenHeight / 2;

    // --- VARIABLES DE ESTADO Y UI ---
    // Figura por defecto (Cuadrado tipo "pixel")
    vector<Vector2> figuraOriginal = { {50, 50}, {150, 50}, {150, 150}, {50, 150} };
    vector<Vector2> figuraObjetivo = figuraOriginal;
    vector<Vector2> figuraAnimada = figuraOriginal;

    // Variables de cajas de texto
    char inputX[16] = "";
    char inputY[16] = "";
    bool editX = false;
    bool editY = false;

    // Variables de Transformación
    bool dropdownEditMode = false;
    int transformacionSeleccionada = 0; // 0: Rotación, 1: Homotecia, 2: Reflexión
    float valorAngulo = 90.0f;
    float valorEscala = 1.5f;
    int ejeReflexion = 0; // 0: Eje X, 1: Eje Y

    // Variables de Animación
    float animacionT = 1.0f; // 1.0 significa que la animación terminó

    while (!WindowShouldClose()) {

        // --- LÓGICA DE ANIMACIÓN PROGRESIVA ---
        if (animacionT < 1.0f) {
            animacionT += 0.015f; // Velocidad de la animación
            if (animacionT > 1.0f) animacionT = 1.0f;

            // Calcular puntos intermedios
            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                figuraAnimada[i] = Lerp(figuraOriginal[i], figuraObjetivo[i], animacionT);
            }
        }

        // ==========================================
        // DIBUJO DEL LIENZO
        // ==========================================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. Dibujar Plano Cartesiano
        DrawLine(0, origenY, screenWidth - 350, origenY, LIGHTGRAY); // Eje X
        DrawLine(origenX, 0, origenX, screenHeight, LIGHTGRAY);      // Eje Y
        DrawCircle(origenX, origenY, 4, RED); // Centro

        // 2. Convertir coordenadas animadas a pantalla y Triangular
        if (figuraAnimada.size() >= 3) {
            vector<Vector2> puntosPantalla;
            for (auto p : figuraAnimada) puntosPantalla.push_back(ConvertirAPantalla(p, origenX, origenY));

            vector<Vector2> triangulos = TriangularPoligono(puntosPantalla);

            // Dibujar Relleno
            Color colorRelleno = { 0, 121, 241, 150 }; // Azul semitransparente
            for (size_t i = 0; i + 2 < triangulos.size(); i += 3) {
                DrawTriangle(triangulos[i], triangulos[i + 1], triangulos[i + 2], colorRelleno);
            }

            // Dibujar Contorno
            for (size_t i = 0; i < puntosPantalla.size(); i++) {
                int sig = (i + 1) % puntosPantalla.size();
                DrawLineEx(puntosPantalla[i], puntosPantalla[sig], 3.0f, DARKBLUE);
            }
        }

        // ==========================================
        // DIBUJO DEL PANEL DE CONTROL (RAYGUI)
        // ==========================================
        float panelX = screenWidth - 350.0f; // <-- Convertido a float para evitar el error C2397
        DrawRectangle((int)panelX, 0, 350, screenHeight, Fade(LIGHTGRAY, 0.4f));
        DrawLine((int)panelX, 0, (int)panelX, screenHeight, GRAY);

        DrawText("DASHBOARD DE MATRICES", (int)panelX + 50, 20, 20, DARKBLUE);

        // --- SECCIÓN 1: Ingreso de Vértices ---
        // Sintaxis C++ correcta: Rectangle{...} en lugar de (Rectangle){...}
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

        // --- SECCIÓN 2: Opciones Dinámicas ---
        GuiGroupBox(Rectangle{ panelX + 20.0f, 220.0f, 310.0f, 200.0f }, "2. PARÁMETROS MATEMÁTICOS");

        if (transformacionSeleccionada == 0) {
            DrawText(TextFormat("Angulo (Grados): %.1f", valorAngulo), (int)panelX + 40, 250, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 280.0f, 270.0f, 20.0f }, "-360", "360", &valorAngulo, -360.0f, 360.0f);
        }
        else if (transformacionSeleccionada == 1) {
            DrawText(TextFormat("Factor Escala (k): %.2f", valorEscala), (int)panelX + 40, 250, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 280.0f, 270.0f, 20.0f }, "0.1", "3.0", &valorEscala, 0.1f, 3.0f);
        }
        else if (transformacionSeleccionada == 2) {
            DrawText("Eje de Reflexion:", (int)panelX + 40, 250, 16, BLACK);
            GuiToggleGroup(Rectangle{ panelX + 40.0f, 280.0f, 130.0f, 30.0f }, "EJE X;EJE Y", &ejeReflexion);
        }

        // --- SECCIÓN 3: Botón de Ejecución (Las Matemáticas) ---
        if (GuiButton(Rectangle{ panelX + 20.0f, 440.0f, 310.0f, 50.0f }, "APLICAR TRANSFORMACIÓN (Animar)")) {
            figuraOriginal = figuraObjetivo;
            animacionT = 0.0f;

            for (size_t i = 0; i < figuraObjetivo.size(); i++) {
                float px = figuraOriginal[i].x;
                float py = figuraOriginal[i].y;

                if (transformacionSeleccionada == 0) {
                    float rad = valorAngulo * (PI / 180.0f);
                    figuraObjetivo[i].x = px * cos(rad) - py * sin(rad);
                    figuraObjetivo[i].y = px * sin(rad) + py * cos(rad);
                }
                else if (transformacionSeleccionada == 1) {
                    figuraObjetivo[i].x = px * valorEscala;
                    figuraObjetivo[i].y = py * valorEscala;
                }
                else if (transformacionSeleccionada == 2) {
                    if (ejeReflexion == 0) figuraObjetivo[i].y = -py;
                    if (ejeReflexion == 1) figuraObjetivo[i].x = -px;
                }
            }
        }

        // --- SECCIÓN 4: Menú Desplegable ---
        GuiLabel(Rectangle{ panelX + 20.0f, 510.0f, 200.0f, 20.0f }, "Tipo de Transformacion:");
        if (GuiDropdownBox(Rectangle{ panelX + 20.0f, 535.0f, 310.0f, 35.0f }, "ROTACION;HOMOTECIA;REFLEXION", &transformacionSeleccionada, dropdownEditMode)) {
            dropdownEditMode = !dropdownEditMode;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}