///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include "IBeamFactory.h"
#include "IBeamFactoryImp.h"
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

#include "IBeamDistFactorEngineer.h"
#include "BoxBeamDistFactorEngineer.h"
#include "BulbTeeDistFactorEngineer.h"
#include "UBeamDistFactorEngineer.h"
#include "MultiWebDistFactorEngineer.h"
#include "VoidedSlabDistFactorEngineer.h"
#include "VoidedSlab2DistFactorEngineer.h"

#include "PsBeamLossEngineer.h"
#include "TimeStepLossEngineer.h"

#include "SplicedIBeamFactoryImpl.h"
#include "SplicedNUBeamFactoryImpl.h"
#include "SplicedUBeamFactory.h"

#include "AgeAdjustedMaterial.h"

#include "NUDeckedBulbTeeFactory.h"
#include "PCIDeckedBulbTeeFactory.h"

#include <Plugins\Beams.h>
#include <Beams\Interfaces.h>

#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PsLossEngineer.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistributionFactors.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\Alignment.h>

#include "StrandMoverImpl.h"

#include <Beams\Helper.h>

#include <WBFLCore_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLGenericBridge_i.c>
#include <WBFLGenericBridgeTools_i.c>

#if defined _BETA_VERSION
#include <EAF\EAFUIIntegration.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

IndexType GetBeamTypeCount()
{
   return sizeof(gs_Beams)/sizeof(gs_Beams[0]);
}

CLSID GetBeamCLSID(IndexType index)
{
   return gs_Beams[index].first;
}

CATID GetBeamCATID(IndexType index)
{
   return gs_Beams[index].second;
}


BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_WFBeamFactory, CIBeamFactory)
   OBJECT_ENTRY(CLSID_UBeamFactory, CUBeamFactory)
   OBJECT_ENTRY(CLSID_UBeam2Factory, CUBeam2Factory)
   OBJECT_ENTRY(CLSID_NUBeamFactory, CNUBeamFactory)
   OBJECT_ENTRY(CLSID_BulbTeeFactory, CBulbTeeFactory)
   OBJECT_ENTRY(CLSID_MultiWebFactory, CMultiWebFactory)
   OBJECT_ENTRY(CLSID_MultiWeb2Factory, CMultiWeb2Factory)
   OBJECT_ENTRY(CLSID_DoubleTeeFactory, CDoubleTeeFactory)
   OBJECT_ENTRY(CLSID_VoidedSlabFactory, CVoidedSlabFactory)
   OBJECT_ENTRY(CLSID_VoidedSlab2Factory, CVoidedSlab2Factory)
   OBJECT_ENTRY(CLSID_BoxBeamFactory, CBoxBeamFactory)
   OBJECT_ENTRY(CLSID_BoxBeam2Factory, CBoxBeamFactory2)
   OBJECT_ENTRY(CLSID_TxDotDoubleTFactory, CTxDotDoubleTFactory)
   OBJECT_ENTRY(CLSID_TaperedIBeamFactory, CTaperedIBeamFactory)
   OBJECT_ENTRY(CLSID_DeckedSlabBeamFactory, CDeckedSlabBeamFactory)

   OBJECT_ENTRY(CLSID_SplicedIBeamFactory, CSplicedIBeamFactory)
   OBJECT_ENTRY(CLSID_SplicedNUBeamFactory, CSplicedNUBeamFactory)
   OBJECT_ENTRY(CLSID_SplicedUBeamFactory, CSplicedUBeamFactory)

   OBJECT_ENTRY(CLSID_WFBeamFamily, CIBeamFamily)
   OBJECT_ENTRY(CLSID_UBeamFamily, CUBeamFamily)
   OBJECT_ENTRY(CLSID_BoxBeamFamily, CBoxBeamFamily)
   OBJECT_ENTRY(CLSID_DeckBulbTeeBeamFamily, CDeckBulbTeeBeamFamily)
   OBJECT_ENTRY(CLSID_DoubleTeeBeamFamily, CDoubleTeeBeamFamily)
   OBJECT_ENTRY(CLSID_RibbedBeamFamily, CRibbedBeamFamily)
   OBJECT_ENTRY(CLSID_SlabBeamFamily, CSlabBeamFamily)
   OBJECT_ENTRY(CLSID_DeckedSlabBeamFamily, CDeckedSlabBeamFamily)

   OBJECT_ENTRY(CLSID_SplicedIBeamFamily, CSplicedIBeamFamily)
   OBJECT_ENTRY(CLSID_SplicedUBeamFamily, CSplicedUBeamFamily)

   OBJECT_ENTRY(CLSID_StrandMoverImpl, CStrandMoverImpl)

   OBJECT_ENTRY(CLSID_BoxBeamDistFactorEngineer, CBoxBeamDistFactorEngineer)
   OBJECT_ENTRY(CLSID_BulbTeeDistFactorEngineer, CBulbTeeDistFactorEngineer)
   OBJECT_ENTRY(CLSID_MultiWebDistFactorEngineer, CMultiWebDistFactorEngineer)
   OBJECT_ENTRY(CLSID_IBeamDistFactorEngineer, CIBeamDistFactorEngineer)
   OBJECT_ENTRY(CLSID_UBeamDistFactorEngineer, CUBeamDistFactorEngineer)
   OBJECT_ENTRY(CLSID_VoidedSlabDistFactorEngineer, CVoidedSlabDistFactorEngineer)
   OBJECT_ENTRY(CLSID_VoidedSlab2DistFactorEngineer, CVoidedSlab2DistFactorEngineer)

   OBJECT_ENTRY(CLSID_PsBeamLossEngineer, CPsBeamLossEngineer)
   OBJECT_ENTRY(CLSID_TimeStepLossEngineer, CTimeStepLossEngineer)

   OBJECT_ENTRY(CLSID_AgeAdjustedMaterial, CAgeAdjustedMaterial)

   OBJECT_ENTRY(CLSID_NUDeckedBulbTeeFactory, CNUDeckedBulbTeeFactory)
   OBJECT_ENTRY(CLSID_PCIDeckedBulbTeeFactory, CPCIDeckedBulbTeeFactory)
END_OBJECT_MAP()

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
	return CWinApp::InitInstance();
}

int CBeamsApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

//extern "C"
//BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
//{
//    if (dwReason == DLL_PROCESS_ATTACH)
//    {
//        _Module.Init(ObjectMap, hInstance);
//        DisableThreadLibraryCalls(hInstance);
//    }
//    else if (dwReason == DLL_PROCESS_DETACH)
//        _Module.Term();
//    return TRUE;    // ok
//}

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

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

void Register(bool bRegister)
{
   //////////////////////////////////////////////////////////////
   // Component categories for beam factories
   //////////////////////////////////////////////////////////////
   if ( bRegister )
   {
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Beam Factory CLSID Translator"), CATID_BeamFactoryCLSIDTranslator);

      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper I-Beam Factories"),             CATID_WFBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper U-Beam Factories"),             CATID_UBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Deck Bulb Tee Beam Factories"), CATID_DeckBulbTeeBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Double Tee Beam Factories"),    CATID_DoubleTeeBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Ribbed Beam Factories"),        CATID_RibbedBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Slab Beam Factories"),          CATID_SlabBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Box Beam Factories"),           CATID_BoxBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSuper Decked Slab Beam Factories"),   CATID_DeckedSlabBeamFactory);

      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSplice I-Beam Factories"),            CATID_SplicedIBeamFactory);
      WBFL::System::ComCatMgr::CreateCategory(CComBSTR("PGSplice U-Beam Factories"),            CATID_SplicedUBeamFactory);
   }
   else
   {
      WBFL::System::ComCatMgr::RemoveCategory(CATID_BeamFactoryCLSIDTranslator,true);

      WBFL::System::ComCatMgr::RemoveCategory(CATID_WFBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_UBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_DeckBulbTeeBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_DoubleTeeBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_RibbedBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_SlabBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_BoxBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_DeckedSlabBeamFactory,true);

      WBFL::System::ComCatMgr::RemoveCategory(CATID_SplicedIBeamFactory,true);
      WBFL::System::ComCatMgr::RemoveCategory(CATID_SplicedUBeamFactory,true);
   }

   //////////////////////////////////////////////////////////////
   // Register beam families
   //////////////////////////////////////////////////////////////

   // PGSuper
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_WFBeamFamily,          CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_UBeamFamily,           CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_BoxBeamFamily,         CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_DeckBulbTeeBeamFamily, CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_DoubleTeeBeamFamily,   CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_RibbedBeamFamily,      CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SlabBeamFamily,        CATID_PGSuperBeamFamily, bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_DeckedSlabBeamFamily,  CATID_PGSuperBeamFamily, bRegister );
   
   // PGSplice
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SplicedIBeamFamily, CATID_PGSpliceBeamFamily,  bRegister );
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SplicedUBeamFamily, CATID_PGSpliceBeamFamily,  bRegister );

   //////////////////////////////////////////////////////////////
   // Register beam factories (PGSuper)
   //////////////////////////////////////////////////////////////
   IndexType nBeams = GetBeamTypeCount();
   for ( IndexType idx = 0; idx < nBeams; idx++ )
   {
      CLSID clsid = GetBeamCLSID(idx);
      CATID catid = GetBeamCATID(idx);
      WBFL::System::ComCatMgr::RegWithCategory(clsid, catid, bRegister);
   }

   //////////////////////////////////////////////////////////////
   // Register beam factories (PGSplice)
   //////////////////////////////////////////////////////////////

   // I-beam factories
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SplicedIBeamFactory, CATID_SplicedIBeamFactory, bRegister);
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SplicedNUBeamFactory, CATID_SplicedIBeamFactory, bRegister);

   // U-beam factories
   WBFL::System::ComCatMgr::RegWithCategory(CLSID_SplicedUBeamFactory, CATID_SplicedUBeamFactory, bRegister);
}

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _Module.RegisterServer(FALSE);
   if ( FAILED(hr) )
      return hr;


   Register(true);

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   Register(false);

   _Module.UnregisterServer();
	return S_OK;
}


