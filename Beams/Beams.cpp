///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// Beams.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Beamsps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

#include <Plugins\Beams.h>

#include "BeamFamilyImpl.h"

#include "UBeamFactory.h"
#include "UBeam2Factory.h"
#include "NUBeamFactory.h"
#include "TaperedIBeamFactory.h"
#include "BulbTeeFactory.h"
#include "MultiWebFactory.h"
#include "MultiWeb2Factory.h"
#include "DoubleTeeFactory.h"
#include "VoidedSlabFactory.h"
#include "VoidedSlabFactory2.h"
#include "BoxBeamFactory.h"
#include "BoxBeamFactory2.h"
#include "TxDotDoubleTFactory.h"
#include "DeckedSlabBeamFactory.h"

#include <Beams/IBeamFactoryImp.h>

#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>

#include <Beams/SplicedIBeamFactoryImpl.h>
#include "SplicedNUBeamFactoryImpl.h"
#include "SplicedUBeamFactory.h"

#include "AgeAdjustedMaterial.h"

#include "NUDeckedBulbTeeFactory.h"
#include "PCIDeckedBulbTeeFactory.h"

#include <Beams\Interfaces.h>

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PsLossEngineer.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistributionFactors.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF/EAFProgress.h>
#include <EAF\EAFStatusCenter.h>

#include "StrandMoverImpl.h"

#include <Beams\Helper.h>


#include <WBFLGeometry_i.c>
#include <WBFLGenericBridge_i.c>
#include <WBFLGenericBridgeTools_i.c>

#if defined _BETA_VERSION
#include <EAF\EAFUIIntegration.h>
#endif

#include <EAF\ComponentModule.h>

WBFL::EAF::ComponentModule Module_;

using namespace PGS::Beams;

CComModule _Module;

#define BEGIN_BEAM_FACTORY_LIST static const std::pair<CLSID,CATID> gs_Beams[] = {
#define BEAM_FACTORY(_ClassID_,_CatID_) std::make_pair(_ClassID_,_CatID_),
#define END_BEAM_FACTORY_LIST };

BEGIN_BEAM_FACTORY_LIST
   // I-beam factories
   BEAM_FACTORY(CLSID_WFBeamFactory,       CATID_WFBeamFactory)
   BEAM_FACTORY(CLSID_NUBeamFactory,       CATID_WFBeamFactory)
   BEAM_FACTORY(CLSID_TaperedIBeamFactory, CATID_WFBeamFactory)

   // U-beam factories
   BEAM_FACTORY(CLSID_UBeamFactory,        CATID_UBeamFactory)
   BEAM_FACTORY(CLSID_UBeam2Factory,       CATID_UBeamFactory)

   // Deck bulb tee factories
   BEAM_FACTORY(CLSID_BulbTeeFactory, CATID_DeckBulbTeeBeamFactory)
   BEAM_FACTORY(CLSID_NUDeckedBulbTeeFactory, CATID_DeckBulbTeeBeamFactory)
   BEAM_FACTORY(CLSID_PCIDeckedBulbTeeFactory, CATID_DeckBulbTeeBeamFactory)

   // Double-tee factories
   BEAM_FACTORY(CLSID_DoubleTeeFactory,    CATID_DoubleTeeBeamFactory)
   BEAM_FACTORY(CLSID_TxDotDoubleTFactory, CATID_DoubleTeeBeamFactory)
   BEAM_FACTORY(CLSID_MultiWeb2Factory,    CATID_DoubleTeeBeamFactory)

   // Ribbed beam factories
   BEAM_FACTORY(CLSID_MultiWebFactory,     CATID_RibbedBeamFactory)

   // Slab factories
   BEAM_FACTORY(CLSID_VoidedSlabFactory,   CATID_SlabBeamFactory)
   BEAM_FACTORY(CLSID_VoidedSlab2Factory,  CATID_SlabBeamFactory)

   // box beam factories
   BEAM_FACTORY(CLSID_BoxBeamFactory,       CATID_BoxBeamFactory)
   BEAM_FACTORY(CLSID_BoxBeam2Factory,      CATID_BoxBeamFactory)

   // decked slab beam factories
   BEAM_FACTORY(CLSID_DeckedSlabBeamFactory,      CATID_DeckedSlabBeamFactory)
END_BEAM_FACTORY_LIST

IndexType PGS::Beams::GetBeamTypeCount()
{
   return sizeof(gs_Beams)/sizeof(gs_Beams[0]);
}

CLSID PGS::Beams::GetBeamCLSID(IndexType index)
{
   return gs_Beams[index].first;
}

CATID PGS::Beams::GetBeamCATID(IndexType index)
{
   return gs_Beams[index].second;
}

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_StrandMoverImpl, CStrandMoverImpl)
   OBJECT_ENTRY(CLSID_AgeAdjustedMaterial, CAgeAdjustedMaterial)
END_OBJECT_MAP()


EAF_BEGIN_OBJECT_MAP(ObjectMap2)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_WFBeamFactory, IBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_UBeamFactory, UBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_UBeam2Factory, UBeam2Factory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_NUBeamFactory, NUBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_BulbTeeFactory, BulbTeeFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_MultiWebFactory, MultiWebFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_MultiWeb2Factory, MultiWeb2Factory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_DoubleTeeFactory, DoubleTeeFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_VoidedSlabFactory, VoidedSlabFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_VoidedSlab2Factory, VoidedSlab2Factory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_BoxBeamFactory, BoxBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_BoxBeam2Factory, BoxBeamFactory2)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_TxDotDoubleTFactory, TxDotDoubleTFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_TaperedIBeamFactory, TaperedIBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_DeckedSlabBeamFactory, DeckedSlabBeamFactory)

   EAF_OBJECT_ENTRY_SINGLETON(CLSID_NUDeckedBulbTeeFactory, NUDeckedBulbTeeFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_PCIDeckedBulbTeeFactory, PCIDeckedBulbTeeFactory)

   EAF_OBJECT_ENTRY_SINGLETON(CLSID_SplicedIBeamFactory, SplicedIBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_SplicedNUBeamFactory, SplicedNUBeamFactory)
   EAF_OBJECT_ENTRY_SINGLETON(CLSID_SplicedUBeamFactory, SplicedUBeamFactory)

   EAF_OBJECT_ENTRY(CLSID_WFBeamFamily, CIBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_UBeamFamily, CUBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_BoxBeamFamily, CBoxBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_DeckBulbTeeBeamFamily, CDeckBulbTeeBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_DoubleTeeBeamFamily, CDoubleTeeBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_RibbedBeamFamily, CRibbedBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_SlabBeamFamily, CSlabBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_DeckedSlabBeamFamily, CDeckedSlabBeamFamily)

   EAF_OBJECT_ENTRY(CLSID_SplicedIBeamFamily, CSplicedIBeamFamily)
   EAF_OBJECT_ENTRY(CLSID_SplicedUBeamFamily, CSplicedUBeamFamily)
EAF_END_OBJECT_MAP()

class CBeamsApp : public CWinApp
{
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
};

CBeamsApp theApp;


BOOL CBeamsApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
   Module_.Init(ObjectMap2);
	return CWinApp::InitInstance();
}

int CBeamsApp::ExitInstance()
{
	_Module.Term();
   Module_.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _Module.RegisterServer(FALSE);
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   _Module.UnregisterServer();
	return S_OK;
}
