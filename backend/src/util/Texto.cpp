#include "Texto.h"

#include <algorithm>
#include <cstring>
#include <ctime>

std::string aTexto(const char *campo, size_t maximo) {
    size_t largo = 0;
    while (largo < maximo && campo[largo] != '\0') ++largo;
    return std::string(campo, largo);
}

void copiarCampo(char *destino, size_t maximo, const std::string &valor) {
    std::memset(destino, 0, maximo);
    std::memcpy(destino, valor.c_str(), std::min(maximo, valor.size()));
}

std::string sinComillas(const std::string &texto) {
    if (texto.size() >= 2 && texto.front() == '"' && texto.back() == '"')
        return texto.substr(1, texto.size() - 2);
    return texto;
}

std::string aMinusculas(const std::string &texto) {
    std::string copia = texto;
    std::transform(copia.begin(), copia.end(), copia.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return copia;
}

std::string aMayusculas(const std::string &texto) {
    std::string copia = texto;
    std::transform(copia.begin(), copia.end(), copia.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return copia;
}

std::string fechaTexto(time_t momento) {
    char buffer[32];
    struct tm desglose;
    localtime_r(&momento, &desglose);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &desglose);
    return std::string(buffer);
}
