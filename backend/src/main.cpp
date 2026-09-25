/* Programa de consola.
   Ejecuta un archivo de comandos (o lo que se escriba por teclado) y
   muestra la salida. Sirve para probar el backend sin levantar el
   frontend durante el desarrollo y la calificacion. */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Ejecutor.h"

int main(int argc, char **argv) {
    std::string entrada;

    if (argc > 1) {
        std::ifstream archivo(argv[1]);
        if (!archivo) {
            std::cerr << "No se pudo abrir el archivo: " << argv[1] << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << archivo.rdbuf();
        entrada = buffer.str();
    } else {
        std::cout << "Escribe los comandos y termina con Ctrl+D:\n";
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        entrada = buffer.str();
    }

    Salida salida = ejecutarTexto(entrada);
    for (const std::string &linea : salida.lineas)
        std::cout << linea << "\n";

    return 0;
}
