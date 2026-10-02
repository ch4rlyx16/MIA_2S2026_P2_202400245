#ifndef DISCO_EXT2_H
#define DISCO_EXT2_H

#include "../estructuras/Estructuras.h"

#include <string>

// acceso al sistema de archivos ext2 de una particion

// el sb se escribe al inicio de la particion asi que de ahi se deduce
int inicioParticion(const SuperBloque &sb);
bool esExt3(const SuperBloque &sb);
int inicioJournal(const SuperBloque &sb);

// posicion en bytes de un inodo o un bloque
long posInodo (const SuperBloque &sb, int indice);
long posBloque(const SuperBloque &sb, int indice);

bool leerSB (const std::string &ruta, int inicioParticion, SuperBloque &sb);
bool escribirSB(const std::string &ruta, int inicioParticion, const SuperBloque &sb);

bool leerInodo (const std::string &ruta, const SuperBloque &sb, int i, Inodo &nodo);
bool escribirInodo(const std::string &ruta, const SuperBloque &sb, int i, const Inodo &nodo);

// bitmap 0 libre 1 ocupado
bool bitInodo(const std::string &ruta, const SuperBloque &sb, int i);
bool bitBloque(const std::string &ruta, const SuperBloque &sb, int i);
void ponerBitInodo (const std::string &ruta, const SuperBloque &sb, int i, char v);
void ponerBitBloque(const std::string &ruta, const SuperBloque &sb, int i, char v);

// reserva el primer inodo o bloque libre y lo marca ocupado -1 si no hay
int asignarInodo (const std::string &ruta, SuperBloque &sb);
int asignarBloque(const std::string &ruta, SuperBloque &sb);

// bloque fisico del bloque logico n directos 0-11 y luego indirectos
// con asignar en true va creando los que falten
int bloqueLogico(const std::string &ruta, SuperBloque &sb, Inodo &nodo,
                 int n, bool asignar);

// cuantos bloques logicos puede direccionar un inodo
int capacidadBloques();

// contenido completo de un archivo
std::string leerArchivo(const std::string &ruta, SuperBloque &sb, Inodo &nodo);

// reescribe el contenido de un archivo asignando bloques si hace falta
bool escribirArchivo(const std::string &ruta, SuperBloque &sb,
                     int indiceInodo, Inodo &nodo, const std::string &contenido);

// agrega una entrada nombre a inodo dentro de una carpeta
// asigna un bloque nuevo si los que tiene ya estan llenos
bool agregarEntrada(const std::string &ruta, SuperBloque &sb, int indiceCarpeta,
                    Inodo &carpeta, const std::string &nombre, int inodoDestino);

// borra una entrada de una carpeta, deja el hueco libre
bool quitarEntrada(const std::string &ruta, const SuperBloque &sb,
                   const Inodo &carpeta, const std::string &nombre);

// cambia el nombre de una entrada sin tocar a que inodo apunta
bool renombrarEntrada(const std::string &ruta, const SuperBloque &sb,
                      const Inodo &carpeta, const std::string &viejo,
                      const std::string &nuevo);

// libera un inodo con sus bloques, y si es carpeta todo lo que cuelga de el
void liberarInodo(const std::string &ruta, SuperBloque &sb, int indice);

// busca una entrada por nombre dentro de una carpeta -1 si no esta
int buscarEntrada(const std::string &ruta, const SuperBloque &sb,
                  const Inodo &carpeta, const std::string &nombre);

// recorre una ruta absoluta desde la raiz -1 si no existe
int buscarInodoPorRuta(const std::string &ruta, const SuperBloque &sb,
                       const std::string &rutaInterna);

#endif
