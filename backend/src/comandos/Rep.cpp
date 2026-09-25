#include "Rep.h"
#include "Montaje.h"

#include "../reportes/Reportes.h"
#include "../util/Texto.h"

void cmdRep(const Parametros &p, Salida &salida) {
    std::string nombre  = aMinusculas(sinComillas(p.obtener("-name")));
    std::string destino = sinComillas(p.obtener("-path"));
    std::string id      = aMayusculas(sinComillas(p.obtener("-id")));

    const Montaje *m = buscarMontaje(id);
    if (m == nullptr) {
        salida.error("rep: no hay ninguna particion montada con id " + id);
        return;
    }

    // mbr y disk leen el disco entero los demas necesitan el ext2
    if (nombre == "mbr")       { repMbr(m, destino, salida);     return; }
    if (nombre == "disk")      { repDisk(m, destino, salida);    return; }
    if (nombre == "sb")        { repSb(m, destino, salida);      return; }
    if (nombre == "bm_inode")  { repBmInode(m, destino, salida); return; }
    if (nombre == "bm_block")  { repBmBlock(m, destino, salida); return; }
    if (nombre == "inode")     { repInode(m, destino, salida);   return; }
    if (nombre == "block")     { repBlock(m, destino, salida);   return; }
    if (nombre == "tree")      { repTree(m, destino, salida);    return; }

    // file y ls informan sobre una ruta concreta de la particion
    std::string interna = sinComillas(p.obtener("-path_file_ls"));
    if (nombre == "file")      { repFile(m, destino, interna, salida); return; }
    if (nombre == "ls")        { repLs(m, destino, interna, salida);   return; }

    salida.error("rep: reporte '" + nombre + "' no reconocido");
}
