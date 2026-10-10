#include "Sesion.h"
#include "Montaje.h"
#include "Usuarios.h"

#include "../disco/Ext2.h"
#include "../util/Texto.h"

#include <sstream>
#include <vector>

namespace {

Sesion sesion;   // la unica sesion en RAM

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

} // namespace

const Sesion &sesionActual() { return sesion; }

// ---------------- LOGIN ----------------

// busca al usuario en users txt y deja la sesion armada en resultado
// no toca la global asi se puede validar sin perder la sesion que ya estaba
bool autenticar(const std::string &id, const std::string &usuario,
                const std::string &pass, Sesion &resultado, std::string &motivo) {
    std::string idMayus = aMayusculas(id);

    const Montaje *m = buscarMontaje(idMayus);
    if (m == nullptr) {
        motivo = "no hay ninguna particion montada con id " + idMayus;
        return false;
    }

    SuperBloque sb; int inodoUsers; Inodo nodo; std::string contenido;
    if (!leerUsersTxt(m, sb, inodoUsers, nodo, contenido)) {
        motivo = "la particion " + idMayus + " no esta formateada";
        return false;
    }

    std::stringstream flujo(contenido);
    std::string linea;
    while (std::getline(flujo, linea)) {
        std::vector<std::string> c = campos(linea);
        if (c.size() < 5 || c[1] != "U" || c[0] == "0") continue;
        if (c[3] != usuario || c[4] != pass) continue;

        // encontrado el gid se saca de la linea del grupo del usuario
        int gid = 0;
        std::stringstream flujo2(contenido);
        std::string linea2;
        while (std::getline(flujo2, linea2)) {
            std::vector<std::string> g = campos(linea2);
            if (g.size() >= 3 && g[1] == "G" && g[0] != "0" && g[2] == c[2]) {
                gid = std::atoi(g[0].c_str());
                break;
            }
        }

        resultado.activa  = true;
        resultado.id      = idMayus;
        resultado.uid     = std::atoi(c[0].c_str());
        resultado.gid     = gid;
        resultado.usuario = usuario;
        resultado.grupo   = c[2];
        return true;
    }

    motivo = "usuario o contrasena incorrectos";
    return false;
}

void cmdLogin(const Parametros &p, Salida &salida) {
    if (sesion.activa) {
        salida.error("login: ya hay una sesion activa, cierrela con logout");
        return;
    }

    std::string usuario = sinComillas(p.obtener("-user"));
    std::string pass    = sinComillas(p.obtener("-pass"));
    std::string id      = aMayusculas(sinComillas(p.obtener("-id")));

    Sesion nueva;
    std::string motivo;
    if (!autenticar(id, usuario, pass, nueva, motivo)) {
        salida.error("login: " + motivo);
        return;
    }

    sesion = nueva;
    salida.exito("login: sesion iniciada como " + usuario + " en " + id);
}

// ---------------- LOGOUT ----------------
void cmdLogout(const Parametros &, Salida &salida) {
    if (!sesion.activa) {
        salida.error("logout: no hay ninguna sesion activa");
        return;
    }
    salida.exito("logout: sesion de " + sesion.usuario + " cerrada");
    sesion = Sesion();
}
