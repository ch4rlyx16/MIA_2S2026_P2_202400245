#include "Loss.h"
#include "Montaje.h"

#include "../disco/Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstdio>

namespace {

// rellena con ceros un tramo del disco, de a 1 KB para no ir byte por byte
bool llenarDeCeros(const std::string &ruta, long desde, long cuantos) {
    FILE *archivo = std::fopen(ruta.c_str(), "rb+");
    if (archivo == nullptr) return false;

    std::fseek(archivo, desde, SEEK_SET);

    char vacio[1024] = { 0 };
    for (long escrito = 0; escrito < cuantos; escrito += sizeof(vacio)) {
        long trozo = cuantos - escrito;
        if (trozo > static_cast<long>(sizeof(vacio))) trozo = sizeof(vacio);
        std::fwrite(vacio, 1, trozo, archivo);
    }

    std::fclose(archivo);
    return true;
}

} // namespace

void cmdLoss(const Parametros &p, Salida &salida) {
    std::string id = aMayusculas(sinComillas(p.obtener("-id")));

    const Montaje *m = buscarMontaje(id);
    if (m == nullptr) {
        salida.error("loss: no hay ninguna particion montada con id " + id);
        return;
    }

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("loss: no se pudo leer la particion " + id);
        return;
    }
    if (!esExt3(sb)) {
        salida.error("loss: la particion " + id +
                     " es ext2 y no tiene bitacora para recuperarse");
        return;
    }

    // los cuatro tramos que pide el enunciado, uno detras del otro:
    // bitmap de inodos, bitmap de bloques, area de inodos y area de bloques
    long desde = sb.s_bm_inode_start;
    long hasta = sb.s_block_start + static_cast<long>(sb.s_blocks_count) * TAM_BLOQUE;

    if (!llenarDeCeros(m->ruta, desde, hasta - desde)) {
        salida.error("loss: no se pudo escribir en " + m->ruta);
        return;
    }

    // el superbloque sobrevive pero sus contadores ya no reflejan nada
    sb.s_free_inodes_count = sb.s_inodes_count;
    sb.s_free_blocks_count = sb.s_blocks_count;
    sb.s_firts_ino = sb.s_inode_start;
    sb.s_first_blo = sb.s_block_start;
    escribirSB(m->ruta, m->inicio, sb);

    salida.exito("loss: se simulo la perdida en " + id + ", se limpiaron " +
                 std::to_string(hasta - desde) + " bytes");
    salida.escribir("loss: el superbloque y la bitacora quedaron intactos");
}
