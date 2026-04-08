#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // Necesario para std::reverse
#include "raylib.h"

using namespace std;

int frameCount = 0;

// ==========================================
// MATEMÁTICAS PARA LA TRIANGULACIÓN (Ear Clipping)
// ==========================================

// 1. Producto cruz en 2D para determinar la orientación de un ángulo
float ProductoCruz(Vector2 a, Vector2 b, Vector2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// 2. Verifica si un punto P está atrapado dentro del triángulo ABC
bool PuntoEnTriangulo(Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
    float cp1 = ProductoCruz(a, b, p);
    float cp2 = ProductoCruz(b, c, p);
    float cp3 = ProductoCruz(c, a, p);
    bool tieneNegativo = (cp1 < 0) || (cp2 < 0) || (cp3 < 0);
    bool tienePositivo = (cp1 > 0) || (cp2 > 0) || (cp3 > 0);
    return !(tieneNegativo && tienePositivo); // Si todos tienen el mismo signo, está dentro
}

// 3. El Algoritmo Principal de Recorte de Orejas
vector<Vector2> TriangularPoligono(vector<Vector2> vertices) {
    vector<Vector2> triangulos;
    if (vertices.size() < 3) return triangulos;

    vector<Vector2> v = vertices;

    // A. Calcular el área signada para conocer el orden (horario o antihorario)
    float area = 0;
    for (int i = 0; i < v.size(); i++) {
        int j = (i + 1) % v.size();
        area += (v[i].x * v[j].y) - (v[j].x * v[i].y);
    }

    // B. Forzar un sentido antihorario predecible para el algoritmo
    // En pantallas (Y hacia abajo), área positiva significa sentido horario.
    if (area > 0) {
        reverse(v.begin(), v.end());
    }

    // C. Bucle principal de recorte (extraer triángulos)
    int intentos = 0;
    while (v.size() > 3 && intentos < 1000) {
        bool orejaCortada = false;
        int n = v.size();

        for (int i = 0; i < n; i++) {
            int prev = (i - 1 + n) % n;
            int next = (i + 1) % n;

            Vector2 A = v[prev];
            Vector2 B = v[i];
            Vector2 C = v[next];

            // Si el ángulo no es convexo, lo saltamos
            if (ProductoCruz(A, B, C) >= 0) continue;

            // Verificar que ningún otro punto del polígono esté DENTRO de este posible triángulo
            bool esOreja = true;
            for (int j = 0; j < n; j++) {
                if (j == prev || j == i || j == next) continue;
                if (PuntoEnTriangulo(v[j], A, B, C)) {
                    esOreja = false;
                    break;
                }
            }

            // Si es una oreja válida, la cortamos y guardamos sus puntos
            if (esOreja) {
                triangulos.push_back(A);
                triangulos.push_back(B);
                triangulos.push_back(C);

                v.erase(v.begin() + i); // Eliminamos el vértice del polígono
                orejaCortada = true;
                break;
            }
        }
        intentos++;
        if (!orejaCortada) break; // Evitar cuelgues si el usuario cruza líneas (figura en forma de 8)
    }

    // D. Añadir el último triángulo que queda
    if (v.size() == 3) {
        triangulos.push_back(v[0]);
        triangulos.push_back(v[1]);
        triangulos.push_back(v[2]);
    }

    return triangulos;
}


// Función matemática vital: Convierte tus coordenadas a coordenadas de pantalla
Vector2 ConvertirAPantalla(float mathX, float mathY, int anchoPantalla, int altoPantalla) {
    Vector2 puntoPantalla;
    puntoPantalla.x = mathX + (anchoPantalla / 2.0f);
    puntoPantalla.y = (altoPantalla / 2.0f) - mathY;
    return puntoPantalla;
}

enum EstadoApp {
    PEDIR_CANTIDAD,
    PEDIR_X,
    PEDIR_Y,
    MOSTRAR_FIGURA
};

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Motor de Triangulacion de Poligonos");
    SetTargetFPS(60);

    EstadoApp estadoActual = PEDIR_CANTIDAD;
    string textoIngresado = "";

    int numVertices = 0;
    int verticeActual = 0;
    float tempX = 0;

    vector<Vector2> puntosMatematicos;
    vector<Vector2> puntosPantalla;

    // NUEVO: Vector para guardar los triángulos ya procesados
    vector<Vector2> triangulosProcesados;

    Color azulTransparente = { 0, 121, 241, 120 };

    while (!WindowShouldClose()) {

        // ==========================================
        // 1. LÓGICA DE CAPTURA DE DATOS (TECLADO)
        // ==========================================
        if (estadoActual != MOSTRAR_FIGURA) {
            int tecla = GetCharPressed();

            while (tecla > 0) {
                if ((tecla >= 48 && tecla <= 57) || tecla == 45) {
                    textoIngresado += (char)tecla;
                }
                tecla = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && textoIngresado.length() > 0) {
                textoIngresado.pop_back();
            }

            if (IsKeyPressed(KEY_ENTER) && textoIngresado.length() > 0) {
                try {
                    if (estadoActual == PEDIR_CANTIDAD) {
                        numVertices = stoi(textoIngresado);
                        if (numVertices >= 3) {
                            estadoActual = PEDIR_X;
                            textoIngresado = "";
                        }
                        else {
                            textoIngresado = "";
                        }
                    }
                    else if (estadoActual == PEDIR_X) {
                        tempX = stof(textoIngresado);
                        estadoActual = PEDIR_Y;
                        textoIngresado = "";
                    }
                    else if (estadoActual == PEDIR_Y) {
                        float tempY = stof(textoIngresado);

                        puntosMatematicos.push_back({ tempX, tempY });
                        Vector2 pPantalla = ConvertirAPantalla(tempX, tempY, screenWidth, screenHeight);
                        puntosPantalla.push_back(pPantalla);

                        verticeActual++;
                        textoIngresado = "";

                        if (verticeActual >= numVertices) {
                            estadoActual = MOSTRAR_FIGURA;

                            // NUEVO: Una vez ingresados todos los puntos, procesamos la triangulación
                            triangulosProcesados = TriangularPoligono(puntosPantalla);
                        }
                        else {
                            estadoActual = PEDIR_X;
                        }
                    }
                }
                catch (...) {
                    textoIngresado = "";
                }
            }
        }

        // ==========================================
        // 2. LÓGICA DE DIBUJO
        // ==========================================
        frameCount++;
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawLine(0, screenHeight / 2, screenWidth, screenHeight / 2, BLACK);
        DrawLine(screenWidth / 2, 0, screenWidth / 2, screenHeight, BLACK);
        DrawCircle(screenWidth / 2, screenHeight / 2, 4, RED);

        if (estadoActual != MOSTRAR_FIGURA) {
            DrawRectangle(150, 150, 500, 200, Fade(LIGHTGRAY, 0.9f));
            DrawRectangleLines(150, 150, 500, 200, DARKGRAY);

            string mensaje = "";
            if (estadoActual == PEDIR_CANTIDAD) mensaje = "Cuantos vertices tiene tu figura? (Minimo 3):";
            else if (estadoActual == PEDIR_X) mensaje = "Ingrese la coordenada X del punto " + to_string(verticeActual + 1) + ":";
            else if (estadoActual == PEDIR_Y) mensaje = "Ingrese la coordenada Y del punto " + to_string(verticeActual + 1) + ":";

            DrawText(mensaje.c_str(), 170, 180, 20, DARKBLUE);
            DrawRectangleLines(170, 230, 460, 40, DARKBLUE);
            DrawText(textoIngresado.c_str(), 180, 240, 20, MAROON);

            if ((frameCount / 30) % 2 == 0) {
                DrawText("_", 180 + MeasureText(textoIngresado.c_str(), 20), 240, 20, MAROON);
            }
            DrawText("Presiona ENTER para confirmar", 170, 300, 15, GRAY);
        }
        else {
            if (puntosPantalla.size() >= 3) {

                // NUEVO: Dibujamos los triángulos procesados uno por uno
                for (int i = 0; i < triangulosProcesados.size(); i += 3) {
                    DrawTriangle(triangulosProcesados[i], triangulosProcesados[i + 1], triangulosProcesados[i + 2], azulTransparente);

                    // Opcional: Dibuja las líneas internas de la triangulación para que veas cómo trabaja la máquina
                    // DrawTriangleLines(triangulosProcesados[i], triangulosProcesados[i + 1], triangulosProcesados[i + 2], Fade(GRAY, 0.3f));
                }

                // Dibujar el contorno original por encima
                for (int i = 0; i < puntosPantalla.size(); i++) {
                    int indiceSiguiente = (i + 1) % puntosPantalla.size();
                    DrawLineEx(puntosPantalla[i], puntosPantalla[indiceSiguiente], 3.0f, DARKBLUE);
                }
            }
            DrawText("Figura triangulada. Presiona ESC para salir.", 10, 10, 20, DARKGRAY);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}