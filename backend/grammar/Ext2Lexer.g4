lexer grammar Ext2Lexer;

/* ============================================================
   Proyecto 2 - MIA 2S2026 - Carnet 202400245
   ============================================================ */

/* ---------- 1. COMANDOS ---------- */
/* MOUNTED va despues de MOUNT sin problema: ANTLR se queda con la
   coincidencia mas larga, asi que "mounted" nunca se lee como MOUNT. */
MKDISK  : M K D I S K      ;
RMDISK  : R M D I S K      ;
FDISK   : F D I S K        ;
MOUNT   : M O U N T        ;
MOUNTED : M O U N T E D    ;
MKFS    : M K F S          ;
CAT     : C A T            ;
LOGIN   : L O G I N        ;
LOGOUT  : L O G O U T      ;
MKGRP   : M K G R P        ;
RMGRP   : R M G R P        ;
MKUSR   : M K U S R        ;
RMUSR   : R M U S R        ;
CHGRP   : C H G R P        ;
MKFILE  : M K F I L E      ;
MKDIR   : M K D I R        ;
REP     : R E P            ;
JOURNALING : J O U R N A L I N G ;
LOSS       : L O S S ;
UNMOUNT    : U N M O U N T ;
REMOVE     : R E M O V E ;
RENAME     : R E N A M E ;
COPY       : C O P Y ;
MOVE       : M O V E ;
FIND       : F I N D ;
CHOWN      : C H O W N ;

/* ---------- 2. PARAMETROS ---------- */
P_SIZE          : GUION S I Z E ;
P_FIT           : GUION F I T   ;
P_UNIT          : GUION U N I T ;
P_PATH          : GUION P A T H ;
P_TYPE          : GUION T Y P E ;
P_NAME          : GUION N A M E ;
P_ID            : GUION I D     ;
P_USER          : GUION U S E R ;
P_PASS          : GUION P A S S ;
P_GRP           : GUION G R P   ;
P_CONT          : GUION C O N T ;
P_R             : GUION R       ;   // bandera de mkfile
P_P             : GUION P       ;   // bandera de mkdir
P_PATH_FILE_LS  : GUION P A T H '_' F I L E '_' L S ;
P_FS            : GUION F S     ;
P_DELETE        : GUION D E L E T E ;
P_ADD           : GUION A D D ;
P_DESTINO       : GUION D E S T I N O ;
P_USUARIO       : GUION U S U A R I O ;

/* cat recibe -file1, -file2 ... -fileN: el numero es parte del nombre
   del parametro, no un valor, asi que se reconoce dentro del token. */
P_FILEN : GUION F I L E DIGITO+ ;

/* ---------- 3. VALORES RESERVADOS ---------- */
VAL_BF   : B F     ;
VAL_FF   : F F     ;
VAL_WF   : W F     ;
VAL_FULL : F U L L ;
VAL_FAST : F A S T ;
VAL_B : B ;
VAL_K : K ;
VAL_M : M ;
VAL_P : P ;
VAL_E : E ;
VAL_L : L ;
VAL_2FS  : '2' F S ;
VAL_3FS  : '3' F S ;

/* ---------- 4. SIMBOLOS ---------- */
EQ : '=' ;

/* ---------- 5. LITERALES ---------- */
NEGATIVO : GUION DIGITO+ ;
ENTERO   : DIGITO+ ;
CADENA   : '"' ~["\r\n]* '"' ;
RUTA     : ( '/' | './' | '../' ) ~[ \t\r\n"=]* ;
/* un patron de busqueda lleva al menos un * o un ?, eso lo separa de ID */
PATRON   : ( LETRA | DIGITO | '.' | '_' | '*' | '?' )* ( '*' | '?' )
           ( LETRA | DIGITO | '.' | '_' | '*' | '?' )* ;
ID       : DIGITO* LETRA ( LETRA | DIGITO | '_' | '.' )* ;

/* ---------- 6. IGNORADOS / SEPARADORES ---------- */
COMENTARIO : '#' ~[\r\n]* -> skip ;
SALTO      : '\r'? '\n' ;
WS         : [ \t]+ -> skip ;

/* ---------- 7. FRAGMENTOS (no producen tokens) ---------- */
fragment GUION  : '-' ;
fragment DIGITO : [0-9] ;
fragment LETRA  : [a-zA-Z] ;

fragment A:[aA]; fragment B:[bB]; fragment C:[cC]; fragment D:[dD];
fragment E:[eE]; fragment F:[fF]; fragment G:[gG]; fragment H:[hH];
fragment I:[iI]; fragment J:[jJ]; fragment K:[kK]; fragment L:[lL];
fragment M:[mM]; fragment N:[nN]; fragment O:[oO]; fragment P:[pP];
fragment Q:[qQ]; fragment R:[rR]; fragment S:[sS]; fragment T:[tT];
fragment U:[uU]; fragment V:[vV]; fragment W:[wW]; fragment X:[xX];
fragment Y:[yY]; fragment Z:[zZ];
