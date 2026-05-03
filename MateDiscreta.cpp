#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // Para usar reverse() y otras utilidades
#include <cmath>     // Para usar funciones matemáticas como sin(), cos(), fabs()
#include "raylib.h"  // Librería principal para gráficos 2D

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"  // Librería para la interfaz de usuario (botones, sliders)

using namespace std;

// ==========================================
// 1. MATEMÁTICAS PARA LA TRIANGULACIÓN
// ==========================================

// Calcula el determinante para saber la orientación de los vértices
float ProductoCruz(Vector2 a, Vector2 b, Vector2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Verifica si un punto está dentro de un triángulo
bool PuntoEnTriangulo(Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
    float cp1 = ProductoCruz(a, b, p);
    float cp2 = ProductoCruz(b, c, p);
    float cp3 = ProductoCruz(c, a, p);

    bool tieneNegativo = (cp1 < 0) || (cp2 < 0) || (cp3 < 0);
    bool tienePositivo = (cp1 > 0) || (cp2 > 0) || (cp3 > 0);
    return !(tieneNegativo && tienePositivo);
}

// Divide un polígono complejo en triángulos simples para colorearlo
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
// 2. MATEMÁTICAS DE TRANSFORMACIONES Y PANTALLA
// ==========================================

// Pasa de coordenadas cartesianas matemáticas a píxeles en la pantalla
Vector2 ConvertirAPantalla(Vector2 mathP, int origenX, int origenY) {
    return { mathP.x + (float)origenX, (float)origenY - mathP.y };
}

// Interpola entre dos vectores (Lerp) para animaciones fluidas
Vector2 InterpolarPuntos(Vector2 a, Vector2 b, float progreso) {
    return { a.x + (b.x - a.x) * progreso, a.y + (b.y - a.y) * progreso };
}

// Interpola entre dos números simples (LerpFloat)
float InterpolarValor(float inicio, float fin, float progreso) {
    return inicio + (fin - inicio) * progreso;
}

int main() {
    const int anchoPantalla = 1000;
    const int altoPantalla = 650;

    InitWindow(anchoPantalla, altoPantalla, "VectraLab - Transformaciones Lineales 2D");
    SetTargetFPS(60);

    Texture2D texturaPersonaje = LoadTexture("personaje.png");
    bool modoVideojuego = false;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    int origenX = (anchoPantalla - 350) / 2;
    int origenY = altoPantalla / 2;

    // Vectores de estado de la figura
    vector<Vector2> figOriginal = { {50, 50}, {150, 50}, {150, 150}, {50, 150} };
    vector<Vector2> figFinal = figOriginal;
    vector<Vector2> figActual = figOriginal;
    vector<Vector2> figInicial = figOriginal;

    // Interfaz: Entradas de texto
    char textoX[16] = "";
    char textoY[16] = "";
    bool editandoX = false;
    bool editandoY = false;

    // Interfaz: Estado de los menús
    bool menuTransfAbierto = false;
    bool menuReflexAbierto = false;

    int tipoTransf = 0;
    int transfPrevia = 0;

    int tipoReflex = 0;
    int reflexPrevia = 0;

    float valorAngulo = 0.0f;
    float valorEscala = 1.0f;

    // Matemáticas de Animación: Polígono
    float anguloActual = 0.0f;
    float anguloInicial = 0.0f;
    float anguloFinal = 0.0f;

    float escalaActual = 1.0f;
    float escalaInicial = 1.0f;
    float escalaFinal = 1.0f;

    // Matemáticas de Animación: Modo Videojuego (Imagen)
    float imgAngInicial = 0.0f;  float imgAngFinal = 0.0f;  float imgAngActual = 0.0f;
    float imgEscalaInicialX = 1.0f; float imgEscalaFinalX = 1.0f; float imgEscalaActualX = 1.0f;
    float imgEscalaInicialY = 1.0f; float imgEscalaFinalY = 1.0f; float imgEscalaActualY = 1.0f;

    float progresoAnim = 1.0f;

    while (!WindowShouldClose()) {

        // Reset de la animación si el usuario cambia de opinión en pleno movimiento
        if (tipoTransf != transfPrevia || (tipoTransf == 2 && tipoReflex != reflexPrevia)) {

            figFinal = figOriginal;
            figActual = figOriginal;
            figInicial = figOriginal;
            progresoAnim = 1.0f;
            valorAngulo = 0.0f;
            valorEscala = 1.0f;

            anguloActual = 0.0f; anguloInicial = 0.0f; anguloFinal = 0.0f;
            escalaActual = 1.0f; escalaInicial = 1.0f; escalaFinal = 1.0f;

            imgAngInicial = 0.0f;  imgAngFinal = 0.0f;  imgAngActual = 0.0f;
            imgEscalaInicialX = 1.0f; imgEscalaFinalX = 1.0f; imgEscalaActualX = 1.0f;
            imgEscalaInicialY = 1.0f; imgEscalaFinalY = 1.0f; imgEscalaActualY = 1.0f;

            transfPrevia = tipoTransf;
            reflexPrevia = tipoReflex;
        }

        // ==========================================
        // CEREBRO DE LA ANIMACIÓN
        // ==========================================
        if (progresoAnim < 1.0f) {
            progresoAnim += 0.015f;
            if (progresoAnim > 1.0f) progresoAnim = 1.0f;

            float progresoSuave = progresoAnim * progresoAnim * (3.0f - 2.0f * progresoAnim);

            imgAngActual = InterpolarValor(imgAngInicial, imgAngFinal, progresoSuave);

            if (tipoTransf == 2) {
                imgEscalaActualX = imgEscalaFinalX;
                imgEscalaActualY = imgEscalaFinalY;
            }
            else {
                imgEscalaActualX = InterpolarValor(imgEscalaInicialX, imgEscalaFinalX, progresoSuave);
                imgEscalaActualY = InterpolarValor(imgEscalaInicialY, imgEscalaFinalY, progresoSuave);
            }

            for (size_t i = 0; i < figOriginal.size(); i++) {
                float px = figOriginal[i].x;
                float py = figOriginal[i].y;

                if (tipoTransf == 0) {
                    float anguloInterpolado = anguloInicial + (anguloFinal - anguloInicial) * progresoSuave;
                    anguloActual = anguloInterpolado;
                    float radianes = anguloInterpolado * (PI / 180.0f);
                    figActual[i].x = px * cos(radianes) - py * sin(radianes);
                    figActual[i].y = px * sin(radianes) + py * cos(radianes);
                }
                else if (tipoTransf == 1) {
                    float escalaInterpolada = escalaInicial + (escalaFinal - escalaInicial) * progresoSuave;
                    escalaActual = escalaInterpolada;
                    figActual[i].x = px * escalaInterpolada;
                    figActual[i].y = py * escalaInterpolada;
                }
                else if (tipoTransf == 2) {
                    if (tipoReflex == 2) {
                        float diferenciaX = fabs(figInicial[0].x - figFinal[0].x);
                        float radFinal = (diferenciaX > 0.1f) ? PI : 0.0f;
                        float radianes = radFinal * progresoSuave;

                        float inicioX = figInicial[i].x;
                        float inicioY = figInicial[i].y;
                        figActual[i].x = inicioX * cos(radianes) - inicioY * sin(radianes);
                        figActual[i].y = inicioX * sin(radianes) + inicioY * cos(radianes);
                    }
                    else {
                        figActual[i] = InterpolarPuntos(figInicial[i], figFinal[i], progresoSuave);
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Ejes X e Y Centrales
        DrawLine(0, origenY, anchoPantalla - 350, origenY, LIGHTGRAY);
        DrawLine(origenX, 0, origenX, altoPantalla, LIGHTGRAY);

        for (int paso = 30; paso < anchoPantalla; paso += 30) {
            if (origenX + paso < anchoPantalla - 350) {
                DrawLine(origenX + paso, origenY - 3, origenX + paso, origenY + 3, GRAY);
                DrawText(TextFormat("%d", paso), origenX + paso - 8, origenY + 6, 10, DARKGRAY);
            }
            if (origenX - paso > 0) {
                DrawLine(origenX - paso, origenY - 3, origenX - paso, origenY + 3, GRAY);
                DrawText(TextFormat("-%d", paso), origenX - paso - 12, origenY + 6, 10, DARKGRAY);
            }
            if (origenY - paso > 0) {
                DrawLine(origenX - 3, origenY - paso, origenX + 3, origenY - paso, GRAY);
                DrawText(TextFormat("%d", paso), origenX + 6, origenY - paso - 5, 10, DARKGRAY);
            }
            if (origenY + paso < altoPantalla) {
                DrawLine(origenX - 3, origenY + paso, origenX + 3, origenY + paso, GRAY);
                DrawText(TextFormat("-%d", paso), origenX + 6, origenY + paso - 5, 10, DARKGRAY);
            }
        }
        DrawCircle(origenX, origenY, 4, RED);

        if (tipoTransf == 2) {
            Color colorEje = Fade(GREEN, 0.6f);
            float grosorEje = 2.0f;
            float anchoGrafico = (float)anchoPantalla - 350.0f;

            switch (tipoReflex) {
            case 0: DrawLineEx({ 0, (float)origenY }, { anchoGrafico, (float)origenY }, grosorEje, colorEje); break;
            case 1: DrawLineEx({ (float)origenX, 0 }, { (float)origenX, (float)altoPantalla }, grosorEje, colorEje); break;
            case 2: DrawCircleLines(origenX, origenY, 10, colorEje); break;
            case 3: DrawLineEx(ConvertirAPantalla({ -1000, -1000 }, origenX, origenY), ConvertirAPantalla({ 1000, 1000 }, origenX, origenY), grosorEje, colorEje); break;
            case 4: DrawLineEx(ConvertirAPantalla({ -1000, 1000 }, origenX, origenY), ConvertirAPantalla({ 1000, -1000 }, origenX, origenY), grosorEje, colorEje); break;
            }
        }

        // FIGURA REFERENCIA
        if (!modoVideojuego && figOriginal.size() >= 2) {
            vector<Vector2> puntosBasePixeles;
            for (auto p : figOriginal) puntosBasePixeles.push_back(ConvertirAPantalla(p, origenX, origenY));

            if (puntosBasePixeles.size() >= 3) {
                vector<Vector2> triangulosBase = TriangularPoligono(puntosBasePixeles);
                for (size_t i = 0; i + 2 < triangulosBase.size(); i += 3) {
                    DrawTriangle(triangulosBase[i], triangulosBase[i + 1], triangulosBase[i + 2], Fade(GRAY, 0.15f));
                }
                for (size_t i = 0; i < puntosBasePixeles.size(); i++) {
                    int sig = (i + 1) % puntosBasePixeles.size();
                    DrawLineEx(puntosBasePixeles[i], puntosBasePixeles[sig], 1.5f, Fade(GRAY, 0.4f));
                }
            }
            else if (puntosBasePixeles.size() == 2) {
                DrawLineEx(puntosBasePixeles[0], puntosBasePixeles[1], 1.5f, Fade(GRAY, 0.4f));
            }
        }

        // FIGURA ANIMADA (AZUL)
        if (!modoVideojuego) {
            vector<Vector2> puntosAnimPixeles;
            for (auto p : figActual) {
                puntosAnimPixeles.push_back(ConvertirAPantalla(p, origenX, origenY));
            }

            if (puntosAnimPixeles.size() >= 3) {
                vector<Vector2> triangulosAnim = TriangularPoligono(puntosAnimPixeles);
                Color colorRelleno = { 0, 121, 241, 150 };
                for (size_t i = 0; i + 2 < triangulosAnim.size(); i += 3) {
                    DrawTriangle(triangulosAnim[i], triangulosAnim[i + 1], triangulosAnim[i + 2], colorRelleno);
                }
                for (size_t i = 0; i < puntosAnimPixeles.size(); i++) {
                    int sig = (i + 1) % puntosAnimPixeles.size();
                    DrawLineEx(puntosAnimPixeles[i], puntosAnimPixeles[sig], 3.0f, DARKBLUE);
                }
            }
            else if (puntosAnimPixeles.size() == 2) {
                DrawLineEx(puntosAnimPixeles[0], puntosAnimPixeles[1], 3.0f, DARKBLUE);
            }
            for (size_t i = 0; i < puntosAnimPixeles.size(); i++) {
                DrawCircleV(puntosAnimPixeles[i], 5.0f, MAROON);
            }
        }
        else {
            // MODO TEXTURA / VIDEOJUEGO
            if (texturaPersonaje.id > 0 && figOriginal.size() > 0) {
                float anchoTex = (float)texturaPersonaje.width;
                float altoTex = (float)texturaPersonaje.height;

                float sumaX = 0, sumaY = 0;
                float minX = figOriginal[0].x, maxX = figOriginal[0].x;
                float minY = figOriginal[0].y, maxY = figOriginal[0].y;

                for (auto p : figOriginal) {
                    sumaX += p.x; sumaY += p.y;
                    minX = min(minX, p.x); maxX = max(maxX, p.x);
                    minY = min(minY, p.y); maxY = max(maxY, p.y);
                }
                Vector2 centroOriginal = { sumaX / figOriginal.size(), sumaY / figOriginal.size() };

                float anchoBase = maxX - minX;
                float altoBase = maxY - minY;

                if (anchoBase == 0) anchoBase = 100.0f;
                if (altoBase == 0) altoBase = 100.0f;

                float sumaXAnim = 0, sumaYAnim = 0;
                for (auto p : figActual) {
                    sumaXAnim += p.x; sumaYAnim += p.y;
                }
                Vector2 centroAnimado = { sumaXAnim / figActual.size(), sumaYAnim / figActual.size() };

                Vector2 posSombra = ConvertirAPantalla(centroOriginal, origenX, origenY);
                Rectangle rectDestSombra = { posSombra.x, posSombra.y, anchoBase, altoBase };
                Vector2 pivoteSombra = { anchoBase / 2.0f, altoBase / 2.0f };
                Rectangle rectCorteSombra = { 0.0f, 0.0f, anchoTex, altoTex };

                DrawTexturePro(texturaPersonaje, rectCorteSombra, rectDestSombra, pivoteSombra, 0.0f, Fade(WHITE, 0.3f));

                Vector2 posFinalPx = ConvertirAPantalla(centroAnimado, origenX, origenY);

                float anchoFinal = anchoBase * fabs(imgEscalaActualX);
                float altoFinal = altoBase * fabs(imgEscalaActualY);

                float anchoCorte = (imgEscalaActualX < 0) ? -anchoTex : anchoTex;
                float altoCorte = (imgEscalaActualY < 0) ? -altoTex : altoTex;

                Rectangle rectCorte = { 0.0f, 0.0f, anchoCorte, altoCorte };
                Rectangle rectDestino = { posFinalPx.x, posFinalPx.y, anchoFinal, altoFinal };
                Vector2 pivoteRotacion = { anchoFinal / 2.0f, altoFinal / 2.0f };

                DrawTexturePro(texturaPersonaje, rectCorte, rectDestino, pivoteRotacion, -imgAngActual, WHITE);
            }
            else if (figOriginal.size() == 0) {
                DrawText("Agrega al menos 1 punto (ej. X=100, Y=100) para anclar el personaje.", origenX - 220, origenY - 20, 16, DARKGRAY);
            }
            else {
                DrawText("ERROR: No se encontro 'personaje.png' en la carpeta.", origenX - 250, origenY - 20, 16, RED);
            }
        }

        // ==========================================
        // DIBUJO DEL PANEL DE CONTROL LATERAL
        // ==========================================
        float xPanel = (float)anchoPantalla - 350.0f;
        DrawRectangle((int)xPanel, 0, 350, altoPantalla, Fade(LIGHTGRAY, 0.4f));
        DrawLine((int)xPanel, 0, (int)xPanel, altoPantalla, GRAY);

        DrawText("DASHBOARD DE MATRICES", (int)xPanel + 50, 15, 20, DARKBLUE);

        GuiCheckBox(Rectangle{ xPanel + 20.0f, 45.0f, 20.0f, 20.0f }, "MODO VIDEOJUEGO (Sprite)", &modoVideojuego);

        GuiGroupBox(Rectangle{ xPanel + 20.0f, 75.0f, 310.0f, 135.0f }, "1. GESTION DE PUNTOS");

        GuiLabel(Rectangle{ xPanel + 40.0f, 90.0f, 20.0f, 30.0f }, "X:");
        if (GuiTextBox(Rectangle{ xPanel + 60.0f, 90.0f, 80.0f, 30.0f }, textoX, 16, editandoX)) editandoX = !editandoX;

        GuiLabel(Rectangle{ xPanel + 160.0f, 90.0f, 20.0f, 30.0f }, "Y:");
        if (GuiTextBox(Rectangle{ xPanel + 180.0f, 90.0f, 80.0f, 30.0f }, textoY, 16, editandoY)) editandoY = !editandoY;

        if (GuiButton(Rectangle{ xPanel + 40.0f, 130.0f, 130.0f, 30.0f }, "Añadir Punto")) {
            try {
                float px = stof(textoX);
                float py = stof(textoY);
                figOriginal.push_back({ px, py });

                figFinal = figOriginal;
                figActual = figOriginal;
                figInicial = figOriginal;

                textoX[0] = '\0'; textoY[0] = '\0';

                valorAngulo = 0.0f; valorEscala = 1.0f;
                anguloActual = 0.0f; escalaActual = 1.0f;

                imgAngInicial = 0.0f;  imgAngFinal = 0.0f;  imgAngActual = 0.0f;
                imgEscalaInicialX = 1.0f; imgEscalaFinalX = 1.0f; imgEscalaActualX = 1.0f;
                imgEscalaInicialY = 1.0f; imgEscalaFinalY = 1.0f; imgEscalaActualY = 1.0f;
            }
            catch (...) {}
        }

        if (GuiButton(Rectangle{ xPanel + 180.0f, 130.0f, 130.0f, 30.0f }, "Limpiar Figura")) {
            figOriginal.clear(); figFinal.clear(); figActual.clear(); figInicial.clear();
        }

        DrawText(TextFormat("Vertices actuales: %d", (int)figOriginal.size()), (int)xPanel + 40, 175, 14, DARKGRAY);

        GuiGroupBox(Rectangle{ xPanel + 20.0f, 230.0f, 310.0f, 220.0f }, "2. PARÁMETROS MATEMÁTICOS");

        if (tipoTransf == 0) {
            DrawText(TextFormat("Angulo (Grados): %.1f", valorAngulo), (int)xPanel + 40, 360, 16, BLACK);
            GuiSliderBar(Rectangle{ xPanel + 40.0f, 390.0f, 270.0f, 20.0f }, "-360", "360", &valorAngulo, -360.0f, 360.0f);
        }
        else if (tipoTransf == 1) {
            DrawText(TextFormat("Factor Escala (k): %.2f", valorEscala), (int)xPanel + 40, 360, 16, BLACK);
            GuiSliderBar(Rectangle{ xPanel + 40.0f, 390.0f, 270.0f, 20.0f }, "0.1", "3.0", &valorEscala, 0.1f, 3.0f);
        }

        if (menuTransfAbierto || menuReflexAbierto) GuiDisable();

        // ==========================================
        // BOTÓN PRINCIPAL
        // ==========================================
        if (GuiButton(Rectangle{ xPanel + 20.0f, 470.0f, 310.0f, 50.0f }, "APLICAR TRANSFORMACIÓN (Animar)")) {

            figInicial = figActual;
            anguloInicial = anguloActual;
            escalaInicial = escalaActual;
            anguloFinal = valorAngulo;
            escalaFinal = valorEscala;

            imgAngInicial = imgAngActual;
            imgEscalaInicialX = imgEscalaActualX;
            imgEscalaInicialY = imgEscalaActualY;

            imgAngFinal = 0.0f;
            imgEscalaFinalX = 1.0f;
            imgEscalaFinalY = 1.0f;

            if (tipoTransf == 0) {
                imgAngFinal = valorAngulo;
            }
            else if (tipoTransf == 1) {
                imgEscalaFinalX = valorEscala;
                imgEscalaFinalY = valorEscala;
            }
            else if (tipoTransf == 2) {
                if (tipoReflex == 0) {
                    imgEscalaFinalX = 1.0f; imgEscalaFinalY = -1.0f;
                }
                else if (tipoReflex == 1) {
                    imgEscalaFinalX = -1.0f; imgEscalaFinalY = 1.0f;
                }
                else if (tipoReflex == 2) {
                    imgAngFinal = 180.0f;
                }
                else if (tipoReflex == 3) {
                    imgEscalaFinalY = -1.0f;
                    imgAngFinal = 90.0f;
                }
                else if (tipoReflex == 4) {
                    imgEscalaFinalY = -1.0f;
                    imgAngFinal = -90.0f;
                }
            }

            progresoAnim = 0.0f;

            for (size_t i = 0; i < figOriginal.size(); i++) {
                float px = figOriginal[i].x;
                float py = figOriginal[i].y;

                if (tipoTransf == 0) {
                    float radianes = anguloFinal * (PI / 180.0f);
                    figFinal[i].x = px * cos(radianes) - py * sin(radianes);
                    figFinal[i].y = px * sin(radianes) + py * cos(radianes);
                }
                else if (tipoTransf == 1) {
                    figFinal[i].x = px * escalaFinal;
                    figFinal[i].y = py * escalaFinal;
                }
                else if (tipoTransf == 2) {
                    if (tipoReflex == 0) { figFinal[i].x = px; figFinal[i].y = -py; }
                    if (tipoReflex == 1) { figFinal[i].x = -px; figFinal[i].y = py; }
                    if (tipoReflex == 2) { figFinal[i].x = -px; figFinal[i].y = -py; }
                    if (tipoReflex == 3) { figFinal[i].x = py; figFinal[i].y = px; }
                    if (tipoReflex == 4) { figFinal[i].x = -py; figFinal[i].y = -px; }
                }
            }
        }

        GuiEnable();

        if (tipoTransf == 2) {
            DrawText("Seleccione la Reflexion:", (int)xPanel + 40, 360, 16, BLACK);
            if (GuiDropdownBox(Rectangle{ xPanel + 40.0f, 385.0f, 270.0f, 30.0f }, "Eje X;Eje Y;Origen;Recta Y = X;Recta Y = -X", &tipoReflex, menuReflexAbierto)) {
                menuReflexAbierto = !menuReflexAbierto;
            }
        }

        GuiLabel(Rectangle{ xPanel + 40.0f, 250.0f, 200.0f, 20.0f }, "Tipo de Transformacion:");
        if (GuiDropdownBox(Rectangle{ xPanel + 40.0f, 275.0f, 270.0f, 30.0f }, "ROTACION;HOMOTECIA;REFLEXION", &tipoTransf, menuTransfAbierto)) {
            menuTransfAbierto = !menuTransfAbierto;
        }

        EndDrawing();
    }

    UnloadTexture(texturaPersonaje);
    CloseWindow();
    return 0;
}