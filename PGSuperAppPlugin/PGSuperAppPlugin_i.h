

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:45:52 2017
 */
/* Compiler settings for PGSuperAppPlugin.idl:
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


#ifndef __PGSuperAppPlugin_i_h__
#define __PGSuperAppPlugin_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __PGSuperAppPlugin_FWD_DEFINED__
#define __PGSuperAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperAppPlugin PGSuperAppPlugin;
#else
typedef struct PGSuperAppPlugin PGSuperAppPlugin;
#endif /* __cplusplus */

#endif 	/* __PGSuperAppPlugin_FWD_DEFINED__ */


#ifndef __PGSuperProjectImporterAppPlugin_FWD_DEFINED__
#define __PGSuperProjectImporterAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperProjectImporterAppPlugin PGSuperProjectImporterAppPlugin;
#else
typedef struct PGSuperProjectImporterAppPlugin PGSuperProjectImporterAppPlugin;
#endif /* __cplusplus */

#endif 	/* __PGSuperProjectImporterAppPlugin_FWD_DEFINED__ */


#ifndef __PGSuperComponentInfo_FWD_DEFINED__
#define __PGSuperComponentInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperComponentInfo PGSuperComponentInfo;
#else
typedef struct PGSuperComponentInfo PGSuperComponentInfo;
#endif /* __cplusplus */

#endif 	/* __PGSuperComponentInfo_FWD_DEFINED__ */


#ifndef __PGSpliceAppPlugin_FWD_DEFINED__
#define __PGSpliceAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSpliceAppPlugin PGSpliceAppPlugin;
#else
typedef struct PGSpliceAppPlugin PGSpliceAppPlugin;
#endif /* __cplusplus */

#endif 	/* __PGSpliceAppPlugin_FWD_DEFINED__ */


#ifndef __PGSpliceProjectImporterAppPlugin_FWD_DEFINED__
#define __PGSpliceProjectImporterAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSpliceProjectImporterAppPlugin PGSpliceProjectImporterAppPlugin;
#else
typedef struct PGSpliceProjectImporterAppPlugin PGSpliceProjectImporterAppPlugin;
#endif /* __cplusplus */

#endif 	/* __PGSpliceProjectImporterAppPlugin_FWD_DEFINED__ */


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

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuperAppPluginLib_LIBRARY_DEFINED__
#define __PGSuperAppPluginLib_LIBRARY_DEFINED__

/* library PGSuperAppPluginLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PGSuperAppPluginLib;

EXTERN_C const CLSID CLSID_PGSuperAppPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("22F091F9-B2BE-4313-BDA3-8F46A44681E9")
PGSuperAppPlugin;
#endif

EXTERN_C const CLSID CLSID_PGSuperProjectImporterAppPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("73B452CE-0955-4ff5-AB8C-EF54BD1DFB72")
PGSuperProjectImporterAppPlugin;
#endif

EXTERN_C const CLSID CLSID_PGSuperComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("339425CE-5469-41c2-B8A2-511A79B7483F")
PGSuperComponentInfo;
#endif

EXTERN_C const CLSID CLSID_PGSpliceAppPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("64EC8856-B47A-4cd3-A8F0-1270ADD7733D")
PGSpliceAppPlugin;
#endif

EXTERN_C const CLSID CLSID_PGSpliceProjectImporterAppPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("398F7443-AA27-442f-ADA4-866E38BE596D")
PGSpliceProjectImporterAppPlugin;
#endif

EXTERN_C const CLSID CLSID_PGSpliceComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("52160B92-F03C-495c-A149-CDF1068E99E3")
PGSpliceComponentInfo;
#endif
#endif /* __PGSuperAppPluginLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


