#include "Reportes.h"
#include "../disco/Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <set>
#include <string>

namespace {

std::string fila(const std::string &campo, const std::string &valor) {
    return "<TR><TD>" + escaparHtml(campo) + "</TD><TD>" +
           escaparHtml(valor) + "</TD></TR>";
}

// dibuja un inodo y baja por sus bloques armando el arbol completo
void pintarInodo(const Montaje *m, SuperBloque &sb, int indice,
                 std::string &nodos, std::string &enlaces,
                 std::set<int> &vistos);

// dibuja un bloque de carpeta y sigue hacia los inodos de sus entradas
void pintarBloqueCarpeta(const Montaje *m, SuperBloque &sb, int bloque,
                         std::string &nodos, std::string &enlaces,
                         std::set<int> &vistos) {
    BloqueCarpeta contenido;
    leerDe(m->ruta, posBloque(sb, bloque), contenido);

    nodos += " b" + std::to_string(bloque) + " [label=<<TABLE BORDER='2' "
             "CELLBORDER='1' CELLSPACING='0'>";
    nodos += "<TR><TD BGCOLOR='#90be6d' COLSPAN='2'><B>Bloque Carpeta " +
             std::to_string(bloque) + "</B></TD></TR>";

    for (const Contenido &c : contenido.b_content)
        nodos += fila(aTexto(c.b_name, 12), std::to_string(c.b_inodo));

    nodos += "</TABLE>>];\n";

    for (const Contenido &c : contenido.b_content) {
        std::string nombre = aTexto(c.b_name, 12);
        // punto y puntopunto apuntan hacia atras y armarian ciclos
        if (c.b_inodo == -1 || nombre == "." || nombre == "..") continue;

        enlaces += " b" + std::to_string(bloque) + " -> i" +
                   std::to_string(c.b_inodo) + ";\n";
        pintarInodo(m, sb, c.b_inodo, nodos, enlaces, vistos);
    }
}

// dibuja un bloque de apuntadores y sigue bajando por sus niveles
// nivel 1 lleva a datos nivel 2 y 3 llevan a mas apuntadores
void pintarApuntadores(const Montaje *m, SuperBloque &sb, int bloque, int nivel,
                       std::string &nodos, std::string &enlaces);

// dibuja un bloque de archivo con su contenido
void pintarBloqueArchivo(const Montaje *m, SuperBloque &sb, int bloque,
                         std::string &nodos) {
    BloqueArchivo contenido;
    leerDe(m->ruta, posBloque(sb, bloque), contenido);

    nodos += " b" + std::to_string(bloque) + " [label=<<TABLE BORDER='2' "
             "CELLBORDER='1' CELLSPACING='0'>";
    nodos += "<TR><TD BGCOLOR='#f9c74f'><B>Bloque Archivo " +
             std::to_string(bloque) + "</B></TD></TR>";
    nodos += "<TR><TD>" + escaparHtml(aTexto(contenido.b_content, 64)) +
             "</TD></TR></TABLE>>];\n";
}

void pintarInodo(const Montaje *m, SuperBloque &sb, int indice,
                 std::string &nodos, std::string &enlaces,
                 std::set<int> &vistos) {
    if (!vistos.insert(indice).second) return;   // ya se dibujo

    Inodo nodo;
    if (!leerInodo(m->ruta, sb, indice, nodo)) return;

    nodos += " i" + std::to_string(indice) + " [label=<<TABLE BORDER='2' "
             "CELLBORDER='1' CELLSPACING='0'>";
    nodos += "<TR><TD BGCOLOR='#f4a261' WIDTH='110'><B>Inodo " +
             std::to_string(indice) + "</B></TD><TD BGCOLOR='#f4a261' "
             "WIDTH='110'></TD></TR>";
    nodos += fila("i_uid",  std::to_string(nodo.i_uid));
    nodos += fila("i_gid",  std::to_string(nodo.i_gid));
    nodos += fila("i_s",    std::to_string(nodo.i_s));
    nodos += fila("i_type", nodo.i_type == '1' ? "archivo" : "carpeta");
    nodos += fila("i_perm", aTexto(nodo.i_perm, 3));

    for (int b = 0; b < 15; ++b)
        nodos += fila("i_block[" + std::to_string(b) + "]",
                      std::to_string(nodo.i_block[b]));

    nodos += "</TABLE>>];\n";

    // los doce directos llevan a bloques de carpeta o de archivo
    for (int b = 0; b < 12; ++b) {
        if (nodo.i_block[b] == -1) continue;

        enlaces += " i" + std::to_string(indice) + " -> b" +
                   std::to_string(nodo.i_block[b]) + ";\n";

        if (nodo.i_type == '0')
            pintarBloqueCarpeta(m, sb, nodo.i_block[b], nodos, enlaces, vistos);
        else
            pintarBloqueArchivo(m, sb, nodo.i_block[b], nodos);
    }

    // los tres ultimos son bloques de apuntadores con 1 2 o 3 niveles
    for (int b = 12; b < 15; ++b) {
        if (nodo.i_block[b] == -1) continue;

        enlaces += " i" + std::to_string(indice) + " -> b" +
                   std::to_string(nodo.i_block[b]) + ";\n";
        pintarApuntadores(m, sb, nodo.i_block[b], b - 11, nodos, enlaces);
    }
}

void pintarApuntadores(const Montaje *m, SuperBloque &sb, int bloque, int nivel,
                       std::string &nodos, std::string &enlaces) {
    BloqueApuntadores bp;
    leerDe(m->ruta, posBloque(sb, bloque), bp);

    nodos += " b" + std::to_string(bloque) + " [label=<<TABLE BORDER='2' "
             "CELLBORDER='1' CELLSPACING='0'>";
    nodos += "<TR><TD BGCOLOR='#bdb2ff'><B>Apuntadores " +
             std::to_string(bloque) + "</B></TD></TR>";

    std::string lista;
    for (int p = 0; p < 16; ++p) {
        lista += std::to_string(bp.b_pointers[p]);
        lista += (p % 4 == 3) ? "\n" : ", ";
    }
    nodos += "<TR><TD>" + escaparHtml(lista) + "</TD></TR></TABLE>>];\n";

    for (int p = 0; p < 16; ++p) {
        if (bp.b_pointers[p] == -1) continue;

        enlaces += " b" + std::to_string(bloque) + " -> b" +
                   std::to_string(bp.b_pointers[p]) + ";\n";

        if (nivel > 1)
            pintarApuntadores(m, sb, bp.b_pointers[p], nivel - 1, nodos, enlaces);
        else
            pintarBloqueArchivo(m, sb, bp.b_pointers[p], nodos);
    }
}

// permisos octal a rwx como los muestra ls
std::string permisosTexto(const char *perm) {
    std::string texto;
    for (int i = 0; i < 3; ++i) {
        int d = perm[i] - '0';
        texto += (d & 4) ? 'r' : '-';
        texto += (d & 2) ? 'w' : '-';
        texto += (d & 1) ? 'x' : '-';
    }
    return texto;
}

} // namespace

// ---------------- REPORTE TREE ----------------
void repTree(const Montaje *m, const std::string &destino, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    std::string nodos, enlaces;
    std::set<int> vistos;
    pintarInodo(m, sb, 0, nodos, enlaces, vistos);   // desde la raiz

    std::string dot = "digraph arbol {\n rankdir=LR;\n node [shape=plaintext];\n"
                    + nodos + enlaces + "}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte tree generado en " + destino);
}

// ---------------- REPORTE FILE ----------------
void repFile(const Montaje *m, const std::string &destino,
             const std::string &rutaInterna, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    int indice = buscarInodoPorRuta(m->ruta, sb, rutaInterna);
    if (indice == -1) {
        salida.error("rep: no existe " + rutaInterna + " en la particion");
        return;
    }

    Inodo nodo;
    leerInodo(m->ruta, sb, indice, nodo);
    if (nodo.i_type != '1') {
        salida.error("rep: " + rutaInterna + " no es un archivo");
        return;
    }

    if (guardarTexto(leerArchivo(m->ruta, sb, nodo), destino, salida))
        salida.exito("rep: reporte file de " + rutaInterna + " generado en " + destino);
}

// ---------------- REPORTE LS ----------------
void repLs(const Montaje *m, const std::string &destino,
           const std::string &rutaInterna, Salida &salida) {
    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("rep: no se pudo leer el superbloque");
        return;
    }

    int indice = buscarInodoPorRuta(m->ruta, sb, rutaInterna);
    if (indice == -1) {
        salida.error("rep: no existe " + rutaInterna + " en la particion");
        return;
    }

    Inodo carpeta;
    leerInodo(m->ruta, sb, indice, carpeta);
    if (carpeta.i_type != '0') {
        salida.error("rep: " + rutaInterna + " no es una carpeta");
        return;
    }

    std::string dot = "digraph ls {\n node [shape=plaintext];\n"
                      " tabla [label=<<TABLE BORDER='2' CELLBORDER='1' "
                      "CELLSPACING='0' CELLPADDING='4'>";
    dot += "<TR BGCOLOR='#5b8fb9'><TD><B>Permisos</B></TD><TD><B>Owner</B></TD>"
           "<TD><B>Grupo</B></TD><TD><B>Size</B></TD><TD><B>Fecha</B></TD>"
           "<TD><B>Tipo</B></TD><TD><B>Name</B></TD></TR>";

    for (int d = 0; d < 12; ++d) {
        if (carpeta.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(m->ruta, posBloque(sb, carpeta.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string nombre = aTexto(c.b_name, 12);
            if (c.b_inodo == -1 || nombre == "." || nombre == "..") continue;

            Inodo hijo;
            if (!leerInodo(m->ruta, sb, c.b_inodo, hijo)) continue;

            dot += "<TR><TD>" + permisosTexto(hijo.i_perm) + "</TD>";
            dot += "<TD>" + std::to_string(hijo.i_uid) + "</TD>";
            dot += "<TD>" + std::to_string(hijo.i_gid) + "</TD>";
            dot += "<TD>" + std::to_string(hijo.i_s) + "</TD>";
            dot += "<TD>" + fechaTexto(hijo.i_mtime) + "</TD>";
            dot += "<TD>" + std::string(hijo.i_type == '1' ? "Archivo" : "Carpeta") +
                   "</TD>";
            dot += "<TD>" + escaparHtml(nombre) + "</TD></TR>";
        }
    }

    dot += "</TABLE>>];\n}\n";

    if (generarGrafico(dot, destino, salida))
        salida.exito("rep: reporte ls de " + rutaInterna + " generado en " + destino);
}
