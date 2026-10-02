#include "Operaciones.h"
#include "Montaje.h"
#include "Permisos.h"
#include "Sesion.h"
#include "Usuarios.h"

#include "../disco/Ext2.h"
#include "../disco/Journal.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstring>
#include <ctime>
#include <sstream>
#include <vector>

namespace {

// parte una ruta interna en sus nombres sin las barras
std::vector<std::string> partesDe(const std::string &ruta) {
    std::vector<std::string> partes;
    std::stringstream flujo(ruta);
    std::string p;
    while (std::getline(flujo, p, '/'))
        if (!p.empty()) partes.push_back(p);
    return partes;
}

// quita el ultimo nombre de una ruta y devuelve lo que queda
std::string carpetaDe(const std::string &ruta) {
    size_t barra = ruta.find_last_of('/');
    if (barra == std::string::npos || barra == 0) return "/";
    return ruta.substr(0, barra);
}

std::string nombreDe(const std::string &ruta) {
    size_t barra = ruta.find_last_of('/');
    return barra == std::string::npos ? ruta : ruta.substr(barra + 1);
}

// prepara lo que todos los comandos necesitan: sesion, montaje y superbloque
bool abrir(const std::string &comando, Salida &salida,
           const Sesion *&s, const Montaje *&m, SuperBloque &sb) {
    s = &sesionActual();
    if (!s->activa) {
        salida.error(comando + ": necesita iniciar sesion");
        return false;
    }
    m = buscarMontaje(s->id);
    if (m == nullptr) {
        salida.error(comando + ": la particion de la sesion ya no esta montada");
        return false;
    }
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error(comando + ": no se pudo leer la particion");
        return false;
    }
    return true;
}

// revisa que se tenga permiso de escritura sobre todo el subarbol
// el enunciado pide no borrar nada si algo adentro no se puede borrar
bool sePuedeBorrarTodo(const std::string &ruta, const SuperBloque &sb,
                       int indice, const Sesion &s) {
    Inodo nodo;
    if (!leerInodo(ruta, sb, indice, nodo)) return false;
    if (!puedeEscribir(s, nodo)) return false;

    if (nodo.i_type != '0') return true;

    for (int d = 0; d < 12; ++d) {
        if (nodo.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, nodo.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string n = aTexto(c.b_name, 12);
            if (c.b_inodo == -1 || n == "." || n == "..") continue;
            if (!sePuedeBorrarTodo(ruta, sb, c.b_inodo, s)) return false;
        }
    }
    return true;
}

// crea una carpeta vacia dentro de padre y devuelve su inodo
int nuevaCarpeta(const std::string &ruta, SuperBloque &sb, int indicePadre,
                 Inodo &padre, const std::string &nombre, const Sesion &s) {
    int nuevo  = asignarInodo(ruta, sb);
    int bloque = asignarBloque(ruta, sb);
    if (nuevo == -1 || bloque == -1) return -1;

    time_t ahora = std::time(nullptr);

    Inodo carpeta;
    std::memset(&carpeta, 0, sizeof(carpeta));
    carpeta.i_uid = s.uid;
    carpeta.i_gid = s.gid;
    carpeta.i_atime = carpeta.i_ctime = carpeta.i_mtime = ahora;
    for (int &b : carpeta.i_block) b = -1;
    carpeta.i_block[0] = bloque;
    carpeta.i_type = '0';
    carpeta.i_perm[0] = '6'; carpeta.i_perm[1] = '6'; carpeta.i_perm[2] = '4';

    BloqueCarpeta contenido;
    std::memset(&contenido, 0, sizeof(contenido));
    for (Contenido &c : contenido.b_content) c.b_inodo = -1;
    copiarCampo(contenido.b_content[0].b_name, 12, ".");
    contenido.b_content[0].b_inodo = nuevo;
    copiarCampo(contenido.b_content[1].b_name, 12, "..");
    contenido.b_content[1].b_inodo = indicePadre;

    escribirEn(ruta, posBloque(sb, bloque), contenido);
    escribirInodo(ruta, sb, nuevo, carpeta);

    if (!agregarEntrada(ruta, sb, indicePadre, padre, nombre, nuevo)) return -1;
    return nuevo;
}

// copia recursiva: solo se lleva lo que el usuario puede leer
void copiarA(const std::string &ruta, SuperBloque &sb, int origen,
             int indiceDestino, Inodo &destino, const std::string &nombre,
             const Sesion &s, int &copiados, int &saltados) {
    Inodo nodo;
    if (!leerInodo(ruta, sb, origen, nodo)) return;

    if (!puedeLeer(s, nodo)) { ++saltados; return; }

    if (nodo.i_type == '1') {
        // archivo: inodo nuevo con el mismo contenido
        std::string texto = leerArchivo(ruta, sb, nodo);

        int nuevo = asignarInodo(ruta, sb);
        if (nuevo == -1) return;

        Inodo copia;
        std::memset(&copia, 0, sizeof(copia));
        copia.i_uid = s.uid;
        copia.i_gid = s.gid;
        copia.i_atime = copia.i_ctime = copia.i_mtime = std::time(nullptr);
        for (int &b : copia.i_block) b = -1;
        copia.i_type = '1';
        std::memcpy(copia.i_perm, nodo.i_perm, 3);

        escribirArchivo(ruta, sb, nuevo, copia, texto);
        agregarEntrada(ruta, sb, indiceDestino, destino, nombre, nuevo);
        ++copiados;
        return;
    }

    // carpeta: se crea el espejo y se baja a sus hijos
    int nuevo = nuevaCarpeta(ruta, sb, indiceDestino, destino, nombre, s);
    if (nuevo == -1) return;
    ++copiados;

    Inodo creada;
    leerInodo(ruta, sb, nuevo, creada);

    for (int d = 0; d < 12; ++d) {
        if (nodo.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, nodo.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string n = aTexto(c.b_name, 12);
            if (c.b_inodo == -1 || n == "." || n == "..") continue;
            copiarA(ruta, sb, c.b_inodo, nuevo, creada, n, s, copiados, saltados);
            leerInodo(ruta, sb, nuevo, creada);   // pudo crecer de bloques
        }
    }
}

// compara un nombre contra un patron con ? y *
bool coincide(const std::string &patron, const std::string &texto) {
    size_t p = 0, t = 0, estrella = std::string::npos, marca = 0;

    while (t < texto.size()) {
        if (p < patron.size() && (patron[p] == '?' || patron[p] == texto[t])) {
            ++p; ++t;
        } else if (p < patron.size() && patron[p] == '*') {
            estrella = p++;        // recordar donde estaba la estrella
            marca = t;
        } else if (estrella != std::string::npos) {
            p = estrella + 1;      // retroceder y que la estrella coma uno mas
            t = ++marca;
        } else {
            return false;
        }
    }
    while (p < patron.size() && patron[p] == '*') ++p;
    return p == patron.size();
}

// recorre el subarbol mostrando lo que coincide con el patron
void buscarEn(const std::string &ruta, const SuperBloque &sb, int indice,
              const std::string &patron, const Sesion &s,
              const std::string &sangria, Salida &salida, int &hallados) {
    Inodo nodo;
    if (!leerInodo(ruta, sb, indice, nodo)) return;
    if (nodo.i_type != '0' || !puedeLeer(s, nodo)) return;

    for (int d = 0; d < 12; ++d) {
        if (nodo.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, nodo.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string n = aTexto(c.b_name, 12);
            if (c.b_inodo == -1 || n == "." || n == "..") continue;

            Inodo hijo;
            if (!leerInodo(ruta, sb, c.b_inodo, hijo)) continue;

            bool esCarpeta = hijo.i_type == '0';
            if (coincide(patron, n)) {
                salida.escribir(sangria + "|_ " + n + (esCarpeta ? "/" : ""));
                ++hallados;
            }
            if (esCarpeta)
                buscarEn(ruta, sb, c.b_inodo, patron, s, sangria + "   ",
                         salida, hallados);
        }
    }
}

// cambia el dueno de un inodo y opcionalmente de todo lo que cuelga
void cambiarDueno(const std::string &ruta, SuperBloque &sb, int indice,
                  int nuevoUid, bool recursivo, const Sesion &s, int &cambiados) {
    Inodo nodo;
    if (!leerInodo(ruta, sb, indice, nodo)) return;

    // root puede con todo, los demas solo con lo suyo
    if (s.usuario == "root" || nodo.i_uid == s.uid) {
        nodo.i_uid = nuevoUid;
        nodo.i_mtime = std::time(nullptr);
        escribirInodo(ruta, sb, indice, nodo);
        ++cambiados;
    }

    if (!recursivo || nodo.i_type != '0') return;

    for (int d = 0; d < 12; ++d) {
        if (nodo.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(ruta, posBloque(sb, nodo.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string n = aTexto(c.b_name, 12);
            if (c.b_inodo == -1 || n == "." || n == "..") continue;
            cambiarDueno(ruta, sb, c.b_inodo, nuevoUid, true, s, cambiados);
        }
    }
}

// busca el uid de un usuario dentro de users.txt
int uidDe(const Montaje *m, const std::string &usuario) {
    SuperBloque sb; int inodo; Inodo nodo; std::string contenido;
    if (!leerUsersTxt(m, sb, inodo, nodo, contenido)) return -1;

    std::stringstream flujo(contenido);
    std::string linea;
    while (std::getline(flujo, linea)) {
        std::vector<std::string> c;
        std::stringstream campos(linea);
        std::string campo;
        while (std::getline(campos, campo, ',')) {
            size_t a = campo.find_first_not_of(" \t");
            size_t b = campo.find_last_not_of(" \t");
            c.push_back(a == std::string::npos ? "" : campo.substr(a, b - a + 1));
        }
        if (c.size() < 5 || c[1] != "U" || c[0] == "0") continue;
        if (c[3] == usuario) return std::atoi(c[0].c_str());
    }
    return -1;
}

} // namespace

// ---------------- REMOVE ----------------
void cmdRemove(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("remove", salida, s, m, sb)) return;

    std::string destino = sinComillas(p.obtener("-path"));
    int indice = buscarInodoPorRuta(m->ruta, sb, destino);
    if (indice == -1) {
        salida.error("remove: no existe " + destino);
        return;
    }
    if (indice == 0) {
        salida.error("remove: no se puede eliminar la raiz");
        return;
    }

    // primero se revisa todo el subarbol, despues se borra
    if (!sePuedeBorrarTodo(m->ruta, sb, indice, *s)) {
        salida.error("remove: sin permiso de escritura sobre " + destino +
                     " o sobre algo que contiene, no se elimino nada");
        return;
    }

    int indicePadre = buscarInodoPorRuta(m->ruta, sb, carpetaDe(destino));
    Inodo padre;
    if (indicePadre == -1 || !leerInodo(m->ruta, sb, indicePadre, padre)) {
        salida.error("remove: no se pudo leer la carpeta padre de " + destino);
        return;
    }
    if (!puedeEscribir(*s, padre)) {
        salida.error("remove: sin permiso de escritura en la carpeta padre");
        return;
    }

    quitarEntrada(m->ruta, sb, padre, nombreDe(destino));
    liberarInodo(m->ruta, sb, indice);
    escribirSB(m->ruta, m->inicio, sb);

    anotarJournal(m, "remove", destino, "");
    salida.exito("remove: " + destino + " eliminado");
}

// ---------------- RENAME ----------------
void cmdRename(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("rename", salida, s, m, sb)) return;

    std::string destino = sinComillas(p.obtener("-path"));
    std::string nuevo   = sinComillas(p.obtener("-name"));

    if (nuevo.empty() || nuevo.size() > 12) {
        salida.error("rename: -name debe tener entre 1 y 12 caracteres");
        return;
    }

    int indice = buscarInodoPorRuta(m->ruta, sb, destino);
    if (indice == -1) {
        salida.error("rename: no existe " + destino);
        return;
    }

    Inodo nodo;
    leerInodo(m->ruta, sb, indice, nodo);
    if (!puedeEscribir(*s, nodo)) {
        salida.error("rename: sin permiso de escritura sobre " + destino);
        return;
    }

    int indicePadre = buscarInodoPorRuta(m->ruta, sb, carpetaDe(destino));
    Inodo padre;
    if (indicePadre == -1 || !leerInodo(m->ruta, sb, indicePadre, padre)) {
        salida.error("rename: no se pudo leer la carpeta padre");
        return;
    }
    if (buscarEntrada(m->ruta, sb, padre, nuevo) != -1) {
        salida.error("rename: ya existe algo llamado " + nuevo + " en esa carpeta");
        return;
    }

    renombrarEntrada(m->ruta, sb, padre, nombreDe(destino), nuevo);

    anotarJournal(m, "rename", destino, nuevo);
    salida.exito("rename: " + destino + " ahora se llama " + nuevo);
}

// ---------------- COPY ----------------
void cmdCopy(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("copy", salida, s, m, sb)) return;

    std::string origen  = sinComillas(p.obtener("-path"));
    std::string destino = sinComillas(p.obtener("-destino"));

    int iOrigen = buscarInodoPorRuta(m->ruta, sb, origen);
    if (iOrigen == -1) {
        salida.error("copy: no existe " + origen);
        return;
    }

    int iDestino = buscarInodoPorRuta(m->ruta, sb, destino);
    if (iDestino == -1) {
        salida.error("copy: no existe la carpeta destino " + destino);
        return;
    }

    Inodo carpetaDestino;
    leerInodo(m->ruta, sb, iDestino, carpetaDestino);
    if (carpetaDestino.i_type != '0') {
        salida.error("copy: " + destino + " no es una carpeta");
        return;
    }
    if (!puedeEscribir(*s, carpetaDestino)) {
        salida.error("copy: sin permiso de escritura en " + destino);
        return;
    }

    std::string nombre = nombreDe(origen);
    if (buscarEntrada(m->ruta, sb, carpetaDestino, nombre) != -1) {
        salida.error("copy: ya existe " + nombre + " en " + destino);
        return;
    }

    int copiados = 0, saltados = 0;
    copiarA(m->ruta, sb, iOrigen, iDestino, carpetaDestino, nombre,
            *s, copiados, saltados);
    escribirSB(m->ruta, m->inicio, sb);

    anotarJournal(m, "copy", origen, destino);
    salida.exito("copy: " + std::to_string(copiados) + " elementos copiados a " +
                 destino + (saltados > 0
                     ? " (" + std::to_string(saltados) + " sin permiso de lectura)"
                     : ""));
}

// ---------------- MOVE ----------------
void cmdMove(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("move", salida, s, m, sb)) return;

    std::string origen  = sinComillas(p.obtener("-path"));
    std::string destino = sinComillas(p.obtener("-destino"));

    int iOrigen = buscarInodoPorRuta(m->ruta, sb, origen);
    if (iOrigen == -1) {
        salida.error("move: no existe " + origen);
        return;
    }
    if (iOrigen == 0) {
        salida.error("move: no se puede mover la raiz");
        return;
    }

    Inodo nodo;
    leerInodo(m->ruta, sb, iOrigen, nodo);
    if (!puedeEscribir(*s, nodo)) {
        salida.error("move: sin permiso de escritura sobre " + origen);
        return;
    }

    int iDestino = buscarInodoPorRuta(m->ruta, sb, destino);
    if (iDestino == -1) {
        salida.error("move: no existe la carpeta destino " + destino);
        return;
    }

    Inodo carpetaDestino;
    leerInodo(m->ruta, sb, iDestino, carpetaDestino);
    if (carpetaDestino.i_type != '0') {
        salida.error("move: " + destino + " no es una carpeta");
        return;
    }
    if (!puedeEscribir(*s, carpetaDestino)) {
        salida.error("move: sin permiso de escritura en " + destino);
        return;
    }

    std::string nombre = nombreDe(origen);
    if (buscarEntrada(m->ruta, sb, carpetaDestino, nombre) != -1) {
        salida.error("move: ya existe " + nombre + " en " + destino);
        return;
    }

    int iPadre = buscarInodoPorRuta(m->ruta, sb, carpetaDe(origen));
    Inodo padre;
    if (iPadre == -1 || !leerInodo(m->ruta, sb, iPadre, padre)) {
        salida.error("move: no se pudo leer la carpeta padre de " + origen);
        return;
    }

    // dentro de la misma particion mover es solo cambiar referencias:
    // se quita del padre viejo y se agrega al nuevo, los datos no se tocan
    quitarEntrada(m->ruta, sb, padre, nombre);
    agregarEntrada(m->ruta, sb, iDestino, carpetaDestino, nombre, iOrigen);

    // si era carpeta hay que corregir su entrada ".." al nuevo padre
    if (nodo.i_type == '0' && nodo.i_block[0] != -1) {
        BloqueCarpeta bloque;
        leerDe(m->ruta, posBloque(sb, nodo.i_block[0]), bloque);
        for (Contenido &c : bloque.b_content) {
            if (aTexto(c.b_name, 12) != "..") continue;
            c.b_inodo = iDestino;
            break;
        }
        escribirEn(m->ruta, posBloque(sb, nodo.i_block[0]), bloque);
    }

    escribirSB(m->ruta, m->inicio, sb);

    anotarJournal(m, "move", origen, destino);
    salida.exito("move: " + origen + " movido a " + destino);
}

// ---------------- FIND ----------------
void cmdFind(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("find", salida, s, m, sb)) return;

    std::string inicio = sinComillas(p.obtener("-path"));
    std::string patron = sinComillas(p.obtener("-name"));

    int indice = buscarInodoPorRuta(m->ruta, sb, inicio);
    if (indice == -1) {
        salida.error("find: no existe la carpeta " + inicio);
        return;
    }

    Inodo nodo;
    leerInodo(m->ruta, sb, indice, nodo);
    if (nodo.i_type != '0') {
        salida.error("find: " + inicio + " no es una carpeta");
        return;
    }

    salida.escribir(inicio);
    int hallados = 0;
    buscarEn(m->ruta, sb, indice, patron, *s, "", salida, hallados);

    salida.escribir("find: " + std::to_string(hallados) +
                    " coincidencias con el patron " + patron);
}

// ---------------- CHOWN ----------------
void cmdChown(const Parametros &p, Salida &salida) {
    const Sesion *s; const Montaje *m; SuperBloque sb;
    if (!abrir("chown", salida, s, m, sb)) return;

    std::string destino = sinComillas(p.obtener("-path"));
    std::string usuario = sinComillas(p.obtener("-usuario"));

    int indice = buscarInodoPorRuta(m->ruta, sb, destino);
    if (indice == -1) {
        salida.error("chown: no existe " + destino);
        return;
    }

    int nuevoUid = uidDe(m, usuario);
    if (nuevoUid == -1) {
        salida.error("chown: no existe el usuario " + usuario);
        return;
    }

    int cambiados = 0;
    cambiarDueno(m->ruta, sb, indice, nuevoUid, p.tiene("-r"), *s, cambiados);

    if (cambiados == 0) {
        salida.error("chown: no tiene permiso para cambiar el dueno de " + destino);
        return;
    }

    anotarJournal(m, "chown", destino, usuario);
    salida.exito("chown: " + std::to_string(cambiados) +
                 " elementos ahora pertenecen a " + usuario);
}
