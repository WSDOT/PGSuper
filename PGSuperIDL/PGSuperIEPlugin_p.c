

/* this ALWAYS GENERATED file contains the proxy stub code */


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


#include "PGSuperIEPlugin.h"

#define TYPE_FORMAT_STRING_SIZE   191                               
#define PROC_FORMAT_STRING_SIZE   457                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   3            

typedef struct _PGSuperIEPlugin_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } PGSuperIEPlugin_MIDL_TYPE_FORMAT_STRING;

typedef struct _PGSuperIEPlugin_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } PGSuperIEPlugin_MIDL_PROC_FORMAT_STRING;

typedef struct _PGSuperIEPlugin_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } PGSuperIEPlugin_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const PGSuperIEPlugin_MIDL_TYPE_FORMAT_STRING PGSuperIEPlugin__MIDL_TypeFormatString;
extern const PGSuperIEPlugin_MIDL_PROC_FORMAT_STRING PGSuperIEPlugin__MIDL_ProcFormatString;
extern const PGSuperIEPlugin_MIDL_EXPR_FORMAT_STRING PGSuperIEPlugin__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPGSDataImporter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPGSDataImporter_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPGSProjectImporter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPGSProjectImporter_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPGSDataExporter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPGSDataExporter_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPGSDocumentation_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPGSDocumentation_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const PGSuperIEPlugin_MIDL_PROC_FORMAT_STRING PGSuperIEPlugin__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure Init */


	/* Procedure Init */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nCmdID */


	/* Parameter nCmdID */

/* 26 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 32 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMenuText */


	/* Procedure GetMenuText */

/* 38 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 40 */	NdrFcLong( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0x4 ),	/* 4 */
/* 46 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 54 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 56 */	NdrFcShort( 0x1 ),	/* 1 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrText */


	/* Parameter bstrText */

/* 64 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 66 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 68 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */


	/* Return value */

/* 70 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 72 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 74 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetBitmapHandle */

/* 76 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 78 */	NdrFcLong( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x5 ),	/* 5 */
/* 84 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x8 ),	/* 8 */
/* 90 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 92 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 94 */	NdrFcShort( 0x1 ),	/* 1 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phBmp */

/* 102 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 104 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 106 */	NdrFcShort( 0x6a ),	/* Type Offset=106 */

	/* Return value */

/* 108 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 110 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCommandHintText */


	/* Procedure GetCommandHintText */

/* 114 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x6 ),	/* 6 */
/* 122 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 130 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 132 */	NdrFcShort( 0x1 ),	/* 1 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrText */


	/* Parameter bstrText */

/* 140 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 142 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 144 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */


	/* Return value */

/* 146 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 148 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 150 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Export */


	/* Procedure Import */

/* 152 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 154 */	NdrFcLong( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x7 ),	/* 7 */
/* 160 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 166 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 168 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pBroker */


	/* Parameter pBroker */

/* 178 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 180 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 182 */	NdrFcShort( 0x74 ),	/* Type Offset=116 */

	/* Return value */


	/* Return value */

/* 184 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 186 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 188 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetItemText */

/* 190 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 192 */	NdrFcLong( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x3 ),	/* 3 */
/* 198 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x8 ),	/* 8 */
/* 204 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 206 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 208 */	NdrFcShort( 0x1 ),	/* 1 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrText */

/* 216 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 218 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 220 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 224 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Import */

/* 228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x4 ),	/* 4 */
/* 236 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 242 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 244 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pBroker */

/* 254 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 256 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 258 */	NdrFcShort( 0x74 ),	/* Type Offset=116 */

	/* Return value */

/* 260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 262 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetIcon */

/* 266 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x5 ),	/* 5 */
/* 274 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x8 ),	/* 8 */
/* 280 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 282 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 284 */	NdrFcShort( 0x1 ),	/* 1 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phIcon */

/* 292 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 294 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 296 */	NdrFcShort( 0xa2 ),	/* Type Offset=162 */

	/* Return value */

/* 298 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 300 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetBitmapHandle */

/* 304 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x5 ),	/* 5 */
/* 312 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 320 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phBmp */

/* 330 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 332 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 334 */	NdrFcShort( 0x6a ),	/* Type Offset=106 */

	/* Return value */

/* 336 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 338 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 340 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocumentationSetName */

/* 342 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 344 */	NdrFcLong( 0x0 ),	/* 0 */
/* 348 */	NdrFcShort( 0x3 ),	/* 3 */
/* 350 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 354 */	NdrFcShort( 0x8 ),	/* 8 */
/* 356 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 358 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 360 */	NdrFcShort( 0x0 ),	/* 0 */
/* 362 */	NdrFcShort( 0x1 ),	/* 1 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 366 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pbstrName */

/* 368 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 370 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 372 */	NdrFcShort( 0xb4 ),	/* Type Offset=180 */

	/* Return value */

/* 374 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 376 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadDocumentationMap */

/* 380 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 382 */	NdrFcLong( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0x4 ),	/* 4 */
/* 388 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 392 */	NdrFcShort( 0x8 ),	/* 8 */
/* 394 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 396 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 406 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 408 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 410 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocumentLocation */

/* 412 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 414 */	NdrFcLong( 0x0 ),	/* 0 */
/* 418 */	NdrFcShort( 0x5 ),	/* 5 */
/* 420 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 422 */	NdrFcShort( 0x8 ),	/* 8 */
/* 424 */	NdrFcShort( 0x8 ),	/* 8 */
/* 426 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 428 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 430 */	NdrFcShort( 0x0 ),	/* 0 */
/* 432 */	NdrFcShort( 0x1 ),	/* 1 */
/* 434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nHID */

/* 438 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 440 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 442 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pbstrURL */

/* 444 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 446 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 448 */	NdrFcShort( 0xb4 ),	/* Type Offset=180 */

	/* Return value */

/* 450 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 452 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 454 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const PGSuperIEPlugin_MIDL_TYPE_FORMAT_STRING PGSuperIEPlugin__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/*  4 */	NdrFcShort( 0x1c ),	/* Offset= 28 (32) */
/*  6 */	
			0x13, 0x0,	/* FC_OP */
/*  8 */	NdrFcShort( 0xe ),	/* Offset= 14 (22) */
/* 10 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 12 */	NdrFcShort( 0x2 ),	/* 2 */
/* 14 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 16 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 18 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 20 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 22 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 24 */	NdrFcShort( 0x8 ),	/* 8 */
/* 26 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (10) */
/* 28 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 30 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 32 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0x8 ),	/* 8 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0xffde ),	/* Offset= -34 (6) */
/* 42 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 44 */	NdrFcShort( 0x3e ),	/* Offset= 62 (106) */
/* 46 */	
			0x13, 0x0,	/* FC_OP */
/* 48 */	NdrFcShort( 0x2 ),	/* Offset= 2 (50) */
/* 50 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x88,		/* 136 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	NdrFcShort( 0x3 ),	/* 3 */
/* 56 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 60 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 62 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 66 */	NdrFcShort( 0xa ),	/* Offset= 10 (76) */
/* 68 */	NdrFcLong( 0x50746457 ),	/* 1349805143 */
/* 72 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 74 */	NdrFcShort( 0xffff ),	/* Offset= -1 (73) */
/* 76 */	
			0x13, 0x0,	/* FC_OP */
/* 78 */	NdrFcShort( 0xe ),	/* Offset= 14 (92) */
/* 80 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 82 */	NdrFcShort( 0x1 ),	/* 1 */
/* 84 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 86 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 88 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 90 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 92 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 94 */	NdrFcShort( 0x18 ),	/* 24 */
/* 96 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (80) */
/* 98 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 100 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 102 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 104 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 106 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 108 */	NdrFcShort( 0x1 ),	/* 1 */
/* 110 */	NdrFcShort( 0x8 ),	/* 8 */
/* 112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0xffbc ),	/* Offset= -68 (46) */
/* 116 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 118 */	NdrFcLong( 0xc0e926d0 ),	/* -1058462000 */
/* 122 */	NdrFcShort( 0x2620 ),	/* 9760 */
/* 124 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 126 */	0x8e,		/* 142 */
			0xb0,		/* 176 */
/* 128 */	0x0,		/* 0 */
			0x60,		/* 96 */
/* 130 */	0x97,		/* 151 */
			0xdf,		/* 223 */
/* 132 */	0x3c,		/* 60 */
			0x68,		/* 104 */
/* 134 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 136 */	NdrFcShort( 0x1a ),	/* Offset= 26 (162) */
/* 138 */	
			0x13, 0x0,	/* FC_OP */
/* 140 */	NdrFcShort( 0x2 ),	/* Offset= 2 (142) */
/* 142 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x48,		/* 72 */
/* 144 */	NdrFcShort( 0x4 ),	/* 4 */
/* 146 */	NdrFcShort( 0x2 ),	/* 2 */
/* 148 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 152 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 154 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 158 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 160 */	NdrFcShort( 0xffff ),	/* Offset= -1 (159) */
/* 162 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 164 */	NdrFcShort( 0x2 ),	/* 2 */
/* 166 */	NdrFcShort( 0x8 ),	/* 8 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (138) */
/* 172 */	
			0x11, 0x0,	/* FC_RP */
/* 174 */	NdrFcShort( 0x6 ),	/* Offset= 6 (180) */
/* 176 */	
			0x12, 0x0,	/* FC_UP */
/* 178 */	NdrFcShort( 0xff64 ),	/* Offset= -156 (22) */
/* 180 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 182 */	NdrFcShort( 0x0 ),	/* 0 */
/* 184 */	NdrFcShort( 0x8 ),	/* 8 */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (176) */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            },
            {
            HBITMAP_UserSize
            ,HBITMAP_UserMarshal
            ,HBITMAP_UserUnmarshal
            ,HBITMAP_UserFree
            },
            {
            HICON_UserSize
            ,HICON_UserMarshal
            ,HICON_UserUnmarshal
            ,HICON_UserFree
            }

        };



/* Standard interface: __MIDL_itf_PGSuperIEPlugin_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IPGSDataImporter, ver. 0.0,
   GUID={0x98B3DF17,0x7E0E,0x4d4a,{0xB8,0xA2,0x54,0x43,0x91,0x4F,0xC6,0x08}} */

#pragma code_seg(".orpc")
static const unsigned short IPGSDataImporter_FormatStringOffsetTable[] =
    {
    0,
    38,
    76,
    114,
    152
    };

static const MIDL_STUBLESS_PROXY_INFO IPGSDataImporter_ProxyInfo =
    {
    &Object_StubDesc,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDataImporter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPGSDataImporter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDataImporter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _IPGSDataImporterProxyVtbl = 
{
    &IPGSDataImporter_ProxyInfo,
    &IID_IPGSDataImporter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IPGSDataImporter::Init */ ,
    (void *) (INT_PTR) -1 /* IPGSDataImporter::GetMenuText */ ,
    (void *) (INT_PTR) -1 /* IPGSDataImporter::GetBitmapHandle */ ,
    (void *) (INT_PTR) -1 /* IPGSDataImporter::GetCommandHintText */ ,
    (void *) (INT_PTR) -1 /* IPGSDataImporter::Import */
};

const CInterfaceStubVtbl _IPGSDataImporterStubVtbl =
{
    &IID_IPGSDataImporter,
    &IPGSDataImporter_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IPGSProjectImporter, ver. 0.0,
   GUID={0x5DB8B1D3,0xC91D,0x4e62,{0x81,0xE6,0xA7,0xB6,0x4B,0x0D,0x38,0xFD}} */

#pragma code_seg(".orpc")
static const unsigned short IPGSProjectImporter_FormatStringOffsetTable[] =
    {
    190,
    228,
    266
    };

static const MIDL_STUBLESS_PROXY_INFO IPGSProjectImporter_ProxyInfo =
    {
    &Object_StubDesc,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSProjectImporter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPGSProjectImporter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSProjectImporter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _IPGSProjectImporterProxyVtbl = 
{
    &IPGSProjectImporter_ProxyInfo,
    &IID_IPGSProjectImporter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IPGSProjectImporter::GetItemText */ ,
    (void *) (INT_PTR) -1 /* IPGSProjectImporter::Import */ ,
    (void *) (INT_PTR) -1 /* IPGSProjectImporter::GetIcon */
};

const CInterfaceStubVtbl _IPGSProjectImporterStubVtbl =
{
    &IID_IPGSProjectImporter,
    &IPGSProjectImporter_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IPGSDataExporter, ver. 0.0,
   GUID={0xBF6EC18A,0x43D2,0x4ea1,{0xBC,0x7F,0x54,0x36,0x5D,0xD6,0x45,0xDA}} */

#pragma code_seg(".orpc")
static const unsigned short IPGSDataExporter_FormatStringOffsetTable[] =
    {
    0,
    38,
    304,
    114,
    152
    };

static const MIDL_STUBLESS_PROXY_INFO IPGSDataExporter_ProxyInfo =
    {
    &Object_StubDesc,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDataExporter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPGSDataExporter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDataExporter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _IPGSDataExporterProxyVtbl = 
{
    &IPGSDataExporter_ProxyInfo,
    &IID_IPGSDataExporter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IPGSDataExporter::Init */ ,
    (void *) (INT_PTR) -1 /* IPGSDataExporter::GetMenuText */ ,
    (void *) (INT_PTR) -1 /* IPGSDataExporter::GetBitmapHandle */ ,
    (void *) (INT_PTR) -1 /* IPGSDataExporter::GetCommandHintText */ ,
    (void *) (INT_PTR) -1 /* IPGSDataExporter::Export */
};

const CInterfaceStubVtbl _IPGSDataExporterStubVtbl =
{
    &IID_IPGSDataExporter,
    &IPGSDataExporter_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IPGSDocumentation, ver. 0.0,
   GUID={0x45C667CB,0x67C4,0x4b2e,{0x89,0xCD,0x51,0xD0,0x7D,0x66,0x55,0x07}} */

#pragma code_seg(".orpc")
static const unsigned short IPGSDocumentation_FormatStringOffsetTable[] =
    {
    342,
    380,
    412
    };

static const MIDL_STUBLESS_PROXY_INFO IPGSDocumentation_ProxyInfo =
    {
    &Object_StubDesc,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDocumentation_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPGSDocumentation_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    PGSuperIEPlugin__MIDL_ProcFormatString.Format,
    &IPGSDocumentation_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _IPGSDocumentationProxyVtbl = 
{
    &IPGSDocumentation_ProxyInfo,
    &IID_IPGSDocumentation,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IPGSDocumentation::GetDocumentationSetName */ ,
    (void *) (INT_PTR) -1 /* IPGSDocumentation::LoadDocumentationMap */ ,
    (void *) (INT_PTR) -1 /* IPGSDocumentation::GetDocumentLocation */
};

const CInterfaceStubVtbl _IPGSDocumentationStubVtbl =
{
    &IID_IPGSDocumentation,
    &IPGSDocumentation_ServerInfo,
    6,
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
    PGSuperIEPlugin__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x800025b, /* MIDL Version 8.0.603 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _PGSuperIEPlugin_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IPGSDataImporterProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPGSDataExporterProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPGSDocumentationProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPGSProjectImporterProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _PGSuperIEPlugin_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IPGSDataImporterStubVtbl,
    ( CInterfaceStubVtbl *) &_IPGSDataExporterStubVtbl,
    ( CInterfaceStubVtbl *) &_IPGSDocumentationStubVtbl,
    ( CInterfaceStubVtbl *) &_IPGSProjectImporterStubVtbl,
    0
};

PCInterfaceName const _PGSuperIEPlugin_InterfaceNamesList[] = 
{
    "IPGSDataImporter",
    "IPGSDataExporter",
    "IPGSDocumentation",
    "IPGSProjectImporter",
    0
};


#define _PGSuperIEPlugin_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _PGSuperIEPlugin, pIID, n)

int __stdcall _PGSuperIEPlugin_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _PGSuperIEPlugin, 4, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _PGSuperIEPlugin, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _PGSuperIEPlugin, 4, *pIndex )
    
}

const ExtendedProxyFileInfo PGSuperIEPlugin_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _PGSuperIEPlugin_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _PGSuperIEPlugin_StubVtblList,
    (const PCInterfaceName * ) & _PGSuperIEPlugin_InterfaceNamesList,
    0, /* no delegation */
    & _PGSuperIEPlugin_IID_Lookup, 
    4,
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

