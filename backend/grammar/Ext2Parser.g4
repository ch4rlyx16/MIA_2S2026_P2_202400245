parser grammar Ext2Parser;
options { tokenVocab = Ext2Lexer; }

/* ============================================================
   Proyecto 2 - MIA 2S2026 - Carnet 202400245

   ============================================================ */

script : ( comando? SALTO )* comando? EOF ;

comando
    : cmdMkdisk  | cmdRmdisk  | cmdFdisk
    | cmdMount   | cmdMounted | cmdMkfs
    | cmdCat     | cmdLogin   | cmdLogout
    | cmdMkgrp   | cmdRmgrp   | cmdMkusr
    | cmdRmusr   | cmdChgrp
    | cmdMkfile  | cmdMkdir   | cmdRep
    ;

/* ---------- Administracion de discos ---------- */
cmdMkdisk : MKDISK paramMkdisk* ;
paramMkdisk
    : P_SIZE EQ valorEntero
    | P_FIT  EQ valorFit
    | P_UNIT EQ valorUnidadKM
    | P_PATH EQ valorRuta
    ;

cmdRmdisk : RMDISK paramRmdisk* ;
paramRmdisk : P_PATH EQ valorRuta ;

cmdFdisk : FDISK paramFdisk* ;
paramFdisk
    : P_SIZE EQ valorEntero
    | P_UNIT EQ valorUnidadBKM
    | P_PATH EQ valorRuta
    | P_TYPE EQ valorTipoParticion
    | P_FIT  EQ valorFit
    | P_NAME EQ valorTexto
    ;

cmdMount : MOUNT paramMount* ;
paramMount
    : P_PATH EQ valorRuta
    | P_NAME EQ valorTexto
    ;


cmdMounted : MOUNTED ;

/* ---------- Administracion del sistema de archivos ---------- */
cmdMkfs : MKFS paramMkfs* ;
paramMkfs
    : P_ID   EQ valorTexto
    | P_TYPE EQ valorTipoFormateo
    ;

cmdCat : CAT paramCat* ;
paramCat : P_FILEN EQ valorRuta ;

/* ---------- Sesiones ---------- */
cmdLogin : LOGIN paramLogin* ;
paramLogin
    : P_USER EQ valorTexto
    | P_PASS EQ valorTexto
    | P_ID   EQ valorTexto
    ;

cmdLogout : LOGOUT ;

/* ---------- Usuarios y grupos ---------- */
cmdMkgrp : MKGRP paramMkgrp* ;
paramMkgrp : P_NAME EQ valorTexto ;

cmdRmgrp : RMGRP paramRmgrp* ;
paramRmgrp : P_NAME EQ valorTexto ;

cmdMkusr : MKUSR paramMkusr* ;
paramMkusr
    : P_USER EQ valorTexto
    | P_PASS EQ valorTexto
    | P_GRP  EQ valorTexto
    ;

cmdRmusr : RMUSR paramRmusr* ;
paramRmusr : P_USER EQ valorTexto ;

cmdChgrp : CHGRP paramChgrp* ;
paramChgrp
    : P_USER EQ valorTexto
    | P_GRP  EQ valorTexto
    ;

/* ---------- Carpetas y archivos ---------- */
cmdMkfile : MKFILE paramMkfile* ;
paramMkfile
    : P_PATH EQ valorRuta
    | P_SIZE EQ valorEntero
    | P_CONT EQ valorRuta
    | P_R                      // bandera: no lleva valor
    ;

cmdMkdir : MKDIR paramMkdir* ;
paramMkdir
    : P_PATH EQ valorRuta
    | P_P                      // bandera: no lleva valor
    ;

/* ---------- Reportes ---------- */
cmdRep : REP paramRep* ;
paramRep
    : P_NAME           EQ valorTexto
    | P_PATH           EQ valorRuta
    | P_ID             EQ valorTexto
    | P_PATH_FILE_LS   EQ valorRuta
    ;

/* ============================================================
   TIPOS DE VALOR
   ============================================================ */
valorEntero        : ENTERO | NEGATIVO ;
valorRuta          : RUTA | CADENA ;
valorFit           : VAL_BF | VAL_FF | VAL_WF ;
valorUnidadKM      : VAL_K | VAL_M ;
valorUnidadBKM     : VAL_B | VAL_K | VAL_M ;
valorTipoParticion : VAL_P | VAL_E | VAL_L ;
valorTipoFormateo  : VAL_FULL ;


valorTexto
    : ID | ENTERO | CADENA
    | VAL_BF | VAL_FF | VAL_WF | VAL_FULL
    | VAL_B | VAL_K | VAL_M | VAL_P | VAL_E | VAL_L
    ;
