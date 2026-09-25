#include "Validador.h"
#include "util/Texto.h"

#include <cstdlib>
#include <map>
#include <set>
#include <vector>

namespace {

/* Parametros que cada comando exige. Los opcionales no se listan:
   la gramatica ya rechaza cualquier parametro que no le corresponda
   al comando. */
const std::map<std::string, std::vector<std::string>> &obligatorios() {
    static const std::map<std::string, std::vector<std::string>> tabla = {
        { "mkdisk",  { "-size", "-path" } },
        { "rmdisk",  { "-path" } },
        { "fdisk",   { "-size", "-path", "-name" } },
        { "mount",   { "-path", "-name" } },
        { "mounted", { } },
        { "mkfs",    { "-id" } },
        { "cat",     { } },              // se revisa aparte: al menos un -fileN
        { "login",   { "-user", "-pass", "-id" } },
        { "logout",  { } },
        { "mkgrp",   { "-name" } },
        { "rmgrp",   { "-name" } },
        { "mkusr",   { "-user", "-pass", "-grp" } },
        { "rmusr",   { "-user" } },
        { "chgrp",   { "-user", "-grp" } },
        { "mkfile",  { "-path" } },
        { "mkdir",   { "-path" } },
        { "rep",     { "-name", "-path", "-id" } },
    };
    return tabla;
}

/* Reportes que acepta el comando rep. */
const std::set<std::string> &reportesValidos() {
    static const std::set<std::string> tabla = {
        "mbr", "disk", "inode", "block", "bm_inode",
        "bm_block", "tree", "sb", "file", "ls"
    };
    return tabla;
}

/* Campos que el enunciado limita a 10 caracteres. */
void revisarLargo(const Parametros &p, const std::string &nombre,
                  std::vector<ErrorAnalisis> &errores) {
    if (!p.tiene(nombre)) return;

    std::string valor = sinComillas(p.obtener(nombre));
    if (valor.size() > 10) {
        errores.push_back({ p.linea, p.columna,
            nombre + " admite maximo 10 caracteres (se recibieron " +
            std::to_string(valor.size()) + ")" });
    }
}

} // namespace

void validarParametros(const Parametros &p,
                       std::vector<ErrorAnalisis> &errores) {

    auto it = obligatorios().find(p.comando);
    if (it == obligatorios().end()) return;

    /* ---- Parametros obligatorios ausentes ---- */
    for (const std::string &requerido : it->second) {
        if (!p.tiene(requerido)) {
            errores.push_back({ p.linea, p.columna,
                "falta el parametro obligatorio " + requerido +
                " en " + p.comando });
        }
    }

    /* ---- cat necesita al menos un archivo ---- */
    if (p.comando == "cat") {
        bool hayArchivo = false;
        for (const auto &par : p.valores)
            if (par.first.rfind("-file", 0) == 0) { hayArchivo = true; break; }

        if (!hayArchivo) {
            errores.push_back({ p.linea, p.columna,
                "cat necesita al menos un parametro -file1" });
        }
    }

    /* ---- Tamanos ---- */
    if (p.tiene("-size")) {
        long long numero = std::atoll(p.obtener("-size").c_str());

        /* mkfile admite 0 bytes; los demas exigen un tamano real. */
        long long minimo = (p.comando == "mkfile") ? 0 : 1;

        if (numero < minimo) {
            errores.push_back({ p.linea, p.columna,
                "-size debe ser un numero " +
                std::string(minimo == 0 ? "mayor o igual a cero"
                                        : "positivo mayor que cero") +
                " (se recibio " + p.obtener("-size") + ")" });
        }
    }

    /* ---- Limites de 10 caracteres en usuarios y grupos ---- */
    if (p.comando == "mkusr" || p.comando == "mkgrp" ||
        p.comando == "rmusr" || p.comando == "rmgrp" ||
        p.comando == "chgrp") {
        revisarLargo(p, "-user", errores);
        revisarLargo(p, "-pass", errores);
        revisarLargo(p, "-grp",  errores);
        revisarLargo(p, "-name", errores);
    }

    /* ---- Nombre del reporte ---- */
    if (p.comando == "rep" && p.tiene("-name")) {
        std::string nombre = aMinusculas(sinComillas(p.obtener("-name")));
        if (reportesValidos().count(nombre) == 0) {
            errores.push_back({ p.linea, p.columna,
                "rep: '" + nombre + "' no es un reporte valido "
                "(mbr, disk, inode, block, bm_inode, bm_block, tree, sb, "
                "file, ls)" });
        }
        /* file y ls necesitan saber sobre que archivo o carpeta informar. */
        if ((nombre == "file" || nombre == "ls") && !p.tiene("-path_file_ls")) {
            errores.push_back({ p.linea, p.columna,
                "rep -name=" + nombre + " necesita el parametro "
                "-path_file_ls" });
        }
    }
}
