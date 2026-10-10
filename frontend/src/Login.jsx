import { useState } from "react";

import { login } from "./api";

export default function Login({ alEntrar }) {
    const [id,       setId]       = useState("");
    const [usuario,  setUsuario]  = useState("");
    const [pass,     setPass]     = useState("");
    const [error,    setError]    = useState(null);
    const [entrando, setEntrando] = useState(false);

    const incompleto = !id.trim() || !usuario.trim() || !pass;

    async function entrar(evento) {
        evento.preventDefault();
        if (incompleto) return;

        setEntrando(true);
        setError(null);

        try {
            alEntrar(await login(id.trim(), usuario.trim(), pass));
        } catch (fallo) {
            setError(fallo.message);
            setPass("");   // la clave se limpia pero el usuario no
        } finally {
            setEntrando(false);
        }
    }

    return (
        <form className="login" onSubmit={entrar}>
            <h2>Iniciar sesion</h2>

            <label>
                Particion
                <input value={id} onChange={(e) => setId(e.target.value)}
                       placeholder="451A" autoComplete="off" />
            </label>

            <label>
                Usuario
                <input value={usuario} onChange={(e) => setUsuario(e.target.value)}
                       placeholder="root" autoComplete="off" />
            </label>

            <label>
                Contrasena
                <input type="password" value={pass}
                       onChange={(e) => setPass(e.target.value)} />
            </label>

            <button type="submit" disabled={entrando || incompleto}>
                {entrando ? "Entrando..." : "Entrar"}
            </button>

            {error && <p className="error">{error}</p>}
        </form>
    );
}