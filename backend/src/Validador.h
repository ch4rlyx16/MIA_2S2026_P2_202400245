#ifndef VALIDADOR_H
#define VALIDADOR_H

#include "Analisis.h"

/* Revisa lo que la gramatica no puede: parametros obligatorios que
   faltan, valores fuera de rango y limites de longitud. */
void validarParametros(const Parametros &p,
                       std::vector<ErrorAnalisis> &errores);

#endif
