

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:36:00 2017
 */
/* Compiler settings for WSDOTAgent.idl:
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


#ifndef __WSDOTAgent_i_h__
#define __WSDOTAgent_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __WSDOTAgent_FWD_DEFINED__
#define __WSDOTAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class WSDOTAgent WSDOTAgent;
#else
typedef struct WSDOTAgent WSDOTAgent;
#endif /* __cplusplus */

#endif 	/* __WSDOTAgent_FWD_DEFINED__ */


#ifndef __PGSuperComponentInfo_FWD_DEFINED__
#define __PGSuperComponentInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperComponentInfo PGSuperComponentInfo;
#else
typedef struct PGSuperComponentInfo PGSuperComponentInfo;
#endif /* __cplusplus */

#endif 	/* __PGSuperComponentInfo_FWD_DEFINED__ */


#ifndef __PGSpliceComponentInfo_FWD_DEFINED__
#define __PGSpliceComponentInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSpliceComponentInfo PGSpliceComponentInfo;
#else
typedef struct PGSpliceComponentInfo PGSpliceComponentInfo;
#endif /* __cplusplus */

#endif 	/* __PGSpliceComponentInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLCore.h"
#include "PGSuperIEPlugin.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuper_LIBRARY_DEFINED__
#define __PGSuper_LIBRARY_DEFINED__

/* library PGSuper */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PGSuper;

EXTERN_C const CLSID CLSID_WSDOTAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("B1A19633-8880-40bc-A3C9-DDF47F7F1844")
WSDOTAgent;
#endif

EXTERN_C const CLSID CLSID_PGSuperComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("5656F52E-4DC8-4299-8AD1-23EB3AE46353")
PGSuperComponentInfo;
#endif

EXTERN_C const CLSID CLSID_PGSpliceComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("E389A200-D722-4977-AE9F-939F9C121A1C")
PGSpliceComponentInfo;
#endif
#endif /* __PGSuper_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


