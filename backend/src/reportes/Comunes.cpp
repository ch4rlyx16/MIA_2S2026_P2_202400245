#include "Reportes.h"
#include "../util/Archivo.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {

// lo que va despues del ultimo punto del nombre jpg png pdf svg
std::string extensionDe(const std::string &destino) {
    size_t punto = destino.find_last_of('.');
    size_t barra = destino.find_last_of('/');
    if (punto == std::string::npos) return "png";
    if (barra != std::string::npos && punto < barra) return "png";
    return destino.substr(punto + 1);
}

} // namespace

std::string escaparHtml(const std::string &texto) {
    std::string salida;
    for (char c : texto) {
        if (c == '&')      salida += "&amp;";
        else if (c == '<') salida += "&lt;";
        else if (c == '>') salida += "&gt;";
        else if (c == '"') salida += "&quot;";
        else if (c == '\n') salida += "<BR/>";
        else salida += c;
    }
    return salida;
}

bool generarGrafico(const std::string &dot, const std::string &destino,
                    Salida &salida) {
    if (!crearCarpetasPadre(destino)) {
        salida.error("rep: no se pudieron crear las carpetas de " + destino);
        return false;
    }

    std::string temporal = destino + ".dot";
    std::ofstream archivo(temporal);
    if (!archivo) {
        salida.error("rep: no se pudo escribir " + temporal);
        return false;
    }
    archivo << dot;
    archivo.close();

    std::string orden = "dot -T" + extensionDe(destino) +
                        " \"" + temporal + "\" -o \"" + destino + "\" 2>/dev/null";
    int estado = std::system(orden.c_str());
    std::remove(temporal.c_str());

    if (estado != 0) {
        salida.error("rep: graphviz fallo, revise que dot este instalado");
        return false;
    }
    return true;
}

bool guardarTexto(const std::string &texto, const std::string &destino,
                  Salida &salida) {
    if (!crearCarpetasPadre(destino)) {
        salida.error("rep: no se pudieron crear las carpetas de " + destino);
        return false;
    }

    std::ofstream archivo(destino);
    if (!archivo) {
        salida.error("rep: no se pudo escribir " + destino);
        return false;
    }
    archivo << texto;
    return true;
}
