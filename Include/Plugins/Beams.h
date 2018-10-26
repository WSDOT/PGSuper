

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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

#ifndef __Beams_h__
#define __Beams_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDummyInterface_FWD_DEFINED__
#define __IDummyInterface_FWD_DEFINED__
typedef interface IDummyInterface IDummyInterface;

#endif 	/* __IDummyInterface_FWD_DEFINED__ */


#ifndef __IConfigureStrandMover_FWD_DEFINED__
#define __IConfigureStrandMover_FWD_DEFINED__
typedef interface IConfigureStrandMover IConfigureStrandMover;

#endif 	/* __IConfigureStrandMover_FWD_DEFINED__ */


#ifndef __WFBeamFactory_FWD_DEFINED__
#define __WFBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class WFBeamFactory WFBeamFactory;
#else
typedef struct WFBeamFactory WFBeamFactory;
#endif /* __cplusplus */

#endif 	/* __WFBeamFactory_FWD_DEFINED__ */


#ifndef __TaperedIBeamFactory_FWD_DEFINED__
#define __TaperedIBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class TaperedIBeamFactory TaperedIBeamFactory;
#else
typedef struct TaperedIBeamFactory TaperedIBeamFactory;
#endif /* __cplusplus */

#endif 	/* __TaperedIBeamFactory_FWD_DEFINED__ */


#ifndef __NUBeamFactory_FWD_DEFINED__
#define __NUBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class NUBeamFactory NUBeamFactory;
#else
typedef struct NUBeamFactory NUBeamFactory;
#endif /* __cplusplus */

#endif 	/* __NUBeamFactory_FWD_DEFINED__ */


#ifndef __UBeamFactory_FWD_DEFINED__
#define __UBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class UBeamFactory UBeamFactory;
#else
typedef struct UBeamFactory UBeamFactory;
#endif /* __cplusplus */

#endif 	/* __UBeamFactory_FWD_DEFINED__ */


#ifndef __UBeam2Factory_FWD_DEFINED__
#define __UBeam2Factory_FWD_DEFINED__

#ifdef __cplusplus
typedef class UBeam2Factory UBeam2Factory;
#else
typedef struct UBeam2Factory UBeam2Factory;
#endif /* __cplusplus */

#endif 	/* __UBeam2Factory_FWD_DEFINED__ */


#ifndef __BulbTeeFactory_FWD_DEFINED__
#define __BulbTeeFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class BulbTeeFactory BulbTeeFactory;
#else
typedef struct BulbTeeFactory BulbTeeFactory;
#endif /* __cplusplus */

#endif 	/* __BulbTeeFactory_FWD_DEFINED__ */


#ifndef __MultiWebFactory_FWD_DEFINED__
#define __MultiWebFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class MultiWebFactory MultiWebFactory;
#else
typedef struct MultiWebFactory MultiWebFactory;
#endif /* __cplusplus */

#endif 	/* __MultiWebFactory_FWD_DEFINED__ */


#ifndef __MultiWeb2Factory_FWD_DEFINED__
#define __MultiWeb2Factory_FWD_DEFINED__

#ifdef __cplusplus
typedef class MultiWeb2Factory MultiWeb2Factory;
#else
typedef struct MultiWeb2Factory MultiWeb2Factory;
#endif /* __cplusplus */

#endif 	/* __MultiWeb2Factory_FWD_DEFINED__ */


#ifndef __DoubleTeeFactory_FWD_DEFINED__
#define __DoubleTeeFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DoubleTeeFactory DoubleTeeFactory;
#else
typedef struct DoubleTeeFactory DoubleTeeFactory;
#endif /* __cplusplus */

#endif 	/* __DoubleTeeFactory_FWD_DEFINED__ */


#ifndef __VoidedSlabFactory_FWD_DEFINED__
#define __VoidedSlabFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class VoidedSlabFactory VoidedSlabFactory;
#else
typedef struct VoidedSlabFactory VoidedSlabFactory;
#endif /* __cplusplus */

#endif 	/* __VoidedSlabFactory_FWD_DEFINED__ */


#ifndef __VoidedSlab2Factory_FWD_DEFINED__
#define __VoidedSlab2Factory_FWD_DEFINED__

#ifdef __cplusplus
typedef class VoidedSlab2Factory VoidedSlab2Factory;
#else
typedef struct VoidedSlab2Factory VoidedSlab2Factory;
#endif /* __cplusplus */

#endif 	/* __VoidedSlab2Factory_FWD_DEFINED__ */


#ifndef __BoxBeamFactory_FWD_DEFINED__
#define __BoxBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class BoxBeamFactory BoxBeamFactory;
#else
typedef struct BoxBeamFactory BoxBeamFactory;
#endif /* __cplusplus */

#endif 	/* __BoxBeamFactory_FWD_DEFINED__ */


#ifndef __BoxBeam2Factory_FWD_DEFINED__
#define __BoxBeam2Factory_FWD_DEFINED__

#ifdef __cplusplus
typedef class BoxBeam2Factory BoxBeam2Factory;
#else
typedef struct BoxBeam2Factory BoxBeam2Factory;
#endif /* __cplusplus */

#endif 	/* __BoxBeam2Factory_FWD_DEFINED__ */


#ifndef __StrandMoverImpl_FWD_DEFINED__
#define __StrandMoverImpl_FWD_DEFINED__

#ifdef __cplusplus
typedef class StrandMoverImpl StrandMoverImpl;
#else
typedef struct StrandMoverImpl StrandMoverImpl;
#endif /* __cplusplus */

#endif 	/* __StrandMoverImpl_FWD_DEFINED__ */


#ifndef __TxDotDoubleTFactory_FWD_DEFINED__
#define __TxDotDoubleTFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class TxDotDoubleTFactory TxDotDoubleTFactory;
#else
typedef struct TxDotDoubleTFactory TxDotDoubleTFactory;
#endif /* __cplusplus */

#endif 	/* __TxDotDoubleTFactory_FWD_DEFINED__ */


#ifndef __DeckedSlabBeamFactory_FWD_DEFINED__
#define __DeckedSlabBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DeckedSlabBeamFactory DeckedSlabBeamFactory;
#else
typedef struct DeckedSlabBeamFactory DeckedSlabBeamFactory;
#endif /* __cplusplus */

#endif 	/* __DeckedSlabBeamFactory_FWD_DEFINED__ */


#ifndef __SplicedIBeamFactory_FWD_DEFINED__
#define __SplicedIBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class SplicedIBeamFactory SplicedIBeamFactory;
#else
typedef struct SplicedIBeamFactory SplicedIBeamFactory;
#endif /* __cplusplus */

#endif 	/* __SplicedIBeamFactory_FWD_DEFINED__ */


#ifndef __SplicedUBeamFactory_FWD_DEFINED__
#define __SplicedUBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class SplicedUBeamFactory SplicedUBeamFactory;
#else
typedef struct SplicedUBeamFactory SplicedUBeamFactory;
#endif /* __cplusplus */

#endif 	/* __SplicedUBeamFactory_FWD_DEFINED__ */


#ifndef __SplicedNUBeamFactory_FWD_DEFINED__
#define __SplicedNUBeamFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class SplicedNUBeamFactory SplicedNUBeamFactory;
#else
typedef struct SplicedNUBeamFactory SplicedNUBeamFactory;
#endif /* __cplusplus */

#endif 	/* __SplicedNUBeamFactory_FWD_DEFINED__ */


#ifndef __IBeamDistFactorEngineer_FWD_DEFINED__
#define __IBeamDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class IBeamDistFactorEngineer IBeamDistFactorEngineer;
#else
typedef struct IBeamDistFactorEngineer IBeamDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __IBeamDistFactorEngineer_FWD_DEFINED__ */


#ifndef __BoxBeamDistFactorEngineer_FWD_DEFINED__
#define __BoxBeamDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class BoxBeamDistFactorEngineer BoxBeamDistFactorEngineer;
#else
typedef struct BoxBeamDistFactorEngineer BoxBeamDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __BoxBeamDistFactorEngineer_FWD_DEFINED__ */


#ifndef __BulbTeeDistFactorEngineer_FWD_DEFINED__
#define __BulbTeeDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class BulbTeeDistFactorEngineer BulbTeeDistFactorEngineer;
#else
typedef struct BulbTeeDistFactorEngineer BulbTeeDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __BulbTeeDistFactorEngineer_FWD_DEFINED__ */


#ifndef __MultiWebDistFactorEngineer_FWD_DEFINED__
#define __MultiWebDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class MultiWebDistFactorEngineer MultiWebDistFactorEngineer;
#else
typedef struct MultiWebDistFactorEngineer MultiWebDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __MultiWebDistFactorEngineer_FWD_DEFINED__ */


#ifndef __UBeamDistFactorEngineer_FWD_DEFINED__
#define __UBeamDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class UBeamDistFactorEngineer UBeamDistFactorEngineer;
#else
typedef struct UBeamDistFactorEngineer UBeamDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __UBeamDistFactorEngineer_FWD_DEFINED__ */


#ifndef __VoidedSlabDistFactorEngineer_FWD_DEFINED__
#define __VoidedSlabDistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class VoidedSlabDistFactorEngineer VoidedSlabDistFactorEngineer;
#else
typedef struct VoidedSlabDistFactorEngineer VoidedSlabDistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __VoidedSlabDistFactorEngineer_FWD_DEFINED__ */


#ifndef __VoidedSlab2DistFactorEngineer_FWD_DEFINED__
#define __VoidedSlab2DistFactorEngineer_FWD_DEFINED__

#ifdef __cplusplus
typedef class VoidedSlab2DistFactorEngineer VoidedSlab2DistFactorEngineer;
#else
typedef struct VoidedSlab2DistFactorEngineer VoidedSlab2DistFactorEngineer;
#endif /* __cplusplus */

#endif 	/* __VoidedSlab2DistFactorEngineer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "WBFLGeometry.h"
#include "WBFLGenericBridgeTools.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IDummyInterface_INTERFACE_DEFINED__
#define __IDummyInterface_INTERFACE_DEFINED__

/* interface IDummyInterface */
/* [object][uuid] */ 


EXTERN_C const IID IID_IDummyInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D3810B3E-91D6-4aed-A748-8ABEB87FCF44")
    IDummyInterface : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Dummy( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDummyInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDummyInterface * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDummyInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDummyInterface * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Dummy )( 
            IDummyInterface * This);
        
        END_INTERFACE
    } IDummyInterfaceVtbl;

    interface IDummyInterface
    {
        CONST_VTBL struct IDummyInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDummyInterface_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDummyInterface_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDummyInterface_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDummyInterface_Dummy(This)	\
    ( (This)->lpVtbl -> Dummy(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDummyInterface_INTERFACE_DEFINED__ */


#ifndef __IConfigureStrandMover_INTERFACE_DEFINED__
#define __IConfigureStrandMover_INTERFACE_DEFINED__

/* interface IConfigureStrandMover */
/* [object][uuid] */ 


EXTERN_C const IID IID_IConfigureStrandMover;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49D89070-3BC9-4b30-97E1-496E0715F636")
    IConfigureStrandMover : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetHarpedStrandOffsetBounds( 
            /* [in] */ Float64 topGirderElevation,
            /* [in] */ Float64 Hg,
            /* [in] */ Float64 topStartElevationBoundary,
            /* [in] */ Float64 botStartElevationBoundary,
            /* [in] */ Float64 topHp1ElevationBoundary,
            /* [in] */ Float64 botHp1ElevationBoundary,
            /* [in] */ Float64 topHp2ElevationBoundary,
            /* [in] */ Float64 botHp2ElevationBoundary,
            /* [in] */ Float64 topEndElevationBoundary,
            /* [in] */ Float64 botEndElevationBoundary,
            /* [in] */ Float64 endIncrement,
            /* [in] */ Float64 hpIncrement) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ClearAll( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddRegion( 
            /* [in] */ IShape *shape,
            /* [in] */ Float64 arcSlope) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_NumRegions( 
            /* [retval][out] */ CollectionIndexType *pNum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetRegion( 
            /* [in] */ CollectionIndexType index,
            /* [out] */ IShape **shape,
            /* [out] */ Float64 *arcSlope) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IConfigureStrandMoverVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConfigureStrandMover * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConfigureStrandMover * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConfigureStrandMover * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetHarpedStrandOffsetBounds )( 
            IConfigureStrandMover * This,
            /* [in] */ Float64 topGirderElevation,
            /* [in] */ Float64 Hg,
            /* [in] */ Float64 topStartElevationBoundary,
            /* [in] */ Float64 botStartElevationBoundary,
            /* [in] */ Float64 topHp1ElevationBoundary,
            /* [in] */ Float64 botHp1ElevationBoundary,
            /* [in] */ Float64 topHp2ElevationBoundary,
            /* [in] */ Float64 botHp2ElevationBoundary,
            /* [in] */ Float64 topEndElevationBoundary,
            /* [in] */ Float64 botEndElevationBoundary,
            /* [in] */ Float64 endIncrement,
            /* [in] */ Float64 hpIncrement);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ClearAll )( 
            IConfigureStrandMover * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddRegion )( 
            IConfigureStrandMover * This,
            /* [in] */ IShape *shape,
            /* [in] */ Float64 arcSlope);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumRegions )( 
            IConfigureStrandMover * This,
            /* [retval][out] */ CollectionIndexType *pNum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetRegion )( 
            IConfigureStrandMover * This,
            /* [in] */ CollectionIndexType index,
            /* [out] */ IShape **shape,
            /* [out] */ Float64 *arcSlope);
        
        END_INTERFACE
    } IConfigureStrandMoverVtbl;

    interface IConfigureStrandMover
    {
        CONST_VTBL struct IConfigureStrandMoverVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConfigureStrandMover_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IConfigureStrandMover_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IConfigureStrandMover_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IConfigureStrandMover_SetHarpedStrandOffsetBounds(This,topGirderElevation,Hg,topStartElevationBoundary,botStartElevationBoundary,topHp1ElevationBoundary,botHp1ElevationBoundary,topHp2ElevationBoundary,botHp2ElevationBoundary,topEndElevationBoundary,botEndElevationBoundary,endIncrement,hpIncrement)	\
    ( (This)->lpVtbl -> SetHarpedStrandOffsetBounds(This,topGirderElevation,Hg,topStartElevationBoundary,botStartElevationBoundary,topHp1ElevationBoundary,botHp1ElevationBoundary,topHp2ElevationBoundary,botHp2ElevationBoundary,topEndElevationBoundary,botEndElevationBoundary,endIncrement,hpIncrement) ) 

#define IConfigureStrandMover_ClearAll(This)	\
    ( (This)->lpVtbl -> ClearAll(This) ) 

#define IConfigureStrandMover_AddRegion(This,shape,arcSlope)	\
    ( (This)->lpVtbl -> AddRegion(This,shape,arcSlope) ) 

#define IConfigureStrandMover_get_NumRegions(This,pNum)	\
    ( (This)->lpVtbl -> get_NumRegions(This,pNum) ) 

#define IConfigureStrandMover_GetRegion(This,index,shape,arcSlope)	\
    ( (This)->lpVtbl -> GetRegion(This,index,shape,arcSlope) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IConfigureStrandMover_INTERFACE_DEFINED__ */



#ifndef __PGSuperBeamFactory_LIBRARY_DEFINED__
#define __PGSuperBeamFactory_LIBRARY_DEFINED__

/* library PGSuperBeamFactory */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_PGSuperBeamFactory;

EXTERN_C const CLSID CLSID_WFBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("EF144A97-4C75-4234-AF3C-71DC89B1C8F8")
WFBeamFactory;
#endif

EXTERN_C const CLSID CLSID_TaperedIBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("9EDBDD8D-ABBB-413a-9B2D-9EB2712BE914")
TaperedIBeamFactory;
#endif

EXTERN_C const CLSID CLSID_NUBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("DA3C413D-6413-4485-BD29-E8A419E981AF")
NUBeamFactory;
#endif

EXTERN_C const CLSID CLSID_UBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("F72BA192-6D82-4d66-92CA-EB13466E6BA9")
UBeamFactory;
#endif

EXTERN_C const CLSID CLSID_UBeam2Factory;

#ifdef __cplusplus

class DECLSPEC_UUID("DD999E90-E181-4ec8-BB82-FDFB364B7620")
UBeam2Factory;
#endif

EXTERN_C const CLSID CLSID_BulbTeeFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("16CC6372-256E-4828-9BDA-3185C27DC65E")
BulbTeeFactory;
#endif

EXTERN_C const CLSID CLSID_MultiWebFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("51CB9247-8200-43e5-BFB0-E386C1E6A0D0")
MultiWebFactory;
#endif

EXTERN_C const CLSID CLSID_MultiWeb2Factory;

#ifdef __cplusplus

class DECLSPEC_UUID("D8824656-6CB6-45c0-B5F7-13CFFA890F0B")
MultiWeb2Factory;
#endif

EXTERN_C const CLSID CLSID_DoubleTeeFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("A367FEEC-9E24-4ed6-9DD3-57F62AD752C9")
DoubleTeeFactory;
#endif

EXTERN_C const CLSID CLSID_VoidedSlabFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("E6F28403-A396-453b-91BE-15A68D194255")
VoidedSlabFactory;
#endif

EXTERN_C const CLSID CLSID_VoidedSlab2Factory;

#ifdef __cplusplus

class DECLSPEC_UUID("171FC948-C4CB-4920-B9FC-72D729F3E91A")
VoidedSlab2Factory;
#endif

EXTERN_C const CLSID CLSID_BoxBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("B195CB90-CEB0-4495-B91D-7FC6DFBF31CF")
BoxBeamFactory;
#endif

EXTERN_C const CLSID CLSID_BoxBeam2Factory;

#ifdef __cplusplus

class DECLSPEC_UUID("B1F474E4-15F7-4ac8-AD2D-7FBD12E3B0EB")
BoxBeam2Factory;
#endif

EXTERN_C const CLSID CLSID_StrandMoverImpl;

#ifdef __cplusplus

class DECLSPEC_UUID("5D6AFD91-84F4-4755-9AF7-B760114A4551")
StrandMoverImpl;
#endif

EXTERN_C const CLSID CLSID_TxDotDoubleTFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("2583C7C1-FF57-4113-B45B-702CFA6AD013")
TxDotDoubleTFactory;
#endif

EXTERN_C const CLSID CLSID_DeckedSlabBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("DEFA27AD-3D22-481b-9006-627C65D2648F")
DeckedSlabBeamFactory;
#endif

EXTERN_C const CLSID CLSID_SplicedIBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("94272D8A-4914-44bb-A0D2-937A6A8BD7B0")
SplicedIBeamFactory;
#endif

EXTERN_C const CLSID CLSID_SplicedUBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("6A00C215-C139-4755-8DD5-929EE2DED305")
SplicedUBeamFactory;
#endif

EXTERN_C const CLSID CLSID_SplicedNUBeamFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("42D8D32D-7A4E-4d07-A5B1-220DFCB99EE6")
SplicedNUBeamFactory;
#endif

EXTERN_C const CLSID CLSID_IBeamDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("DFC0191F-F606-46aa-AC82-13B44878AA3B")
IBeamDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_BoxBeamDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("FBD11B34-65A4-4d87-90FF-39A42EEACABA")
BoxBeamDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_BulbTeeDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("49222DB8-F784-4576-811F-A831075FDDAF")
BulbTeeDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_MultiWebDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("27013EA9-0AA9-4b85-80D0-0904A3D762DA")
MultiWebDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_UBeamDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("62C15951-4A40-46cf-B326-D40D248F1299")
UBeamDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_VoidedSlabDistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("53884B6C-07D5-4e5e-9ED3-DF162DE1C27E")
VoidedSlabDistFactorEngineer;
#endif

EXTERN_C const CLSID CLSID_VoidedSlab2DistFactorEngineer;

#ifdef __cplusplus

class DECLSPEC_UUID("7B95DAC5-4E68-4e16-8504-22D21A0E6B52")
VoidedSlab2DistFactorEngineer;
#endif
#endif /* __PGSuperBeamFactory_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


