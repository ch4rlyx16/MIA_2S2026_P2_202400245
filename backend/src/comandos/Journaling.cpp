#include "Journaling.h"
#include "Montaje.h"

#include "../disco/Ext2.h"
#include "../disco/Journal.h"
#include "../util/Texto.h"

#include <ctime>
#include <vector>

namespace {

// rellena o recorta un texto para que la columna siempre mida igual
std::string columna(const std::string &texto, size_t ancho) {
    std::string copia = texto;
    if (copia.size() >= ancho) {
        copia.resize(ancho - 3);
        copia += ".. ";
    } else {
        copia.resize(ancho, ' ');
    }
    return copia;
}

// aplana el texto a una sola linea: el contenido de un archivo puede
// traer saltos de linea y eso romperia la alineacion de la tabla
std::string unaLinea(const std::string &texto) {
    std::string copia = texto;
    for (char &c : copia)
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    return copia;
}

} // namespace

void cmdJournaling(const Parametros &p, Salida &salida) {
    std::string id = aMayusculas(sinComillas(p.obtener("-id")));

    const Montaje *m = buscarMontaje(id);
    if (m == nullptr) {
        salida.error("journaling: no hay ninguna particion montada con id " + id);
        return;
    }

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("journaling: no se pudo leer la particion " + id);
        return;
    }
    if (!esExt3(sb)) {
        salida.error("journaling: la particion " + id +
                     " es ext2 y solo ext3 lleva bitacora");
        return;
    }

    std::vector<Journal> entradas = leerJournal(m);
    if (entradas.empty()) {
        salida.escribir("journaling: la bitacora de " + id + " esta vacia");
        return;
    } 

    salida.escribir(columna("No", 5) + columna("OPERACION", 12) +
                    columna("PATH", 34) + columna("CONTENIDO", 34) + "FECHA");

    for (const Journal &j : entradas) {
        std::string contenido = unaLinea(aTexto(j.j_content.i_content, 64));
        if (contenido.empty()) contenido = "-";

        salida.escribir(
            columna(std::to_string(j.j_count), 5) +
            columna(aTexto(j.j_content.i_operation, 10), 12) +
            columna(aTexto(j.j_content.i_path, 32), 34) +
            columna(contenido, 34) +
            fechaTexto(static_cast<time_t>(j.j_content.i_date)));
    }

    salida.escribir("journaling: " + std::to_string(entradas.size()) + " de " +
                    std::to_string(JOURNAL_ENTRADAS) + " entradas usadas");
}