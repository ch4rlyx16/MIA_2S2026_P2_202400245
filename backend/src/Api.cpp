#include "Api.h"

#include "comandos/Discos.h"
#include "comandos/Montaje.h"
#include "comandos/Permisos.h"
#include "comandos/Sesion.h"

#include "disco/Ext2.h"
#include "disco/Journal.h"
#include "estructuras/Estructuras.h"
#include "util/Archivo.h"
#include "util/Texto.h"

#include <ctime>
#include <vector>

using json = nlohmann::json;

namespace {

// el nombre del archivo sin la ruta, para mostrarlo en el selector
std::string nombreDeArchivo(const std::string &ruta) {
    size_t barra = ruta.find_last_of('/');
    return barra == std::string::npos ? ruta : ruta.substr(barra + 1);
}

std::string textoFit(char fit) {
    if (fit == 'B') return "Best Fit";
    if (fit == 'F') return "First Fit";
    if (fit == 'W') return "Worst Fit";
    return "?";
}

std::string textoEstado(char estado) {
    if (estado == '2') return "Montada";
    if (estado == '1') return "Creada";
    return "Libre";
}

// permisos octales a rwxrwxr--
std::string textoPermisos(const char *perm) {
    std::string texto;
    for (int i = 0; i < 3; ++i) {
        int d = perm[i] - '0';
        texto += (d & 4) ? 'r' : '-';
        texto += (d & 2) ? 'w' : '-';
        texto += (d & 1) ? 'x' : '-';
    }
    return texto;
}

// recorre la cadena de ebr para listar las logicas de una extendida
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

// resuelve el montaje y el superbloque de un id, o deja el error listo
bool abrirParticion(const std::string &id, const Montaje *&m,
                    SuperBloque &sb, json &error) {
    m = buscarMontaje(aMayusculas(id));
    if (m == nullptr) {
        error = json{{ "error", "no hay ninguna particion montada con id " + id }};
        return false;
    }
    if (!leerSB(m->ruta, m->inicio, sb)) {
        error = json{{ "error", "la particion " + id + " no esta formateada" }};
        return false;
    }
    return true;
}

} // namespace

/* ---------- Discos ---------- */
json apiDiscos() {
    json lista = json::array();

    for (const std::string &ruta : discosRegistrados()) {
        MBR mbr;
        if (!leerDe(ruta, 0, mbr)) continue;

        int montadas = 0;
        for (const Particion &p : mbr.mbr_partitions)
            if (p.part_status == '2') ++montadas;

        lista.push_back({
            { "ruta",      ruta                               },
            { "nombre",    nombreDeArchivo(ruta)              },
            { "tamano",    mbr.mbr_tamano                     },
            { "fit",       textoFit(mbr.dsk_fit)              },
            { "fecha",     fechaTexto(mbr.mbr_fecha_creacion) },
            { "firma",     mbr.mbr_dsk_signature              },
            { "montadas",  montadas                           }
        });
    }
    return json{{ "discos", lista }};
}

/* ---------- Particiones de un disco ---------- */
json apiParticiones(const std::string &rutaDisco) {
    MBR mbr;
    if (!leerDe(rutaDisco, 0, mbr))
        return json{{ "error", "no se pudo leer el disco " + rutaDisco }};

    json lista = json::array();

    for (const Particion &p : mbr.mbr_partitions) {
        if (p.part_status == '0') continue;

        lista.push_back({
            { "nombre",  aTexto(p.part_name, 16)                        },
            { "tipo",    p.part_type == 'E' ? "Extendida" : "Primaria"  },
            { "fit",     textoFit(p.part_fit)                           },
            { "tamano",  p.part_s                                       },
            { "inicio",  p.part_start                                   },
            { "estado",  textoEstado(p.part_status)                     },
            { "id",      aTexto(p.part_id, 4)                           }
        });

        if (p.part_type != 'E') continue;

        for (const EBR &l : logicasDe(rutaDisco, p)) {
            lista.push_back({
                { "nombre",  aTexto(l.part_name, 16)   },
                { "tipo",    "Logica"                  },
                { "fit",     textoFit(l.part_fit)      },
                { "tamano",  l.part_s                  },
                { "inicio",  l.part_start              },
                { "estado",  l.part_mount == '1' ? "Montada" : "Creada" },
                { "id",      ""                        }
            });
        }
    }
    return json{{ "particiones", lista }};
}

/* ---------- Sesion ---------- */
json apiLogin(const std::string &id, const std::string &usuario,
              const std::string &pass) {
    Sesion nueva;
    std::string motivo;
    if (!autenticar(id, usuario, pass, nueva, motivo))
        return json{{ "ok", false }, { "error", motivo }};

    // ya se sabe que son buenas asi que el relevo es seguro
    if (sesionActual().activa) {
        Parametros salir;
        salir.comando = "logout";
        Salida descartable;
        cmdLogout(salir, descartable);
    }

    Parametros p;
    p.comando = "login";
    p.valores["-id"]   = id;
    p.valores["-user"] = usuario;
    p.valores["-pass"] = pass;

    Salida salida;
    cmdLogin(p, salida);

    const Sesion &s = sesionActual();
    return json{
        { "ok",      true      },
        { "usuario", s.usuario },
        { "grupo",   s.grupo   },
        { "id",      s.id      },
        { "uid",     s.uid     }
    };
}

json apiLogout() {
    Parametros p;
    p.comando = "logout";
    Salida salida;
    cmdLogout(p, salida);

    return json{{ "ok", !sesionActual().activa }};
}

json apiSesion() {
    const Sesion &s = sesionActual();
    if (!s.activa) return json{{ "activa", false }};

    return json{
        { "activa",  true      },
        { "usuario", s.usuario },
        { "grupo",   s.grupo   },
        { "id",      s.id      },
        { "uid",     s.uid     }
    };
}

/* ---------- Contenido de una carpeta ---------- */
json apiCarpeta(const std::string &id, const std::string &ruta) {
    const Montaje *m; SuperBloque sb; json error;
    if (!abrirParticion(id, m, sb, error)) return error;

    int indice = buscarInodoPorRuta(m->ruta, sb, ruta);
    if (indice == -1)
        return json{{ "error", "no existe la carpeta " + ruta }};

    Inodo carpeta;
    if (!leerInodo(m->ruta, sb, indice, carpeta))
        return json{{ "error", "no se pudo leer " + ruta }};
    if (carpeta.i_type != '0')
        return json{{ "error", ruta + " no es una carpeta" }};

    json lista = json::array();

    for (int d = 0; d < 12; ++d) {
        if (carpeta.i_block[d] == -1) continue;

        BloqueCarpeta bloque;
        leerDe(m->ruta, posBloque(sb, carpeta.i_block[d]), bloque);

        for (const Contenido &c : bloque.b_content) {
            std::string nombre = aTexto(c.b_name, 12);
            // el visualizador no muestra las entradas de navegacion
            if (c.b_inodo == -1 || nombre == "." || nombre == "..") continue;

            Inodo hijo;
            if (!leerInodo(m->ruta, sb, c.b_inodo, hijo)) continue;

            lista.push_back({
                { "nombre",    nombre                               },
                { "tipo",      hijo.i_type == '1' ? "archivo" : "carpeta" },
                { "tamano",    hijo.i_s                             },
                { "permisos",  textoPermisos(hijo.i_perm)           },
                { "octal",     aTexto(hijo.i_perm, 3)               },
                { "uid",       hijo.i_uid                           },
                { "gid",       hijo.i_gid                           },
                { "modificado", fechaTexto(hijo.i_mtime)            }
            });
        }
    }

    return json{{ "ruta", ruta }, { "contenido", lista }};
}

/* ---------- Contenido de un archivo ---------- */
json apiArchivo(const std::string &id, const std::string &ruta) {
    const Montaje *m; SuperBloque sb; json error;
    if (!abrirParticion(id, m, sb, error)) return error;

    int indice = buscarInodoPorRuta(m->ruta, sb, ruta);
    if (indice == -1)
        return json{{ "error", "no existe el archivo " + ruta }};

    Inodo nodo;
    if (!leerInodo(m->ruta, sb, indice, nodo))
        return json{{ "error", "no se pudo leer " + ruta }};
    if (nodo.i_type != '1')
        return json{{ "error", ruta + " no es un archivo" }};

    const Sesion &s = sesionActual();
    if (s.activa && !puedeLeer(s, nodo))
        return json{{ "error", "sin permiso de lectura sobre " + ruta }};

    return json{
        { "ruta",       ruta                            },
        { "contenido",  leerArchivo(m->ruta, sb, nodo)  },
        { "tamano",     nodo.i_s                        },
        { "permisos",   textoPermisos(nodo.i_perm)      },
        { "uid",        nodo.i_uid                      },
        { "modificado", fechaTexto(nodo.i_mtime)        }
    };
}

/* ---------- Bitacora ---------- */
json apiJournaling(const std::string &id) {
    const Montaje *m; SuperBloque sb; json error;
    if (!abrirParticion(id, m, sb, error)) return error;

    if (!esExt3(sb))
        return json{{ "error", "la particion " + id + " es ext2 y no lleva bitacora" }};

    json lista = json::array();
    for (const Journal &j : leerJournal(m)) {
        lista.push_back({
            { "numero",    j.j_count                                      },
            { "operacion", aTexto(j.j_content.i_operation, 10)            },
            { "path",      aTexto(j.j_content.i_path, 32)                 },
            { "contenido", aTexto(j.j_content.i_content, 64)              },
            { "fecha",     fechaTexto(static_cast<time_t>(j.j_content.i_date)) }
        });
    }

    return json{
        { "entradas", lista              },
        { "maximo",   JOURNAL_ENTRADAS   }
    };
}
