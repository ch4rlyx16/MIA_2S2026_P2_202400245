#include "Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstring>
#include <sstream>

int inicioParticion(const SuperBloque &sb) {
    return sb.s_bm_inode_start - static_cast<int>(sizeof(SuperBloque));
}

long posInodo(const SuperBloque &sb, int i) {
    return sb.s_inode_start + i * static_cast<long>(sizeof(Inodo));
}

long posBloque(const SuperBloque &sb, int i) {
    return sb.s_block_start + i * static_cast<long>(TAM_BLOQUE);
}

bool leerSB(const std::string &ruta, int inicio, SuperBloque &sb) {
    return leerDe(ruta, inicio, sb);
}

bool escribirSB(const std::string &ruta, int inicio, const SuperBloque &sb) {
    return escribirEn(ruta, inicio, sb);
}

bool leerInodo(const std::string &ruta, const SuperBloque &sb, int i, Inodo &nodo) {
    return leerDe(ruta, posInodo(sb, i), nodo);
}

bool escribirInodo(const std::string &ruta, const SuperBloque &sb, int i, const Inodo &nodo) {
    return escribirEn(ruta, posInodo(sb, i), nodo);
}

// bitmaps

bool bitInodo(const std::string &ruta, const SuperBloque &sb, int i) {
    char b = 0;
    leerDe(ruta, sb.s_bm_inode_start + i, b);
    return b == 1;
}

bool bitBloque(const std::string &ruta, const SuperBloque &sb, int i) {
    char b = 0;
    leerDe(ruta, sb.s_bm_block_start + i, b);
    return b == 1;
}

void ponerBitInodo(const std::string &ruta, const SuperBloque &sb, int i, char v) {
    escribirEn(ruta, sb.s_bm_inode_start + i, v);
}

void ponerBitBloque(const std::string &ruta, const SuperBloque &sb, int i, char v) {
    escribirEn(ruta, sb.s_bm_block_start + i, v);
}

int asignarInodo(const std::string &ruta, SuperBloque &sb) {
    for (int i = 0; i < sb.s_inodes_count; ++i) {
        if (!bitInodo(ruta, sb, i)) {
            ponerBitInodo(ruta, sb, i, 1);
            sb.s_free_inodes_count--;
            sb.s_firts_ino = static_cast<int>(posInodo(sb, i)) + static_cast<int>(sizeof(Inodo));
            return i;
        }
    }
    return -1;
}

int asignarBloque(const std::string &ruta, SuperBloque &sb) {
    for (int i = 0; i < sb.s_blocks_count; ++i) {
        if (!bitBloque(ruta, sb, i)) {
            ponerBitBloque(ruta, sb, i, 1);
            sb.s_free_blocks_count--;
            sb.s_first_blo = static_cast<int>(posBloque(sb, i)) + TAM_BLOQUE;
            return i;
        }
    }
    return -1;
}

// bloques indirectos

namespace {

const int PUNTEROS = 16;   // cuantos apuntadores caben en un bloque

// reserva un bloque de apuntadores con todas sus posiciones en -1
int nuevoBloquePunteros(const std::string &ruta, SuperBloque &sb) {
    int indice = asignarBloque(ruta, sb);
    if (indice == -1) return -1;

    BloqueApuntadores bp;
    for (int &p : bp.b_pointers) p = -1;
    escribirEn(ruta, posBloque(sb, indice), bp);
    return indice;
}

// lee un apuntador y si esta vacio reserva el bloque al que apuntara
// comoPunteros indica si lo nuevo es otro bloque de apuntadores
int puntero(const std::string &ruta, SuperBloque &sb, int bloque, int indice,
            bool asignar, bool comoPunteros) {
    BloqueApuntadores bp;
    leerDe(ruta, posBloque(sb, bloque), bp);

    if (bp.b_pointers[indice] == -1) {
        if (!asignar) return -1;

        int nuevo = comoPunteros ? nuevoBloquePunteros(ruta, sb)
                                 : asignarBloque(ruta, sb);
        if (nuevo == -1) return -1;

        bp.b_pointers[indice] = nuevo;
        escribirEn(ruta, posBloque(sb, bloque), bp);
    }
    return bp.b_pointers[indice];
}

// reserva el bloque de apuntadores de i_block[12] 13 o 14 si falta
bool prepararIndirecto(const std::string &ruta, SuperBloque &sb, Inodo &nodo,
                       int posicion, bool asignar) {
    if (nodo.i_block[posicion] != -1) return true;
    if (!asignar) return false;

    nodo.i_block[posicion] = nuevoBloquePunteros(ruta, sb);
    return nodo.i_block[posicion] != -1;
}

} // namespace

int capacidadBloques() {
    return 12 + PUNTEROS + PUNTEROS * PUNTEROS
              + PUNTEROS * PUNTEROS * PUNTEROS;
}

int bloqueLogico(const std::string &ruta, SuperBloque &sb, Inodo &nodo,
                 int n, bool asignar) {
    // directos
    if (n < 12) {
        if (nodo.i_block[n] == -1) {
            if (!asignar) return -1;
            nodo.i_block[n] = asignarBloque(ruta, sb);
        }
        return nodo.i_block[n];
    }
    n -= 12;

    // simple indirecto inodo -> apuntadores -> datos
    if (n < PUNTEROS) {
        if (!prepararIndirecto(ruta, sb, nodo, 12, asignar)) return -1;
        return puntero(ruta, sb, nodo.i_block[12], n, asignar, false);
    }
    n -= PUNTEROS;

    // doble indirecto inodo -> apuntadores -> apuntadores -> datos
    if (n < PUNTEROS * PUNTEROS) {
        if (!prepararIndirecto(ruta, sb, nodo, 13, asignar)) return -1;

        int nivel1 = puntero(ruta, sb, nodo.i_block[13], n / PUNTEROS, asignar, true);
        if (nivel1 == -1) return -1;
        return puntero(ruta, sb, nivel1, n % PUNTEROS, asignar, false);
    }
    n -= PUNTEROS * PUNTEROS;

    // triple indirecto un nivel mas de apuntadores
    if (n < PUNTEROS * PUNTEROS * PUNTEROS) {
        if (!prepararIndirecto(ruta, sb, nodo, 14, asignar)) return -1;

        int nivel1 = puntero(ruta, sb, nodo.i_block[14],
                             n / (PUNTEROS * PUNTEROS), asignar, true);
        if (nivel1 == -1) return -1;

        int nivel2 = puntero(ruta, sb, nivel1,
                             (n / PUNTEROS) % PUNTEROS, asignar, true);
        if (nivel2 == -1) return -1;

        return puntero(ruta, sb, nivel2, n % PUNTEROS, asignar, false);
    }
    return -1;   // mas grande de lo que el inodo puede direccionar
}

// contenido de archivos

std::string leerArchivo(const std::string &ruta, SuperBloque &sb, Inodo &nodo) {
    std::string contenido;
    int faltan = nodo.i_s;

    for (int n = 0; faltan > 0; ++n) {
        int fisico = bloqueLogico(ruta, sb, nodo, n, false);
        if (fisico == -1) break;

        BloqueArchivo bloque;
        leerDe(ruta, posBloque(sb, fisico), bloque);

        int trozo = faltan < 64 ? faltan : 64;
        contenido.append(bloque.b_content, trozo);
        faltan -= trozo;
    }
    return contenido;
}

bool escribirArchivo(const std::string &ruta, SuperBloque &sb,
                     int indiceInodo, Inodo &nodo, const std::string &contenido) {
    int total = static_cast<int>(contenido.size());
    int necesarios = (total + 63) / 64;
    if (necesarios > capacidadBloques()) return false;

    // reusar los bloques que ya tenia y asignar los que falten
    for (int n = 0; n < necesarios; ++n) {
        int fisico = bloqueLogico(ruta, sb, nodo, n, true);
        if (fisico == -1) return false;

        BloqueArchivo bloque;
        std::memset(&bloque, 0, sizeof(bloque));
        int desde = n * 64;
        int trozo = total - desde < 64 ? total - desde : 64;
        std::memcpy(bloque.b_content, contenido.data() + desde, trozo);
        escribirEn(ruta, posBloque(sb, fisico), bloque);
    }

    nodo.i_s = total;
    escribirInodo(ruta, sb, indiceInodo, nodo);
    escribirSB(ruta, inicioParticion(sb), sb);
    return true;
}

// navegacion de carpetas

int buscarEntrada(const std::string &ruta, const SuperBloque &sb,
                  const Inodo &carpeta, const std::string &nombre) {
    for (int d = 0; d < 12; ++d) {
        if (carpeta.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, carpeta.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            if (c.b_inodo == -1) continue;
            if (aTexto(c.b_name, 12) == nombre) return c.b_inodo;
        }
    }
    return -1;
}

int buscarInodoPorRuta(const std::string &ruta, const SuperBloque &sb,
                       const std::string &rutaInterna) {
    int actual = 0;   // raiz
    std::stringstream flujo(rutaInterna);
    std::string parte;

    while (std::getline(flujo, parte, '/')) {
        if (parte.empty()) continue;

        Inodo nodo;
        if (!leerInodo(ruta, sb, actual, nodo)) return -1;

        int siguiente = buscarEntrada(ruta, sb, nodo, parte);
        if (siguiente == -1) return -1;
        actual = siguiente;
    }
    return actual;
}

bool agregarEntrada(const std::string &ruta, SuperBloque &sb, int indiceCarpeta,
                    Inodo &carpeta, const std::string &nombre, int inodoDestino) {
    // buscar un hueco en los bloques que la carpeta ya tiene
    for (int d = 0; d < 12; ++d) {
        if (carpeta.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, carpeta.i_block[d]), bloque);

        for (Contenido &c : bloque.b_content) {
            if (c.b_inodo != -1) continue;

            copiarCampo(c.b_name, 12, nombre);
            c.b_inodo = inodoDestino;
            escribirEn(ruta, posBloque(sb, carpeta.i_block[d]), bloque);
            return true;
        }
    }

    // no habia hueco asi que se estrena un bloque de carpeta
    for (int d = 0; d < 12; ++d) {
        if (carpeta.i_block[d] != -1) continue;

        int nuevo = asignarBloque(ruta, sb);
        if (nuevo == -1) return false;

        BloqueCarpeta bloque;
        std::memset(&bloque, 0, sizeof(bloque));
        for (Contenido &c : bloque.b_content) c.b_inodo = -1;
        copiarCampo(bloque.b_content[0].b_name, 12, nombre);
        bloque.b_content[0].b_inodo = inodoDestino;

        carpeta.i_block[d] = nuevo;
        escribirEn(ruta, posBloque(sb, nuevo), bloque);
        escribirInodo(ruta, sb, indiceCarpeta, carpeta);
        return true;
    }
    return false;   // carpeta llena
}
