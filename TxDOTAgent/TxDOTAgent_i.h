

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:33:53 2017
 */
/* Compiler settings for TxDOTAgent.idl:
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


#ifndef __TxDOTAgent_i_h__
#define __TxDOTAgent_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __TxDOTAgent_FWD_DEFINED__
#define __TxDOTAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class TxDOTAgent TxDOTAgent;
#else
typedef struct TxDOTAgent TxDOTAgent;
#endif /* __cplusplus */

#endif 	/* __TxDOTAgent_FWD_DEFINED__ */


#ifndef __TxDOTCadExporter_FWD_DEFINED__
#define __TxDOTCadExporter_FWD_DEFINED__

#ifdef __cplusplus
typedef class TxDOTCadExporter TxDOTCadExporter;
#else
typedef struct TxDOTCadExporter TxDOTCadExporter;
#endif /* __cplusplus */

#endif 	/* __TxDOTCadExporter_FWD_DEFINED__ */


#ifndef __TxDOTAppPlugin_FWD_DEFINED__
#define __TxDOTAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class TxDOTAppPlugin TxDOTAppPlugin;
#else
typedef struct TxDOTAppPlugin TxDOTAppPlugin;
#endif /* __cplusplus */

#endif 	/* __TxDOTAppPlugin_FWD_DEFINED__ */


#ifndef __TxDOTComponentInfo_FWD_DEFINED__
#define __TxDOTComponentInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class TxDOTComponentInfo TxDOTComponentInfo;
#else
typedef struct TxDOTComponentInfo TxDOTComponentInfo;
#endif /* __cplusplus */

#endif 	/* __TxDOTComponentInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLCore.h"
#include "PGSuperIEPlugin.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __TxDOTAgentLib_LIBRARY_DEFINED__
#define __TxDOTAgentLib_LIBRARY_DEFINED__

/* library TxDOTAgentLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TxDOTAgentLib;

EXTERN_C const CLSID CLSID_TxDOTAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("3700B253-8489-457c-8A6D-D174F95C457C")
TxDOTAgent;
#endif

EXTERN_C const CLSID CLSID_TxDOTCadExporter;

#ifdef __cplusplus

class DECLSPEC_UUID("9274354C-D0B7-437c-A5B3-3FFBFB17ADE3")
TxDOTCadExporter;
#endif

EXTERN_C const CLSID CLSID_TxDOTAppPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("F4629B75-7EF8-4159-A09A-9F4F30B60501")
TxDOTAppPlugin;
#endif

EXTERN_C const CLSID CLSID_TxDOTComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("785F2ACE-127B-4647-8062-ED49537E962C")
TxDOTComponentInfo;
#endif
#endif /* __TxDOTAgentLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


