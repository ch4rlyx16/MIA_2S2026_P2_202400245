#ifndef COMANDOS_PERMISOS_H
#define COMANDOS_PERMISOS_H

#include "Sesion.h"

#include "../estructuras/Estructuras.h"

// permisos UGO en octal root siempre puede todo
bool puedeLeer   (const Sesion &s, const Inodo &nodo);
bool puedeEscribir(const Sesion &s, const Inodo &nodo);

#endif
