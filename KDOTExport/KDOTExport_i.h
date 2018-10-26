

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:31:14 2017
 */
/* Compiler settings for KDOTExport.idl:
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


#ifndef __KDOTExport_i_h__
#define __KDOTExport_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __PGSuperDataExporter_FWD_DEFINED__
#define __PGSuperDataExporter_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperDataExporter PGSuperDataExporter;
#else
typedef struct PGSuperDataExporter PGSuperDataExporter;
#endif /* __cplusplus */

#endif 	/* __PGSuperDataExporter_FWD_DEFINED__ */


#ifndef __KDOTComponentInfo_FWD_DEFINED__
#define __KDOTComponentInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class KDOTComponentInfo KDOTComponentInfo;
#else
typedef struct KDOTComponentInfo KDOTComponentInfo;
#endif /* __cplusplus */

#endif 	/* __KDOTComponentInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "PGSuperIEPlugin.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __KDOTExport_LIBRARY_DEFINED__
#define __KDOTExport_LIBRARY_DEFINED__

/* library KDOTExport */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_KDOTExport;

EXTERN_C const CLSID CLSID_PGSuperDataExporter;

#ifdef __cplusplus

class DECLSPEC_UUID("775F87BC-07DF-4177-B001-F98E011C6AB4")
PGSuperDataExporter;
#endif

EXTERN_C const CLSID CLSID_KDOTComponentInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("75AAA5FD-16D5-415d-B2AD-8FF123C27B45")
KDOTComponentInfo;
#endif
#endif /* __KDOTExport_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


