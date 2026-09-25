#include "Analizador.h"
#include "Validador.h"
#include "util/Texto.h"

#include "antlr4-runtime.h"
#include "Ext2Lexer.h"
#include "Ext2Parser.h"

using namespace antlr4;

namespace {

/* recolectamos los errores de ANTLR en vez de imprimirlos  */
class RecolectorErrores : public BaseErrorListener {
public:
    std::vector<ErrorAnalisis> errores;
    size_t lineaReal = 1;

    void syntaxError(Recognizer *, Token *, size_t,
                     size_t columna, const std::string &mensaje,
                     std::exception_ptr) override {
        errores.push_back({ lineaReal, columna + 1, mensaje });
    }
};

} // namespace

AnalisisLinea analizarLinea(const std::string &texto, size_t numeroLinea) {
    AnalisisLinea resultado;
    resultado.parametros.linea = numeroLinea;

    /* ----- Fase 1: analisis lexico ----- */
    ANTLRInputStream flujo(texto);
    Ext2Lexer lexer(&flujo);

    RecolectorErrores erroresLexicos;
    erroresLexicos.lineaReal = numeroLinea;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&erroresLexicos);

    CommonTokenStream tokens(&lexer);
    tokens.fill();

    /* Una linea sin tokens no es un comando. */
    bool hayTokens = false;
    for (Token *token : tokens.getTokens())
        if (token->getType() != Token::EOF) { hayTokens = true; break; }

    if (!hayTokens) {
        resultado.errores = erroresLexicos.errores;
        return resultado;
    }
    resultado.hayComando = true;

    /* ----- Fase 2: analisis sintactico ----- */
    tokens.seek(0);
    Ext2Parser parser(&tokens);

    RecolectorErrores erroresSintacticos;
    erroresSintacticos.lineaReal = numeroLinea;
    parser.removeErrorListeners();
    parser.addErrorListener(&erroresSintacticos);

    Ext2Parser::ScriptContext *arbol = parser.script();

    resultado.errores = erroresLexicos.errores;
    for (const ErrorAnalisis &e : erroresSintacticos.errores)
        resultado.errores.push_back(e);


    if (!resultado.errores.empty()) return resultado;

    for (Ext2Parser::ComandoContext *comandoCtx : arbol->comando()) {
        if (comandoCtx->children.empty()) continue;

        auto *cmd = dynamic_cast<ParserRuleContext *>(comandoCtx->children[0]);
        if (cmd == nullptr) continue;

        resultado.parametros.comando =
            aMinusculas(cmd->getStart()->getText());
        resultado.parametros.columna =
            cmd->getStart()->getCharPositionInLine() + 1;

        for (tree::ParseTree *hijo : cmd->children) {
            auto *param = dynamic_cast<ParserRuleContext *>(hijo);
            if (param == nullptr) continue;

            std::string nombre = aMinusculas(param->getStart()->getText());
            /* Las banderas (-r, -p) no tienen valor: se marcan presentes. */
            std::string valor = param->children.size() >= 3
                              ? param->children[2]->getText()
                              : "";

            if (resultado.parametros.tiene(nombre)) {
                resultado.errores.push_back({
                    numeroLinea,
                    param->getStart()->getCharPositionInLine() + 1,
                    "el parametro " + nombre + " esta repetido en " +
                    resultado.parametros.comando
                });
                continue;
            }
            resultado.parametros.valores[nombre] = valor;
        }
    }


    if (resultado.errores.empty())
        validarParametros(resultado.parametros, resultado.errores);

    return resultado;
}
