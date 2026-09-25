#include "Montaje.h"

#include "../estructuras/Estructuras.h"
#include "../util/Archivo.h"
#include "../util/Texto.h"

#include <map>

namespace {


const std::string CARNET = "45";

std::vector<Montaje> tabla;

/* Letra ya asignada a cada disco, por ruta. */
std::map<std::string, char> letraPorDisco;

/* Siguiente correlativo disponible dentro de cada disco. */
std::map<char, int> siguienteNumero;

char proximaLetra() {
    return static_cast<char>('A' + letraPorDisco.size());
}

} // namespace

const Montaje *buscarMontaje(const std::string &id) {
    for (const Montaje &m : tabla)
        if (m.id == id) return &m;
    return nullptr;
}

const std::vector<Montaje> &montajes() { return tabla; }

/* ============================================================
   MOUNT
   ============================================================ */
void cmdMount(const Parametros &p, Salida &salida) {
    std::string ruta   = sinComillas(p.obtener("-path"));
    std::string nombre = sinComillas(p.obtener("-name"));

    if (!existeArchivo(ruta)) {
        salida.error("mount: no existe el disco " + ruta);
        return;
    }

    MBR mbr;
    if (!leerDe(ruta, 0, mbr)) {
        salida.error("mount: no se pudo leer el MBR de " + ruta);
        return;
    }

    /* Solo se montan primarias: es lo que pide el enunciado. */
    int indice = -1;
    for (int i = 0; i < 4; ++i) {
        const Particion &particion = mbr.mbr_partitions[i];
        if (particion.part_status == '0') continue;
        if (aTexto(particion.part_name, 16) != nombre) continue;

        if (particion.part_type != 'P') {
            salida.error("mount: " + nombre + " no es una particion primaria; "
                         "solo se montan primarias");
            return;
        }
        indice = i;
        break;
    }
    if (indice == -1) {
        salida.error("mount: no existe la particion " + nombre + " en " + ruta);
        return;
    }

    Particion &particion = mbr.mbr_partitions[indice];

    // el disco puede decir montada de una corrida anterior pero la tabla
    // vive en RAM asi que solo es error si tambien esta en la tabla
    if (particion.part_status == '2' &&
        buscarMontaje(aTexto(particion.part_id, 4)) != nullptr) {
        salida.error("mount: la particion " + nombre + " ya esta montada con id " +
                     aTexto(particion.part_id, 4));
        return;
    }

    /* La letra depende del disco y el numero del orden dentro de ese
       disco. Un disco nuevo estrena letra y reinicia el numero en 1 */
    auto it = letraPorDisco.find(ruta);
    if (it == letraPorDisco.end()) {
        char letra = proximaLetra();
        letraPorDisco[ruta]     = letra;
        siguienteNumero[letra]  = 1;
        it = letraPorDisco.find(ruta);
    }
    char letra  = it->second;
    int  numero = siguienteNumero[letra]++;

    std::string id = CARNET + std::to_string(numero) + std::string(1, letra);
    
    particion.part_status      = '2';
    particion.part_correlative = numero;
    copiarCampo(particion.part_id, 4, id);

    if (!escribirEn(ruta, 0, mbr)) {
        salida.error("mount: no se pudo actualizar el MBR de " + ruta);
        return;
    }

    tabla.push_back({ id, ruta, nombre,
                      particion.part_start, particion.part_s, letra, numero });

    salida.exito("mount: particion " + nombre + " montada con id " + id);
}

/* ============================================================
   MOUNTED
   ============================================================ */
void cmdMounted(const Parametros &, Salida &salida) {
    if (tabla.empty()) {
        salida.escribir("mounted: no hay particiones montadas");
        return;
    }

    salida.escribir("ID      PARTICION        DISCO");
    for (const Montaje &m : tabla) {
        std::string fila = m.id;
        fila.resize(8, ' ');
        std::string nombre = m.nombre;
        nombre.resize(17, ' ');
        salida.escribir(fila + nombre + m.ruta);
    }
}
