#include "Archivo.h"

#include <sys/stat.h>
#include <sys/types.h>

bool existeArchivo(const std::string &ruta) {
    struct stat info;
    return stat(ruta.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

bool crearCarpetasPadre(const std::string &rutaArchivo) {
    /* Recorre la ruta creando cada carpeta intermedia. Se detiene antes
       del ultimo '/' porque lo que sigue es el nombre del archivo. */
    size_t ultimaBarra = rutaArchivo.find_last_of('/');
    if (ultimaBarra == std::string::npos || ultimaBarra == 0) return true;

    std::string carpetas = rutaArchivo.substr(0, ultimaBarra);

    for (size_t i = 1; i <= carpetas.size(); ++i) {
        if (i == carpetas.size() || carpetas[i] == '/') {
            std::string parcial = carpetas.substr(0, i);
            /* EEXIST no es un fallo: la carpeta ya estaba. */
            if (mkdir(parcial.c_str(), 0755) != 0) {
                struct stat info;
                if (stat(parcial.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
                    return false;
            }
        }
    }
    return true;
}
