/* API REST del proyecto.
   /ejecutar devuelve texto para la terminal; el resto son endpoints
   con JSON estructurado que alimentan el visualizador del frontend. */

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

#include "Api.h"
#include "Ejecutor.h"

using json = nlohmann::json;

namespace {

void responder(httplib::Response &res, const json &cuerpo) {
    res.set_content(cuerpo.dump(), "application/json");
}

// lee un parametro de la url, devuelve el valor por defecto si no vino
std::string parametro(const httplib::Request &req, const std::string &nombre,
                      const std::string &pordefecto = "") {
    return req.has_param(nombre.c_str()) ? req.get_param_value(nombre.c_str())
                                         : pordefecto;
}

} // namespace

int main() {
    httplib::Server servidor;

    servidor.set_default_headers({
        { "Access-Control-Allow-Origin",  "*" },
        { "Access-Control-Allow-Headers", "Content-Type" },
        { "Access-Control-Allow-Methods", "GET, POST, OPTIONS" }
    });

    /* el navegador manda un OPTIONS de sondeo antes de cada POST */
    servidor.Options(".*", [](const httplib::Request &, httplib::Response &res) {
        res.status = 204;
    });

    /* ---------- terminal ---------- */
    servidor.Post("/ejecutar",
        [](const httplib::Request &req, httplib::Response &res) {
            std::string comandos;
            try {
                json cuerpo = json::parse(req.body);
                comandos = cuerpo.value("comandos", "");
            } catch (const std::exception &) {
                res.status = 400;
                responder(res, json{{ "error", "El cuerpo no es un JSON valido" }});
                return;
            }

            Salida salida = ejecutarTexto(comandos);

            json lineas = json::array();
            for (const std::string &l : salida.lineas) lineas.push_back(l);

            responder(res, json{{ "salida", lineas }});
        });

    /* ---------- sesion ---------- */
    servidor.Post("/login",
        [](const httplib::Request &req, httplib::Response &res) {
            try {
                json c = json::parse(req.body);
                responder(res, apiLogin(c.value("id", ""),
                                        c.value("usuario", ""),
                                        c.value("pass", "")));
            } catch (const std::exception &) {
                res.status = 400;
                responder(res, json{{ "error", "El cuerpo no es un JSON valido" }});
            }
        });

    servidor.Post("/logout", [](const httplib::Request &, httplib::Response &res) {
        responder(res, apiLogout());
    });

    servidor.Get("/sesion", [](const httplib::Request &, httplib::Response &res) {
        responder(res, apiSesion());
    });

    /* ---------- visualizador ---------- */
    servidor.Get("/discos", [](const httplib::Request &, httplib::Response &res) {
        responder(res, apiDiscos());
    });

    servidor.Get("/particiones",
        [](const httplib::Request &req, httplib::Response &res) {
            responder(res, apiParticiones(parametro(req, "disco")));
        });

    servidor.Get("/carpeta",
        [](const httplib::Request &req, httplib::Response &res) {
            responder(res, apiCarpeta(parametro(req, "id"),
                                      parametro(req, "path", "/")));
        });

    servidor.Get("/archivo",
        [](const httplib::Request &req, httplib::Response &res) {
            responder(res, apiArchivo(parametro(req, "id"),
                                      parametro(req, "path")));
        });

    servidor.Get("/journaling",
        [](const httplib::Request &req, httplib::Response &res) {
            responder(res, apiJournaling(parametro(req, "id")));
        });

    /* devuelve la imagen misma no una ruta: el navegador no puede abrir
       un archivo que vive en el disco del servidor */
    servidor.Get("/reporte",
        [](const httplib::Request &req, httplib::Response &res) {
            std::string datos, mime, error;

            if (!apiReporte(parametro(req, "id"),
                            parametro(req, "name"),
                            parametro(req, "path_file_ls"),
                            parametro(req, "formato", "png"),
                            datos, mime, error)) {
                res.status = 400;
                responder(res, json{{ "error", error }});
                return;
            }

            res.set_content(datos, mime.c_str());
        });

    std::cout << "Servidor escuchando en http://localhost:8080\n";
    std::cout << "  POST /ejecutar  /login  /logout\n";
    std::cout << "  GET  /sesion  /discos  /particiones  /carpeta  /archivo\n";
    std::cout << "       /journaling  /reporte\n";
    std::cout << "  Ctrl+C para detener.\n";

    servidor.listen("0.0.0.0", 8080);
    return 0;
}
