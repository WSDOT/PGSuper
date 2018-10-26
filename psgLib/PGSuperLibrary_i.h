

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:06:47 2017
 */
/* Compiler settings for PGSuperLibrary.idl:
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


#ifndef __PGSuperLibrary_i_h__
#define __PGSuperLibrary_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __LibraryAppPlugin_FWD_DEFINED__
#define __LibraryAppPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class LibraryAppPlugin LibraryAppPlugin;
#else
typedef struct LibraryAppPlugin LibraryAppPlugin;
#endif /* __cplusplus */

#endif 	/* __LibraryAppPlugin_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __PGSuperLibrary_LIBRARY_DEFINED__
#define __PGSuperLibrary_LIBRARY_DEFINED__

/* library PGSuperLibrary */
/* [helpstring][version][uuid] */ 


DEFINE_GUID(LIBID_PGSuperLibrary,0xB9603863,0x5166,0x4c1c,0xBD,0xD9,0x96,0x28,0x19,0xA4,0x8C,0xC1);

DEFINE_GUID(CLSID_LibraryAppPlugin,0xE96DEDC8,0x4D09,0x47ea,0x98,0xE9,0x97,0x31,0x69,0x2D,0x40,0xAB);

#ifdef __cplusplus

class DECLSPEC_UUID("E96DEDC8-4D09-47ea-98E9-9731692D40AB")
LibraryAppPlugin;
#endif
#endif /* __PGSuperLibrary_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


