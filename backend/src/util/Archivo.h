#ifndef UTIL_ARCHIVO_H
#define UTIL_ARCHIVO_H

#include <cstdio>
#include <string>

/* ============================================================
   Acceso binario al archivo .mia
por estas 2 plantillas viajan los datos unicamente
   ============================================================ */

/* Escribe la estructura en el byte indicado del archivo. */
template <typename T>
bool escribirEn(const std::string &ruta, long desplazamiento, const T &dato) {
    FILE *archivo = std::fopen(ruta.c_str(), "rb+");
    if (archivo == nullptr) return false;

    std::fseek(archivo, desplazamiento, SEEK_SET);
    bool bien = std::fwrite(&dato, sizeof(T), 1, archivo) == 1;

    std::fclose(archivo);
    return bien;
}

/* Lee la estructura que empieza en el byte indicado. */
template <typename T>
bool leerDe(const std::string &ruta, long desplazamiento, T &dato) {
    FILE *archivo = std::fopen(ruta.c_str(), "rb");
    if (archivo == nullptr) return false;

    std::fseek(archivo, desplazamiento, SEEK_SET);
    bool bien = std::fread(&dato, sizeof(T), 1, archivo) == 1;

    std::fclose(archivo);
    return bien;
}

/* True si el archivo existe y se puede abrir para lectura. */
bool existeArchivo(const std::string &ruta);

/* Crea todas las carpetas de la ruta que falten (como mkdir -p).
   Recibe la ruta de un ARCHIVO: crea solo sus carpetas padre. */
bool crearCarpetasPadre(const std::string &rutaArchivo);

#endif
