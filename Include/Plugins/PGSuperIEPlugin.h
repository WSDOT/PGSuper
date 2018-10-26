

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __PGSuperIEPlugin_h__
#define __PGSuperIEPlugin_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPGSDataImporter_FWD_DEFINED__
#define __IPGSDataImporter_FWD_DEFINED__
typedef interface IPGSDataImporter IPGSDataImporter;

#endif 	/* __IPGSDataImporter_FWD_DEFINED__ */


#ifndef __IPGSProjectImporter_FWD_DEFINED__
#define __IPGSProjectImporter_FWD_DEFINED__
typedef interface IPGSProjectImporter IPGSProjectImporter;

#endif 	/* __IPGSProjectImporter_FWD_DEFINED__ */


#ifndef __IPGSDataExporter_FWD_DEFINED__
#define __IPGSDataExporter_FWD_DEFINED__
typedef interface IPGSDataExporter IPGSDataExporter;

#endif 	/* __IPGSDataExporter_FWD_DEFINED__ */


#ifndef __IPGSDocumentation_FWD_DEFINED__
#define __IPGSDocumentation_FWD_DEFINED__
typedef interface IPGSDocumentation IPGSDocumentation;

#endif 	/* __IPGSDocumentation_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLCore.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_PGSuperIEPlugin_0000_0000 */
/* [local] */ 

// {289D1CFF-D1A4-4b65-B673-867D7F41C7DB}
DEFINE_GUID(CATID_PGSuperProjectImporter,
0x289d1cff, 0xd1a4, 0x4b65, 0xb6, 0x73, 0x86, 0x7d, 0x7f, 0x41, 0xc7, 0xdb);
// {BD3B6F1E-7826-478b-99C0-A946C12C89CF}
DEFINE_GUID(CATID_PGSuperDataImporter, 
0xbd3b6f1e, 0x7826, 0x478b, 0x99, 0xc0, 0xa9, 0x46, 0xc1, 0x2c, 0x89, 0xcf);
// {369A62A2-8995-4404-9C16-15AE5A0681E2}
DEFINE_GUID(CATID_PGSuperDataExporter, 
0x369a62a2, 0x8995, 0x4404, 0x9c, 0x16, 0x15, 0xae, 0x5a, 0x6, 0x81, 0xe2);
// {DB115813-3828-4564-A2FA-D8DDB368B1DB}
DEFINE_GUID(CATID_PGSpliceProjectImporter, 
0xdb115813, 0x3828, 0x4564, 0xa2, 0xfa, 0xd8, 0xdd, 0xb3, 0x68, 0xb1, 0xdb);
// {88E6E707-A7EA-431a-B787-41377D75E0F3}
DEFINE_GUID(CATID_PGSpliceDataImporter, 
0x88e6e707, 0xa7ea, 0x431a, 0xb7, 0x87, 0x41, 0x37, 0x7d, 0x75, 0xe0, 0xf3);
// {D889AF1D-0CA1-4f01-AA2D-84F8F9F3A2DD}
DEFINE_GUID(CATID_PGSpliceDataExporter, 
0xd889af1d, 0xca1, 0x4f01, 0xaa, 0x2d, 0x84, 0xf8, 0xf9, 0xf3, 0xa2, 0xdd);


extern RPC_IF_HANDLE __MIDL_itf_PGSuperIEPlugin_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_PGSuperIEPlugin_0000_0000_v0_0_s_ifspec;

#ifndef __IPGSDataImporter_INTERFACE_DEFINED__
#define __IPGSDataImporter_INTERFACE_DEFINED__

/* interface IPGSDataImporter */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IPGSDataImporter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98B3DF17-7E0E-4d4a-B8A2-5443914FC608")
    IPGSDataImporter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            UINT nCmdID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMenuText( 
            /* [out] */ BSTR *bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitmapHandle( 
            /* [out] */ HBITMAP *phBmp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCommandHintText( 
            /* [out] */ BSTR *bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ IBroker *pBroker) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPGSDataImporterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPGSDataImporter * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPGSDataImporter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPGSDataImporter * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IPGSDataImporter * This,
            UINT nCmdID);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenuText )( 
            IPGSDataImporter * This,
            /* [out] */ BSTR *bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmapHandle )( 
            IPGSDataImporter * This,
            /* [out] */ HBITMAP *phBmp);
        
        HRESULT ( STDMETHODCALLTYPE *GetCommandHintText )( 
            IPGSDataImporter * This,
            /* [out] */ BSTR *bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *Import )( 
            IPGSDataImporter * This,
            /* [in] */ IBroker *pBroker);
        
        END_INTERFACE
    } IPGSDataImporterVtbl;

    interface IPGSDataImporter
    {
        CONST_VTBL struct IPGSDataImporterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPGSDataImporter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPGSDataImporter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPGSDataImporter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPGSDataImporter_Init(This,nCmdID)	\
    ( (This)->lpVtbl -> Init(This,nCmdID) ) 

#define IPGSDataImporter_GetMenuText(This,bstrText)	\
    ( (This)->lpVtbl -> GetMenuText(This,bstrText) ) 

#define IPGSDataImporter_GetBitmapHandle(This,phBmp)	\
    ( (This)->lpVtbl -> GetBitmapHandle(This,phBmp) ) 

#define IPGSDataImporter_GetCommandHintText(This,bstrText)	\
    ( (This)->lpVtbl -> GetCommandHintText(This,bstrText) ) 

#define IPGSDataImporter_Import(This,pBroker)	\
    ( (This)->lpVtbl -> Import(This,pBroker) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPGSDataImporter_INTERFACE_DEFINED__ */


#ifndef __IPGSProjectImporter_INTERFACE_DEFINED__
#define __IPGSProjectImporter_INTERFACE_DEFINED__

/* interface IPGSProjectImporter */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IPGSProjectImporter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5DB8B1D3-C91D-4e62-81E6-A7B64B0D38FD")
    IPGSProjectImporter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetItemText( 
            /* [out] */ BSTR *bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ IBroker *pBroker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIcon( 
            /* [out] */ HICON *phIcon) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPGSProjectImporterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPGSProjectImporter * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPGSProjectImporter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPGSProjectImporter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemText )( 
            IPGSProjectImporter * This,
            /* [out] */ BSTR *bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *Import )( 
            IPGSProjectImporter * This,
            /* [in] */ IBroker *pBroker);
        
        HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            IPGSProjectImporter * This,
            /* [out] */ HICON *phIcon);
        
        END_INTERFACE
    } IPGSProjectImporterVtbl;

    interface IPGSProjectImporter
    {
        CONST_VTBL struct IPGSProjectImporterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPGSProjectImporter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPGSProjectImporter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPGSProjectImporter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPGSProjectImporter_GetItemText(This,bstrText)	\
    ( (This)->lpVtbl -> GetItemText(This,bstrText) ) 

#define IPGSProjectImporter_Import(This,pBroker)	\
    ( (This)->lpVtbl -> Import(This,pBroker) ) 

#define IPGSProjectImporter_GetIcon(This,phIcon)	\
    ( (This)->lpVtbl -> GetIcon(This,phIcon) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPGSProjectImporter_INTERFACE_DEFINED__ */


#ifndef __IPGSDataExporter_INTERFACE_DEFINED__
#define __IPGSDataExporter_INTERFACE_DEFINED__

/* interface IPGSDataExporter */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IPGSDataExporter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF6EC18A-43D2-4ea1-BC7F-54365DD645DA")
    IPGSDataExporter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            UINT nCmdID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMenuText( 
            /* [out] */ BSTR *bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitmapHandle( 
            /* [out] */ HBITMAP *phBmp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCommandHintText( 
            /* [out] */ BSTR *bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Export( 
            /* [in] */ IBroker *pBroker) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPGSDataExporterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPGSDataExporter * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPGSDataExporter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPGSDataExporter * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IPGSDataExporter * This,
            UINT nCmdID);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenuText )( 
            IPGSDataExporter * This,
            /* [out] */ BSTR *bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmapHandle )( 
            IPGSDataExporter * This,
            /* [out] */ HBITMAP *phBmp);
        
        HRESULT ( STDMETHODCALLTYPE *GetCommandHintText )( 
            IPGSDataExporter * This,
            /* [out] */ BSTR *bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *Export )( 
            IPGSDataExporter * This,
            /* [in] */ IBroker *pBroker);
        
        END_INTERFACE
    } IPGSDataExporterVtbl;

    interface IPGSDataExporter
    {
        CONST_VTBL struct IPGSDataExporterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPGSDataExporter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPGSDataExporter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPGSDataExporter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPGSDataExporter_Init(This,nCmdID)	\
    ( (This)->lpVtbl -> Init(This,nCmdID) ) 

#define IPGSDataExporter_GetMenuText(This,bstrText)	\
    ( (This)->lpVtbl -> GetMenuText(This,bstrText) ) 

#define IPGSDataExporter_GetBitmapHandle(This,phBmp)	\
    ( (This)->lpVtbl -> GetBitmapHandle(This,phBmp) ) 

#define IPGSDataExporter_GetCommandHintText(This,bstrText)	\
    ( (This)->lpVtbl -> GetCommandHintText(This,bstrText) ) 

#define IPGSDataExporter_Export(This,pBroker)	\
    ( (This)->lpVtbl -> Export(This,pBroker) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPGSDataExporter_INTERFACE_DEFINED__ */


#ifndef __IPGSDocumentation_INTERFACE_DEFINED__
#define __IPGSDocumentation_INTERFACE_DEFINED__

/* interface IPGSDocumentation */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IPGSDocumentation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45C667CB-67C4-4b2e-89CD-51D07D665507")
    IPGSDocumentation : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDocumentationSetName( 
            BSTR *pbstrName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE LoadDocumentationMap( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDocumentLocation( 
            UINT nHID,
            BSTR *pbstrURL) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPGSDocumentationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPGSDocumentation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPGSDocumentation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPGSDocumentation * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDocumentationSetName )( 
            IPGSDocumentation * This,
            BSTR *pbstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *LoadDocumentationMap )( 
            IPGSDocumentation * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDocumentLocation )( 
            IPGSDocumentation * This,
            UINT nHID,
            BSTR *pbstrURL);
        
        END_INTERFACE
    } IPGSDocumentationVtbl;

    interface IPGSDocumentation
    {
        CONST_VTBL struct IPGSDocumentationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPGSDocumentation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPGSDocumentation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPGSDocumentation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPGSDocumentation_GetDocumentationSetName(This,pbstrName)	\
    ( (This)->lpVtbl -> GetDocumentationSetName(This,pbstrName) ) 

#define IPGSDocumentation_LoadDocumentationMap(This)	\
    ( (This)->lpVtbl -> LoadDocumentationMap(This) ) 

#define IPGSDocumentation_GetDocumentLocation(This,nHID,pbstrURL)	\
    ( (This)->lpVtbl -> GetDocumentLocation(This,nHID,pbstrURL) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPGSDocumentation_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long *, unsigned long            , HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserMarshal(  unsigned long *, unsigned char *, HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long *, unsigned char *, HBITMAP * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long *, HBITMAP * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


