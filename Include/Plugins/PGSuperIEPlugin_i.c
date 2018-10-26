

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Jun 13 12:04:40 2017
 */
/* Compiler settings for ..\Include\Plugins\PGSuperIEPlugin.idl:
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

MIDL_DEFINE_GUID(IID, IID_IPGSDataImporter,0x98B3DF17,0x7E0E,0x4d4a,0xB8,0xA2,0x54,0x43,0x91,0x4F,0xC6,0x08);


MIDL_DEFINE_GUID(IID, IID_IPGSProjectImporter,0x5DB8B1D3,0xC91D,0x4e62,0x81,0xE6,0xA7,0xB6,0x4B,0x0D,0x38,0xFD);


MIDL_DEFINE_GUID(IID, IID_IPGSDataExporter,0xBF6EC18A,0x43D2,0x4ea1,0xBC,0x7F,0x54,0x36,0x5D,0xD6,0x45,0xDA);


MIDL_DEFINE_GUID(IID, IID_IPGSDocumentation,0x45C667CB,0x67C4,0x4b2e,0x89,0xCD,0x51,0xD0,0x7D,0x66,0x55,0x07);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



