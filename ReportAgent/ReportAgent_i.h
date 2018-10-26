

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Thu Jun 29 14:52:47 2017
 */
/* Compiler settings for ReportAgent.idl:
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


#ifndef __ReportAgent_i_h__
#define __ReportAgent_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __PGSuperReportAgent_FWD_DEFINED__
#define __PGSuperReportAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSuperReportAgent PGSuperReportAgent;
#else
typedef struct PGSuperReportAgent PGSuperReportAgent;
#endif /* __cplusplus */

#endif 	/* __PGSuperReportAgent_FWD_DEFINED__ */


#ifndef __PGSpliceReportAgent_FWD_DEFINED__
#define __PGSpliceReportAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class PGSpliceReportAgent PGSpliceReportAgent;
#else
typedef struct PGSpliceReportAgent PGSpliceReportAgent;
#endif /* __cplusplus */

#endif 	/* __PGSpliceReportAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuper_LIBRARY_DEFINED__
#define __PGSuper_LIBRARY_DEFINED__

/* library PGSuper */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PGSuper;

EXTERN_C const CLSID CLSID_PGSuperReportAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("1FFED5EC-7A32-4837-A1F1-99481AFF2825")
PGSuperReportAgent;
#endif

EXTERN_C const CLSID CLSID_PGSpliceReportAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("CC903228-D6FD-441f-AE4E-3DCF034F6D6C")
PGSpliceReportAgent;
#endif
#endif /* __PGSuper_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


