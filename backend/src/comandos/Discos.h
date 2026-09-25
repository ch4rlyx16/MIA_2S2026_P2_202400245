#ifndef COMANDOS_DISCOS_H
#define COMANDOS_DISCOS_H

#include "Comando.h"

/* Administracion de discos virtuales y de sus particiones. */
void cmdMkdisk(const Parametros &p, Salida &salida);
void cmdRmdisk(const Parametros &p, Salida &salida);
void cmdFdisk (const Parametros &p, Salida &salida);

#endif
