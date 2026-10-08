#!/bin/bash
set -e

ANTLR_JAR=/usr/local/lib/antlr-4.10.1-complete.jar
cd "$(dirname "$0")"

echo ">> Generando analizador lexico..."
cd grammar
java -jar "$ANTLR_JAR" -Dlanguage=Cpp -o ../generated Ext2Lexer.g4

echo ">> Generando analizador sintactico..."
java -jar "$ANTLR_JAR" -Dlanguage=Cpp -lib ../generated -o ../generated Ext2Parser.g4
cd ..

FLAGS="-std=c++17 -Wall -I/usr/include/antlr4-runtime -Igenerated \
       -Isrc -Isrc/comandos -Isrc/estructuras -Isrc/util -Isrc/disco -Isrc/reportes"

# Codigo compartido por los dos programas (todo menos los dos main).
# Se compila una sola vez a objetos .o para no repetir el trabajo.
echo ">> Compilando codigo comun..."
mkdir -p build
OBJETOS=""

# Codigo generado por ANTLR
for fuente in generated/*.cpp; do
    objeto="build/$(basename "$fuente" .cpp).o"
    g++ $FLAGS -c "$fuente" -o "$objeto"
    OBJETOS="$OBJETOS $objeto"
done

# Codigo propio (analizador, ejecutor, comandos, utilidades).
# Se excluyen main.cpp y servidor.cpp porque son los puntos de entrada.
for fuente in src/Analizador.cpp src/Validador.cpp src/Ejecutor.cpp src/Api.cpp \
              src/comandos/*.cpp src/util/*.cpp src/disco/*.cpp src/reportes/*.cpp; do
    objeto="build/$(basename "$fuente" .cpp).o"
    g++ $FLAGS -c "$fuente" -o "$objeto"
    OBJETOS="$OBJETOS $objeto"
done

echo ">> Enlazando 'consola'..."
g++ $FLAGS src/main.cpp $OBJETOS -lantlr4-runtime -o consola

echo ">> Enlazando 'servidor' (API REST)..."
g++ $FLAGS src/servidor.cpp $OBJETOS \
    -lantlr4-runtime -lcpp-httplib -lpthread -o servidor

echo ""
echo ">> Listo."
echo "   Consola : ./consola pruebas/entrada.smia"
echo "   Servidor: ./servidor"
