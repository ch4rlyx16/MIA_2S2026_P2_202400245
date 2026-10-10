import { useEffect, useRef, useState } from "react";

import { pedirDiscos, pedirParticiones, pedirReporte } from "./api";

const TIPOS = [
    { valor: "mbr",      texto: "MBR" },
    { valor: "disk",     texto: "Disco" },
    { valor: "sb",       texto: "Superbloque" },
    { valor: "inode",    texto: "Inodos" },
    { valor: "block",    texto: "Bloques" },
    { valor: "bm_inode", texto: "Bitmap de inodos" },
    { valor: "bm_block", texto: "Bitmap de bloques" },
    { valor: "tree",     texto: "Arbol del sistema" },
    { valor: "ls",       texto: "Listado de una carpeta" },
    { valor: "file",     texto: "Contenido de un archivo" }
];

// estos salen en texto plano asi que no se les elige formato
const DE_TEXTO = ["bm_inode", "bm_block", "file"];

// estos necesitan una ruta dentro de la particion
const CON_RUTA = ["ls", "file"];

export default function Reportes({ sesion }) {
    const [particiones, setParticiones] = useState([]);
    const [id,       setId]       = useState(sesion?.id ?? "");
    const [tipo,     setTipo]     = useState("mbr");
    const [ruta,     setRuta]     = useState("/");
    const [formato,  setFormato]  = useState("png");
    const [salida,   setSalida]   = useState(null);
    const [error,    setError]    = useState(null);
    const [generando, setGenerando] = useState(false);

    const urlActual = useRef(null);

    const esTexto = DE_TEXTO.includes(tipo);
    const pideRuta = CON_RUTA.includes(tipo);

    // las particiones montadas son las unicas que se pueden reportar
    useEffect(() => {
        let cancelado = false;

        (async () => {
            try {
                const discos = await pedirDiscos();
                const listas = await Promise.all(
                    discos.map((d) => pedirParticiones(d.ruta).catch(() => []))
                );
                const montadas = listas.flat().filter((p) => p.id);
                if (cancelado) return;

                setParticiones(montadas);
                // si la de la sesion ya no esta montada se cae a la primera
                setId((actual) =>
                    montadas.some((p) => p.id === actual)
                        ? actual
                        : (montadas[0]?.id ?? ""));
            } catch {
                if (!cancelado) setParticiones([]);
            }
        })();

        return () => { cancelado = true; };
    }, []);

    // el navegador no libera solo las urls de los blob hay que revocarlas
    useEffect(() => {
        return () => {
            if (urlActual.current) URL.revokeObjectURL(urlActual.current);
        };
    }, []);

    function limpiarAnterior() {
        if (urlActual.current) {
            URL.revokeObjectURL(urlActual.current);
            urlActual.current = null;
        }
    }

    async function generar() {
        if (!id.trim()) return;

        setGenerando(true);
        setError(null);

        try {
            const { blob } = await pedirReporte(
                id.trim(), tipo, pideRuta ? ruta.trim() : "", formato);

            limpiarAnterior();
            const url = URL.createObjectURL(blob);
            urlActual.current = url;

            setSalida({
                url,
                texto: esTexto ? await blob.text() : null,
                // un pdf no se puede pintar en una etiqueta img
                incrustado: !esTexto && formato === "pdf",
                archivo: `${tipo}.${esTexto ? "txt" : formato}`
            });
        } catch (fallo) {
            limpiarAnterior();
            setSalida(null);
            setError(fallo.message);
        } finally {
            setGenerando(false);
        }
    }

    return (
        <div className="reportes">
            <div className="formulario">
                <label>
                    Particion
                    <select value={id} onChange={(e) => setId(e.target.value)}>
                        {!particiones.length && <option value="">sin particiones montadas</option>}
                        {particiones.map((p) => (
                            <option key={p.id} value={p.id}>{p.id} &middot; {p.nombre}</option>
                        ))}
                    </select>
                </label>

                <label>
                    Reporte
                    <select value={tipo} onChange={(e) => setTipo(e.target.value)}>
                        {TIPOS.map((t) => (
                            <option key={t.valor} value={t.valor}>{t.texto}</option>
                        ))}
                    </select>
                </label>

                {pideRuta && (
                    <label className="ancho">
                        Ruta dentro de la particion
                        <input
                            value={ruta}
                            onChange={(e) => setRuta(e.target.value)}
                            placeholder={tipo === "ls" ? "/home" : "/home/notas.txt"}
                        />
                    </label>
                )}

                {!esTexto && (
                    <label>
                        Formato
                        <select value={formato} onChange={(e) => setFormato(e.target.value)}>
                            <option value="png">PNG</option>
                            <option value="jpg">JPG</option>
                            <option value="svg">SVG</option>
                            <option value="pdf">PDF</option>
                        </select>
                    </label>
                )}

                <button onClick={generar} disabled={generando || !id.trim()}>
                    {generando ? "Generando..." : "Generar"}
                </button>
            </div>

            {error && <p className="error">{error}</p>}

            {salida && (
                <>
                    <p className="meta">
                        <a href={salida.url} download={salida.archivo}>
                            Descargar {salida.archivo}
                        </a>
                    </p>

                    {salida.texto !== null && (
                        <pre className="consola salida">{salida.texto}</pre>
                    )}

                    {salida.incrustado && (
                        <iframe className="visor" src={salida.url} title={`Reporte ${tipo}`} />
                    )}

                    {salida.texto === null && !salida.incrustado && (
                        <div className="lienzo">
                            <img src={salida.url} alt={`Reporte ${tipo}`} />
                        </div>
                    )}
                </>
            )}
        </div>
    );
}
