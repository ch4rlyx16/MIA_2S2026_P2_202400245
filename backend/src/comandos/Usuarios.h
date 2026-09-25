#ifndef COMANDOS_USUARIOS_H
#define COMANDOS_USUARIOS_H

#include "Comando.h"
#include "Montaje.h"

#include "../estructuras/Estructuras.h"

#include <string>

// lee users.txt de la particion montada devuelve contenido
// inodo donde vive y el superbloque cargado
bool leerUsersTxt(const Montaje *m, SuperBloque &sb, int &inodoUsers,
                  Inodo &nodo, std::string &contenido);

void cmdMkgrp(const Parametros &p, Salida &salida);
void cmdRmgrp(const Parametros &p, Salida &salida);
void cmdMkusr(const Parametros &p, Salida &salida);
void cmdRmusr(const Parametros &p, Salida &salida);
void cmdChgrp(const Parametros &p, Salida &salida);

#endif
