#ifndef COMANDOS_DISCOS_H
#define COMANDOS_DISCOS_H

#include "Comando.h"

#include <string>
#include <vector>

/* Administracion de discos virtuales y de sus particiones. */
void cmdMkdisk(const Parametros &p, Salida &salida);
void cmdRmdisk(const Parametros &p, Salida &salida);
void cmdFdisk (const Parametros &p, Salida &salida);

// los discos que el servidor ha visto, para que el visualizador los liste
void registrarDisco(const std::string &ruta);
void olvidarDisco(const std::string &ruta);
const std::vector<std::string> &discosRegistrados();

#endif
