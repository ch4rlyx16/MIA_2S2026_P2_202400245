import { useEffect, useState } from "react";

import {
    pedirArchivo,
    pedirCarpeta,
    pedirDiscos,
    pedirJournaling,
    pedirParticiones
} from "./api";

// une una ruta con un nombre sin duplicar la barra
function unir(ruta, nombre) {
    return ruta === "/" ? "/" + nombre : ruta + "/" + nombre;
}

function tamano(bytes) {
    if (bytes < 1024) return bytes + " B";
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB";
    return (bytes / (1024 * 1024)).toFixed(1) + " MB";
}

export default function Visualizador() {
    const [vista,     setVista]     = useState("discos");
    const [disco,     setDisco]     = useState(null);
    const [particion, setParticion] = useState(null);
    const [ruta,      setRuta]      = useState("/");
    const [datos,     setDatos]     = useState({});
    const [error,     setError]     = useState(null);
    const [mensaje,   setMensaje]   = useState(null);
    const [cargando,  setCargando]  = useState(false);
    const [refresco,  setRefresco]  = useState(0);

    useEffect(() => {
        let cancelado = false;
        setCargando(true);
        setError(null);
        setMensaje(null);

        const cargar = async () => {
            if (vista === "particiones") return { lista:   await pedirParticiones(disco.ruta) };
            if (vista === "carpeta")     return { lista:   await pedirCarpeta(particion.id, ruta) };
            if (vista === "archivo")     return { archivo: await pedirArchivo(particion.id, ruta) };
            if (vista === "journaling")  return { journal: await pedirJournaling(particion.id) };
            return { lista: await pedirDiscos() };
        };

        cargar()
            .then((d)  => { if (!cancelado) setDatos(d); })
            .catch((e) => { if (!cancelado) { setDatos({}); setError(e.message); } })
            .finally(() => { if (!cancelado) setCargando(false); });

        // si navega rapido la respuesta vieja ya no sirve y no debe pintarse
        return () => { cancelado = true; };
    }, [vista, disco, particion, ruta, refresco]);

    /* ---------- navegacion ---------- */

    function irADiscos() {
        setDisco(null);
        setParticion(null);
        setRuta("/");
        setVista("discos");
    }

    function abrirDisco(d) {
        setDisco(d);
        setParticion(null);
        setRuta("/");
        setVista("particiones");
    }

    function abrirParticion(p) {
        // sin id no se puede leer y hay dos motivos distintos para no tenerlo
        // es un aviso y no un error de carga asi que la tabla se queda
        if (p.tipo !== "Primaria") {
            setMensaje(`las particiones ${p.tipo.toLowerCase()}s no se montan, solo las primarias`);
            return;
        }
        if (!p.id) {
            setMensaje(`la particion ${p.nombre} no esta montada, montala desde la terminal`);
            return;
        }
        setMensaje(null);
        setParticion(p);
        setRuta("/");
        setVista("carpeta");
    }

    function abrirEntrada(e) {
        setRuta(unir(ruta, e.nombre));
        setVista(e.tipo === "carpeta" ? "carpeta" : "archivo");
    }

    const segmentos = ruta === "/" ? [] : ruta.slice(1).split("/");

    // -1 es la raiz de la particion
    function irASegmento(indice) {
        setRuta(indice < 0 ? "/" : "/" + segmentos.slice(0, indice + 1).join("/"));
        setVista("carpeta");
    }

    /* ---------- partes ---------- */

    const miga = (
        <nav className="miga">
            <button onClick={irADiscos}>Discos</button>

            {disco && (
                <>
                    {" / "}
                    <button onClick={() => abrirDisco(disco)}>{disco.nombre}</button>
                </>
            )}

            {particion && (
                <>
                    {" / "}
                    <button onClick={() => irASegmento(-1)}>{particion.nombre}</button>
                </>
            )}

            {particion && segmentos.map((s, i) => (
                <span key={i}>
                    {" / "}
                    {vista === "archivo" && i === segmentos.length - 1
                        ? <span className="actual">{s}</span>
                        : <button onClick={() => irASegmento(i)}>{s}</button>}
                </span>
            ))}
        </nav>
    );

    const lista = datos.lista ?? [];

    function contenido() {
        if (cargando) return <p className="aviso">Cargando...</p>;
        if (error)    return <p className="error">{error}</p>;

        if (vista === "discos") {
            if (!lista.length) {
                return <p className="aviso">No hay discos todavia, crea uno con mkdisk desde la terminal</p>;
            }
            return (
                <table className="tabla">
                    <thead>
                        <tr><th>Disco</th><th>Tamano</th><th>Fit</th><th>Creado</th><th>Montadas</th></tr>
                    </thead>
                    <tbody>
                        {lista.map((d) => (
                            <tr key={d.ruta} className="clicable" onClick={() => abrirDisco(d)}>
                                <td className="nombre">{d.nombre}</td>
                                <td>{tamano(d.tamano)}</td>
                                <td>{d.fit}</td>
                                <td>{d.fecha}</td>
                                <td>{d.montadas}</td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            );
        }

        if (vista === "particiones") {
            if (!lista.length) {
                return <p className="aviso">El disco no tiene particiones, crea una con fdisk</p>;
            }
            return (
                <table className="tabla">
                    <thead>
                        <tr><th>Particion</th><th>Tipo</th><th>Tamano</th><th>Fit</th><th>Estado</th><th>Id</th></tr>
                    </thead>
                    <tbody>
                        {lista.map((p, i) => (
                            <tr key={i} className={p.id ? "clicable" : "inerte"}
                                onClick={() => abrirParticion(p)}>
                                <td className="nombre">{p.nombre}</td>
                                <td>{p.tipo}</td>
                                <td>{tamano(p.tamano)}</td>
                                <td>{p.fit}</td>
                                <td>{p.estado}</td>
                                <td className="mono">{p.id || "-"}</td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            );
        }

        if (vista === "carpeta") {
            if (!lista.length) return <p className="aviso">La carpeta esta vacia</p>;
            return (
                <table className="tabla">
                    <thead>
                        <tr><th>Nombre</th><th>Tipo</th><th>Tamano</th><th>Permisos</th><th>UID</th><th>GID</th><th>Modificado</th></tr>
                    </thead>
                    <tbody>
                        {lista.map((e) => (
                            <tr key={e.nombre} className="clicable" onClick={() => abrirEntrada(e)}>
                                <td className="nombre">
                                    {e.tipo === "carpeta" ? "\u{1F4C1}" : "\u{1F4C4}"} {e.nombre}
                                </td>
                                <td>{e.tipo}</td>
                                <td>{e.tipo === "carpeta" ? "-" : tamano(e.tamano)}</td>
                                <td className="mono">{e.permisos} <span className="octal">{e.octal}</span></td>
                                <td>{e.uid}</td>
                                <td>{e.gid}</td>
                                <td>{e.modificado}</td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            );
        }

        if (vista === "archivo") {
            const a = datos.archivo;
            if (!a) return null;
            return (
                <>
                    <p className="meta">
                        <span className="mono">{a.permisos}</span>
                        {" · "}{tamano(a.tamano)}
                        {" · uid "}{a.uid}
                        {" · "}{a.modificado}
                    </p>
                    <pre className="consola salida">{a.contenido || "(archivo vacio)"}</pre>
                </>
            );
        }

        if (vista === "journaling") {
            const j = datos.journal;
            if (!j) return null;
            if (!j.entradas.length) return <p className="aviso">La bitacora esta vacia</p>;
            return (
                <>
                    <p className="meta">{j.entradas.length} de {j.maximo} entradas usadas</p>
                    <table className="tabla">
                        <thead>
                            <tr><th>No</th><th>Operacion</th><th>Path</th><th>Contenido</th><th>Fecha</th></tr>
                        </thead>
                        <tbody>
                            {j.entradas.map((e) => (
                                <tr key={e.numero}>
                                    <td>{e.numero}</td>
                                    <td className="mono">{e.operacion}</td>
                                    <td className="mono">{e.path}</td>
                                    <td className="recortado">{e.contenido || "-"}</td>
                                    <td>{e.fecha}</td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </>
            );
        }

        return null;
    }

    return (
        <div className="visualizador">
            <div className="barra">
                {miga}
                <div className="herramientas">
                    {particion && (
                        <button
                            className={vista === "journaling" ? "activa" : ""}
                            onClick={() => setVista(vista === "journaling" ? "carpeta" : "journaling")}
                        >
                            Journaling
                        </button>
                    )}
                    <button onClick={() => setRefresco((n) => n + 1)}>Recargar</button>
                </div>
            </div>

            {mensaje && <p className="error">{mensaje}</p>}
            {contenido()}
        </div>
    );
}
