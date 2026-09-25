#ifndef REPORTES_H
#define REPORTES_H

#include "../comandos/Comando.h"
#include "../comandos/Montaje.h"
#include "../estructuras/Estructuras.h"

#include <string>

// escribe el codigo dot en un temporal y llama a graphviz
// la extension del -path decide el formato de salida
bool generarGrafico(const std::string &dot, const std::string &destino,
                    Salida &salida);

// guarda texto plano tal cual lo usan bm_inode bm_block y file
bool guardarTexto(const std::string &texto, const std::string &destino,
                  Salida &salida);

// escapa los caracteres que rompen el html de las tablas dot
std::string escaparHtml(const std::string &texto);

// cada reporte recibe la particion montada y donde guardar
void repMbr    (const Montaje *m, const std::string &destino, Salida &salida);
void repDisk   (const Montaje *m, const std::string &destino, Salida &salida);
void repSb     (const Montaje *m, const std::string &destino, Salida &salida);
void repInode  (const Montaje *m, const std::string &destino, Salida &salida);
void repBlock  (const Montaje *m, const std::string &destino, Salida &salida);
void repBmInode(const Montaje *m, const std::string &destino, Salida &salida);
void repBmBlock(const Montaje *m, const std::string &destino, Salida &salida);
void repTree   (const Montaje *m, const std::string &destino, Salida &salida);
void repFile   (const Montaje *m, const std::string &destino,
                const std::string &rutaInterna, Salida &salida);
void repLs     (const Montaje *m, const std::string &destino,
                const std::string &rutaInterna, Salida &salida);

#endif
