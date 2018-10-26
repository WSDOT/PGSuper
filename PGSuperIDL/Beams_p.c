

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Mon Oct 30 09:14:21 2017
 */
/* Compiler settings for ..\Include\Plugins\Beams.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "Beams.h"

#define TYPE_FORMAT_STRING_SIZE   33                                
#define PROC_FORMAT_STRING_SIZE   301                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _Beams_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } Beams_MIDL_TYPE_FORMAT_STRING;

typedef struct _Beams_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } Beams_MIDL_PROC_FORMAT_STRING;

typedef struct _Beams_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } Beams_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const Beams_MIDL_TYPE_FORMAT_STRING Beams__MIDL_TypeFormatString;
extern const Beams_MIDL_PROC_FORMAT_STRING Beams__MIDL_ProcFormatString;
extern const Beams_MIDL_EXPR_FORMAT_STRING Beams__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IDummyInterface_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IDummyInterface_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IConfigureStrandMover_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IConfigureStrandMover_ProxyInfo;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const Beams_MIDL_PROC_FORMAT_STRING Beams__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure Dummy */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 26 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetHarpedStrandOffsetBounds */

/* 32 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 34 */	NdrFcLong( 0x0 ),	/* 0 */
/* 38 */	NdrFcShort( 0x3 ),	/* 3 */
/* 40 */	NdrFcShort( 0x70 ),	/* X64 Stack size/offset = 112 */
/* 42 */	NdrFcShort( 0xc0 ),	/* 192 */
/* 44 */	NdrFcShort( 0x8 ),	/* 8 */
/* 46 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0xd,		/* 13 */
/* 48 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0xaaa8 ),	/* -21848 */

	/* Parameter topGirderElevation */

/* 58 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 60 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 62 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter Hg */

/* 64 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 66 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 68 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter topStartElevationBoundary */

/* 70 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 72 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 74 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter botStartElevationBoundary */

/* 76 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 78 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 80 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter topHp1ElevationBoundary */

/* 82 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 84 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 86 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter botHp1ElevationBoundary */

/* 88 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 90 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 92 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter topHp2ElevationBoundary */

/* 94 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 96 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 98 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter botHp2ElevationBoundary */

/* 100 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 102 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 104 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter topEndElevationBoundary */

/* 106 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 108 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 110 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter botEndElevationBoundary */

/* 112 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 114 */	NdrFcShort( 0x50 ),	/* X64 Stack size/offset = 80 */
/* 116 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter endIncrement */

/* 118 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 120 */	NdrFcShort( 0x58 ),	/* X64 Stack size/offset = 88 */
/* 122 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Parameter hpIncrement */

/* 124 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 126 */	NdrFcShort( 0x60 ),	/* X64 Stack size/offset = 96 */
/* 128 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Return value */

/* 130 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 132 */	NdrFcShort( 0x68 ),	/* X64 Stack size/offset = 104 */
/* 134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ClearAll */

/* 136 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 138 */	NdrFcLong( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x4 ),	/* 4 */
/* 144 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 150 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 152 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure AddRegion */

/* 168 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x5 ),	/* 5 */
/* 176 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 178 */	NdrFcShort( 0x10 ),	/* 16 */
/* 180 */	NdrFcShort( 0x8 ),	/* 8 */
/* 182 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 184 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x20 ),	/* 32 */

	/* Parameter shape */

/* 194 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 196 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 198 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter arcSlope */

/* 200 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 202 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 204 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Return value */

/* 206 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 208 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_NumRegions */

/* 212 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 214 */	NdrFcLong( 0x0 ),	/* 0 */
/* 218 */	NdrFcShort( 0x6 ),	/* 6 */
/* 220 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 222 */	NdrFcShort( 0x0 ),	/* 0 */
/* 224 */	NdrFcShort( 0x2c ),	/* 44 */
/* 226 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 228 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pNum */

/* 238 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 240 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 242 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 244 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 246 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 248 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegion */

/* 250 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 252 */	NdrFcLong( 0x0 ),	/* 0 */
/* 256 */	NdrFcShort( 0x7 ),	/* 7 */
/* 258 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 260 */	NdrFcShort( 0x10 ),	/* 16 */
/* 262 */	NdrFcShort( 0x2c ),	/* 44 */
/* 264 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 266 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 268 */	NdrFcShort( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 276 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 278 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 280 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter shape */

/* 282 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 284 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 286 */	NdrFcShort( 0x18 ),	/* Type Offset=24 */

	/* Parameter arcSlope */

/* 288 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 290 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 292 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Return value */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const Beams_MIDL_TYPE_FORMAT_STRING Beams__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/*  4 */	NdrFcLong( 0x8a8ff0a0 ),	/* -1970278240 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 12 */	0x8c,		/* 140 */
			0xdf,		/* 223 */
/* 14 */	0xf4,		/* 244 */
			0x3c,		/* 60 */
/* 16 */	0x39,		/* 57 */
			0x28,		/* 40 */
/* 18 */	0xa3,		/* 163 */
			0x34,		/* 52 */
/* 20 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 22 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 24 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 26 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (2) */
/* 28 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 30 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */

			0x0
        }
    };


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDummyInterface, ver. 0.0,
   GUID={0xD3810B3E,0x91D6,0x4aed,{0xA7,0x48,0x8A,0xBE,0xB8,0x7F,0xCF,0x44}} */

#pragma code_seg(".orpc")
static const unsigned short IDummyInterface_FormatStringOffsetTable[] =
    {
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IDummyInterface_ProxyInfo =
    {
    &Object_StubDesc,
    Beams__MIDL_ProcFormatString.Format,
    &IDummyInterface_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IDummyInterface_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Beams__MIDL_ProcFormatString.Format,
    &IDummyInterface_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IDummyInterfaceProxyVtbl = 
{
    &IDummyInterface_ProxyInfo,
    &IID_IDummyInterface,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IDummyInterface::Dummy */
};

const CInterfaceStubVtbl _IDummyInterfaceStubVtbl =
{
    &IID_IDummyInterface,
    &IDummyInterface_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IConfigureStrandMover, ver. 0.0,
   GUID={0x49D89070,0x3BC9,0x4b30,{0x97,0xE1,0x49,0x6E,0x07,0x15,0xF6,0x36}} */

#pragma code_seg(".orpc")
static const unsigned short IConfigureStrandMover_FormatStringOffsetTable[] =
    {
    32,
    136,
    168,
    212,
    250
    };

static const MIDL_STUBLESS_PROXY_INFO IConfigureStrandMover_ProxyInfo =
    {
    &Object_StubDesc,
    Beams__MIDL_ProcFormatString.Format,
    &IConfigureStrandMover_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IConfigureStrandMover_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Beams__MIDL_ProcFormatString.Format,
    &IConfigureStrandMover_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _IConfigureStrandMoverProxyVtbl = 
{
    &IConfigureStrandMover_ProxyInfo,
    &IID_IConfigureStrandMover,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IConfigureStrandMover::SetHarpedStrandOffsetBounds */ ,
    (void *) (INT_PTR) -1 /* IConfigureStrandMover::ClearAll */ ,
    (void *) (INT_PTR) -1 /* IConfigureStrandMover::AddRegion */ ,
    (void *) (INT_PTR) -1 /* IConfigureStrandMover::get_NumRegions */ ,
    (void *) (INT_PTR) -1 /* IConfigureStrandMover::GetRegion */
};

const CInterfaceStubVtbl _IConfigureStrandMoverStubVtbl =
{
    &IID_IConfigureStrandMover,
    &IConfigureStrandMover_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    Beams__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x800025b, /* MIDL Version 8.0.603 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _Beams_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IDummyInterfaceProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IConfigureStrandMoverProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _Beams_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IDummyInterfaceStubVtbl,
    ( CInterfaceStubVtbl *) &_IConfigureStrandMoverStubVtbl,
    0
};

PCInterfaceName const _Beams_InterfaceNamesList[] = 
{
    "IDummyInterface",
    "IConfigureStrandMover",
    0
};


#define _Beams_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _Beams, pIID, n)

int __stdcall _Beams_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _Beams, 2, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _Beams, 2, *pIndex )
    
}

const ExtendedProxyFileInfo Beams_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _Beams_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _Beams_StubVtblList,
    (const PCInterfaceName * ) & _Beams_InterfaceNamesList,
    0, /* no delegation */
    & _Beams_IID_Lookup, 
    2,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

