#ifndef COMANDOS_SESION_H
#define COMANDOS_SESION_H

#include "Comando.h"

#include <string>

// sesion activa vive en RAM solo puede haber una a la vez
struct Sesion {
    bool        activa = false;
    std::string id;        // id de la particion montada
    int         uid = 0;
    int         gid = 0;
    std::string usuario;
    std::string grupo;
};

const Sesion &sesionActual();
// valida credenciales
bool autenticar(const std::string &id, const std::string &usuario,
                const std::string &pass, Sesion &resultado, std::string &motivo);
void cmdLogin (const Parametros &p, Salida &salida);
void cmdLogout(const Parametros &p, Salida &salida);

#endif
