/* API REST del proyecto.
   Recibe un bloque de comandos por POST y devuelve la salida ya
   ejecutada, linea por linea, para que el frontend la muestre en su
   area de salida. */

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

#include "Ejecutor.h"

using json = nlohmann::json;

int main() {
    httplib::Server servidor;

    servidor.set_default_headers({
        { "Access-Control-Allow-Origin",  "*" },
        { "Access-Control-Allow-Headers", "Content-Type" },
        { "Access-Control-Allow-Methods", "POST, OPTIONS" }
    });

    /* El navegador manda un OPTIONS de sondeo antes del POST real. */
    servidor.Options("/ejecutar",
        [](const httplib::Request &, httplib::Response &res) {
            res.status = 204;
        });

    servidor.Post("/ejecutar",
        [](const httplib::Request &req, httplib::Response &res) {
            std::string comandos;
            try {
                json cuerpo = json::parse(req.body);
                comandos = cuerpo.value("comandos", "");
            } catch (const std::exception &) {
                res.status = 400;
                res.set_content(
                    json{{ "error", "El cuerpo no es un JSON valido" }}.dump(),
                    "application/json");
                return;
            }

            Salida salida = ejecutarTexto(comandos);

            json lineas = json::array();
            for (const std::string &l : salida.lineas)
                lineas.push_back(l);

            res.set_content(json{{ "salida", lineas }}.dump(),
                            "application/json");
        });

    std::cout << "Servidor escuchando en http://localhost:8080\n";
    std::cout << "  POST /ejecutar   {\"comandos\": \"mkdisk -size=100 ...\"}\n";
    std::cout << "  Ctrl+C para detener.\n";

    servidor.listen("0.0.0.0", 8080);
    return 0;
}
