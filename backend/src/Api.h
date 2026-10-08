#ifndef API_H
#define API_H

#include <nlohmann/json.hpp>
#include <string>

/* ============================================================
   Datos que el visualizador del frontend necesita.
   A diferencia de /ejecutar, que devuelve texto para la terminal,
   aqui todo sale como JSON estructurado.
   ============================================================ */

nlohmann::json apiDiscos();
nlohmann::json apiParticiones(const std::string &rutaDisco);

nlohmann::json apiLogin(const std::string &id, const std::string &usuario,
                        const std::string &pass);
nlohmann::json apiLogout();
nlohmann::json apiSesion();

nlohmann::json apiCarpeta(const std::string &id, const std::string &ruta);
nlohmann::json apiArchivo(const std::string &id, const std::string &ruta);
nlohmann::json apiJournaling(const std::string &id);

#endif
