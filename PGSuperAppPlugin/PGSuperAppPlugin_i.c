

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

MIDL_DEFINE_GUID(IID, LIBID_PGSuperAppPluginLib,0xD72410B0,0x3CD9,0x476C,0x87,0xFD,0xA0,0x50,0xE4,0xF0,0x8B,0x65);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSuperAppPlugin,0x22F091F9,0xB2BE,0x4313,0xBD,0xA3,0x8F,0x46,0xA4,0x46,0x81,0xE9);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSuperProjectImporterAppPlugin,0x73B452CE,0x0955,0x4ff5,0xAB,0x8C,0xEF,0x54,0xBD,0x1D,0xFB,0x72);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSuperComponentInfo,0x339425CE,0x5469,0x41c2,0xB8,0xA2,0x51,0x1A,0x79,0xB7,0x48,0x3F);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSpliceAppPlugin,0x64EC8856,0xB47A,0x4cd3,0xA8,0xF0,0x12,0x70,0xAD,0xD7,0x73,0x3D);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSpliceProjectImporterAppPlugin,0x398F7443,0xAA27,0x442f,0xAD,0xA4,0x86,0x6E,0x38,0xBE,0x59,0x6D);


MIDL_DEFINE_GUID(CLSID, CLSID_PGSpliceComponentInfo,0x52160B92,0xF03C,0x495c,0xA1,0x49,0xCD,0xF1,0x06,0x8E,0x99,0xE3);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



