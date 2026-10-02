#include "Formateo.h"
#include "Montaje.h"

#include "../disco/Ext2.h"
#include "../estructuras/Estructuras.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstring>
#include <ctime>

namespace {

// contenido inicial de users.txt grupo root y usuario root
const std::string USERS_INICIAL =
    "1, G, root\n"
    "1, U, root, root, 123\n";

} // namespace

void cmdMkfs(const Parametros &p, Salida &salida) {
    std::string id = aMayusculas(sinComillas(p.obtener("-id")));

    // -type solo admite full sin el se asume full igual
    std::string tipo = aMinusculas(sinComillas(p.obtener("-type", "full")));
    if (tipo != "full") {
        salida.error("mkfs: -type solo admite full");
        return;
    }

    std::string fs = aMinusculas(sinComillas(p.obtener("-fs", "2fs")));
    if (fs != "2fs" && fs != "3fs") {
        salida.error("mkfs: -fs solo admite 2fs o 3fs");
        return;
    }

    bool ext3 = fs == "3fs";
    const Montaje *m = buscarMontaje(id);
    if (m == nullptr) {
        salida.error("mkfs: no hay ninguna particion montada con id " + id);
        return;
    }

    // despejar n de la ecuacion del enunciado
    // part_s = sizeof(SB) + n + 3n + n*sizeof(Inodo) + 3n*sizeof(bloque)
    // para Journal es igual pero primero se lleva su parte antes de repartir
    int denominador = 1 + 3 + static_cast<int>(sizeof(Inodo)) + 3 * TAM_BLOQUE;

    int disponible = m->tam - static_cast<int>(sizeof(SuperBloque));
    if (ext3) disponible -= JOURNAL_ENTRADAS * static_cast<int>(sizeof(Journal));

    int n = disponible / denominador;
    if (n < 2) {
        salida.error("mkfs: la particion es muy pequena para formatear");
        return;
    }

    int inicio = m->inicio;

    // armar el superbloque con las posiciones de cada area
    SuperBloque sb;
    std::memset(&sb, 0, sizeof(sb));
    sb.s_filesystem_type   = ext3 ? 3: 2;
    sb.s_inodes_count      = n;
    sb.s_blocks_count      = 3 * n;
    sb.s_free_inodes_count = n;
    sb.s_free_blocks_count = 3 * n;
    sb.s_mtime             = std::time(nullptr);
    sb.s_umtime            = 0;
    sb.s_mnt_count         = 1;
    sb.s_magic             = 0xEF53;
    sb.s_inode_s           = sizeof(Inodo);
    sb.s_block_s           = TAM_BLOQUE;
    int trasSuperBloque = inicio + static_cast<int>(sizeof(SuperBloque));
    int trasJournal = ext3
        ? trasSuperBloque + JOURNAL_ENTRADAS * static_cast<int>(sizeof(Journal))
        : trasSuperBloque;

    sb.s_bm_inode_start    = trasJournal;
    sb.s_bm_block_start    = sb.s_bm_inode_start + n;
    sb.s_inode_start       = sb.s_bm_block_start + 3 * n;
    sb.s_block_start       = sb.s_inode_start + n * static_cast<int>(sizeof(Inodo));
    sb.s_firts_ino         = sb.s_inode_start;
    sb.s_first_blo         = sb.s_block_start;

    // limpiar bitmaps todo libre
    for (int i = 0; i < n; ++i)      ponerBitInodo(m->ruta, sb, i, 0);
    for (int i = 0; i < 3 * n; ++i)  ponerBitBloque(m->ruta, sb, i, 0);

    if (ext3) {
        Journal vacio;
        std::memset(&vacio, 0, sizeof(Journal));
        for (int i = 0; i < JOURNAL_ENTRADAS; ++i)
            escribirEn(m->ruta, trasSuperBloque + i * static_cast<int>(sizeof(Journal)), vacio);
    }

    escribirSB(m->ruta, inicio, sb);

    time_t ahora = std::time(nullptr);

    // inodo 0 carpeta raiz
    int inodoRaiz = asignarInodo(m->ruta, sb);
    int bloqueRaiz = asignarBloque(m->ruta, sb);

    Inodo raiz;
    std::memset(&raiz, 0, sizeof(raiz));
    raiz.i_uid = 1;
    raiz.i_gid = 1;
    raiz.i_s   = 0;
    raiz.i_atime = raiz.i_ctime = raiz.i_mtime = ahora;
    for (int &b : raiz.i_block) b = -1;
    raiz.i_block[0] = bloqueRaiz;
    raiz.i_type = '0';
    raiz.i_perm[0] = '6'; raiz.i_perm[1] = '6'; raiz.i_perm[2] = '4';

    // inodo 1 archivo users.txt
    int inodoUsers = asignarInodo(m->ruta, sb);

    Inodo users;
    std::memset(&users, 0, sizeof(users));
    users.i_uid = 1;
    users.i_gid = 1;
    users.i_atime = users.i_ctime = users.i_mtime = ahora;
    for (int &b : users.i_block) b = -1;
    users.i_type = '1';
    users.i_perm[0] = '6'; users.i_perm[1] = '6'; users.i_perm[2] = '4';

    // bloque de la raiz punto puntopunto y users.txt
    BloqueCarpeta carpeta;
    std::memset(&carpeta, 0, sizeof(carpeta));
    for (Contenido &c : carpeta.b_content) c.b_inodo = -1;
    copiarCampo(carpeta.b_content[0].b_name, 12, ".");
    carpeta.b_content[0].b_inodo = inodoRaiz;
    copiarCampo(carpeta.b_content[1].b_name, 12, "..");
    carpeta.b_content[1].b_inodo = inodoRaiz;
    copiarCampo(carpeta.b_content[2].b_name, 12, "users.txt");
    carpeta.b_content[2].b_inodo = inodoUsers;

    escribirEn(m->ruta, posBloque(sb, bloqueRaiz), carpeta);
    escribirInodo(m->ruta, sb, inodoRaiz, raiz);

    // escribir el contenido de users.txt asigna su bloque de archivo
    escribirArchivo(m->ruta, sb, inodoUsers, users, USERS_INICIAL);

    escribirSB(m->ruta, inicio, sb);

    salida.exito("mkfs: particion " + id + " formateada como " +
                 (ext3 ? "ext3" : "ext2") + " (" +
                 std::to_string(n) + " inodos, " + std::to_string(3 * n) +
                 " bloques)");
}
