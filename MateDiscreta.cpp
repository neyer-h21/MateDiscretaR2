#define _CRT_SECURE_NO_WARNINGS
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
    for (int i = 0; i < (int)v.size(); i++) {
        int j = (i + 1) % v.size();
        area += (v[i].x * v[j].y) - (v[j].x * v[i].y);
    }
    if (area > 0) reverse(v.begin(), v.end());

    int intentos = 0;
    while (v.size() > 3 && intentos < 1000) {
        bool orejaCortada = false;
        int n = (int)v.size();
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
    return { mathP.x + (float)origenX, (float)origenY - mathP.y };
}

Vector2 InterpolarPuntos(Vector2 a, Vector2 b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

float InterpolarValor(float start, float end, float t) {
    return start + (end - start) * t;
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 650;

    InitWindow(screenWidth, screenHeight, "VectraLab - Transformaciones Lineales 2D");
    SetTargetFPS(60);

    Texture2D texturaPersonaje = LoadTexture("personaje.png");
    bool modoVideojuego = false;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    int origenX = (screenWidth - 350) / 2;
    int origenY = screenHeight / 2;

    vector<Vector2> figuraOriginal = { {50, 50}, {150, 50}, {150, 150}, {50, 150} };
    vector<Vector2> figuraObjetivo = figuraOriginal;
    vector<Vector2> figuraAnimada = figuraOriginal;
    vector<Vector2> figuraInicioAnimacion = figuraOriginal;

    char inputX[16] = "";
    char inputY[16] = "";
    bool editX = false;
    bool editY = false;

    // --- VARIABLES PARA CENTRO DE HOMOTECIA ---
    char inputCx[16] = "0";
    char inputCy[16] = "0";
    bool editCx = false;
    bool editCy = false;
    float centroHomoteciaX = 0.0f;
    float centroHomoteciaY = 0.0f;

    bool menuTransfAbierto = false;
    bool menuReflexAbierto = false;

    int transformacionSeleccionada = 0;
    int transformacionPrevia = 0;

    int ejeReflexion = 0;
    int ejeReflexionPrevio = 0;

    float valorAngulo = 0.0f;
    float valorEscala = 1.0f;

    float anguloActualVisual = 0.0f;
    float anguloInicioAnimacion = 0.0f;
    float anguloDestinoAnimacion = 0.0f;

    float escalaActualVisual = 1.0f;
    float escalaInicioAnimacion = 1.0f;
    float escalaDestinoAnimacion = 1.0f;

    float imgAngInicial = 0.0f;  float imgAngFinal = 0.0f;  float imgAngActual = 0.0f;
    float imgEscalaInicialX = 1.0f; float imgEscalaFinalX = 1.0f; float imgEscalaActualX = 1.0f;
    float imgEscalaInicialY = 1.0f; float imgEscalaFinalY = 1.0f; float imgEscalaActualY = 1.0f;

    float animacionT = 1.0f;

    while (!WindowShouldClose()) {

        if (transformacionSeleccionada != transformacionPrevia ||
            (transformacionSeleccionada == 2 && ejeReflexion != ejeReflexionPrevio)) {

            figuraObjetivo = figuraOriginal;
            figuraAnimada = figuraOriginal;
            figuraInicioAnimacion = figuraOriginal;
            animacionT = 1.0f;
            valorAngulo = 0.0f;
            valorEscala = 1.0f;

            inputCx[0] = '0'; inputCx[1] = '\0';
            inputCy[0] = '0'; inputCy[1] = '\0';
            centroHomoteciaX = 0.0f;
            centroHomoteciaY = 0.0f;

            anguloActualVisual = 0.0f; anguloInicioAnimacion = 0.0f; anguloDestinoAnimacion = 0.0f;
            escalaActualVisual = 1.0f; escalaInicioAnimacion = 1.0f; escalaDestinoAnimacion = 1.0f;

            imgAngInicial = 0.0f;  imgAngFinal = 0.0f;  imgAngActual = 0.0f;
            imgEscalaInicialX = 1.0f; imgEscalaFinalX = 1.0f; imgEscalaActualX = 1.0f;
            imgEscalaInicialY = 1.0f; imgEscalaFinalY = 1.0f; imgEscalaActualY = 1.0f;

            transformacionPrevia = transformacionSeleccionada;
            ejeReflexionPrevio = ejeReflexion;
        }

        if (animacionT < 1.0f) {
            animacionT += 0.015f;
            if (animacionT > 1.0f) animacionT = 1.0f;

            float smoothT = animacionT * animacionT * (3.0f - 2.0f * animacionT);

            imgAngActual = InterpolarValor(imgAngInicial, imgAngFinal, smoothT);

            if (transformacionSeleccionada == 2) {
                imgEscalaActualX = imgEscalaFinalX;
                imgEscalaActualY = imgEscalaFinalY;
            }
            else {
                imgEscalaActualX = InterpolarValor(imgEscalaInicialX, imgEscalaFinalX, smoothT);
                imgEscalaActualY = InterpolarValor(imgEscalaInicialY, imgEscalaFinalY, smoothT);
            }

            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                float px = figuraOriginal[i].x;
                float py = figuraOriginal[i].y;

                if (transformacionSeleccionada == 0) {
                    float anguloInterpolado = anguloInicioAnimacion + (anguloDestinoAnimacion - anguloInicioAnimacion) * smoothT;
                    anguloActualVisual = anguloInterpolado;
                    float rad = anguloInterpolado * (PI / 180.0f);
                    figuraAnimada[i].x = px * cos(rad) - py * sin(rad);
                    figuraAnimada[i].y = px * sin(rad) + py * cos(rad);
                }
                else if (transformacionSeleccionada == 1) {
                    float escalaInterpolada = escalaInicioAnimacion + (escalaDestinoAnimacion - escalaInicioAnimacion) * smoothT;
                    escalaActualVisual = escalaInterpolada;
                    figuraAnimada[i].x = centroHomoteciaX + (px - centroHomoteciaX) * escalaInterpolada;
                    figuraAnimada[i].y = centroHomoteciaY + (py - centroHomoteciaY) * escalaInterpolada;
                }
                else if (transformacionSeleccionada == 2) {
                    if (ejeReflexion == 2) {
                        float diffX = fabs(figuraInicioAnimacion[0].x - figuraObjetivo[0].x);
                        float targetRad = (diffX > 0.1f) ? PI : 0.0f;
                        float rad = targetRad * smoothT;

                        float startX = figuraInicioAnimacion[i].x;
                        float startY = figuraInicioAnimacion[i].y;
                        figuraAnimada[i].x = startX * cos(rad) - startY * sin(rad);
                        figuraAnimada[i].y = startX * sin(rad) + startY * cos(rad);
                    }
                    else {
                        figuraAnimada[i] = InterpolarPuntos(figuraInicioAnimacion[i], figuraObjetivo[i], smoothT);
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawLine(0, origenY, screenWidth - 350, origenY, LIGHTGRAY);
        DrawLine(origenX, 0, origenX, screenHeight, LIGHTGRAY);

        for (int step = 30; step < screenWidth; step += 30) {
            if (origenX + step < screenWidth - 350) {
                DrawLine(origenX + step, origenY - 3, origenX + step, origenY + 3, GRAY);
                DrawText(TextFormat("%d", step), origenX + step - 8, origenY + 6, 10, DARKGRAY);
            }
            if (origenX - step > 0) {
                DrawLine(origenX - step, origenY - 3, origenX - step, origenY + 3, GRAY);
                DrawText(TextFormat("-%d", step), origenX - step - 12, origenY + 6, 10, DARKGRAY);
            }
            if (origenY - step > 0) {
                DrawLine(origenX - 3, origenY - step, origenX + 3, origenY - step, GRAY);
                DrawText(TextFormat("%d", step), origenX + 6, origenY - step - 5, 10, DARKGRAY);
            }
            if (origenY + step < screenHeight) {
                DrawLine(origenX - 3, origenY + step, origenX + 3, origenY + step, GRAY);
                DrawText(TextFormat("-%d", step), origenX + 6, origenY + step - 5, 10, DARKGRAY);
            }
        }
        DrawCircle(origenX, origenY, 4, RED);

        if (transformacionSeleccionada == 2) {
            Color colorRef = Fade(GREEN, 0.6f);
            float grosorRef = 2.0f;
            float graphAreaWidth = (float)screenWidth - 350.0f;

            switch (ejeReflexion) {
            case 0: DrawLineEx({ 0, (float)origenY }, { graphAreaWidth, (float)origenY }, grosorRef, colorRef); break;
            case 1: DrawLineEx({ (float)origenX, 0 }, { (float)origenX, (float)screenHeight }, grosorRef, colorRef); break;
            case 2: DrawCircleLines(origenX, origenY, 10, colorRef); break;
            case 3: DrawLineEx(ConvertirAPantalla({ -1000, -1000 }, origenX, origenY), ConvertirAPantalla({ 1000, 1000 }, origenX, origenY), grosorRef, colorRef); break;
            case 4: DrawLineEx(ConvertirAPantalla({ -1000, 1000 }, origenX, origenY), ConvertirAPantalla({ 1000, -1000 }, origenX, origenY), grosorRef, colorRef); break;
            }
        }

        // --- DIBUJAR MARCADOR DEL CENTRO DE HOMOTECIA Y LÍNEAS GUÍA ---
        if (transformacionSeleccionada == 1) {
            Vector2 centroP = ConvertirAPantalla({ centroHomoteciaX, centroHomoteciaY }, origenX, origenY);

            // Trazar las líneas guía infinitas
            if (!modoVideojuego) {
                for (size_t i = 0; i < figuraOriginal.size(); i++) {
                    Vector2 pOriginalP = ConvertirAPantalla(figuraOriginal[i], origenX, origenY);
                    float dx = pOriginalP.x - centroP.x;
                    float dy = pOriginalP.y - centroP.y;
                    float dist = sqrt(dx * dx + dy * dy);

                    // Solo dibujamos si el punto no es exactamente el centro
                    if (dist > 0.01f) {
                        Vector2 p1 = { centroP.x + (dx / dist) * 2000, centroP.y + (dy / dist) * 2000 };
                        Vector2 p2 = { centroP.x - (dx / dist) * 2000, centroP.y - (dy / dist) * 2000 };
                        DrawLineEx(p1, p2, 1.0f, Fade(ORANGE, 0.4f));
                    }
                }
            }
            else {
                // Si es un sprite, proyectamos líneas desde las 4 esquinas (Bounding Box)
                if (figuraOriginal.size() > 0) {
                    float minX = figuraOriginal[0].x, maxX = figuraOriginal[0].x;
                    float minY = figuraOriginal[0].y, maxY = figuraOriginal[0].y;

                    // Encontrar los límites de la figura para formar el rectángulo
                    for (auto p : figuraOriginal) {
                        minX = min(minX, p.x); maxX = max(maxX, p.x);
                        minY = min(minY, p.y); maxY = max(maxY, p.y);
                    }

                    // Definimos los 4 vértices virtuales del Sprite
                    Vector2 esquinas[4] = {
                        {minX, minY}, // Esquina inferior izquierda
                        {maxX, minY}, // Esquina inferior derecha
                        {maxX, maxY}, // Esquina superior derecha
                        {minX, maxY}  // Esquina superior izquierda
                    };

                    // Trazamos un rayo infinito por cada esquina
                    for (int i = 0; i < 4; i++) {
                        Vector2 esquinaP = ConvertirAPantalla(esquinas[i], origenX, origenY);
                        float dx = esquinaP.x - centroP.x;
                        float dy = esquinaP.y - centroP.y;
                        float dist = sqrt(dx * dx + dy * dy);
                        if (dist > 0.01f) {
                            Vector2 p1 = { centroP.x + (dx / dist) * 2000, centroP.y + (dy / dist) * 2000 };
                            Vector2 p2 = { centroP.x - (dx / dist) * 2000, centroP.y - (dy / dist) * 2000 };
                            DrawLineEx(p1, p2, 1.0f, Fade(ORANGE, 0.4f));
                        }
                    }
                }
            }

            // Dibujar el punto 'C' sobre las líneas
            DrawCircleV(centroP, 6.0f, ORANGE);
            DrawCircleLines(centroP.x, centroP.y, 12.0f, ORANGE);
            DrawText("C", centroP.x + 10, centroP.y - 10, 14, ORANGE);
        }

        if (!modoVideojuego && figuraOriginal.size() >= 2) {
            vector<Vector2> puntosOriginalesPantalla;
            for (auto p : figuraOriginal) puntosOriginalesPantalla.push_back(ConvertirAPantalla(p, origenX, origenY));

            if (puntosOriginalesPantalla.size() >= 3) {
                vector<Vector2> triangulosRef = TriangularPoligono(puntosOriginalesPantalla);
                for (size_t i = 0; i + 2 < triangulosRef.size(); i += 3) {
                    DrawTriangle(triangulosRef[i], triangulosRef[i + 1], triangulosRef[i + 2], Fade(GRAY, 0.15f));
                }
                for (size_t i = 0; i < puntosOriginalesPantalla.size(); i++) {
                    int sig = (i + 1) % puntosOriginalesPantalla.size();
                    DrawLineEx(puntosOriginalesPantalla[i], puntosOriginalesPantalla[sig], 1.5f, Fade(GRAY, 0.4f));
                }
            }
            else if (puntosOriginalesPantalla.size() == 2) {
                DrawLineEx(puntosOriginalesPantalla[0], puntosOriginalesPantalla[1], 1.5f, Fade(GRAY, 0.4f));
            }
        }

        if (!modoVideojuego) {
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
        }
        else {
            if (texturaPersonaje.id > 0 && figuraOriginal.size() > 0) {
                float texW = (float)texturaPersonaje.width;
                float texH = (float)texturaPersonaje.height;

                float sumX_orig = 0, sumY_orig = 0;
                float minX_orig = figuraOriginal[0].x, maxX_orig = figuraOriginal[0].x;
                float minY_orig = figuraOriginal[0].y, maxY_orig = figuraOriginal[0].y;

                for (auto p : figuraOriginal) {
                    sumX_orig += p.x; sumY_orig += p.y;
                    minX_orig = min(minX_orig, p.x); maxX_orig = max(maxX_orig, p.x);
                    minY_orig = min(minY_orig, p.y); maxY_orig = max(maxY_orig, p.y);
                }
                Vector2 centroOriginalMath = { sumX_orig / figuraOriginal.size(), sumY_orig / figuraOriginal.size() };

                float baseWidth = maxX_orig - minX_orig;
                float baseHeight = maxY_orig - minY_orig;
                if (baseWidth == 0) baseWidth = 100.0f;
                if (baseHeight == 0) baseHeight = 100.0f;

                float sumX_anim = 0, sumY_anim = 0;
                for (auto p : figuraAnimada) {
                    sumX_anim += p.x; sumY_anim += p.y;
                }
                Vector2 centroAnimadoMath = { sumX_anim / figuraAnimada.size(), sumY_anim / figuraAnimada.size() };

                Vector2 posFantasma = ConvertirAPantalla(centroOriginalMath, origenX, origenY);
                Rectangle destFantasma = { posFantasma.x, posFantasma.y, baseWidth, baseHeight };
                Vector2 origFantasma = { baseWidth / 2.0f, baseHeight / 2.0f };
                Rectangle sourceFantasma = { 0.0f, 0.0f, texW, texH };

                DrawTexturePro(texturaPersonaje, sourceFantasma, destFantasma, origFantasma, 0.0f, Fade(WHITE, 0.3f));

                Vector2 posPantalla = ConvertirAPantalla(centroAnimadoMath, origenX, origenY);

                float finalW = baseWidth * fabs(imgEscalaActualX);
                float finalH = baseHeight * fabs(imgEscalaActualY);

                float sourceW = (imgEscalaActualX < 0) ? -texW : texW;
                float sourceH = (imgEscalaActualY < 0) ? -texH : texH;

                Rectangle sourceRec = { 0.0f, 0.0f, sourceW, sourceH };
                Rectangle destRec = { posPantalla.x, posPantalla.y, finalW, finalH };
                Vector2 origin = { finalW / 2.0f, finalH / 2.0f };

                DrawTexturePro(texturaPersonaje, sourceRec, destRec, origin, -imgAngActual, WHITE);
            }
            else if (figuraOriginal.size() == 0) {
                DrawText("Agrega al menos 1 punto (ej. X=100, Y=100) para anclar el personaje.", origenX - 220, origenY - 20, 16, DARKGRAY);
            }
            else {
                DrawText("ERROR: No se encontro 'personaje.png' en la carpeta o tiene formato invalido.", origenX - 250, origenY - 20, 16, RED);
            }
        }

        // ==========================================
        // DIBUJO DEL PANEL DE CONTROL LATERAL (INTERFAZ RAYGUI)
        // ==========================================
        float panelX = (float)screenWidth - 350.0f;

        // --- AQUÍ ESTÁ EL CAMBIO PARA HACERLO MENOS TRANSPARENTE ---
        // Cambié Fade(LIGHTGRAY, 0.4f) a Fade(LIGHTGRAY, 0.9f)
        DrawRectangle((int)panelX, 0, 350, screenHeight, Fade(LIGHTGRAY, 0.9f));
        DrawLine((int)panelX, 0, (int)panelX, screenHeight, GRAY);

        DrawText("DASHBOARD DE MATRICES", (int)panelX + 50, 15, 20, DARKBLUE);

        GuiCheckBox(Rectangle{ panelX + 20.0f, 45.0f, 20.0f, 20.0f }, "MODO VIDEOJUEGO (Sprite)", &modoVideojuego);

        GuiGroupBox(Rectangle{ panelX + 20.0f, 75.0f, 310.0f, 135.0f }, "1. GESTION DE PUNTOS");

        GuiLabel(Rectangle{ panelX + 40.0f, 90.0f, 20.0f, 30.0f }, "X:");
        if (GuiTextBox(Rectangle{ panelX + 60.0f, 90.0f, 80.0f, 30.0f }, inputX, 16, editX)) editX = !editX;

        GuiLabel(Rectangle{ panelX + 160.0f, 90.0f, 20.0f, 30.0f }, "Y:");
        if (GuiTextBox(Rectangle{ panelX + 180.0f, 90.0f, 80.0f, 30.0f }, inputY, 16, editY)) editY = !editY;

        if (GuiButton(Rectangle{ panelX + 40.0f, 130.0f, 130.0f, 30.0f }, "Añadir Punto")) {
            try {
                float px = stof(inputX);
                float py = stof(inputY);
                figuraOriginal.push_back({ px, py });
                figuraObjetivo = figuraOriginal;
                figuraAnimada = figuraOriginal;
                figuraInicioAnimacion = figuraOriginal;
                inputX[0] = '\0'; inputY[0] = '\0';

                valorAngulo = 0.0f; valorEscala = 1.0f;
                anguloActualVisual = 0.0f; escalaActualVisual = 1.0f;

                imgAngInicial = 0.0f;  imgAngFinal = 0.0f;  imgAngActual = 0.0f;
                imgEscalaInicialX = 1.0f; imgEscalaFinalX = 1.0f; imgEscalaActualX = 1.0f;
                imgEscalaInicialY = 1.0f; imgEscalaFinalY = 1.0f; imgEscalaActualY = 1.0f;
            }
            catch (...) {}
        }

        if (GuiButton(Rectangle{ panelX + 180.0f, 130.0f, 130.0f, 30.0f }, "Limpiar Figura")) {
            figuraOriginal.clear(); figuraObjetivo.clear(); figuraAnimada.clear(); figuraInicioAnimacion.clear();
        }

        DrawText(TextFormat("Vertices actuales: %d", (int)figuraOriginal.size()), (int)panelX + 40, 175, 14, DARKGRAY);

        GuiGroupBox(Rectangle{ panelX + 20.0f, 230.0f, 310.0f, 220.0f }, "2. PARÁMETROS MATEMÁTICOS");

        if (transformacionSeleccionada == 0) {
            DrawText(TextFormat("Angulo (Grados): %.1f", valorAngulo), (int)panelX + 40, 310, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 340.0f, 270.0f, 20.0f }, "-360", "360", &valorAngulo, -360.0f, 360.0f);
        }
        else if (transformacionSeleccionada == 1) {
            DrawText(TextFormat("Factor Escala (k): %.2f", valorEscala), (int)panelX + 40, 310, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 340.0f, 270.0f, 20.0f }, "-3.0", "3.0", &valorEscala, -3.0f, 3.0f);

            DrawText("Centro de Homotecia (C):", (int)panelX + 40, 380, 16, DARKGRAY);
            GuiLabel(Rectangle{ panelX + 40.0f, 405.0f, 20.0f, 30.0f }, "X:");
            if (GuiTextBox(Rectangle{ panelX + 60.0f, 405.0f, 80.0f, 30.0f }, inputCx, 16, editCx)) editCx = !editCx;

            GuiLabel(Rectangle{ panelX + 160.0f, 405.0f, 20.0f, 30.0f }, "Y:");
            if (GuiTextBox(Rectangle{ panelX + 180.0f, 405.0f, 80.0f, 30.0f }, inputCy, 16, editCy)) editCy = !editCy;
        }

        if (menuTransfAbierto || menuReflexAbierto) GuiDisable();

        if (GuiButton(Rectangle{ panelX + 20.0f, 470.0f, 310.0f, 50.0f }, "APLICAR TRANSFORMACIÓN (Animar)")) {

            figuraInicioAnimacion = figuraAnimada;
            anguloInicioAnimacion = anguloActualVisual;
            escalaInicioAnimacion = escalaActualVisual;
            anguloDestinoAnimacion = valorAngulo;
            escalaDestinoAnimacion = valorEscala;

            imgAngInicial = imgAngActual;
            imgEscalaInicialX = imgEscalaActualX;
            imgEscalaInicialY = imgEscalaActualY;

            imgAngFinal = 0.0f;
            imgEscalaFinalX = 1.0f;
            imgEscalaFinalY = 1.0f;

            if (transformacionSeleccionada == 1) {
                try { centroHomoteciaX = stof(inputCx); }
                catch (...) { centroHomoteciaX = 0.0f; }
                try { centroHomoteciaY = stof(inputCy); }
                catch (...) { centroHomoteciaY = 0.0f; }
            }

            if (transformacionSeleccionada == 0) {
                imgAngFinal = valorAngulo;
            }
            else if (transformacionSeleccionada == 1) {
                imgEscalaFinalX = valorEscala;
                imgEscalaFinalY = valorEscala;
            }
            else if (transformacionSeleccionada == 2) {
                if (ejeReflexion == 0) {
                    imgEscalaFinalX = 1.0f; imgEscalaFinalY = -1.0f;
                }
                else if (ejeReflexion == 1) {
                    imgEscalaFinalX = -1.0f; imgEscalaFinalY = 1.0f;
                }
                else if (ejeReflexion == 2) {
                    imgAngFinal = 180.0f;
                }
                else if (ejeReflexion == 3) {
                    imgEscalaFinalY = -1.0f;
                    imgAngFinal = 90.0f;
                }
                else if (ejeReflexion == 4) {
                    imgEscalaFinalY = -1.0f;
                    imgAngFinal = -90.0f;
                }
            }

            animacionT = 0.0f;

            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                float px = figuraOriginal[i].x;
                float py = figuraOriginal[i].y;

                if (transformacionSeleccionada == 0) {
                    float rad = anguloDestinoAnimacion * (PI / 180.0f);
                    figuraObjetivo[i].x = px * cos(rad) - py * sin(rad);
                    figuraObjetivo[i].y = px * sin(rad) + py * cos(rad);
                }
                else if (transformacionSeleccionada == 1) {
                    figuraObjetivo[i].x = centroHomoteciaX + (px - centroHomoteciaX) * escalaDestinoAnimacion;
                    figuraObjetivo[i].y = centroHomoteciaY + (py - centroHomoteciaY) * escalaDestinoAnimacion;
                }
                else if (transformacionSeleccionada == 2) {
                    if (ejeReflexion == 0) { figuraObjetivo[i].x = px; figuraObjetivo[i].y = -py; }
                    if (ejeReflexion == 1) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = py; }
                    if (ejeReflexion == 2) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = -py; }
                    if (ejeReflexion == 3) { figuraObjetivo[i].x = py; figuraObjetivo[i].y = px; }
                    if (ejeReflexion == 4) { figuraObjetivo[i].x = -py; figuraObjetivo[i].y = -px; }
                }
            }
        }

        GuiEnable();

        if (transformacionSeleccionada == 2) {
            DrawText("Seleccione la Reflexion:", (int)panelX + 40, 360, 16, BLACK);
            if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 385.0f, 270.0f, 30.0f }, "Eje X;Eje Y;Origen;Recta Y = X;Recta Y = -X", &ejeReflexion, menuReflexAbierto)) {
                menuReflexAbierto = !menuReflexAbierto;
            }
        }

        GuiLabel(Rectangle{ panelX + 40.0f, 250.0f, 200.0f, 20.0f }, "Tipo de Transformacion:");
        if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 275.0f, 270.0f, 30.0f }, "ROTACION;HOMOTECIA;REFLEXION", &transformacionSeleccionada, menuTransfAbierto)) {
            menuTransfAbierto = !menuTransfAbierto;
        }

        EndDrawing();
    }

    UnloadTexture(texturaPersonaje);
    CloseWindow();
    return 0;
}