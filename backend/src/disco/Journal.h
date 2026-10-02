#ifndef DISCO_JOURNAL_H
#define DISCO_JOURNAL_H

#include "../comandos/Montaje.h"
#include "../estructuras/Estructuras.h"

#include <string>
#include <vector>

// anota una operacion en la bitacora, solo si la particion es ext3
void anotarJournal(const Montaje *m, const std::string &operacion,
                   const std::string &ruta, const std::string &contenido);

// devuelve las entradas usadas de la bitacora en orden
std::vector<Journal> leerJournal(const Montaje *m);

#endif