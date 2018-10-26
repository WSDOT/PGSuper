

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

MIDL_DEFINE_GUID(IID, LIBID_TxDOTAgentLib,0x17D09C05,0xE1AC,0x42D5,0x9C,0xFE,0xB7,0x24,0x14,0x56,0x4E,0x26);


MIDL_DEFINE_GUID(CLSID, CLSID_TxDOTAgent,0x3700B253,0x8489,0x457c,0x8A,0x6D,0xD1,0x74,0xF9,0x5C,0x45,0x7C);


MIDL_DEFINE_GUID(CLSID, CLSID_TxDOTCadExporter,0x9274354C,0xD0B7,0x437c,0xA5,0xB3,0x3F,0xFB,0xFB,0x17,0xAD,0xE3);


MIDL_DEFINE_GUID(CLSID, CLSID_TxDOTAppPlugin,0xF4629B75,0x7EF8,0x4159,0xA0,0x9A,0x9F,0x4F,0x30,0xB6,0x05,0x01);


MIDL_DEFINE_GUID(CLSID, CLSID_TxDOTComponentInfo,0x785F2ACE,0x127B,0x4647,0x80,0x62,0xED,0x49,0x53,0x7E,0x96,0x2C);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



