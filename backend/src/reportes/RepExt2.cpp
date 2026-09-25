#include "Reportes.h"
#include "../disco/Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <string>

namespace {

std::string fila(const std::string &campo, const std::string &valor) {
    return "<TR><TD>" + escaparHtml(campo) + "</TD><TD>" +
           escaparHtml(valor) + "</TD></TR>";
}

// los bitmaps se piden en texto con 20 registros por linea
std::string bitmapTexto(const std::string &ruta, int inicio, int cuantos) {
    std::string texto;
    for (int i = 0; i < cuantos; ++i) {
        char bit = 0;
        leerDe(ruta, inicio + i, bit);
        texto += bit == 1 ? '1' : '0';
        texto += (i % 20 == 19) ? '\n' : ' ';
    }
    if (!texto.empty() && texto.back() != '\n') texto += '\n';
    return texto;
}

} // namespace

// ---------------- REPORTE SB ----------------
void repSb(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    std::string dot = "digraph sb {\n node [shape=plaintext];\n"
                      " tabla [label=<<TABLE BORDER='2' CELLBORDER='1' "
                      "CELLSPACING='0'>";
    dot += "<TR><TD BGCOLOR='#1e6f5c' WIDTH='240'><B>REPORTE DE SUPERBLOQUE"
           "</B></TD><TD BGCOLOR='#1e6f5c' WIDTH='240'></TD></TR>";

    dot += fila("s_filesystem_type",   std::to_string(sb.s_filesystem_type));
    dot += fila("s_inodes_count",      std::to_string(sb.s_inodes_count));
    dot += fila("s_blocks_count",      std::to_string(sb.s_blocks_count));
    dot += fila("s_free_inodes_count", std::to_string(sb.s_free_inodes_count));
    dot += fila("s_free_blocks_count", std::to_string(sb.s_free_blocks_count));
    dot += fila("s_mtime",             fechaTexto(sb.s_mtime));
    dot += fila("s_umtime",            sb.s_umtime ? fechaTexto(sb.s_umtime) : "-");
    dot += fila("s_mnt_count",         std::to_string(sb.s_mnt_count));
    dot += fila("s_magic",             "0xEF53");
    dot += fila("s_inode_s",           std::to_string(sb.s_inode_s));
    dot += fila("s_block_s",           std::to_string(sb.s_block_s));
    dot += fila("s_firts_ino",         std::to_string(sb.s_firts_ino));
    dot += fila("s_first_blo",         std::to_string(sb.s_first_blo));
    dot += fila("s_bm_inode_start",    std::to_string(sb.s_bm_inode_start));
    dot += fila("s_bm_block_start",    std::to_string(sb.s_bm_block_start));
    dot += fila("s_inode_start",       std::to_string(sb.s_inode_start));
    dot += fila("s_block_start",       std::to_string(sb.s_block_start));

    dot += "</TABLE>>];\n}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte sb generado en " + destino);
}

// ---------------- REPORTE BM_INODE ----------------
void repBmInode(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }
    std::string texto = bitmapTexto(m->ruta, sb.s_bm_inode_start, sb.s_inodes_count);
    if (guardarTexto(texto, destino, salida))
        salida.exito("rep: reporte bm_inode generado en " + destino);
}

// ---------------- REPORTE BM_BLOCK ----------------
void repBmBlock(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }
    std::string texto = bitmapTexto(m->ruta, sb.s_bm_block_start, sb.s_blocks_count);
    if (guardarTexto(texto, destino, salida))
        salida.exito("rep: reporte bm_bloc generado en " + destino);
}

// ---------------- REPORTE INODE ----------------
void repInode(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    std::string dot = "digraph inodos {\n rankdir=LR;\n node [shape=plaintext];\n";
    std::string enlaces;
    int anterior = -1;

    // solo se muestran los inodos que el bitmap marca como usados
    for (int i = 0; i < sb.s_inodes_count; ++i) {
        if (!bitInodo(m->ruta, sb, i)) continue;

        Inodo nodo;
        if (!leerInodo(m->ruta, sb, i, nodo)) continue;

        dot += " i" + std::to_string(i) + " [label=<<TABLE BORDER='2' "
               "CELLBORDER='1' CELLSPACING='0'>";
        dot += "<TR><TD BGCOLOR='#f4a261' WIDTH='130'><B>Inodo " +
               std::to_string(i) + "</B></TD><TD BGCOLOR='#f4a261' "
               "WIDTH='130'></TD></TR>";
        dot += fila("i_uid",   std::to_string(nodo.i_uid));
        dot += fila("i_gid",   std::to_string(nodo.i_gid));
        dot += fila("i_s",     std::to_string(nodo.i_s));
        dot += fila("i_atime", fechaTexto(nodo.i_atime));
        dot += fila("i_ctime", fechaTexto(nodo.i_ctime));
        dot += fila("i_mtime", fechaTexto(nodo.i_mtime));
        dot += fila("i_type",  nodo.i_type == '1' ? "1 (archivo)" : "0 (carpeta)");
        dot += fila("i_perm",  aTexto(nodo.i_perm, 3));

        for (int b = 0; b < 15; ++b)
            dot += fila("i_block[" + std::to_string(b) + "]",
                        std::to_string(nodo.i_block[b]));

        dot += "</TABLE>>];\n";

        if (anterior != -1)
            enlaces += " i" + std::to_string(anterior) + " -> i" +
                       std::to_string(i) + ";\n";
        anterior = i;
    }

    dot += enlaces + "}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte inode generado en " + destino);
}

// ---------------- REPORTE BLOCK ----------------
void repBlock(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    // para saber de que tipo es cada bloque se miran los inodos que lo usan
    // los de carpeta apuntan a bloques carpeta los de archivo a bloques archivo
    std::string tipos(sb.s_blocks_count, '?');
    for (int i = 0; i < sb.s_inodes_count; ++i) {
        if (!bitInodo(m->ruta, sb, i)) continue;

        Inodo nodo;
        if (!leerInodo(m->ruta, sb, i, nodo)) continue;

        char tipo = nodo.i_type == '0' ? 'C' : 'A';
        for (int d = 0; d < 12; ++d)
            if (nodo.i_block[d] >= 0 && nodo.i_block[d] < sb.s_blocks_count)
                tipos[nodo.i_block[d]] = tipo;

        // los tres ultimos apuntadores siempre son bloques de apuntadores
        for (int d = 12; d < 15; ++d)
            if (nodo.i_block[d] >= 0 && nodo.i_block[d] < sb.s_blocks_count)
                tipos[nodo.i_block[d]] = 'P';
    }

    std::string dot = "digraph bloques {\n rankdir=LR;\n node [shape=plaintext];\n";
    std::string enlaces;
    int anterior = -1;

    for (int i = 0; i < sb.s_blocks_count; ++i) {
        if (!bitBloque(m->ruta, sb, i)) continue;

        dot += " b" + std::to_string(i) + " [label=<<TABLE BORDER='2' "
               "CELLBORDER='1' CELLSPACING='0'>";

        if (tipos[i] == 'C') {
            BloqueCarpeta bloque;
            leerDe(m->ruta, posBloque(sb, i), bloque);

            dot += "<TR><TD BGCOLOR='#90be6d' WIDTH='130'><B>Bloque Carpeta " +
                   std::to_string(i) + "</B></TD><TD BGCOLOR='#90be6d' "
                   "WIDTH='130'></TD></TR>";
            dot += "<TR><TD><B>b_name</B></TD><TD><B>b_inodo</B></TD></TR>";
            for (const Contenido &c : bloque.b_content)
                dot += fila(aTexto(c.b_name, 12), std::to_string(c.b_inodo));

        } else if (tipos[i] == 'P') {
            BloqueApuntadores bloque;
            leerDe(m->ruta, posBloque(sb, i), bloque);

            dot += "<TR><TD BGCOLOR='#bdb2ff' WIDTH='260' COLSPAN='2'>"
                   "<B>Bloque Apuntadores " + std::to_string(i) +
                   "</B></TD></TR>";
            std::string lista;
            for (int p = 0; p < 16; ++p) {
                lista += std::to_string(bloque.b_pointers[p]);
                lista += (p % 4 == 3) ? "\n" : ", ";
            }
            dot += "<TR><TD COLSPAN='2'>" + escaparHtml(lista) + "</TD></TR>";

        } else {
            BloqueArchivo bloque;
            leerDe(m->ruta, posBloque(sb, i), bloque);

            dot += "<TR><TD BGCOLOR='#f9c74f' WIDTH='260' COLSPAN='2'>"
                   "<B>Bloque Archivo " + std::to_string(i) + "</B></TD></TR>";
            dot += "<TR><TD COLSPAN='2'>" +
                   escaparHtml(aTexto(bloque.b_content, 64)) + "</TD></TR>";
        }

        dot += "</TABLE>>];\n";

        if (anterior != -1)
            enlaces += " b" + std::to_string(anterior) + " -> b" +
                       std::to_string(i) + ";\n";
        anterior = i;
    }

    dot += enlaces + "}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte block generado en " + destino);
}
