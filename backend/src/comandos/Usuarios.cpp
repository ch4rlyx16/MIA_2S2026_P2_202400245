#include "Usuarios.h"
#include "Sesion.h"

#include "../disco/Ext2.h"
#include "../util/Texto.h"

#include <sstream>
#include <vector>

namespace {

// parte una linea de users.txt por comas y quita espacios
std::vector<std::string> campos(const std::string &linea) {
    std::vector<std::string> partes;
    std::stringstream flujo(linea);
    std::string p;
    while (std::getline(flujo, p, ',')) {
        size_t a = p.find_first_not_of(" \t");
        size_t b = p.find_last_not_of(" \t");
        partes.push_back(a == std::string::npos ? "" : p.substr(a, b - a + 1));
    }
    return partes;
}

// divide el contenido de users.txt en lineas sin las vacias del final
std::vector<std::string> lineas(const std::string &contenido) {
    std::vector<std::string> res;
    std::stringstream flujo(contenido);
    std::string l;
    while (std::getline(flujo, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (!l.empty()) res.push_back(l);
    }
    return res;
}

std::string unir(const std::vector<std::string> &ls) {
    std::string res;
    for (const std::string &l : ls) res += l + "\n";
    return res;
}

// el siguiente id para un tipo G o U es el mayor activo mas 1
int siguienteId(const std::vector<std::string> &ls, char tipo) {
    int maximo = 0;
    for (const std::string &l : ls) {
        std::vector<std::string> c = campos(l);
        if (c.size() >= 2 && c[1].size() == 1 && c[1][0] == tipo) {
            int id = std::atoi(c[0].c_str());
            if (id > maximo) maximo = id;
        }
    }
    return maximo + 1;
}

// busca un grupo activo por nombre y devuelve su indice de linea o -1
int buscarGrupo(const std::vector<std::string> &ls, const std::string &nombre) {
    for (size_t i = 0; i < ls.size(); ++i) {
        std::vector<std::string> c = campos(ls[i]);
        if (c.size() >= 3 && c[1] == "G" && c[0] != "0" && c[2] == nombre)
            return static_cast<int>(i);
    }
    return -1;
}

// busca un usuario activo por nombre y devuelve su indice de linea o -1
int buscarUsuario(const std::vector<std::string> &ls, const std::string &nombre) {
    for (size_t i = 0; i < ls.size(); ++i) {
        std::vector<std::string> c = campos(ls[i]);
        if (c.size() >= 5 && c[1] == "U" && c[0] != "0" && c[3] == nombre)
            return static_cast<int>(i);
    }
    return -1;
}

// exige sesion activa y usuario root false si ya reporto error
bool exigeRoot(const std::string &comando, Salida &salida) {
    const Sesion &s = sesionActual();
    if (!s.activa) {
        salida.error(comando + ": necesita iniciar sesion");
        return false;
    }
    if (s.usuario != "root") {
        salida.error(comando + ": solo el usuario root puede ejecutarlo");
        return false;
    }
    return true;
}

// carga users.txt para modificarlo deja todo listo para reescribir
bool abrirParaEditar(const std::string &comando, Salida &salida,
                     const Montaje *&m, SuperBloque &sb, int &inodoUsers,
                     Inodo &nodo, std::vector<std::string> &ls) {
    m = buscarMontaje(sesionActual().id);
    if (m == nullptr) {
        salida.error(comando + ": la particion de la sesion ya no esta montada");
        return false;
    }
    std::string contenido;
    if (!leerUsersTxt(m, sb, inodoUsers, nodo, contenido)) {
        salida.error(comando + ": no se pudo leer users.txt");
        return false;
    }
    ls = lineas(contenido);
    return true;
}

} // namespace

bool leerUsersTxt(const Montaje *m, SuperBloque &sb, int &inodoUsers,
                  Inodo &nodo, std::string &contenido) {
    if (!leerSB(m->ruta, m->inicio, sb)) return false;

    inodoUsers = buscarInodoPorRuta(m->ruta, sb, "/users.txt");
    if (inodoUsers == -1) return false;

    if (!leerInodo(m->ruta, sb, inodoUsers, nodo)) return false;
    contenido = leerArchivo(m->ruta, sb, nodo);
    return true;
}

// ---------------- MKGRP ----------------
void cmdMkgrp(const Parametros &p, Salida &salida) {
    if (!exigeRoot("mkgrp", salida)) return;
    std::string nombre = sinComillas(p.obtener("-name"));

    const Montaje *m; SuperBloque sb; int inodoUsers; Inodo nodo;
    std::vector<std::string> ls;
    if (!abrirParaEditar("mkgrp", salida, m, sb, inodoUsers, nodo, ls)) return;

    if (buscarGrupo(ls, nombre) != -1) {
        salida.error("mkgrp: el grupo " + nombre + " ya existe");
        return;
    }

    int gid = siguienteId(ls, 'G');
    ls.push_back(std::to_string(gid) + ", G, " + nombre);

    escribirArchivo(m->ruta, sb, inodoUsers, nodo, unir(ls));
    salida.exito("mkgrp: grupo " + nombre + " creado (gid " + std::to_string(gid) + ")");
}

// ---------------- RMGRP ----------------
void cmdRmgrp(const Parametros &p, Salida &salida) {
    if (!exigeRoot("rmgrp", salida)) return;
    std::string nombre = sinComillas(p.obtener("-name"));

    const Montaje *m; SuperBloque sb; int inodoUsers; Inodo nodo;
    std::vector<std::string> ls;
    if (!abrirParaEditar("rmgrp", salida, m, sb, inodoUsers, nodo, ls)) return;

    int i = buscarGrupo(ls, nombre);
    if (i == -1) {
        salida.error("rmgrp: el grupo " + nombre + " no existe");
        return;
    }

    // eliminar poner el id en 0 conservando la linea
    ls[i] = "0, G, " + nombre;

    escribirArchivo(m->ruta, sb, inodoUsers, nodo, unir(ls));
    salida.exito("rmgrp: grupo " + nombre + " eliminado");
}

// ---------------- MKUSR ----------------
void cmdMkusr(const Parametros &p, Salida &salida) {
    if (!exigeRoot("mkusr", salida)) return;
    std::string usuario = sinComillas(p.obtener("-user"));
    std::string pass    = sinComillas(p.obtener("-pass"));
    std::string grupo   = sinComillas(p.obtener("-grp"));

    const Montaje *m; SuperBloque sb; int inodoUsers; Inodo nodo;
    std::vector<std::string> ls;
    if (!abrirParaEditar("mkusr", salida, m, sb, inodoUsers, nodo, ls)) return;

    if (buscarGrupo(ls, grupo) == -1) {
        salida.error("mkusr: el grupo " + grupo + " no existe");
        return;
    }
    if (buscarUsuario(ls, usuario) != -1) {
        salida.error("mkusr: el usuario " + usuario + " ya existe");
        return;
    }

    int uid = siguienteId(ls, 'U');
    ls.push_back(std::to_string(uid) + ", U, " + grupo + ", " + usuario + ", " + pass);

    escribirArchivo(m->ruta, sb, inodoUsers, nodo, unir(ls));
    salida.exito("mkusr: usuario " + usuario + " creado (uid " + std::to_string(uid) + ")");
}

// ---------------- RMUSR ----------------
void cmdRmusr(const Parametros &p, Salida &salida) {
    if (!exigeRoot("rmusr", salida)) return;
    std::string usuario = sinComillas(p.obtener("-user"));

    const Montaje *m; SuperBloque sb; int inodoUsers; Inodo nodo;
    std::vector<std::string> ls;
    if (!abrirParaEditar("rmusr", salida, m, sb, inodoUsers, nodo, ls)) return;

    int i = buscarUsuario(ls, usuario);
    if (i == -1) {
        salida.error("rmusr: el usuario " + usuario + " no existe");
        return;
    }

    // poner el uid en 0 conservando el resto de la linea
    std::vector<std::string> c = campos(ls[i]);
    ls[i] = "0, U, " + c[2] + ", " + c[3] + ", " + c[4];

    escribirArchivo(m->ruta, sb, inodoUsers, nodo, unir(ls));
    salida.exito("rmusr: usuario " + usuario + " eliminado");
}

// ---------------- CHGRP ----------------
void cmdChgrp(const Parametros &p, Salida &salida) {
    if (!exigeRoot("chgrp", salida)) return;
    std::string usuario = sinComillas(p.obtener("-user"));
    std::string grupo   = sinComillas(p.obtener("-grp"));

    const Montaje *m; SuperBloque sb; int inodoUsers; Inodo nodo;
    std::vector<std::string> ls;
    if (!abrirParaEditar("chgrp", salida, m, sb, inodoUsers, nodo, ls)) return;

    int iu = buscarUsuario(ls, usuario);
    if (iu == -1) {
        salida.error("chgrp: el usuario " + usuario + " no existe");
        return;
    }
    if (buscarGrupo(ls, grupo) == -1) {
        salida.error("chgrp: el grupo " + grupo + " no existe");
        return;
    }

    std::vector<std::string> c = campos(ls[iu]);
    ls[iu] = c[0] + ", U, " + grupo + ", " + c[3] + ", " + c[4];

    escribirArchivo(m->ruta, sb, inodoUsers, nodo, unir(ls));
    salida.exito("chgrp: " + usuario + " cambiado al grupo " + grupo);
}
