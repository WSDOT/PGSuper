

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_PGSuper,0x3B6BF48B,0xC530,0x486E,0x91,0x8F,0x77,0x05,0xE1,0x3D,0x63,0x30);


MIDL_DEFINE_GUID(CLSID, CLSID_WSDOTAgent,0xB1A19633,0x8880,0x40bc,0xA3,0xC9,0xDD,0xF4,0x7F,0x7F,0x18,0x44);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSuperComponentInfo,0x5656F52E,0x4DC8,0x4299,0x8A,0xD1,0x23,0xEB,0x3A,0xE4,0x63,0x53);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSpliceComponentInfo,0xE389A200,0xD722,0x4977,0xAE,0x9F,0x93,0x9F,0x9C,0x12,0x1A,0x1C);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



