#include "Reportes.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

// una fila nombre valor de las tablas del reporte
std::string fila(const std::string &campo, const std::string &valor) {
    return "<TR><TD>" + escaparHtml(campo) + "</TD><TD>" +
           escaparHtml(valor) + "</TD></TR>";
}

// encabezado de color que separa cada bloque del mbr
std::string titulo(const std::string &texto, const std::string &color) {
    return "<TR><TD BGCOLOR='" + color + "' WIDTH='220'><B>" + texto +
           "</B></TD><TD BGCOLOR='" + color + "' WIDTH='220'></TD></TR>";
}

// recorre la cadena de ebr de una extendida
std::vector<EBR> logicasDe(const std::string &ruta, const Particion &ext) {
    std::vector<EBR> encontradas;
    int posicion = ext.part_start;

    while (posicion != -1) {
        EBR ebr;
        if (!leerDe(ruta, posicion, ebr)) break;
        if (ebr.part_s > 0) encontradas.push_back(ebr);
        if (ebr.part_next == posicion) break;
        posicion = ebr.part_next;
    }
    return encontradas;
}

// un tramo del disco para el reporte disk
struct Tramo {
    std::string tipo;     // MBR Primaria Extendida EBR Logica Libre
    int         tam;
    bool        dentroExt;
};

} // namespace

// ---------------- REPORTE MBR ----------------
void repMbr(const Montaje *m, const std::string &destino, Salida &salida) {
    MBR mbr;
    if (!leerDe(m->ruta, 0, mbr)) {
        salida.error("rep: no se pudo leer el MBR de " + m->ruta);
        return;
    }

    std::string dot = "digraph mbr {\n rankdir=LR;\n node [shape=plaintext];\n"
                      " tabla [label=<<TABLE BORDER='2' CELLBORDER='1' "
                      "CELLSPACING='0'>";

    dot += titulo("REPORTE DE MBR", "#5b8fb9");
    dot += fila("mbr_tamano", std::to_string(mbr.mbr_tamano));
    dot += fila("mbr_fecha_creacion", fechaTexto(mbr.mbr_fecha_creacion));
    dot += fila("mbr_dsk_signature", std::to_string(mbr.mbr_dsk_signature));
    dot += fila("dsk_fit", std::string(1, mbr.dsk_fit));

    for (const Particion &p : mbr.mbr_partitions) {
        if (p.part_status == '0') continue;

        bool extendida = p.part_type == 'E';
        dot += titulo(extendida ? "PARTICION EXTENDIDA" : "PARTICION PRIMARIA",
                      extendida ? "#d1ac00" : "#7fb77e");

        dot += fila("part_status", std::string(1, p.part_status));
        dot += fila("part_type",   std::string(1, p.part_type));
        dot += fila("part_fit",    std::string(1, p.part_fit));
        dot += fila("part_start",  std::to_string(p.part_start));
        dot += fila("part_s",      std::to_string(p.part_s));
        dot += fila("part_name",   aTexto(p.part_name, 16));
        dot += fila("part_correlative", std::to_string(p.part_correlative));
        dot += fila("part_id",     aTexto(p.part_id, 4));

        // las logicas de la extendida se muestran debajo de ella
        if (extendida) {
            for (const EBR &l : logicasDe(m->ruta, p)) {
                dot += titulo("PARTICION LOGICA", "#bdb2ff");
                dot += fila("part_mount", std::string(1, l.part_mount));
                dot += fila("part_fit",   std::string(1, l.part_fit));
                dot += fila("part_start", std::to_string(l.part_start));
                dot += fila("part_s",     std::to_string(l.part_s));
                dot += fila("part_next",  std::to_string(l.part_next));
                dot += fila("part_name",  aTexto(l.part_name, 16));
            }
        }
    }

    dot += "</TABLE>>];\n}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte mbr generado en " + destino);
}

// ---------------- REPORTE DISK ----------------
void repDisk(const Montaje *m, const std::string &destino, Salida &salida) {
    MBR mbr;
    if (!leerDe(m->ruta, 0, mbr)) {
        salida.error("rep: no se pudo leer el MBR de " + m->ruta);
        return;
    }

    // ordenar las particiones por donde empiezan para recorrer el disco
    std::vector<Particion> usadas;
    for (const Particion &p : mbr.mbr_partitions)
        if (p.part_status != '0' && p.part_s > 0) usadas.push_back(p);

    std::sort(usadas.begin(), usadas.end(),
              [](const Particion &a, const Particion &b) {
                  return a.part_start < b.part_start;
              });

    std::vector<Tramo> tramos;
    tramos.push_back({ "MBR", static_cast<int>(sizeof(MBR)), false });

    int cursor = static_cast<int>(sizeof(MBR));
    for (const Particion &p : usadas) {
        if (p.part_start > cursor)
            tramos.push_back({ "Libre", p.part_start - cursor, false });

        if (p.part_type == 'E') {
            // dentro de la extendida van los ebr sus logicas y lo libre
            int interno = p.part_start;
            for (const EBR &l : logicasDe(m->ruta, p)) {
                if (l.part_start > interno)
                    tramos.push_back({ "Libre", l.part_start - interno, true });

                tramos.push_back({ "EBR", static_cast<int>(sizeof(EBR)), true });
                tramos.push_back({ "Logica", l.part_s, true });
                interno = l.part_start + static_cast<int>(sizeof(EBR)) + l.part_s;
            }
            int finExt = p.part_start + p.part_s;
            if (interno < finExt)
                tramos.push_back({ "Libre", finExt - interno, true });
        } else {
            tramos.push_back({ "Primaria", p.part_s, false });
        }
        cursor = p.part_start + p.part_s;
    }
    if (cursor < mbr.mbr_tamano)
        tramos.push_back({ "Libre", mbr.mbr_tamano - cursor, false });

    // el nombre del disco es lo que va despues de la ultima barra
    size_t barra = m->ruta.find_last_of('/');
    std::string nombreDisco = barra == std::string::npos
                            ? m->ruta : m->ruta.substr(barra + 1);

    // fila de arriba con la extendida abarcando sus tramos internos
    int internos = 0;
    for (const Tramo &t : tramos) if (t.dentroExt) ++internos;

    std::string arriba, abajo;
    bool extPuesta = false;

    for (const Tramo &t : tramos) {
        char porcentaje[32];
        std::snprintf(porcentaje, sizeof(porcentaje), "%.2f%% del disco",
                      100.0 * t.tam / mbr.mbr_tamano);

        std::string celda = "<TD>" + t.tipo + "<BR/>" + porcentaje + "</TD>";

        if (t.dentroExt) {
            if (!extPuesta) {
                arriba += "<TD COLSPAN='" + std::to_string(internos) +
                          "' BGCOLOR='#d1ac00'><B>EXTENDIDA</B></TD>";
                extPuesta = true;
            }
            abajo += celda;
        } else {
            // los tramos de fuera ocupan las dos filas
            arriba += "<TD ROWSPAN='2'>" + t.tipo + "<BR/>" + porcentaje + "</TD>";
        }
    }

    std::string dot = "digraph disk {\n node [shape=plaintext];\n"
                      " tabla [label=<<TABLE BORDER='1' CELLBORDER='1' "
                      "CELLSPACING='0' CELLPADDING='8'>";
    dot += "<TR><TD COLSPAN='" + std::to_string(tramos.size() + 1) +
           "' BGCOLOR='#5b8fb9'><B>" + escaparHtml(nombreDisco) +
           "</B></TD></TR>";
    dot += "<TR>" + arriba + "</TR>";
    if (!abajo.empty()) dot += "<TR>" + abajo + "</TR>";
    dot += "</TABLE>>];\n}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte disk generado en " + destino);
}
