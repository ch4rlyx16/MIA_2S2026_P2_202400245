#ifndef COMANDOS_OPERACIONES_H
#define COMANDOS_OPERACIONES_H

#include "Comando.h"

// operaciones sobre archivos y carpetas ya existentes
void cmdRemove(const Parametros &p, Salida &salida);
void cmdRename(const Parametros &p, Salida &salida);
void cmdCopy  (const Parametros &p, Salida &salida);
void cmdMove  (const Parametros &p, Salida &salida);
void cmdFind  (const Parametros &p, Salida &salida);
void cmdChown (const Parametros &p, Salida &salida);

#endif
