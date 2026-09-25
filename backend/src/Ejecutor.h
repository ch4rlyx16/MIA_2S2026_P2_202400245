#ifndef EJECUTOR_H
#define EJECUTOR_H

#include "comandos/Comando.h"

#include <string>

/* Recorre el texto de entrada linea por linea: analiza cada una,
   ejecuta el comando si es valido y va acumulando la salida que
   vera el usuario*/
Salida ejecutarTexto(const std::string &entrada);

#endif
