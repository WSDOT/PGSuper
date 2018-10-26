

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Thu Jun 29 14:54:01 2017
 */
/* Compiler settings for TestAgent.idl:
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


#ifndef __TestAgent_i_h__
#define __TestAgent_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __TestAgent_FWD_DEFINED__
#define __TestAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class TestAgent TestAgent;
#else
typedef struct TestAgent TestAgent;
#endif /* __cplusplus */

#endif 	/* __TestAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuper_LIBRARY_DEFINED__
#define __PGSuper_LIBRARY_DEFINED__

/* library PGSuper */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_PGSuper;

EXTERN_C const CLSID CLSID_TestAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("7D692AAD-39D0-4e73-842C-854457EA0EE6")
TestAgent;
#endif
#endif /* __PGSuper_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


