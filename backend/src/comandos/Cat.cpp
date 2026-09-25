#include "Cat.h"
#include "Montaje.h"
#include "Permisos.h"
#include "Sesion.h"

#include "../disco/Ext2.h"
#include "../util/Texto.h"

#include <algorithm>
#include <vector>

void cmdCat(const Parametros &p, Salida &salida) {
    const Sesion &s = sesionActual();
    if (!s.activa) {
        salida.error("cat: necesita iniciar sesion");
        return;
    }

    const Montaje *m = buscarMontaje(s.id);
    if (m == nullptr) {
        salida.error("cat: la particion de la sesion ya no esta montada");
        return;
    }

    SuperBloque sb;
    if (!leerSB(m->ruta, m->inicio, sb)) {
        salida.error("cat: no se pudo leer la particion");
        return;
    }

    // recoger los -fileN en orden por su numero
    std::vector<std::pair<int, std::string>> archivos;
    for (const auto &par : p.valores) {
        if (par.first.rfind("-file", 0) != 0) continue;
        int num = std::atoi(par.first.c_str() + 5);
        archivos.push_back({ num, sinComillas(par.second) });
    }
    std::sort(archivos.begin(), archivos.end());

    for (const auto &archivo : archivos) {
        int i = buscarInodoPorRuta(m->ruta, sb, archivo.second);
        if (i == -1) {
            salida.error("cat: no existe el archivo " + archivo.second);
            continue;
        }

        Inodo nodo;
        leerInodo(m->ruta, sb, i, nodo);

        if (nodo.i_type != '1') {
            salida.error("cat: " + archivo.second + " no es un archivo");
            continue;
        }
        if (!puedeLeer(s, nodo)) {
            salida.error("cat: sin permiso de lectura sobre " + archivo.second);
            continue;
        }

        salida.escribir(leerArchivo(m->ruta, sb, nodo));
    }
}
