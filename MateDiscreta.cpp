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
// (Raylib solo pinta triángulos, así que necesitamos "rebanar" cualquier polígono 
// que el usuario dibuje en varios triángulos para poder rellenarlo de color).
// ==========================================

// Función que calcula la orientación geométrica de 3 puntos (si giran a la izquierda o derecha)
// Matemáticamente, esto es el determinante de una matriz formada por los vectores ab y ac.
float ProductoCruz(Vector2 a, Vector2 b, Vector2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Verifica si un punto 'p' se encuentra dentro del triángulo formado por a, b y c.
bool PuntoEnTriangulo(Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
    float cp1 = ProductoCruz(a, b, p);
    float cp2 = ProductoCruz(b, c, p);
    float cp3 = ProductoCruz(c, a, p);
    // Si todos los productos cruzados tienen el mismo signo, el punto está dentro.
    bool tieneNegativo = (cp1 < 0) || (cp2 < 0) || (cp3 < 0);
    bool tienePositivo = (cp1 > 0) || (cp2 > 0) || (cp3 > 0);
    return !(tieneNegativo && tienePositivo);
}

// Algoritmo "Ear Clipping" (Corte de Orejas) para dividir un polígono en triángulos.
vector<Vector2> TriangularPoligono(vector<Vector2> vertices) {
    vector<Vector2> triangulos; // Aquí guardaremos los triángulos resultantes
    if (vertices.size() < 3) return triangulos; // Una figura de menos de 3 lados no se puede triangular
    vector<Vector2> v = vertices;

    // Calcular el área usando la fórmula del polígono (Shoelace) para saber si los vértices
    // están en sentido horario o antihorario.
    float area = 0;
    for (int i = 0; i < (int)v.size(); i++) {
        int j = (i + 1) % v.size();
        area += (v[i].x * v[j].y) - (v[j].x * v[i].y);
    }
    // Si el área es positiva, invertimos el orden de los vértices para estandarizarlos.
    if (area > 0) reverse(v.begin(), v.end());

    int intentos = 0;
    // Bucle para ir "cortando orejas" (triángulos) hasta que solo quede un triángulo.
    while (v.size() > 3 && intentos < 1000) {
        bool orejaCortada = false;
        int n = (int)v.size();
        for (int i = 0; i < n; i++) {
            int prev = (i - 1 + n) % n; // Vértice anterior
            int next = (i + 1) % n;     // Vértice siguiente
            Vector2 A = v[prev], B = v[i], C = v[next];

            // Si el ángulo es cóncavo, no es una "oreja" válida para cortar.
            if (ProductoCruz(A, B, C) >= 0) continue;

            bool esOreja = true;
            // Verificamos que ningún otro vértice del polígono esté dentro de esta posible oreja.
            for (int j = 0; j < n; j++) {
                if (j == prev || j == i || j == next) continue;
                if (PuntoEnTriangulo(v[j], A, B, C)) {
                    esOreja = false; break;
                }
            }
            // Si es una oreja válida, la guardamos como triángulo y borramos el vértice del polígono original.
            if (esOreja) {
                triangulos.push_back(A); triangulos.push_back(B); triangulos.push_back(C);
                v.erase(v.begin() + i);
                orejaCortada = true; break;
            }
        }
        intentos++; // Evitar bucles infinitos en polígonos inválidos (ej. líneas cruzadas)
        if (!orejaCortada) break;
    }
    // Guardamos el último triángulo restante
    if (v.size() == 3) {
        triangulos.push_back(v[0]); triangulos.push_back(v[1]); triangulos.push_back(v[2]);
    }
    return triangulos;
}

// ==========================================
// 2. MATEMÁTICAS DE TRANSFORMACIONES Y PANTALLA
// ==========================================

// Convierte un punto del Plano Cartesiano (centro 0,0 y la Y crece hacia arriba) 
// a Coordenadas de Pantalla (donde el 0,0 es la esquina superior izquierda y la Y crece hacia abajo).
Vector2 ConvertirAPantalla(Vector2 mathP, int origenX, int origenY) {
    return { mathP.x + (float)origenX, (float)origenY - mathP.y };
}

// Interpolación Lineal (InterpolarPuntos) para Vectores: Calcula un punto intermedio entre A y B según un porcentaje 't'.
// Es clave para que la animación se vea fluida y no "salte" de golpe al resultado final.
Vector2 InterpolarPuntos(Vector2 a, Vector2 b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

// Interpolación Lineal para números flotantes sueltos (usado para ángulos y escalas).
float InterpolarValor(float start, float end, float t) {
    return start + (end - start) * t;
}

int main() {
    // Configuración inicial de la ventana
    const int screenWidth = 1000;
    const int screenHeight = 650;

    InitWindow(screenWidth, screenHeight, "VectraLab - Transformaciones Lineales 2D");
    SetTargetFPS(60); // Fija el juego a 60 Fotogramas por Segundo

    // Carga de la imagen para el Modo Videojuego
    Texture2D texturaPersonaje = LoadTexture("personaje.png");
    bool modoVideojuego = false;

    // Configura el tamaño de fuente para la interfaz de RayGUI
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    // Define el origen (0,0) del plano cartesiano, desplazado a la izquierda para dejar espacio al panel
    int origenX = (screenWidth - 350) / 2;
    int origenY = screenHeight / 2;

    // Vectores que guardan el estado de la figura geométrica
    vector<Vector2> figuraOriginal = { {50, 50}, {150, 50}, {150, 150}, {50, 150} }; // Cuadrado inicial por defecto
    vector<Vector2> figuraObjetivo = figuraOriginal;        // Hacia dónde debe llegar la figura tras la transformación
    vector<Vector2> figuraAnimada = figuraOriginal;         // La figura actual que se está moviendo en pantalla
    vector<Vector2> figuraInicioAnimacion = figuraOriginal; // Desde dónde empezó a moverse la figura

    // Variables para las cajas de texto de entrada de coordenadas (Interfaz)
    char inputX[16] = "";
    char inputY[16] = "";
    bool editX = false;
    bool editY = false;

    // Variables de control para los menús desplegables
    bool dropdownTransformacionEdit = false;
    bool dropdownReflexionEdit = false;

    // Estados de las transformaciones
    int transformacionSeleccionada = 0; // 0=Rotación, 1=Homotecia, 2=Reflexión
    int transformacionPrevia = 0;       // Para detectar si el usuario cambió de opinión

    int ejeReflexion = 0;       // 0=Eje X, 1=Eje Y, 2=Origen, 3=Y=X, 4=Y=-X
    int ejeReflexionPrevio = 0;

    // Valores ingresados por el usuario mediante los Sliders (barras)
    float valorAngulo = 0.0f;
    float valorEscala = 1.0f;

    // Variables matemáticas para llevar el control temporal de la animación del POLÍGONO
    float anguloActualVisual = 0.0f;
    float anguloInicioAnimacion = 0.0f;
    float anguloDestinoAnimacion = 0.0f;

    float escalaActualVisual = 1.0f;
    float escalaInicioAnimacion = 1.0f;
    float escalaDestinoAnimacion = 1.0f;

    // Variables matemáticas para llevar el control temporal de la animación del SPRITE (Modo Videojuego)
    float spriteStartAngle = 0.0f;  float spriteTargetAngle = 0.0f;  float spriteCurrentAngle = 0.0f;
    float spriteStartScaleX = 1.0f; float spriteTargetScaleX = 1.0f; float spriteCurrentScaleX = 1.0f;
    float spriteStartScaleY = 1.0f; float spriteTargetScaleY = 1.0f; float spriteCurrentScaleY = 1.0f;

    // El parámetro 't' de tiempo para las interpolaciones (Va de 0.0 a 1.0)
    float animacionT = 1.0f;

    // ==========================================
    // BUCLE PRINCIPAL DEL PROGRAMA (Se ejecuta 60 veces por segundo)
    // ==========================================
    while (!WindowShouldClose()) {

        // Si el usuario cambia el tipo de transformación en el menú, reiniciamos todo 
        // a su estado original para evitar comportamientos gráficos erráticos.
        if (transformacionSeleccionada != transformacionPrevia ||
            (transformacionSeleccionada == 2 && ejeReflexion != ejeReflexionPrevio)) {

            figuraObjetivo = figuraOriginal;
            figuraAnimada = figuraOriginal;
            figuraInicioAnimacion = figuraOriginal;
            animacionT = 1.0f; // Detenemos la animación
            valorAngulo = 0.0f;
            valorEscala = 1.0f;

            anguloActualVisual = 0.0f; anguloInicioAnimacion = 0.0f; anguloDestinoAnimacion = 0.0f;
            escalaActualVisual = 1.0f; escalaInicioAnimacion = 1.0f; escalaDestinoAnimacion = 1.0f;

            spriteStartAngle = 0.0f;  spriteTargetAngle = 0.0f;  spriteCurrentAngle = 0.0f;
            spriteStartScaleX = 1.0f; spriteTargetScaleX = 1.0f; spriteCurrentScaleX = 1.0f;
            spriteStartScaleY = 1.0f; spriteTargetScaleY = 1.0f; spriteCurrentScaleY = 1.0f;

            transformacionPrevia = transformacionSeleccionada;
            ejeReflexionPrevio = ejeReflexion;
        }

        // ==========================================
        // LÓGICA DE ANIMACIÓN MATEMÁTICA
        // ==========================================
        // Si 'animacionT' es menor a 1, significa que la animación está en proceso.
        if (animacionT < 1.0f) {
            animacionT += 0.015f; // Velocidad de la animación
            if (animacionT > 1.0f) animacionT = 1.0f; // Aseguramos que no pase del 100%

            // Función matemática Smoothstep: Hace que la animación acelere al inicio y desacelere al final.
            float smoothT = animacionT * animacionT * (3.0f - 2.0f * animacionT);

            // Interpolamos el ángulo para el modo videojuego
            spriteCurrentAngle = InterpolarValor(spriteStartAngle, spriteTargetAngle, smoothT);

            // Interpolamos la escala para el modo videojuego (evitando distorsiones raras si es reflexión)
            if (transformacionSeleccionada == 2) {
                spriteCurrentScaleX = spriteTargetScaleX;
                spriteCurrentScaleY = spriteTargetScaleY;
            }
            else {
                spriteCurrentScaleX = InterpolarValor(spriteStartScaleX, spriteTargetScaleX, smoothT);
                spriteCurrentScaleY = InterpolarValor(spriteStartScaleY, spriteTargetScaleY, smoothT);
            }

            // Aplicamos las transformaciones algebraicas vértice por vértice
            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                float px = figuraOriginal[i].x; // X original
                float py = figuraOriginal[i].y; // Y original

                if (transformacionSeleccionada == 0) {
                    // ROTACIÓN: Se convierte de grados a radianes y se aplica la MATRIZ DE ROTACIÓN 2D
                    float anguloInterpolado = anguloInicioAnimacion + (anguloDestinoAnimacion - anguloInicioAnimacion) * smoothT;
                    anguloActualVisual = anguloInterpolado;
                    float rad = anguloInterpolado * (PI / 180.0f);
                    figuraAnimada[i].x = px * cos(rad) - py * sin(rad); // x' = x*cos(θ) - y*sin(θ)
                    figuraAnimada[i].y = px * sin(rad) + py * cos(rad); // y' = x*sin(θ) + y*cos(θ)
                }
                else if (transformacionSeleccionada == 1) {
                    // HOMOTECIA: Multiplicación por un escalar 'k'
                    float escalaInterpolada = escalaInicioAnimacion + (escalaDestinoAnimacion - escalaInicioAnimacion) * smoothT;
                    escalaActualVisual = escalaInterpolada;
                    figuraAnimada[i].x = px * escalaInterpolada; // x' = k*x
                    figuraAnimada[i].y = py * escalaInterpolada; // y' = k*y
                }
                else if (transformacionSeleccionada == 2) {
                    // REFLEXIÓN: Dependiendo del eje, usamos interpolación o una animación de semicírculo
                    if (ejeReflexion == 2) { // Reflexión respecto al origen (es una rotación de 180 grados)
                        float diffX = fabs(figuraInicioAnimacion[0].x - figuraObjetivo[0].x);
                        float targetRad = (diffX > 0.1f) ? PI : 0.0f; // PI Radianes = 180 grados
                        float rad = targetRad * smoothT;

                        float startX = figuraInicioAnimacion[i].x;
                        float startY = figuraInicioAnimacion[i].y;
                        figuraAnimada[i].x = startX * cos(rad) - startY * sin(rad);
                        figuraAnimada[i].y = startX * sin(rad) + startY * cos(rad);
                    }
                    else {
                        // Las demás reflexiones simplemente se mueven en línea recta hacia su destino
                        figuraAnimada[i] = InterpolarPuntos(figuraInicioAnimacion[i], figuraObjetivo[i], smoothT);
                    }
                }
            }
        }

        // ==========================================
        // DIBUJO EN PANTALLA (RENDERIZADO)
        // ==========================================
        BeginDrawing();
        ClearBackground(RAYWHITE); // Fondo blanco

        // Dibuja los Ejes X e Y principales (gris claro)
        DrawLine(0, origenY, screenWidth - 350, origenY, LIGHTGRAY);
        DrawLine(origenX, 0, origenX, screenHeight, LIGHTGRAY);

        // Bucle para dibujar las marcas en los ejes cartesianos (cada 30 unidades) y sus números
        for (int step = 30; step < screenWidth; step += 30) {
            // Marcas positivas en X
            if (origenX + step < screenWidth - 350) {
                DrawLine(origenX + step, origenY - 3, origenX + step, origenY + 3, GRAY);
                DrawText(TextFormat("%d", step), origenX + step - 8, origenY + 6, 10, DARKGRAY);
            }
            // Marcas negativas en X
            if (origenX - step > 0) {
                DrawLine(origenX - step, origenY - 3, origenX - step, origenY + 3, GRAY);
                DrawText(TextFormat("-%d", step), origenX - step - 12, origenY + 6, 10, DARKGRAY);
            }
            // Marcas positivas en Y (Recuerda que en pantalla la Y se resta hacia arriba)
            if (origenY - step > 0) {
                DrawLine(origenX - 3, origenY - step, origenX + 3, origenY - step, GRAY);
                DrawText(TextFormat("%d", step), origenX + 6, origenY - step - 5, 10, DARKGRAY);
            }
            // Marcas negativas en Y
            if (origenY + step < screenHeight) {
                DrawLine(origenX - 3, origenY + step, origenX + 3, origenY + step, GRAY);
                DrawText(TextFormat("-%d", step), origenX + 6, origenY + step - 5, 10, DARKGRAY);
            }
        }
        // Dibuja un punto rojo central que indica el punto (0,0) del plano
        DrawCircle(origenX, origenY, 4, RED);

        // Si estamos en modo REFLEXIÓN, dibuja la línea (o punto) verde que representa el Eje de Reflexión
        if (transformacionSeleccionada == 2) {
            Color colorRef = Fade(GREEN, 0.6f); // Color verde semi-transparente
            float grosorRef = 2.0f;
            float graphAreaWidth = (float)screenWidth - 350.0f;

            switch (ejeReflexion) {
            case 0: DrawLineEx({ 0, (float)origenY }, { graphAreaWidth, (float)origenY }, grosorRef, colorRef); break; // Eje X
            case 1: DrawLineEx({ (float)origenX, 0 }, { (float)origenX, (float)screenHeight }, grosorRef, colorRef); break; // Eje Y
            case 2: DrawCircleLines(origenX, origenY, 10, colorRef); break; // Origen
            case 3: DrawLineEx(ConvertirAPantalla({ -1000, -1000 }, origenX, origenY), ConvertirAPantalla({ 1000, 1000 }, origenX, origenY), grosorRef, colorRef); break; // Recta Y = X
            case 4: DrawLineEx(ConvertirAPantalla({ -1000, 1000 }, origenX, origenY), ConvertirAPantalla({ 1000, -1000 }, origenX, origenY), grosorRef, colorRef); break; // Recta Y = -X
            }
        }

        // DIBUJO DE LA FIGURA ORIGINAL (Fantasma de referencia en gris claro)
        if (!modoVideojuego && figuraOriginal.size() >= 2) {
            vector<Vector2> puntosOriginalesPantalla;
            // Convertimos las coordenadas matemáticas a coordenadas de píxeles en la pantalla
            for (auto p : figuraOriginal) puntosOriginalesPantalla.push_back(ConvertirAPantalla(p, origenX, origenY));

            if (puntosOriginalesPantalla.size() >= 3) { // Si es un polígono
                // Usamos la función de matemáticas que creamos arriba para triangular la figura
                vector<Vector2> triangulosRef = TriangularPoligono(puntosOriginalesPantalla);
                for (size_t i = 0; i + 2 < triangulosRef.size(); i += 3) {
                    DrawTriangle(triangulosRef[i], triangulosRef[i + 1], triangulosRef[i + 2], Fade(GRAY, 0.15f));
                }
                // Dibujamos las líneas del contorno
                for (size_t i = 0; i < puntosOriginalesPantalla.size(); i++) {
                    int sig = (i + 1) % puntosOriginalesPantalla.size();
                    DrawLineEx(puntosOriginalesPantalla[i], puntosOriginalesPantalla[sig], 1.5f, Fade(GRAY, 0.4f));
                }
            }
            else if (puntosOriginalesPantalla.size() == 2) { // Si solo es una línea
                DrawLineEx(puntosOriginalesPantalla[0], puntosOriginalesPantalla[1], 1.5f, Fade(GRAY, 0.4f));
            }
        }

        // DIBUJO DE LA FIGURA ANIMADA (La figura azul sólida)
        if (!modoVideojuego) {
            vector<Vector2> puntosPantalla;
            for (auto p : figuraAnimada) {
                puntosPantalla.push_back(ConvertirAPantalla(p, origenX, origenY));
            }

            if (puntosPantalla.size() >= 3) {
                vector<Vector2> triangulos = TriangularPoligono(puntosPantalla);
                Color colorRelleno = { 0, 121, 241, 150 }; // Azul semi-transparente
                // Rellenamos el polígono mediante triángulos
                for (size_t i = 0; i + 2 < triangulos.size(); i += 3) {
                    DrawTriangle(triangulos[i], triangulos[i + 1], triangulos[i + 2], colorRelleno);
                }
                // Dibujamos el contorno azul oscuro
                for (size_t i = 0; i < puntosPantalla.size(); i++) {
                    int sig = (i + 1) % puntosPantalla.size();
                    DrawLineEx(puntosPantalla[i], puntosPantalla[sig], 3.0f, DARKBLUE);
                }
            }
            else if (puntosPantalla.size() == 2) {
                DrawLineEx(puntosPantalla[0], puntosPantalla[1], 3.0f, DARKBLUE);
            }
            // Dibujamos pequeños círculos rojos oscuros en los vértices
            for (size_t i = 0; i < puntosPantalla.size(); i++) {
                DrawCircleV(puntosPantalla[i], 5.0f, MAROON);
            }
        }
        else {
            // MODO VIDEOJUEGO (Renderizado de Texturas en lugar de Polígonos)
            if (texturaPersonaje.id > 0 && figuraOriginal.size() > 0) {
                float texW = (float)texturaPersonaje.width;
                float texH = (float)texturaPersonaje.height;

                // Cálculo del punto central matemático del personaje para poder rotarlo desde su centro
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
                // Valores de seguridad por si el usuario solo pone 1 punto
                if (baseWidth == 0) baseWidth = 100.0f;
                if (baseHeight == 0) baseHeight = 100.0f;

                float sumX_anim = 0, sumY_anim = 0;
                for (auto p : figuraAnimada) {
                    sumX_anim += p.x; sumY_anim += p.y;
                }
                Vector2 centroAnimadoMath = { sumX_anim / figuraAnimada.size(), sumY_anim / figuraAnimada.size() };

                // Dibujar el Sprite "Fantasma" en la posición original (Transparente)
                Vector2 posFantasma = ConvertirAPantalla(centroOriginalMath, origenX, origenY);
                Rectangle destFantasma = { posFantasma.x, posFantasma.y, baseWidth, baseHeight };
                Vector2 origFantasma = { baseWidth / 2.0f, baseHeight / 2.0f }; // El pivote está en el medio de la imagen
                Rectangle sourceFantasma = { 0.0f, 0.0f, texW, texH };

                DrawTexturePro(texturaPersonaje, sourceFantasma, destFantasma, origFantasma, 0.0f, Fade(WHITE, 0.3f));

                // Dibujar el Sprite Animado aplicando los parámetros matemáticos transformados
                Vector2 posPantalla = ConvertirAPantalla(centroAnimadoMath, origenX, origenY);

                float finalW = baseWidth * fabs(spriteCurrentScaleX); // Aplicar factor de escalado a la base
                float finalH = baseHeight * fabs(spriteCurrentScaleY);

                // Truco de gráficos: Para hacer una reflexión de una imagen, le decimos al programa
                // que la dibuje con anchura o altura negativa (-texW o -texH).
                float sourceW = (spriteCurrentScaleX < 0) ? -texW : texW;
                float sourceH = (spriteCurrentScaleY < 0) ? -texH : texH;

                Rectangle sourceRec = { 0.0f, 0.0f, sourceW, sourceH }; // Qué parte de la imagen mostrar (Toda)
                Rectangle destRec = { posPantalla.x, posPantalla.y, finalW, finalH }; // Dónde dibujarla y de qué tamaño
                Vector2 origin = { finalW / 2.0f, finalH / 2.0f };

                DrawTexturePro(texturaPersonaje, sourceRec, destRec, origin, -spriteCurrentAngle, WHITE); // Dibuja rotado
            }
            else if (figuraOriginal.size() == 0) {
                DrawText("Agrega al menos 1 punto (ej. X=100, Y=100) para anclar el personaje.", origenX - 220, origenY - 20, 16, DARKGRAY);
            }
            else { // Si falla la carga de "personaje.png"
                DrawText("ERROR: No se encontro 'personaje.png' en la carpeta o tiene formato invalido.", origenX - 250, origenY - 20, 16, RED);
            }
        }

        // ==========================================
        // DIBUJO DEL PANEL DE CONTROL LATERAL (INTERFAZ RAYGUI)
        // ==========================================
        float panelX = (float)screenWidth - 350.0f; // El panel ocupa 350px a la derecha
        DrawRectangle((int)panelX, 0, 350, screenHeight, Fade(LIGHTGRAY, 0.4f)); // Fondo del panel
        DrawLine((int)panelX, 0, (int)panelX, screenHeight, GRAY);               // Borde del panel

        DrawText("DASHBOARD DE MATRICES", (int)panelX + 50, 15, 20, DARKBLUE); // Título

        // Checkbox para cambiar al modo textura
        GuiCheckBox(Rectangle{ panelX + 20.0f, 45.0f, 20.0f, 20.0f }, "MODO VIDEOJUEGO (Sprite)", &modoVideojuego);

        // Bloque 1: Gestión de Puntos (Coordenadas X, Y)
        GuiGroupBox(Rectangle{ panelX + 20.0f, 75.0f, 310.0f, 135.0f }, "1. GESTION DE PUNTOS");

        GuiLabel(Rectangle{ panelX + 40.0f, 90.0f, 20.0f, 30.0f }, "X:");
        if (GuiTextBox(Rectangle{ panelX + 60.0f, 90.0f, 80.0f, 30.0f }, inputX, 16, editX)) editX = !editX; // TextBox para X

        GuiLabel(Rectangle{ panelX + 160.0f, 90.0f, 20.0f, 30.0f }, "Y:");
        if (GuiTextBox(Rectangle{ panelX + 180.0f, 90.0f, 80.0f, 30.0f }, inputY, 16, editY)) editY = !editY; // TextBox para Y

        // Botón: Añadir Punto
        if (GuiButton(Rectangle{ panelX + 40.0f, 130.0f, 130.0f, 30.0f }, "Añadir Punto")) {
            try {
                // stof() convierte texto (String) a número flotante (Float).
                float px = stof(inputX);
                float py = stof(inputY);
                // Añadimos la coordenada al vector dinámico
                figuraOriginal.push_back({ px, py });
                // Actualizamos las copias
                figuraObjetivo = figuraOriginal;
                figuraAnimada = figuraOriginal;
                figuraInicioAnimacion = figuraOriginal;
                // Vaciamos las cajas de texto para el próximo punto
                inputX[0] = '\0'; inputY[0] = '\0';

                // Reseteamos las animaciones al añadir un punto nuevo
                valorAngulo = 0.0f; valorEscala = 1.0f;
                anguloActualVisual = 0.0f; escalaActualVisual = 1.0f;

                spriteStartAngle = 0.0f;  spriteTargetAngle = 0.0f;  spriteCurrentAngle = 0.0f;
                spriteStartScaleX = 1.0f; spriteTargetScaleX = 1.0f; spriteCurrentScaleX = 1.0f;
                spriteStartScaleY = 1.0f; spriteTargetScaleY = 1.0f; spriteCurrentScaleY = 1.0f;
            }
            catch (...) {} // El bloque Try-Catch evita que el programa se cierre si el usuario escribe letras en lugar de números
        }

        // Botón: Limpiar Figura (Borra todos los elementos del arreglo)
        if (GuiButton(Rectangle{ panelX + 180.0f, 130.0f, 130.0f, 30.0f }, "Limpiar Figura")) {
            figuraOriginal.clear(); figuraObjetivo.clear(); figuraAnimada.clear(); figuraInicioAnimacion.clear();
        }

        DrawText(TextFormat("Vertices actuales: %d", (int)figuraOriginal.size()), (int)panelX + 40, 175, 14, DARKGRAY);

        // Bloque 2: Controles Matemáticos (Sliders y Transformaciones)
        GuiGroupBox(Rectangle{ panelX + 20.0f, 230.0f, 310.0f, 220.0f }, "2. PARÁMETROS MATEMÁTICOS");

        // Slider (Barra deslizadora) dinámica que cambia según la transformación elegida
        if (transformacionSeleccionada == 0) { // Si es Rotación, muestra slider de -360 a 360 grados
            DrawText(TextFormat("Angulo (Grados): %.1f", valorAngulo), (int)panelX + 40, 360, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 390.0f, 270.0f, 20.0f }, "-360", "360", &valorAngulo, -360.0f, 360.0f);
        }
        else if (transformacionSeleccionada == 1) { // Si es Homotecia, muestra slider de factor de escala k
            DrawText(TextFormat("Factor Escala (k): %.2f", valorEscala), (int)panelX + 40, 360, 16, BLACK);
            GuiSliderBar(Rectangle{ panelX + 40.0f, 390.0f, 270.0f, 20.0f }, "0.1", "3.0", &valorEscala, 0.1f, 3.0f);
        }

        // Si los menús desplegables están abiertos, deshabilitamos el botón de atrás para que no se presione por error
        if (dropdownTransformacionEdit || dropdownReflexionEdit) GuiDisable();

        // ==========================================
        // BOTÓN: APLICAR TRANSFORMACIÓN (EL CEREBRO MATEMÁTICO PRINCIPAL)
        // ==========================================
        if (GuiButton(Rectangle{ panelX + 20.0f, 470.0f, 310.0f, 50.0f }, "APLICAR TRANSFORMACIÓN (Animar)")) {

            // Preparamos los valores para que el bloque de animación inicie
            figuraInicioAnimacion = figuraAnimada;
            anguloInicioAnimacion = anguloActualVisual;
            escalaInicioAnimacion = escalaActualVisual;
            anguloDestinoAnimacion = valorAngulo;
            escalaDestinoAnimacion = valorEscala;

            spriteStartAngle = spriteCurrentAngle;
            spriteStartScaleX = spriteCurrentScaleX;
            spriteStartScaleY = spriteCurrentScaleY;

            spriteTargetAngle = 0.0f;
            spriteTargetScaleX = 1.0f;
            spriteTargetScaleY = 1.0f;

            // Configuración de escalas destino para sprites basados en la transformación
            if (transformacionSeleccionada == 0) {
                spriteTargetAngle = valorAngulo;
            }
            else if (transformacionSeleccionada == 1) {
                spriteTargetScaleX = valorEscala;
                spriteTargetScaleY = valorEscala;
            }
            else if (transformacionSeleccionada == 2) { // Efectos espejo para la imagen
                if (ejeReflexion == 0) {
                    spriteTargetScaleX = 1.0f; spriteTargetScaleY = -1.0f;
                }
                else if (ejeReflexion == 1) {
                    spriteTargetScaleX = -1.0f; spriteTargetScaleY = 1.0f;
                }
                else if (ejeReflexion == 2) {
                    spriteTargetAngle = 180.0f;
                }
                else if (ejeReflexion == 3) {
                    spriteTargetScaleY = -1.0f;
                    spriteTargetAngle = 90.0f;
                }
                else if (ejeReflexion == 4) {
                    spriteTargetScaleY = -1.0f;
                    spriteTargetAngle = -90.0f;
                }
            }

            animacionT = 0.0f; // Reiniciar 't' a 0 detona que la animación empiece a dibujarse

            // APLICACIÓN INMEDIATA DEL ÁLGEBRA LINEAL SOBRE EL VECTOR 'figuraObjetivo'
            for (size_t i = 0; i < figuraOriginal.size(); i++) {
                float px = figuraOriginal[i].x;
                float py = figuraOriginal[i].y;

                if (transformacionSeleccionada == 0) {
                    // ROTACIÓN: Se convierte el slider a Radianes y se aplica la matriz de transformación
                    float rad = anguloDestinoAnimacion * (PI / 180.0f);
                    figuraObjetivo[i].x = px * cos(rad) - py * sin(rad);
                    figuraObjetivo[i].y = px * sin(rad) + py * cos(rad);
                }
                else if (transformacionSeleccionada == 1) {
                    // HOMOTECIA: Multiplicación por escalar
                    figuraObjetivo[i].x = px * escalaDestinoAnimacion;
                    figuraObjetivo[i].y = py * escalaDestinoAnimacion;
                }
                else if (transformacionSeleccionada == 2) {
                    // REFLEXIÓN: Modificación de signos según las reglas formales de transformación matricial en R2
                    if (ejeReflexion == 0) { figuraObjetivo[i].x = px; figuraObjetivo[i].y = -py; }      // T(x,y) = (x, -y)
                    if (ejeReflexion == 1) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = py; }      // T(x,y) = (-x, y)
                    if (ejeReflexion == 2) { figuraObjetivo[i].x = -px; figuraObjetivo[i].y = -py; }     // T(x,y) = (-x, -y)
                    if (ejeReflexion == 3) { figuraObjetivo[i].x = py; figuraObjetivo[i].y = px; }       // T(x,y) = (y, x)
                    if (ejeReflexion == 4) { figuraObjetivo[i].x = -py; figuraObjetivo[i].y = -px; }     // T(x,y) = (-y, -x)
                }
            }
        }

        GuiEnable(); // Reactivar UI

        // Menú desplegable para elegir el EJE de REFLEXIÓN (Solo se muestra si la opción de transformación es 2)
        if (transformacionSeleccionada == 2) {
            DrawText("Seleccione la Reflexion:", (int)panelX + 40, 360, 16, BLACK);
            if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 385.0f, 270.0f, 30.0f }, "Eje X;Eje Y;Origen;Recta Y = X;Recta Y = -X", &ejeReflexion, dropdownReflexionEdit)) {
                dropdownReflexionEdit = !dropdownReflexionEdit;
            }
        }

        // Menú desplegable para elegir el TIPO de TRANSFORMACIÓN (El orden importa para los índices: 0, 1, 2)
        GuiLabel(Rectangle{ panelX + 40.0f, 250.0f, 200.0f, 20.0f }, "Tipo de Transformacion:");
        if (GuiDropdownBox(Rectangle{ panelX + 40.0f, 275.0f, 270.0f, 30.0f }, "ROTACION;HOMOTECIA;REFLEXION", &transformacionSeleccionada, dropdownTransformacionEdit)) {
            dropdownTransformacionEdit = !dropdownTransformacionEdit;
        }

        EndDrawing(); // Finaliza este fotograma y lo envía al monitor
    }

    // ==========================================
    // LIMPIEZA DE MEMORIA
    // ==========================================
    UnloadTexture(texturaPersonaje); // Libera la imagen de la memoria RAM
    CloseWindow(); // Destruye la ventana de Raylib
    return 0; // Termina el programa exitosamente
}