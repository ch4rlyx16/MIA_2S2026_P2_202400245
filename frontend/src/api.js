// conexion con el backend

const URL_API = import.meta.env.VITE_API_URL ?? "http://localhost:8080";

// manda los comandos y devuelve las lineas de salida
export async function ejecutarComandos(comandos) {
    let respuesta;

    try {
        respuesta = await fetch(`${URL_API}/ejecutar`, {
            method:  "POST",
            headers: { "Content-Type": "application/json" },
            body:    JSON.stringify({ comandos })
        });
    } catch {
        throw new Error(
            `No se pudo conectar con el backend en ${URL_API}. ` +
            "Verifica que el servidor este corriendo (./servidor)."
        );
    }

    if (!respuesta.ok) {
        throw new Error(`El servidor respondio con el codigo ${respuesta.status}.`);
    }

    const datos = await respuesta.json();
    return datos.salida ?? [];
}
