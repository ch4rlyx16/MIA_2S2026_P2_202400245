#include "Journal.h"
#include "Ext2.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <cstring>
#include <ctime>

void anotarJournal(const Montaje *m, const std::string &operacion,
                   const std::string &ruta, const std::string &contenido) {
    if (m == nullptr) return;

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) return;
    if (!esExt3(sb)) return;          // ext2 no lleva bitacora

    long base = inicioJournal(sb);

    // la primera entrada con j_count en cero es la que esta libre
    for (int i = 0; i < JOURNAL_ENTRADAS; ++i) {
        long pos = base + i * static_cast<long>(sizeof(Journal));

        Journal entrada;
        if (!leerDe(m->ruta, pos, entrada)) return;
        if (entrada.j_count != 0) continue;

        std::memset(&entrada, 0, sizeof(Journal));
        entrada.j_count = i + 1;
        copiarCampo(entrada.j_content.i_operation, 10, operacion);
        copiarCampo(entrada.j_content.i_path,      32, ruta);
        copiarCampo(entrada.j_content.i_content,   64, contenido);
        entrada.j_content.i_date = static_cast<float>(std::time(nullptr));

        escribirEn(m->ruta, pos, entrada);
        return;
    }
    // bitacora llena, ya no se anota nada mas
}

std::vector<Journal> leerJournal(const Montaje *m) {
    std::vector<Journal> entradas;
    if (m == nullptr) return entradas;

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) return entradas;
    if (!esExt3(sb)) return entradas;

    long base = inicioJournal(sb);

    for (int i = 0; i < JOURNAL_ENTRADAS; ++i) {
        Journal entrada;
        if (!leerDe(m->ruta, base + i * static_cast<long>(sizeof(Journal)), entrada))
            break;
        if (entrada.j_count == 0) break;   // de aqui en adelante esta vacio
        entradas.push_back(entrada);
    }
    return entradas;
}