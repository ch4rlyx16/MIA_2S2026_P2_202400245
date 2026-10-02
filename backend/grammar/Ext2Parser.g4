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
    | cmdJournaling | cmdLoss | cmdUnmount
    | cmdRemove | cmdRename | cmdCopy
    | cmdMove | cmdFind | cmdChown
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
    | P_DELETE EQ valorTipoBorrado
    | P_ADD EQ valorEntero
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
    | P_FS   EQ valorSistemaArchivos
    ;
valorSistemaArchivos : VAL_2FS | VAL_3FS ;
cmdJournaling : JOURNALING paramJournaling* ;
paramJournaling : P_ID EQ valorTexto ;

cmdLoss : LOSS paramLoss* ;
paramLoss : P_ID EQ valorTexto ;

cmdUnmount : UNMOUNT paramUnmount* ;
paramUnmount : P_ID EQ valorTexto ;

/* ---------- Operaciones sobre archivos y carpetas ---------- */
cmdRemove : REMOVE paramRemove* ;
paramRemove : P_PATH EQ valorRuta ;

cmdRename : RENAME paramRename* ;
paramRename
    : P_PATH EQ valorRuta
    | P_NAME EQ valorTexto
    ;

cmdCopy : COPY paramCopyMove* ;
cmdMove : MOVE paramCopyMove* ;
paramCopyMove
    : P_PATH    EQ valorRuta
    | P_DESTINO EQ valorRuta
    ;

cmdFind : FIND paramFind* ;
paramFind
    : P_PATH EQ valorRuta
    | P_NAME EQ valorTexto
    ;

cmdChown : CHOWN paramChown* ;
paramChown
    : P_PATH    EQ valorRuta
    | P_USUARIO EQ valorTexto
    | P_R
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
valorTipoBorrado   : VAL_FAST | VAL_FULL ;


valorTexto
    : ID | ENTERO | CADENA
    | VAL_BF | VAL_FF | VAL_WF | VAL_FULL
    | VAL_B | VAL_K | VAL_M | VAL_P | VAL_E | VAL_L
    | VAL_2FS | VAL_3FS | VAL_FAST | PATRON
   ;
