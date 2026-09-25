#ifndef ANALISIS_H
#define ANALISIS_H

#include "comandos/Comando.h"

#include <string>
#include <vector>

/* Un error detectado al leer o validar un comando. */
struct ErrorAnalisis {
    size_t      linea;
    size_t      columna;
    std::string mensaje;
};

/* Lo que produce el analisis de UNA linea de entrada. Si no hay
   errores, parametros trae el comando listo para ejecutarse. */
struct AnalisisLinea {
    bool                       hayComando = false;
    Parametros                 parametros;
    std::vector<ErrorAnalisis> errores;
};

#endif
