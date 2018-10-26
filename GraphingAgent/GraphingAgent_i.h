

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Thu Jun 29 14:51:13 2017
 */
/* Compiler settings for GraphingAgent.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __GraphingAgent_i_h__
#define __GraphingAgent_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __PGSuperGraphingAgent_FWD_DEFINED__
#define __PGSuperGraphingAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperGraphingAgent PGSuperGraphingAgent;
#else
typedef struct PGSuperGraphingAgent PGSuperGraphingAgent;
#endif /* __cplusplus */

#endif 	/* __PGSuperGraphingAgent_FWD_DEFINED__ */


#ifndef __PGSpliceGraphingAgent_FWD_DEFINED__
#define __PGSpliceGraphingAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSpliceGraphingAgent PGSpliceGraphingAgent;
#else
typedef struct PGSpliceGraphingAgent PGSpliceGraphingAgent;
#endif /* __cplusplus */

#endif 	/* __PGSpliceGraphingAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuperGraphingAgent_LIBRARY_DEFINED__
#define __PGSuperGraphingAgent_LIBRARY_DEFINED__

/* library PGSuperGraphingAgent */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PGSuperGraphingAgent;

EXTERN_C const CLSID CLSID_PGSuperGraphingAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("B4639189-ED38-4a68-8A18-38026202E9DE")
PGSuperGraphingAgent;
#endif

EXTERN_C const CLSID CLSID_PGSpliceGraphingAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("EDAAB718-3128-4b1c-956C-CCAAD32D01DB")
PGSpliceGraphingAgent;
#endif
#endif /* __PGSuperGraphingAgent_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


