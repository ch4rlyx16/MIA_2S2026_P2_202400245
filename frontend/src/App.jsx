import { useCallback, useEffect, useState } from "react";

import { logout, pedirSesion } from "./api";
import Login from "./Login";
import Reportes from "./Reportes";
import Terminal from "./Terminal";
import Visualizador from "./Visualizador";
import "./App.css";

export default function App() {
    const [sesion,   setSesion]   = useState(null);
    const [seccion,  setSeccion]  = useState("terminal");
    const [cargando, setCargando] = useState(true);

    // la sesion vive en el backend asi que la fuente de verdad es el
    // no se guarda nada en el navegador
    const refrescarSesion = useCallback(async () => {
        try {
            const s = await pedirSesion();
            setSesion(s.activa ? s : null);
        } catch {
            setSesion(null);
        }
    }, []);

    // al abrir se pregunta si ya habia una sesion viva
    useEffect(() => {
        refrescarSesion().finally(() => setCargando(false));
    }, [refrescarSesion]);

    function entrar(datos) {
        setSesion(datos);
    }

    async function salir() {
        try {
            await logout();
        } catch {
            // si ya estaba cerrada da igual
        }
        setSesion(null);
        setSeccion("terminal");
    }

    if (cargando) {
        return <p className="cargando">Conectando con el backend...</p>;
    }

    return (
        <div className="aplicacion">
            <header className="cabecera">
                <div>
                    <h1>ExtreamFS &middot; Sistema de archivos EXT2 y EXT3</h1>
                    <p className="subtitulo">Proyecto 2 &middot; Carne 202400245</p>
                </div>

                {sesion && (
                    <div className="sesion-activa">
                        <span className="quien">
                            {sesion.usuario} &middot; {sesion.grupo} &middot; {sesion.id}
                        </span>
                        <button onClick={salir}>Cerrar sesion</button>
                    </div>
                )}
            </header>

            <nav className="pestanas">
                <button
                    className={seccion === "terminal" ? "activa" : ""}
                    onClick={() => setSeccion("terminal")}
                >
                    Terminal
                </button>
                <button
                    className={seccion === "visualizador" ? "activa" : ""}
                    onClick={() => setSeccion("visualizador")}
                >
                    Visualizador
                </button>
                <button
                    className={seccion === "reportes" ? "activa" : ""}
                    onClick={() => setSeccion("reportes")}
                >
                    Reportes
                </button>
            </nav>

            {seccion === "terminal" && <Terminal alTerminar={refrescarSesion} />}

            {/* la terminal es la unica que no pide sesion porque sin ella
                no habria forma de crear el primer disco */}
            {seccion !== "terminal" && !sesion && (
                <>
                    <p className="aviso">
                        Inicia sesion para explorar el sistema de archivos.
                        Si todavia no tienes particiones creala desde la terminal
                        con mkdisk fdisk mount y mkfs
                    </p>
                    <Login alEntrar={entrar} />
                </>
            )}

            {seccion === "visualizador" && sesion && <Visualizador />}
            {seccion === "reportes"     && sesion && <Reportes sesion={sesion} />}
        </div>
    );
}
