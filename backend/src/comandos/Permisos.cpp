#include "Permisos.h"

namespace {

// el digito octal que le toca al usuario segun sea dueno grupo u otro
char digitoDe(const Sesion &s, const Inodo &nodo) {
    if (nodo.i_uid == s.uid) return nodo.i_perm[0];   // dueno
    if (nodo.i_gid == s.gid) return nodo.i_perm[1];   // grupo
    return nodo.i_perm[2];                            // otros
}

} // namespace

bool puedeLeer(const Sesion &s, const Inodo &nodo) {
    if (s.usuario == "root") return true;          // root todo lo puede
    return ((digitoDe(s, nodo) - '0') & 4) != 0;   // bit de lectura
}

bool puedeEscribir(const Sesion &s, const Inodo &nodo) {
    if (s.usuario == "root") return true;
    return ((digitoDe(s, nodo) - '0') & 2) != 0;   // bit de escritura
}
