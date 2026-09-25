#ifndef COMANDOS_MONTAJE_H
#define COMANDOS_MONTAJE_H

#include "Comando.h"

#include <string>
#include <vector>

/* ============================================================
   Tabla de particiones montadas.

   ============================================================ */

struct Montaje {
    std::string id;        // 451A se empieza por los 2 ultimos digitos de mi carné
    std::string ruta;      // archivo .mia donde esta la particion
    std::string nombre;    // nombre de la particion
    int         inicio;    // part_start
    int         tam;       // part_s
    char        letra;     // letra pa identificar el disco
    int         numero;    // correlativo dentro de ese disco
};

/* Devuelve la particion montada con ese id, o nullptr si no existe. */
const Montaje *buscarMontaje(const std::string &id);

/* Todas las particiones montadas, en el orden en que se montaron. */
const std::vector<Montaje> &montajes();

void cmdMount  (const Parametros &p, Salida &salida);
void cmdMounted(const Parametros &p, Salida &salida);

#endif
