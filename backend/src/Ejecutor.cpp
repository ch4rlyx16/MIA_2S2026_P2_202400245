#include "Ejecutor.h"

#include "Analizador.h"
#include "comandos/Discos.h"
#include "comandos/Montaje.h"
#include "comandos/Formateo.h"
#include "comandos/Sesion.h"
#include "comandos/Usuarios.h"
#include "comandos/Cat.h"
#include "comandos/Archivos.h"
#include "comandos/Rep.h"

#include <functional>
#include <map>
#include <sstream>
#include <string>

namespace {


using Manejador = std::function<void(const Parametros &, Salida &)>;

const std::map<std::string, Manejador> &manejadores() {
    static const std::map<std::string, Manejador> tabla = {
        { "mkdisk",  cmdMkdisk  },
        { "rmdisk",  cmdRmdisk  },
        { "fdisk",   cmdFdisk   },
        { "mount",   cmdMount   },
        { "mounted", cmdMounted },
        { "mkfs",    cmdMkfs    },
        { "login",   cmdLogin   },
        { "logout",  cmdLogout  },
        { "mkgrp",   cmdMkgrp   },
        { "rmgrp",   cmdRmgrp   },
        { "mkusr",   cmdMkusr   },
        { "rmusr",   cmdRmusr   },
        { "chgrp",   cmdChgrp   },
        { "cat",     cmdCat     },
        { "mkfile",  cmdMkfile  },
        { "mkdir",   cmdMkdir   },
        { "rep",     cmdRep     },
    };
    return tabla;
}

/* Quita espacios de los dos extremos, para reconocer lineas en blanco
   y comentarios que empiezan con sangria */
std::string recortar(const std::string &texto) {
    size_t inicio = texto.find_first_not_of(" \t\r");
    if (inicio == std::string::npos) return "";
    size_t fin = texto.find_last_not_of(" \t\r");
    return texto.substr(inicio, fin - inicio + 1);
}

} // namespace

Salida ejecutarTexto(const std::string &entrada) {
    Salida salida;

    std::istringstream flujo(entrada);
    std::string linea;
    size_t numeroLinea = 0;

    while (std::getline(flujo, linea)) {
        ++numeroLinea;
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();

        std::string limpia = recortar(linea);

        /* Las lineas en blanco se conservan para respetar el formato
           del script. */
        if (limpia.empty()) {
            salida.escribir("");
            continue;
        }
        /* Los comentarios se muestran tal cual, sin analizarlos. */
        if (limpia[0] == '#') {
            salida.escribir(limpia);
            continue;
        }

        AnalisisLinea analisis = analizarLinea(linea, numeroLinea);

        if (!analisis.errores.empty()) {
            for (const ErrorAnalisis &e : analisis.errores)
                salida.error("linea " + std::to_string(e.linea) +
                             ", columna " + std::to_string(e.columna) +
                             ": " + e.mensaje);
            continue;
        }
        if (!analisis.hayComando) continue;

        auto it = manejadores().find(analisis.parametros.comando);
        if (it == manejadores().end()) {
            salida.error("el comando '" + analisis.parametros.comando +
                         "' aun no esta implementado");
            continue;
        }

        it->second(analisis.parametros, salida);
    }

    return salida;
}
