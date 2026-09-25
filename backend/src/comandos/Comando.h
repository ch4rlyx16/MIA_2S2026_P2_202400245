#ifndef COMANDO_H
#define COMANDO_H

#include <map>
#include <string>
#include <vector>

/* ============================================================
   Frontera entre el analizador y la ejecucion
   ============================================================ */

struct Parametros {
    std::string comando;                          // "mkdisk", "fdisk"...
    std::map<std::string, std::string> valores;   // "-size" -> "3000"
    size_t linea   = 0;
    size_t columna = 1;

    bool tiene(const std::string &nombre) const {
        return valores.count(nombre) > 0;
    }

    std::string obtener(const std::string &nombre,
                        const std::string &pordefecto = "") const {
        auto it = valores.find(nombre);
        return it != valores.end() ? it->second : pordefecto;
    }
};

/* Lo que el usuario vera en el area de salida del frontend. */
struct Salida {
    std::vector<std::string> lineas;

    void escribir(const std::string &texto) { lineas.push_back(texto); }
    void exito(const std::string &texto)    { lineas.push_back(texto); }
    void error(const std::string &texto)    { lineas.push_back("Error: " + texto); }
};

#endif
