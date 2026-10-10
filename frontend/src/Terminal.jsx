import { useRef, useState } from "react";

import { ejecutarComandos } from "./api";

export default function Terminal({ alTerminar }) {
    const [comandos,   setComandos]   = useState("");
    const [salida,     setSalida]     = useState([]);
    const [errorRed,   setErrorRed]   = useState(null);
    const [ejecutando, setEjecutando] = useState(false);
    const [archivo,    setArchivo]    = useState(null);

    const archivoRef = useRef(null);

    async function ejecutar() {
        if (!comandos.trim()) return;

        setEjecutando(true);
        setErrorRed(null);

        try {
            setSalida(await ejecutarComandos(comandos));
        } catch (fallo) {
            setSalida([]);
            setErrorRed(fallo.message);
        } finally {
            setEjecutando(false);
            // el script pudo traer un login o un logout asi que se revisa
            alTerminar?.();
        }
    }

    async function cargarArchivo(evento) {
        const seleccionado = evento.target.files?.[0];
        if (!seleccionado) return;

        setComandos(await seleccionado.text());
        setArchivo(seleccionado.name);
        setSalida([]);
        setErrorRed(null);
        evento.target.value = "";   // permite recargar el mismo archivo
    }

    function limpiar() {
        setComandos("");
        setSalida([]);
        setErrorRed(null);
        setArchivo(null);
    }

    // ctrl+enter ejecuta
    function alTeclear(e) {
        if (e.ctrlKey && e.key === "Enter") ejecutar();
    }

    return (
        <>
            <div className="acciones">
                <button onClick={() => archivoRef.current?.click()}>
                    Cargar archivo
                </button>
                <button onClick={ejecutar} disabled={ejecutando || !comandos.trim()}>
                    {ejecutando ? "Ejecutando..." : "Ejecutar"}
                </button>
                <button onClick={limpiar} disabled={!comandos && salida.length === 0}>
                    Limpiar
                </button>
                {archivo && <span className="nombre-archivo">{archivo}</span>}

                <input
                    ref={archivoRef}
                    type="file"
                    accept=".smia,.txt,text/plain"
                    onChange={cargarArchivo}
                    hidden
                />
            </div>

            <h2>Entrada</h2>
            <textarea
                className="consola entrada"
                value={comandos}
                onChange={(e) => setComandos(e.target.value)}
                onKeyDown={alTeclear}
                spellCheck={false}
                placeholder={"mkdisk -size=5 -unit=M -path=/home/user/Disco1.mia"}
            />

            <h2>Salida</h2>
            <pre className="consola salida">
                {errorRed
                    ? errorRed
                    : salida.length
                        ? salida.join("\n")
                        : "# Aca se veran los mensajes de la ejecucion"}
            </pre>

            <p className="nota">Ctrl + Enter para ejecutar</p>
        </>
    );
}
