#include "Archivos.h"
#include "Montaje.h"
#include "Permisos.h"
#include "Sesion.h"

#include "../disco/Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstring>
#include <ctime>
#include <fstream>
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

// crea una carpeta dentro de padre y devuelve su inodo -1 si fallo
int crearCarpeta(const std::string &ruta, SuperBloque &sb, int indicePadre,
                 Inodo &padre, const std::string &nombre, const Sesion &s) {
    int nuevo  = asignarInodo(ruta, sb);
    int bloque = asignarBloque(ruta, sb);
    if (nuevo == -1 || bloque == -1) return -1;

    time_t ahora = std::time(nullptr);

    Inodo carpeta;
    std::memset(&carpeta, 0, sizeof(carpeta));
    carpeta.i_uid = s.uid;
    carpeta.i_gid = s.gid;
    carpeta.i_s   = 0;
    carpeta.i_atime = carpeta.i_ctime = carpeta.i_mtime = ahora;
    for (int &b : carpeta.i_block) b = -1;
    carpeta.i_block[0] = bloque;
    carpeta.i_type = '0';
    carpeta.i_perm[0] = '6'; carpeta.i_perm[1] = '6'; carpeta.i_perm[2] = '4';

    // toda carpeta empieza con punto y puntopunto apuntando a ella y a su padre
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

    escribirSB(ruta, inicioParticion(sb), sb);
    return nuevo;
}

// baja por las carpetas de la ruta y devuelve el inodo del padre
// con crear en true las va creando como hacen -r y -p
int recorrerPadres(const std::string &ruta, SuperBloque &sb,
                   const std::vector<std::string> &partes, bool crear,
                   const Sesion &s, std::string &fallo) {
    int actual = 0;   // raiz

    for (size_t i = 0; i + 1 < partes.size(); ++i) {
        Inodo nodo;
        if (!leerInodo(ruta, sb, actual, nodo)) { fallo = "no se pudo leer la carpeta"; return -1; }

        int siguiente = buscarEntrada(ruta, sb, nodo, partes[i]);

        if (siguiente == -1) {
            if (!crear) {
                fallo = "no existe la carpeta " + partes[i];
                return -1;
            }
            if (!puedeEscribir(s, nodo)) {
                fallo = "sin permiso de escritura en " + partes[i];
                return -1;
            }
            siguiente = crearCarpeta(ruta, sb, actual, nodo, partes[i], s);
            if (siguiente == -1) { fallo = "no se pudo crear " + partes[i]; return -1; }
        }
        actual = siguiente;
    }
    return actual;
}

} // namespace

// ---------------- MKFILE ----------------
void cmdMkfile(const Parametros &p, Salida &salida) {
    const Sesion &s = sesionActual();
    if (!s.activa) {
        salida.error("mkfile: necesita iniciar sesion");
        return;
    }

    const Montaje *m = buscarMontaje(s.id);
    if (m == nullptr) {
        salida.error("mkfile: la particion de la sesion ya no esta montada");
        return;
    }

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("mkfile: no se pudo leer la particion");
        return;
    }

    std::string destino = sinComillas(p.obtener("-path"));
    std::vector<std::string> partes = partesDe(destino);
    if (partes.empty()) {
        salida.error("mkfile: -path no puede ser la raiz");
        return;
    }

    // el contenido de -cont manda sobre el de -size
    std::string contenido;
    if (p.tiene("-cont")) {
        std::string origen = sinComillas(p.obtener("-cont"));
        std::ifstream archivo(origen, std::ios::binary);
        if (!archivo) {
            salida.error("mkfile: no existe el archivo " + origen +
                         " en el disco de la computadora");
            return;
        }
        std::stringstream buffer;
        buffer << archivo.rdbuf();
        contenido = buffer.str();
    } else if (p.tiene("-size")) {
        int tamano = std::atoi(p.obtener("-size").c_str());
        // numeros del 0 al 9 repetidos hasta cumplir el tamano
        for (int i = 0; i < tamano; ++i)
            contenido += static_cast<char>('0' + (i % 10));
    }

    std::string fallo;
    int indicePadre = recorrerPadres(m->ruta, sb, partes, p.tiene("-r"), s, fallo);
    if (indicePadre == -1) {
        salida.error("mkfile: " + fallo +
                     (p.tiene("-r") ? "" : " use -r para crear las carpetas padre"));
        return;
    }

    Inodo padre;
    leerInodo(m->ruta, sb, indicePadre, padre);
    if (!puedeEscribir(s, padre)) {
        salida.error("mkfile: sin permiso de escritura en la carpeta padre");
        return;
    }

    const std::string &nombre = partes.back();
    if (nombre.size() > 12) {
        salida.error("mkfile: el nombre " + nombre + " pasa de 12 caracteres");
        return;
    }

    time_t ahora = std::time(nullptr);
    int indiceArchivo = buscarEntrada(m->ruta, sb, padre, nombre);
    bool sobrescribe = indiceArchivo != -1;

    Inodo archivo;
    if (sobrescribe) {
        leerInodo(m->ruta, sb, indiceArchivo, archivo);
        if (archivo.i_type != '1') {
            salida.error("mkfile: " + nombre + " ya existe y es una carpeta");
            return;
        }
        if (!puedeEscribir(s, archivo)) {
            salida.error("mkfile: sin permiso de escritura sobre " + nombre);
            return;
        }
    } else {
        indiceArchivo = asignarInodo(m->ruta, sb);
        if (indiceArchivo == -1) {
            salida.error("mkfile: ya no hay inodos libres");
            return;
        }
        std::memset(&archivo, 0, sizeof(archivo));
        archivo.i_uid = s.uid;
        archivo.i_gid = s.gid;
        archivo.i_ctime = ahora;
        for (int &b : archivo.i_block) b = -1;
        archivo.i_type = '1';
        archivo.i_perm[0] = '6'; archivo.i_perm[1] = '6'; archivo.i_perm[2] = '4';
    }
    archivo.i_atime = archivo.i_mtime = ahora;

    if (!escribirArchivo(m->ruta, sb, indiceArchivo, archivo, contenido)) {
        salida.error("mkfile: no hay espacio suficiente para el contenido");
        return;
    }

    if (!sobrescribe &&
        !agregarEntrada(m->ruta, sb, indicePadre, padre, nombre, indiceArchivo)) {
        salida.error("mkfile: la carpeta padre ya no admite mas entradas");
        return;
    }

    escribirSB(m->ruta, inicioParticion(sb), sb);

    salida.exito("mkfile: archivo " + destino +
                 (sobrescribe ? " sobrescrito" : " creado") +
                 " (" + std::to_string(contenido.size()) + " bytes)");
}

// ---------------- MKDIR ----------------
void cmdMkdir(const Parametros &p, Salida &salida) {
    const Sesion &s = sesionActual();
    if (!s.activa) {
        salida.error("mkdir: necesita iniciar sesion");
        return;
    }

    const Montaje *m = buscarMontaje(s.id);
    if (m == nullptr) {
        salida.error("mkdir: la particion de la sesion ya no esta montada");
        return;
    }

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("mkdir: no se pudo leer la particion");
        return;
    }

    std::string destino = sinComillas(p.obtener("-path"));
    std::vector<std::string> partes = partesDe(destino);
    if (partes.empty()) {
        salida.error("mkdir: -path no puede ser la raiz");
        return;
    }

    std::string fallo;
    int indicePadre = recorrerPadres(m->ruta, sb, partes, p.tiene("-p"), s, fallo);
    if (indicePadre == -1) {
        salida.error("mkdir: " + fallo +
                     (p.tiene("-p") ? "" : " use -p para crear las carpetas padre"));
        return;
    }

    Inodo padre;
    leerInodo(m->ruta, sb, indicePadre, padre);

    const std::string &nombre = partes.back();
    if (nombre.size() > 12) {
        salida.error("mkdir: el nombre " + nombre + " pasa de 12 caracteres");
        return;
    }

    // con -p una carpeta que ya existe no es error simplemente no hace nada
    if (buscarEntrada(m->ruta, sb, padre, nombre) != -1) {
        if (p.tiene("-p")) {
            salida.escribir("mkdir: la carpeta " + destino + " ya existia");
            return;
        }
        salida.error("mkdir: la carpeta " + destino + " ya existe");
        return;
    }

    if (!puedeEscribir(s, padre)) {
        salida.error("mkdir: sin permiso de escritura en la carpeta padre");
        return;
    }

    if (crearCarpeta(m->ruta, sb, indicePadre, padre, nombre, s) == -1) {
        salida.error("mkdir: no se pudo crear la carpeta " + destino);
        return;
    }

    salida.exito("mkdir: carpeta " + destino + " creada");
}
