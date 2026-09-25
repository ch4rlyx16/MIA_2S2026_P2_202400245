#include "Discos.h"

#include "../estructuras/Estructuras.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace {

/* ------------------------------------------------------------
   Ayudas comunes
   ------------------------------------------------------------ */

/* Convierte la letra de -unit al numero de bytes que representa.
   Devuelve 0 si la letra no es valida. */
long long factorUnidad(const std::string &unidad, bool permiteBytes) {
    std::string u = aMayusculas(unidad);
    if (u == "B") return permiteBytes ? 1 : 0;
    if (u == "K") return 1024;
    if (u == "M") return 1024LL * 1024LL;
    return 0;
}

/* BF/FF/WF -> 'B'/'F'/'W'. Devuelve 0 si no es valido. */
char letraAjuste(const std::string &ajuste) {
    std::string a = aMayusculas(ajuste);
    if (a == "BF") return 'B';
    if (a == "FF") return 'F';
    if (a == "WF") return 'W';
    return 0;
}

/* Un tramo de disco sin usar. */
struct Hueco {
    int inicio;
    int tam;
};

/* Tramos libres del disco, en orden. Un hueco es todo lo que queda
   entre el final de una particion y el inicio de la siguiente */
std::vector<Hueco> huecosDelDisco(const MBR &mbr) {
    std::vector<Hueco> ocupados;
    for (const Particion &p : mbr.mbr_partitions)
        if (p.part_status != '0' && p.part_s > 0)
            ocupados.push_back({ p.part_start, p.part_s });

    std::sort(ocupados.begin(), ocupados.end(),
              [](const Hueco &a, const Hueco &b) { return a.inicio < b.inicio; });

    std::vector<Hueco> libres;
    int cursor = static_cast<int>(sizeof(MBR));   // el MBR ocupa el inicio

    for (const Hueco &usado : ocupados) {
        if (usado.inicio > cursor)
            libres.push_back({ cursor, usado.inicio - cursor });
        cursor = std::max(cursor, usado.inicio + usado.tam);
    }
    if (cursor < mbr.mbr_tamano)
        libres.push_back({ cursor, mbr.mbr_tamano - cursor });

    return libres;
}

/* Elige un hueco segun el ajuste del disco.
   First: el primero que alcance. Best: el mas ajustado. Worst: el mayor.
   Devuelve -1 si ninguno alcanza. */
int elegirHueco(const std::vector<Hueco> &libres, int necesario, char ajuste) {
    int elegido = -1;
    for (size_t i = 0; i < libres.size(); ++i) {
        if (libres[i].tam < necesario) continue;

        if (elegido == -1) { elegido = static_cast<int>(i); continue; }

        if (ajuste == 'F') break;                                   // ya esta
        if (ajuste == 'B' && libres[i].tam < libres[elegido].tam)
            elegido = static_cast<int>(i);
        if (ajuste == 'W' && libres[i].tam > libres[elegido].tam)
            elegido = static_cast<int>(i);
    }
    return elegido;
}

/* Recorre la cadena de EBR de una extendida y devuelve todos los que
   describen una logica ya creada. */
std::vector<EBR> logicasDe(const std::string &ruta, const Particion &extendida) {
    std::vector<EBR> encontradas;
    int posicion = extendida.part_start;

    while (posicion != -1) {
        EBR ebr;
        if (!leerDe(ruta, posicion, ebr)) break;
        if (ebr.part_s > 0) encontradas.push_back(ebr);
        if (ebr.part_next == posicion) break;      // proteccion ante ciclos
        posicion = ebr.part_next;
    }
    return encontradas;
}

/* True si el nombre ya lo usa otra particion del disco (primaria,
   extendida o logica). */
bool nombreRepetido(const std::string &ruta, const MBR &mbr,
                    const std::string &nombre) {
    for (const Particion &p : mbr.mbr_partitions) {
        if (p.part_status == '0') continue;
        if (aTexto(p.part_name, 16) == nombre) return true;

        if (p.part_type == 'E') {
            for (const EBR &l : logicasDe(ruta, p))
                if (aTexto(l.part_name, 16) == nombre) return true;
        }
    }
    return false;
}

} // namespace

/* ============================================================
   MKDISK
   ============================================================ */
void cmdMkdisk(const Parametros &p, Salida &salida) {
    std::string ruta = sinComillas(p.obtener("-path"));

    long long tamano = std::atoll(p.obtener("-size", "0").c_str());
    if (tamano <= 0) {
        salida.error("mkdisk: -size debe ser un numero mayor que cero");
        return;
    }

    /* Sin -unit el disco se mide en Megabytes. */
    long long factor = factorUnidad(p.obtener("-unit", "M"), false);
    if (factor == 0) {
        salida.error("mkdisk: -unit solo admite K o M");
        return;
    }

    /* Sin -fit el disco usa primer ajuste. */
    char ajuste = letraAjuste(p.obtener("-fit", "FF"));
    if (ajuste == 0) {
        salida.error("mkdisk: -fit solo admite BF, FF o WF");
        return;
    }

    if (existeArchivo(ruta)) {
        salida.error("mkdisk: ya existe un disco en " + ruta);
        return;
    }
    if (!crearCarpetasPadre(ruta)) {
        salida.error("mkdisk: no se pudieron crear las carpetas de " + ruta);
        return;
    }

    long long total = tamano * factor;

    /* El archivo se rellena de ceros binarios para representar espacio
       disponible. Se escribe de a 1 KB: hacerlo byte por byte tarda
       demasiado en discos de varios megabytes. */
    FILE *archivo = std::fopen(ruta.c_str(), "wb");
    if (archivo == nullptr) {
        salida.error("mkdisk: no se pudo crear el archivo " + ruta);
        return;
    }

    char vacio[1024] = { 0 };
    for (long long escrito = 0; escrito < total; escrito += sizeof(vacio)) {
        size_t bloque = static_cast<size_t>(
            std::min<long long>(sizeof(vacio), total - escrito));
        std::fwrite(vacio, 1, bloque, archivo);
    }
    std::fclose(archivo);

    /* El MBR va en el primer sector, encima de los ceros. */
    MBR mbr;
    std::memset(&mbr, 0, sizeof(MBR));
    mbr.mbr_tamano         = static_cast<int>(total);
    mbr.mbr_fecha_creacion = std::time(nullptr);
    mbr.mbr_dsk_signature  = std::rand();
    mbr.dsk_fit            = ajuste;

    for (Particion &particion : mbr.mbr_partitions) {
        particion.part_status      = '0';
        particion.part_start       = -1;
        particion.part_s           = -1;
        particion.part_correlative = -1;
    }

    if (!escribirEn(ruta, 0, mbr)) {
        salida.error("mkdisk: no se pudo escribir el MBR en " + ruta);
        return;
    }

    salida.exito("mkdisk: disco creado en " + ruta +
                 " (" + std::to_string(total) + " bytes)");
}

/* ============================================================
   RMDISK
   ============================================================ */
void cmdRmdisk(const Parametros &p, Salida &salida) {
    std::string ruta = sinComillas(p.obtener("-path"));

    if (!existeArchivo(ruta)) {
        salida.error("rmdisk: no existe el disco " + ruta);
        return;
    }
    if (std::remove(ruta.c_str()) != 0) {
        salida.error("rmdisk: no se pudo eliminar " + ruta);
        return;
    }
    salida.exito("rmdisk: disco eliminado " + ruta);
}

/* ============================================================
   FDISK
   ============================================================ */
void cmdFdisk(const Parametros &p, Salida &salida) {
    std::string ruta   = sinComillas(p.obtener("-path"));
    std::string nombre = sinComillas(p.obtener("-name"));

    long long tamano = std::atoll(p.obtener("-size", "0").c_str());
    if (tamano <= 0) {
        salida.error("fdisk: -size debe ser un numero mayor que cero");
        return;
    }

    /* Sin -unit la particion se mide en Kilobytes. */
    long long factor = factorUnidad(p.obtener("-unit", "K"), true);
    if (factor == 0) {
        salida.error("fdisk: -unit solo admite B, K o M");
        return;
    }

    /* Sin -fit la particion usa peor ajuste. */
    char ajuste = letraAjuste(p.obtener("-fit", "WF"));
    if (ajuste == 0) {
        salida.error("fdisk: -fit solo admite BF, FF o WF");
        return;
    }

    char tipo = static_cast<char>(std::toupper(p.obtener("-type", "P")[0]));
    if (tipo != 'P' && tipo != 'E' && tipo != 'L') {
        salida.error("fdisk: -type solo admite P, E o L");
        return;
    }

    if (nombre.empty()) {
        salida.error("fdisk: -name no puede estar vacio");
        return;
    }
    if (!existeArchivo(ruta)) {
        salida.error("fdisk: no existe el disco " + ruta);
        return;
    }

    MBR mbr;
    if (!leerDe(ruta, 0, mbr)) {
        salida.error("fdisk: no se pudo leer el MBR de " + ruta);
        return;
    }
    if (nombreRepetido(ruta, mbr, nombre)) {
        salida.error("fdisk: ya existe una particion llamada " + nombre +
                     " en este disco");
        return;
    }

    int necesario = static_cast<int>(tamano * factor);

    /* ---------- Particion logica: vive dentro de la extendida ---------- */
    if (tipo == 'L') {
        const Particion *extendida = nullptr;
        for (const Particion &particion : mbr.mbr_partitions)
            if (particion.part_status != '0' && particion.part_type == 'E')
                extendida = &particion;

        if (extendida == nullptr) {
            salida.error("fdisk: no se puede crear una logica porque el disco "
                         "no tiene particion extendida");
            return;
        }

        /* Cada logica gasta su propio EBR ademas de sus datos. */
        int gasto = static_cast<int>(sizeof(EBR)) + necesario;

   
        int posicion = extendida->part_start;
        EBR actual;
        if (!leerDe(ruta, posicion, actual)) {
            salida.error("fdisk: no se pudo leer el EBR de la extendida");
            return;
        }

        if (actual.part_s <= 0) {
            /* El primer EBR sigue sin usarse: esta logica lo estrena. */
            if (gasto > extendida->part_s) {
                salida.error("fdisk: no hay espacio en la extendida para " + nombre);
                return;
            }
            actual.part_mount = '0';
            actual.part_fit   = ajuste;
            actual.part_start = posicion;
            actual.part_s     = necesario;
            actual.part_next  = -1;
            copiarCampo(actual.part_name, 16, nombre);

            escribirEn(ruta, posicion, actual);
            salida.exito("fdisk: particion logica " + nombre + " creada en " + ruta);
            return;
        }

        /* Ya hay logicas: avanzar hasta la ultima de la cadena. */
        while (actual.part_next != -1) {
            posicion = actual.part_next;
            if (!leerDe(ruta, posicion, actual)) {
                salida.error("fdisk: la cadena de EBR esta danada");
                return;
            }
        }

        int nuevaPosicion = actual.part_start + static_cast<int>(sizeof(EBR))
                          + actual.part_s;
        int finExtendida  = extendida->part_start + extendida->part_s;

        if (nuevaPosicion + gasto > finExtendida) {
            salida.error("fdisk: no hay espacio en la extendida para " + nombre);
            return;
        }

        EBR nueva;
        std::memset(&nueva, 0, sizeof(EBR));
        nueva.part_mount = '0';
        nueva.part_fit   = ajuste;
        nueva.part_start = nuevaPosicion;
        nueva.part_s     = necesario;
        nueva.part_next  = -1;
        copiarCampo(nueva.part_name, 16, nombre);

        actual.part_next = nuevaPosicion;      // enlazar la anterior
        escribirEn(ruta, actual.part_start, actual);
        escribirEn(ruta, nuevaPosicion, nueva);

        salida.exito("fdisk: particion logica " + nombre + " creada en " + ruta);
        return;
    }

    /* ---------- Primaria o extendida: van en el MBR ---------- */
    int usadas = 0;
    int libre  = -1;
    for (int i = 0; i < 4; ++i) {
        if (mbr.mbr_partitions[i].part_status != '0') {
            ++usadas;
            if (tipo == 'E' && mbr.mbr_partitions[i].part_type == 'E') {
                salida.error("fdisk: el disco ya tiene una particion extendida");
                return;
            }
        } else if (libre == -1) {
            libre = i;
        }
    }
    if (usadas >= 4 || libre == -1) {
        salida.error("fdisk: el disco ya tiene 4 particiones, no se pueden "
                     "crear mas primarias ni extendidas");
        return;
    }

  
    std::vector<Hueco> libres = huecosDelDisco(mbr);
    int indice = elegirHueco(libres, necesario, mbr.dsk_fit);
    if (indice == -1) {
        salida.error("fdisk: no hay espacio contiguo suficiente en el disco "
                     "para " + nombre + " (" + std::to_string(necesario) +
                     " bytes)");
        return;
    }

    Particion &nueva = mbr.mbr_partitions[libre];
    std::memset(&nueva, 0, sizeof(Particion));
    nueva.part_status      = '1';
    nueva.part_type        = tipo;
    nueva.part_fit         = ajuste;
    nueva.part_start       = libres[indice].inicio;
    nueva.part_s           = necesario;
    nueva.part_correlative = -1;
    copiarCampo(nueva.part_name, 16, nombre);

    if (!escribirEn(ruta, 0, mbr)) {
        salida.error("fdisk: no se pudo actualizar el MBR");
        return;
    }

    if (tipo == 'E') {
        EBR primero;
        std::memset(&primero, 0, sizeof(EBR));
        primero.part_mount = '0';
        primero.part_fit   = ajuste;
        primero.part_start = nueva.part_start;
        primero.part_s     = 0;
        primero.part_next  = -1;
        escribirEn(ruta, nueva.part_start, primero);
    }

    salida.exito(std::string("fdisk: particion ") +
                 (tipo == 'E' ? "extendida " : "primaria ") + nombre +
                 " creada en " + ruta);
}
