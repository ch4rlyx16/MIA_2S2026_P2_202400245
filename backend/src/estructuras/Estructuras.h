#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <ctime>

/* ============================================================
   Estructuras que se escriben como son del archivo .mia
   Proyecto 2 - MIA 2S2026 - 202400245
   ============================================================ */
#pragma pack(push, 1)



/* Una particion primaria o extendida q Vive dentro del mbr */
struct Particion {
    char   part_status;       // '0' libre, '1' creada, '2' montada
    char   part_type;         // 'P' primaria, 'E' extendida
    char   part_fit;          // 'B' best, 'F' first, 'W' worst
    int    part_start;        // byte del disco donde inicia
    int    part_s;            // tamano total en bytes
    char   part_name[16];     // nombre
    int    part_correlative;  // -1 hasta que se monta; luego 1, 2, 3...
    char   part_id[4];        // id asignado al montarla
};


struct MBR {
    int       mbr_tamano;          // tamano total del disco en bytes
    time_t    mbr_fecha_creacion;
    int       mbr_dsk_signature;   // numero aleatorio, identifica el disco
    char      dsk_fit;             // ajuste con el que se ubican las particiones
    Particion mbr_partitions[4];
};

struct EBR {
    char part_mount;      // '0' no montada, '1' montada
    char part_fit;
    int  part_start;      // byte donde inicia (el propio EBR)
    int  part_s;          // tamano de la logica en bytes
    int  part_next;       // byte del siguiente EBR, -1 si no hay
    char part_name[16];
};

/* ---------- Estructuras para el sistema de archivos EXT2 ---------- */

struct SuperBloque {
    int    s_filesystem_type;   // 2 = ext2
    int    s_inodes_count;
    int    s_blocks_count;
    int    s_free_blocks_count;
    int    s_free_inodes_count;
    time_t s_mtime;             // ultimo montaje
    time_t s_umtime;            // ultimo desmontaje
    int    s_mnt_count;
    int    s_magic;             // 0xEF53
    int    s_inode_s;           // sizeof - inodo
    int    s_block_s;           // sizeof bloq
    int    s_firts_ino;         // primer inodo libre
    int    s_first_blo;         // primer bloque libre
    int    s_bm_inode_start;    // inicio del bitmap de inodos
    int    s_bm_block_start;    // inicio del bitmap de bloques
    int    s_inode_start;       // inicio de la tabla de inodos
    int    s_block_start;       // inicio de la tabla de bloques
};

struct Inodo {
    int    i_uid;         // dueno
    int    i_gid;         // grupo del dueno
    int    i_s;           // tamano del archivo en bytes
    time_t i_atime;       // ultima lectura
    time_t i_ctime;       // creacion
    time_t i_mtime;       // ultima modificacion
    int    i_block[15];   // 0-11 directos, 12 simple, 13 doble, 14 triple
    char   i_type;        // '1' archivo, '0' carpeta
    char   i_perm[3];     // permisos UGO en octal, p.ej. {'6','6','4'}
};


struct Contenido {
    char b_name[12];
    int  b_inodo;         // -1 si la entrada esta vacia
};


struct BloqueCarpeta {
    Contenido b_content[4];
};

/* Bloque de archivo: 64 caracteres de contenido. */
struct BloqueArchivo {
    char b_content[64];
};


struct BloqueApuntadores {
    int b_pointers[16];
};

#pragma pack(pop)

const int TAM_BLOQUE = sizeof(BloqueCarpeta);

static_assert(sizeof(BloqueCarpeta)     == 64, "bloque carpeta debe medir 64");
static_assert(sizeof(BloqueArchivo)     == 64, "bloque archivo debe medir 64");
static_assert(sizeof(BloqueApuntadores) == 64, "bloque apuntadores debe medir 64");

#endif
