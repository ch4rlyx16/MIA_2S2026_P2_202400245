#ifndef UTIL_TEXTO_H
#define UTIL_TEXTO_H

#include <cstddef>
#include <ctime>
#include <string>

/* Convierte un campo char[N] del disco a std::string. Los campos no
   siempre terminan en '\0', por eso se limita a los N bytes. */
std::string aTexto(const char *campo, size_t maximo);

/* Copia un valor a un campo char[N], rellenando el resto con ceros.
   Trunca si el valor no cabe. */
void copiarCampo(char *destino, size_t maximo, const std::string &valor);

/* Quita las comillas dobles que el lexer conserva en los valores. */
std::string sinComillas(const std::string &texto);

std::string aMinusculas(const std::string &texto);
std::string aMayusculas(const std::string &texto);

/* Formatea un time_t como "AAAA-MM-DD HH:MM" para los reportes. */
std::string fechaTexto(time_t momento);

#endif
