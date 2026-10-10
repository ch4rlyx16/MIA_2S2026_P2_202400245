// conexion con el backend

const URL_API = import.meta.env.VITE_API_URL ?? "http://localhost:8080";


async function peticion(ruta, opciones) {
    let respuesta;

    try {
        respuesta = await fetch(`${URL_API}${ruta}`, opciones);
    } catch {
        throw new Error(
            `No se pudo conectar con el backend en ${URL_API}. ` +
            "Verifica que el servidor este corriendo."
        );
    }

    if (!respuesta.ok) {
        throw new Error(`El servidor respondio con el codigo ${respuesta.status}.`);
    }

    const datos = await respuesta.json();
    if (datos.error) throw new Error(datos.error);

    return datos;
}

// arma la query string sin preocuparse por las barras de las rutas
function obtener(ruta, parametros = {}) {
    const query = new URLSearchParams(parametros).toString();
    return peticion(query ? `${ruta}?${query}` : ruta);
}

function enviar(ruta, cuerpo = {}) {
    return peticion(ruta, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify(cuerpo)
    });
}

/* ---------- terminal ---------- */

// manda los comandos y devuelve las lineas de salida
export async function ejecutarComandos(comandos) {
    const datos = await enviar("/ejecutar", { comandos });
    return datos.salida ?? [];
}

/* ---------- sesion ---------- */

export function login(id, usuario, pass) {
    return enviar("/login", { id, usuario, pass });
}

export function logout() {
    return enviar("/logout");
}

export function pedirSesion() {
    return obtener("/sesion");
}

/* ---------- visualizador ---------- */

export async function pedirDiscos() {
    const datos = await obtener("/discos");
    return datos.discos ?? [];
}

export async function pedirParticiones(rutaDisco) {
    const datos = await obtener("/particiones", { disco: rutaDisco });
    return datos.particiones ?? [];
}

export async function pedirCarpeta(id, path = "/") {
    const datos = await obtener("/carpeta", { id, path });
    return datos.contenido ?? [];
}

export function pedirArchivo(id, path) {
    return obtener("/archivo", { id, path });
}

export function pedirJournaling(id) {
    return obtener("/journaling", { id });
}