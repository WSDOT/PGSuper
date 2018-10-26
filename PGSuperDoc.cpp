///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// PGSuperDoc.cpp : implementation of the CPGSuperDoc class
//
#include "stdafx.h"

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <objbase.h>
#include <initguid.h>

#include "PGSuper.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"

#include <WBFLCore_i.c>
#include <WBFLReportManagerAgent_i.c>
#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>
#include <WBFLDManip_i.c>
#include <WBFLDManipTools_i.c>


#include "SupportDrawStrategy.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategy.h"
#include "DistributedLoadDrawStrategy.h"
#include "MomentLoadDrawStrategy.h"
#include "GevEditLoad.h"


#include "HtmlHelp\HelpTopics.hh"

#include "Convert.h"

#include <ComCat.h>

#include "PGSuperCatCom.h"

#include "Hints.h"

#include "PGSuperException.h"
#include <System\FileStream.h>
#include <System\StructuredLoadXmlPrs.h>

// Helpers
#include <MathEx.h>

// Agents
#include <BridgeAgent\BridgeAgent.h>
#include <ProjectAgent\ProjectAgent.h>
#include <AnalysisAgent\AnalysisAgent.h>
#include <EngAgent\EngAgent.h>
#include <SpecAgent\SpecAgent.h>
#include "DocProxyAgent.h"

// Dialogs
#include <MfcTools\DocTemplateFinder.h>
#include <MfcTools\LoadModifiersDlg.h>
#include "ProjectPropertiesDlg.h"
#include "UnitsDlg.h"
#include "EnvironmentDlg.h"
#include "BridgeDescDlg.h"
#include "GirderDescDlg.h"
#include "SpecDlg.h"
#include "BridgeEditorSettingsSheet.h"
#include "GirderEditorSettingsSheet.h"
#include "DesignGirderDlg.h"
#include "DesignOutcomeDlg.h"
#include "CopyGirderDlg.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LiveLoadSelectDlg.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include "AlignmentDescriptionDlg.h"
#include "SelectGirderDlg.h"
#include "SelectItemDlg.h"
#include "StructuralAnalysisMethodDlg.h"
#include "SpanDetailsDlg.h"
#include "PierDetailsDlg.h"
#include "GirderLabelFormatDlg.h"

#include <IFace\Test1250.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Artifact.h>
#include <IFace\TxDOTCadExport.h>

#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\AutoProgress.h>
#include <PgsExt\DesignArtifact.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>

// Transactions
#include <System\TxnManager.h>
#include "EditAlignment.h"
#include "EditBridge.h"
#include "EditPier.h"
#include "EditSpan.h"
#include "EditGirder.h"
#include "DesignGirder.h"
#include "InsertDeleteSpan.h"
#include "EditLLDF.h"
#include "ChangeUnits.h"
#include "CopyGirder.h"
#include "EditEnvironment.h"
#include "EditProjectCriteria.h"
#include "EditLiveLoad.h"
#include "EditAnalysisType.h"
#include "InsertDeleteLoad.h"

// Logging
#include <iostream>
#include <fstream>
#include <System\Time.h>

#include <algorithm>

#include <PgsExt\StatusItem.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const Float64 FILE_VERSION=2.0;

static bool DoesFolderExist(const CString& dirname);
static bool DoesFileExist(const CString& filname);

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc

IMPLEMENT_DYNCREATE(CPGSuperDoc, CDocument)

BEGIN_MESSAGE_MAP(CPGSuperDoc, CDocument)
	//{{AFX_MSG_MAP(CPGSuperDoc)
	ON_COMMAND(ID_FILE_PROJECT_PROPERTIES, OnFileProjectProperties)
	ON_COMMAND(ID_PROJECT_UNITS, OnProjectUnits)
   ON_COMMAND(ID_UNITS_SI,OnSiUnits)
   ON_UPDATE_COMMAND_UI(ID_UNITS_SI, OnUpdateSiUnits)
   ON_COMMAND(ID_UNITS_US,OnUsUnits)
   ON_UPDATE_COMMAND_UI(ID_UNITS_US, OnUpdateUsUnits)
	ON_COMMAND(ID_PROJECT_ENVIRONMENT, OnProjectEnvironment)
	ON_COMMAND(ID_PROJECT_BRIDGEDESC, OnProjectBridgeDesc)
	ON_COMMAND(ID_PROJECT_SPEC, OnProjectSpec)
	ON_COMMAND(ID_PROJECT_AUTOCALC, OnProjectAutoCalc)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_AUTOCALC, OnUpdateProjectAutoCalc)
	ON_COMMAND(IDM_EXPORT_TEMPLATE, OnExportToTemplateFile)
	ON_COMMAND(ID_VIEWSETTINGS_BRIDGEMODELEDITOR, OnViewsettingsBridgemodelEditor)
	ON_COMMAND(ID_LOADS_LOADMODIFIERS, OnLoadsLoadModifiers)
	ON_COMMAND(ID_VIEWSETTINGS_GIRDEREDITOR, OnViewsettingsGirderEditor)
	ON_COMMAND(ID_PROJECT_DESIGNGIRDER, OnProjectDesignGirder)
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECT, OnProjectDesignGirderDirect)
   ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDERDIRECT, OnUpdateProjectDesignGirderDirect)
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnProjectDesignGirderDirectHoldSlabOffset)
   ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnUpdateProjectDesignGirderDirectHoldSlabOffset)
	ON_COMMAND(IDM_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(IDM_COPY_GIRDER_PROPS, OnCopyGirderProps)
	ON_COMMAND(IDM_IMPORT_PROJECT_LIBRARY, OnImportProjectLibrary)
	ON_COMMAND(ID_ADD_POINTLOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMomentLoad)
	ON_COMMAND(ID_PROJECT_ALIGNMENT, OnProjectAlignment)
   ON_COMMAND(ID_VIEW_ANALYSISRESULTS,OnViewAnalysisResults)
	ON_COMMAND(ID_PROJECT_ANALYSIS, OnProjectAnalysis)
	ON_COMMAND(ID_PROJECT_PIERDESC, OnEditPier)
	ON_COMMAND(ID_PROJECT_SPANDESC, OnEditSpan)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
	ON_COMMAND(ID_EDIT_UNDO, OnUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateUndo)
	ON_COMMAND(ID_EDIT_REDO, OnRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateRedo)
	ON_COMMAND(ID_LOADS_LLDF, OnLoadsLldf)
   ON_COMMAND(IDR_LIVE_LOADS,OnLiveLoads)
	ON_COMMAND(ID_INSERT, OnInsert)
	ON_COMMAND(ID_OPTIONS_HINTS, OnOptionsHints)
	ON_COMMAND(ID_OPTIONS_LABELS, OnOptionsLabels)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_PROJECT_UPDATENOW, CAutoCalcDoc::OnUpdateNow)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_UPDATENOW, CAutoCalcDoc::OnUpdateUpdateNow)
	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_STATUSCENTER, ID_VIEW_STATUSCENTER3, OnUpdateStatusCenter)
	ON_COMMAND_RANGE(ID_VIEW_STATUSCENTER, ID_VIEW_STATUSCENTER3, OnViewStatusCenter)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CPGSuperDoc, CDocument)
	//{{AFX_DISPATCH_MAP(CPGSuperDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//      DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IPGSuper to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {59D503E6-265C-11D2-8EB0-006097DF3C68}
static const IID IID_IPGSuper =
{ 0x59d503e6, 0x265c, 0x11d2, { 0x8e, 0xb0, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68 } };

BEGIN_INTERFACE_MAP(CPGSuperDoc, CDocument)
	INTERFACE_PART(CPGSuperDoc, IID_IPGSuper, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc construction/destruction

CPGSuperDoc::CPGSuperDoc():
m_CurrentPierIdx(-1),
m_CurrentSpanIdx(-1),
m_CurrentGirderIdx(-1),
m_DesignSlabOffset(true),
m_bIsReportMenuPopulated(false),
m_StatusCenterDlg(m_StatusCenter)
{
	EnableAutomation();

	AfxOleLockApp();

   m_LibMgr.SetName( "PGSuper Library" );

   m_pBroker = 0;

   m_StatusCenter.SinkEvents(this);
}

CPGSuperDoc::~CPGSuperDoc()
{
   m_DocUnitSystem.Release();
   AfxOleUnlockApp();
}

void CPGSuperDoc::OnUpdateError(const CString& errorMsg)
{
   CString my_string = errorMsg;
   UpdateAllViews(0,HINT_UPDATEERROR,(CObject*)&my_string);
}

bool CPGSuperDoc::IsAutoCalcEnabled() const
{
   return theApp.IsAutoCalcEnabled();
}

void CPGSuperDoc::EnableAutoCalc(bool bEnable)
{
   bool bWasDisabled = !IsAutoCalcEnabled();

   theApp.EnableAutoCalc( bEnable );

   // If AutoCalc was off and now it is on,
   // Update the views.
   if ( bWasDisabled && IsAutoCalcEnabled() )
     OnUpdateNow();
}

bool CPGSuperDoc::IsDesignFlexureEnabled() const
{
   return theApp.IsDesignFlexureEnabled();
}

void CPGSuperDoc::EnableDesignFlexure( bool bEnable )
{
   theApp.EnableDesignFlexure(bEnable);
}

bool CPGSuperDoc::IsDesignShearEnabled() const
{
   return theApp.IsDesignShearEnabled();
}

void CPGSuperDoc::EnableDesignShear( bool bEnable )
{
   theApp.EnableDesignShear(bEnable);
}

void CPGSuperDoc::OnLibMgrChanged(psgLibraryManager* pNewLibMgr)
{
   GET_IFACE( ILibrary, pLib );
   pLib->SetLibraryManager( pNewLibMgr );
}

// libISupportLibraryManager implementation
int CPGSuperDoc::GetNumberOfLibraryManagers() const
{
   return 1;
}

libLibraryManager* CPGSuperDoc::GetLibraryManager(int num)
{
   PRECONDITION( num == 0 );
   return &m_LibMgr;
}

libLibraryManager* CPGSuperDoc::GetTargetLibraryManager()
{
   return &m_LibMgr;
}

void CPGSuperDoc::SetUnitsMode(libUnitsMode::Mode mode)
{
   // This method should never be called from PGSuper.
   // This method must be implemented because it is part of the
   // ISupportLibraryManager interface and is called from the library editor
   // utility program.
   ASSERT(false);
}

libUnitsMode::Mode CPGSuperDoc::GetUnitsMode() const
{
   GET_IFACE( IProjectSettings, pProjSettings );
   Int32 units = pProjSettings->GetUnitsMode();
   return (units == pgsTypes::umUS ? libUnitsMode::UNITS_US : libUnitsMode::UNITS_SI );
}

void CPGSuperDoc::GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem)
{
   (*ppDocUnitSystem) = m_DocUnitSystem;
   (*ppDocUnitSystem)->AddRef();
}

HRESULT CPGSuperDoc::GetBroker(IBroker** ppBroker)
{
   (*ppBroker) = m_pBroker;
   (*ppBroker)->AddRef();
   return S_OK;
}

pgsStatusCenter& CPGSuperDoc::GetStatusCenter()
{
   return m_StatusCenter;
}

void CPGSuperDoc::EditAlignmentDescription(int nPage)
{
   GET_IFACE(IRoadwayData,pAlignment);
   CAlignmentDescriptionDlg dlg("Alignment Description",m_pBroker);

   dlg.m_AlignmentPage.m_AlignmentData = pAlignment->GetAlignmentData2();
   dlg.m_ProfilePage.m_ProfileData = pAlignment->GetProfileData2();
   dlg.m_CrownSlopePage.m_RoadwaySectionData = pAlignment->GetRoadwaySectionData();

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditAlignment* pTxn = new txnEditAlignment(pAlignment->GetAlignmentData2(),     dlg.m_AlignmentPage.m_AlignmentData,
                                                    pAlignment->GetProfileData2(),       dlg.m_ProfilePage.m_ProfileData,
                                                    pAlignment->GetRoadwaySectionData(), dlg.m_CrownSlopePage.m_RoadwaySectionData );

      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CPGSuperDoc::EditBridgeDescription(int nPage)
{
   GET_IFACE(IRoadwayData,pAlignment);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(ILiveLoads, pLiveLoads);
   GET_IFACE(IEnvironment, pEnvironment );

   const CBridgeDescription* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
   double oldRelHumidity = pEnvironment->GetRelHumidity();

   CBridgeDescDlg dlg;
   dlg.SetBridgeDescription(*pOldBridgeDesc);

   dlg.m_EnvironmentalPage.m_Exposure    = oldExposureCondition == expNormal ? 0 : 1;
   dlg.m_EnvironmentalPage.m_RelHumidity = oldRelHumidity;

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      bool bOldBridgeHasSidewalks = (pOldBridgeDesc->GetLeftRailingSystem()->bUseSidewalk || pOldBridgeDesc->GetRightRailingSystem()->bUseSidewalk);
      bool bOldEnablePedLL = pLiveLoads->IsPedestianLoadEnabled(pgsTypes::lltDesign);

      const CBridgeDescription& newBridge = dlg.GetBridgeDescription();
      bool bNewEnablePedLL = false;
      bool bNewBridgeHasSidewalks = (newBridge.GetLeftRailingSystem()->bUseSidewalk || newBridge.GetRightRailingSystem()->bUseSidewalk);

      if ( !bOldBridgeHasSidewalks && bNewBridgeHasSidewalks )
      {
         // sidewalks were added, enable pedestrian live load
         bNewEnablePedLL = true;
      }
      else if ( bOldBridgeHasSidewalks && !bNewBridgeHasSidewalks )
      {
         // sidewalks were removed, disable pedestrian live load
         bNewEnablePedLL = false;
      }
      else
      {
         // sidewalks stayed the same to don't change anything
         bNewEnablePedLL = bOldEnablePedLL;
      }

      txnEditBridge* pTxn = new txnEditBridge(*pOldBridgeDesc,      dlg.GetBridgeDescription(),
                                              oldExposureCondition, dlg.m_EnvironmentalPage.m_Exposure == 0 ? expNormal : expSevere,
                                              oldRelHumidity,       dlg.m_EnvironmentalPage.m_RelHumidity,
                                              bOldEnablePedLL,      bNewEnablePedLL);

      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

bool CPGSuperDoc::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   const CBridgeDescription* pBridge = pBridgeDesc->GetBridgeDescription();
 
   const CPierData* pPierData = pBridge->GetPier(pierIdx);

   CPierDetailsDlg dlg(pPierData);
   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditPierData oldPierData(pPierData);
      txnEditPierData newPierData = dlg.GetEditPierData();

      txnEditPier* pTxn = new txnEditPier(pierIdx,oldPierData,newPierData,dlg.GetMovePierOption());
      txnTxnManager::GetInstance()->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDoc::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   const CBridgeDescription* pBridge = pBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpanData = pBridgeDesc->GetSpan(spanIdx);

   CSpanDetailsDlg dlg(pSpanData);
   txnEditSpanData oldData(pSpanData);

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditSpanData newData = dlg.GetEditSpanData();
      txnEditSpan* pTxn = new txnEditSpan(spanIdx,oldData,newData);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDoc::EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage)
{
   GET_IFACE(IGirderData,pGirderData);

   GET_IFACE(IShear,pShear);
   GET_IFACE(ILongitudinalRebar,pLongitudinalRebar);
   GET_IFACE(IGirderLifting,pGirderLifting);
   GET_IFACE(IGirderHauling,pGirderHauling);
   GET_IFACE(IBridge,pBridge);

   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;

   SpanIndexType nspans = pBridge->GetSpanCount();
   if (span < nspans)
      spanIdx = span;
   else
      spanIdx = nspans-1;

   GirderIndexType ngrds = pBridge->GetGirderCount(m_CurrentSpanIdx == ALL_SPANS ? 0 : m_CurrentSpanIdx);
   if ( m_CurrentGirderIdx < ngrds)
      gdrIdx = girder;
   else
      gdrIdx = ngrds-1;

   // collect current values for later undo
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   bool bUseSameGirder              = pBridgeDesc->UseSameGirderForEntireBridge();
   std::string strGirderName        = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderName(gdrIdx);
   CGirderData girderData           = pGirderData->GetGirderData(spanIdx,gdrIdx);
   CShearData  shearData            = pShear->GetShearData(spanIdx,gdrIdx);
   CLongitudinalRebarData rebarData = pLongitudinalRebar->GetLongitudinalRebarData( spanIdx, gdrIdx );
   double liftingLocation           = pGirderLifting->GetLeftLiftingLoopLocation( spanIdx, gdrIdx );
   double trailingOverhang          = pGirderHauling->GetTrailingOverhang( spanIdx, gdrIdx );
   double leadingOverhang           = pGirderHauling->GetLeadingOverhang( spanIdx, gdrIdx );

   pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();
   Float64 slabOffset[2];
   pIBridgeDesc->GetSlabOffset(spanIdx,gdrIdx,&slabOffset[pgsTypes::metStart],&slabOffset[pgsTypes::metEnd]);

   CGirderDescDlg dlg(spanIdx,gdrIdx);
   dlg.m_strGirderName = strGirderName;
   dlg.m_GirderData    = girderData;

   // resequence page if no debonding
   CGirderData girderDatad = pGirderData->GetGirderData(dlg.m_CurrentSpanIdx,dlg.m_CurrentGirderIdx);
   if (EGD_DEBONDING <= nPage  && !girderData.bSymmetricDebond)
      nPage--;

   dlg.SetActivePage(nPage);

   int st = dlg.DoModal();
   if ( st == IDOK )
   {
      txnEditGirder* pTxn = new txnEditGirder(spanIdx,gdrIdx,
                                              bUseSameGirder,   dlg.m_General.m_bUseSameGirderType,
                                              strGirderName,    dlg.m_strGirderName,
                                              girderData,       dlg.m_GirderData,
                                              shearData,        dlg.m_Shear.m_ShearData, 
                                              rebarData,        dlg.m_LongRebar.m_RebarData ,
                                              liftingLocation,  dlg.m_Lifting.m_LiftingLocation ,
                                              trailingOverhang, dlg.m_Lifting.m_TrailingOverhang,
                                              leadingOverhang,  dlg.m_Lifting.m_LeadingOverhang,
                                              slabOffsetType,   dlg.m_General.m_SlabOffsetType,
                                              slabOffset[pgsTypes::metStart], dlg.m_General.m_SlabOffset[pgsTypes::metStart],
                                              slabOffset[pgsTypes::metEnd], dlg.m_General.m_SlabOffset[pgsTypes::metEnd]
                                              );

      txnTxnManager::GetInstance()->Execute(pTxn);

      return true;
   }
   else
   {
      return false;
   }
}

bool CPGSuperDoc::EditPointLoad(CollectionIndexType loadIdx)
{
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData& loadData = pUserDefinedLoads->GetPointLoad(loadIdx);

   CEditPointLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditPointLoad* pTxn = new txnEditPointLoad(loadIdx,loadData,dlg.m_Load);
         txnTxnManager::GetInstance()->Execute(pTxn);
         return true;
      }
   }

   return false;
}

bool CPGSuperDoc::EditDistributedLoad(CollectionIndexType loadIdx)
{
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData& loadData = pUserDefinedLoads->GetDistributedLoad(loadIdx);

   CEditDistributedLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditDistributedLoad* pTxn = new txnEditDistributedLoad(loadIdx,loadData,dlg.m_Load);
         txnTxnManager::GetInstance()->Execute(pTxn);
         return true;
      }
   }

   return false;
}

bool CPGSuperDoc::EditMomentLoad(CollectionIndexType loadIdx)
{
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData& loadData = pUserDefinedLoads->GetMomentLoad(loadIdx);

   CEditMomentLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditMomentLoad* pTxn = new txnEditMomentLoad(loadIdx,loadData,dlg.m_Load);
         txnTxnManager::GetInstance()->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDoc::EditGirderViewSettings(int nPage)
{
   CPGSuperApp* papp =(CPGSuperApp*) AfxGetApp();
   UINT settings = papp->GetGirderEditorSettings();

	CGirderEditorSettingsSheet dlg(IDS_GM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      papp->SetGirderEditorSettings(settings);

      // tell the world we've changed settings
      UpdateAllViews( 0, HINT_GIRDERVIEWSETTINGSCHANGED, 0 );
   }
}

void CPGSuperDoc::EditBridgeViewSettings(int nPage)
{
   CPGSuperApp* papp =(CPGSuperApp*) AfxGetApp();
   UINT settings = papp->GetBridgeEditorSettings();

	CBridgeEditorSettingsSheet dlg(IDS_BM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      settings |= IDB_PV_DRAW_ISOTROPIC;
      papp->SetBridgeEditorSettings(settings);
   }

   // tell the world we've changed settings
   UpdateAllViews( 0, HINT_BRIDGEVIEWSETTINGSCHANGED, 0 );
	
}

BOOL CPGSuperDoc::UpdateTemplates()
{
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();

   CString workgroup_folder;
   pApp->GetTemplateFolders(workgroup_folder);

   if  ( !Init() ) // load the agents and other necessary stuff
      return false;

   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   CFileFind dir_finder;
   BOOL bMoreDir = dir_finder.FindFile(workgroup_folder+"\\*");

   while ( bMoreDir )
   {
      bMoreDir = dir_finder.FindNextFile();
      if ( dir_finder.IsDots() || !dir_finder.IsDirectory() )
         continue;

      CString strDir = dir_finder.GetFilePath();

      CString strMessage;
      strMessage.Format("Updating templates in %s",dir_finder.GetFileTitle());
      pProgress->UpdateMessage(strMessage);

      CFileFind template_finder;
      BOOL bMoreTemplates = template_finder.FindFile(strDir + "\\*.pgt");
      while ( bMoreTemplates )
      {
         bMoreTemplates = template_finder.FindNextFile();
         CString strTemplate = template_finder.GetFilePath();

         strMessage.Format("Updating %s - %s",dir_finder.GetFileTitle(),template_finder.GetFileTitle());
         pProgress->UpdateMessage(strMessage);

         if ( !OpenTheDocument(strTemplate) )
            return FALSE;

         SaveTheDocument(strTemplate);

         m_pBroker->Reset();

         CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pInit(m_pBroker);
         pInit->InitAgents();
      }
   }

   return FALSE; // didn't really open a file
}

BOOL CPGSuperDoc::OnNewDocument()
{
   // set up to read from a template file if possible
   // get template file directories out of app class
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();

   if ( pApp->UpdatingTemplate() )
      return UpdateTemplates();



	if (!CDocument::OnNewDocument())
		return FALSE;

   CString workgroup_folder;
   pApp->GetTemplateFolders(workgroup_folder);

   // load up vector of directory locations
   // check to make sure that template directories are valid
   std::vector<std::string> dirvec;
   if (DoesFolderExist(workgroup_folder))
   {
      dirvec.push_back(std::string(workgroup_folder));
   }
   else
   {
      if (!workgroup_folder.IsEmpty())
      {
         CString msg = "The current Workgroup Templates Folder " + workgroup_folder +
            " cannot be accessed.\nTo correct this problem, " +
            "select Program Settings from the File menu and specify a valid folder location";
         AfxMessageBox(msg);
      }
   }

   // get template file suffix
   CString template_suffix;
   VERIFY(template_suffix.LoadString(IDS_TEMPLATE_FILE_SUFFIX));
   ASSERT(!template_suffix.IsEmpty());

   // create a template finder
   mfcDocTemplateFinder fndlg(dirvec, std::string(template_suffix));
   //   fndlg.SetDefaultFileName("Default Project", "Normal");
   fndlg.OmitDefaultFile(true);

   // set the template icon for the list box
   HICON hentry = pApp->LoadIcon(IDR_PGSUPER_TEMPLATE_ICON);
   ASSERT(hentry);
   fndlg.SetIcon(hentry);

   // get the file name, or let user cancel
   std::string fn;
   UINT mode = theApp.GetDocTemplateViewMode();
   fndlg.SetListMode( mode == 0 ? mfcDocTemplateFinder::SmallIconMode :
                      mode == 1 ? mfcDocTemplateFinder::LargeIconMode :
                      mode == 2 ? mfcDocTemplateFinder::ReportMode : mfcDocTemplateFinder::SmallIconMode );

   mfcDocTemplateFinder::GetTemplateFileResult res = fndlg.GetTemplateFile(fn);

   if ( res == mfcDocTemplateFinder::NoTemplatesAvailable )
   {
      AfxMessageBox("PGSuper project templates could not be found. Update the User and Workgroup Template Folders.",MB_OK | MB_ICONSTOP);
      pApp->OnProgramSettings(TRUE);
      return FALSE;
   }

   CWaitCursor cursor; // The rest can talk a while... Show the wait cursor

   switch( fndlg.GetListMode() )
   {
   case mfcDocTemplateFinder::SmallIconMode:
      theApp.SetDocTemplateViewMode(0);
      break;

   case mfcDocTemplateFinder::LargeIconMode:
      theApp.SetDocTemplateViewMode(1);
      break;

   case mfcDocTemplateFinder::ReportMode:
      theApp.SetDocTemplateViewMode(2);
      break;
   }

   if ( res == mfcDocTemplateFinder::CancelledSelection )
      return FALSE;

   // Initialize the document (loads agents, etc)
   if ( !Init() )
      return FALSE;

   if (res==mfcDocTemplateFinder::ValidFileSelected)
   {
      if ( !OpenTheDocument( fn.c_str() ) )
         return FALSE; // There was an error creating the file
   }
   else
   {
      ATLASSERT(false); // RAB: should never get here (I got rid of the "default project")
   }

   InitProjectProperties();

   SetModifiedFlag( TRUE );

   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->EnableModifiedFlag( IsModified() );

   // update the mainframe title
   pMainFrame->UpdateFrameTitle("Untitled");

   return TRUE;
}

BOOL CPGSuperDoc::OnImportDocument(UINT nID)
{
	if (!CDocument::OnNewDocument())
		return FALSE;

   if ( !Init() )
      return FALSE;

   // get the importer
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();
   CComPtr<IPGSuperImporter> importer;
   pApp->GetImporter(nID,&importer);


   // NOTE: These events are held here, but not released in this function
   //       See below
   GET_IFACE(IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE(IUIEvents,pUIEvents);
   pUIEvents->HoldEvents();

   // do the importing
   try
   {
      if ( FAILED(importer->Import(m_pBroker)) )
      {
         return FALSE;
      }
   }
   catch(...)
   {
   }


   InitProjectProperties();
   SetModifiedFlag( TRUE );

   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->EnableModifiedFlag( IsModified() );

   // update the mainframe title
   pMainFrame->UpdateFrameTitle("Untitled");

   // NOTE: Don't release the events here. Doing so will caush a crash
   //       because the views aren't created yet.
   //       The calling function CPGSuperImportPluginDocTemplate::Import()
   //       will release the events after a call to InitialUpdateFrame() which
   //       creates the views
   //pEvents->FirePendingEvents(); 
   //pUIEvents->HoldEvents(false); // stop holding events, but don't fire then either

   UpdateAnalysisTypeStatusIndicator();

   return TRUE;
}

BOOL CPGSuperDoc::OnExportDocument(UINT nID)
{
   // get the importer
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();
   CComPtr<IPGSuperExporter> exporter;
   pApp->GetExporter(nID,&exporter);

   // do the exporting
   if ( FAILED(exporter->Export(m_pBroker)) )
   {
      return FALSE;
   }

   return TRUE;
}

void CPGSuperDoc::InitProjectProperties()
{
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();
   
   CString engineer_name = pApp->GetEngineerName();
   CString company = pApp->GetEngineerCompany();

   GET_IFACE( IProjectProperties, pProjProp );

   pProjProp->SetEngineer(std::string(engineer_name));
   pProjProp->SetCompany(std::string(company));

   if ( pApp->ShowProjectPropertiesOnNewProject() )
      OnFileProjectProperties();
}

BOOL CPGSuperDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
   CWaitCursor cursor;

   // We want to by-pass the default OnOpenDocument and do our own thing
	//if (!CDocument::OnOpenDocument(lpszPathName))
	//	return FALSE;
	
   if ( !Init() )
      return FALSE;

   BOOL st = OpenTheDocument( lpszPathName );

   // update the mainframe title
   if (st==TRUE)
   {
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      pMainFrame->UpdateFrameTitle(lpszPathName);
   }

   return st;
}

BOOL CPGSuperDoc::OpenTheDocument(LPCTSTR lpszPathName)
{
   HRESULT hr = S_OK;
   CComQIPtr<IBrokerPersist,&IID_IBrokerPersist> pPersist(m_pBroker);
   ASSERT( pPersist );

   CString real_file_name; // name of actual file to be read may be different than lpszPathName
   long convert_status = ConvertTheDocument(lpszPathName, &real_file_name);
   // convert document. if file was converted, then we need to delete the converted file at the end
   if ( -1== convert_status)
   {
      return FALSE;
   }

   {
      // NOTE: this scoping block is here for a reason. The IStructuredLoad must be
      //       destroyed before the file can be deleted.
      CComPtr<IStructuredLoad> pStrLoad;
      hr = ::CoCreateInstance( CLSID_StructuredLoad, NULL, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
      if ( FAILED(hr) )
      {
         // We are not aggregating so we should CoCreateInstance should
         // never fail with this HRESULT
         ASSERT( hr != CLASS_E_NOAGGREGATION );

         HandleOpenDocumentError( hr, lpszPathName );
         return FALSE;
      }

      hr = pStrLoad->Open( real_file_name );
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
         return FALSE;
      }

   
      // get the file version
      hr = pStrLoad->BeginUnit("PGSuper");
      if ( FAILED(hr) )
         return FALSE;

      double ver;
      pStrLoad->get_Version(&ver);
      if ( 1.0 < ver )
      {
         CComVariant var;
         var.vt = VT_BSTR;
         pStrLoad->get_Property("Version",&var);
#if defined _DEBUG
         USES_CONVERSION;
         TRACE("Loading data saved with PGSuper Version %s\n",OLE2A(var.bstrVal));
      }
      else
      {
         TRACE("Loading data saved with PGSuper Version 2.1 or earlier\n");
#endif
      } // clses the bracket for if ( 1.0 < ver )
      

      hr = pPersist->Load( pStrLoad );
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
         return FALSE;
      }


   // end unit wrapping entire file
   try
   {
      if (S_OK != pStrLoad->EndUnit())
         return E_FAIL;
   }
   catch(...)
   {
      return E_FAIL;
   }
      
      hr = pStrLoad->Close();
      if ( FAILED(hr) )
      {
         HandleOpenDocumentError( hr, lpszPathName );
         return FALSE;
      }
   }

   if (convert_status==1)
   {
      // file was converted and written to a temporary file. delete the temp file
      VERIFY(::DeleteFile(real_file_name));
   }

   // sets the status bar indicator for structural analysis type
   UpdateAnalysisTypeStatusIndicator();

   m_DocUnitSystem->put_UnitMode(GetUnitMode() == pgsTypes::umUS ? umUS : umSI);
	return TRUE;
}


long CPGSuperDoc::ConvertTheDocument(LPCTSTR lpszPathName, CString* prealFileName)
{
   // Open the document and look at the second line
   // If the version tag is 0.80, then the document needs to be converted

   std::ifstream ifile(lpszPathName);
   if ( !ifile )
   {
      HandleConvertDocumentError(E_INVALIDARG,lpszPathName);
      return -1;
   }

   char line[50];
   ifile.getline(line,50);
   ifile.getline(line,50);
   CString strLine(line);
   strLine.TrimLeft();
   int loc = strLine.Find(">",0);
   if (loc!=-1)
   {
      strLine = strLine.Left(loc+1);
      if ( strLine == CString("<PGSuper version=\"0.8\">") ||
           strLine == CString("<PGSuper version=\"0.83\">")  )
      {
         ifile.close();

         _PgsFileConvert1 convert;
         if ( !convert.CreateDispatch("convert.PgsFileConvert1") )
         {
            HandleConvertDocumentError(REGDB_E_CLASSNOTREG,lpszPathName);
            return -1;
         }

         BSTR bstrPathName = CString(lpszPathName).AllocSysString();
         BSTR bstrRealName=0;
         long convert_status = convert.Convert( &bstrPathName, &bstrRealName );
         if ( convert_status == -1  )
         {
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
            HandleConvertDocumentError(E_FAIL,lpszPathName);
            return -1;
         }
         else if (convert_status==1)
         {
            // file was converted
            ::SysFreeString(bstrPathName);
            *prealFileName = CString(bstrRealName);
            ::SysFreeString(bstrRealName);
            return 1;
         }
         else
         {
            // converter did not convert file - this should be impossible becuase
            // we are checking versions above. must be wrong version of the converter
            CHECK(0);
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
         }
      }
      else
      {
         // no file conversion needed - give realFile same name as MFC's file
         *prealFileName = CString(lpszPathName);
         return 0;
      }
   }
   else
   {
      HandleConvertDocumentError(REGDB_E_CLASSNOTREG,lpszPathName);
      return -1;
   }

   return -1;
}

BOOL CPGSuperDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
   // Not using MFC Serialization so don't call base class.
//	return CDocument::OnSaveDocument(lpszPathName);

   GET_IFACE( IProjectLog, pLog );

   // Make sure this file actually exists before we attempt to back it up
   BOOL bDidCopy = false;
   CString strBackup(lpszPathName);

   HANDLE hFile;
   WIN32_FIND_DATA find_file_data;
   hFile = ::FindFirstFile( lpszPathName, &find_file_data );
   if ( hFile != INVALID_HANDLE_VALUE )
   {
      ::FindClose(hFile); // don't want no stinkin resource leaks.
      // OK, The file exists.
      // check to make sure it's not read-only
	   DWORD dwAttrib = GetFileAttributes(lpszPathName);
	   if (dwAttrib & FILE_ATTRIBUTE_READONLY)
      {
         CString msg;
         msg.Format("Cannot save file. The file %s is read-only. Please try to save again to a different file.", lpszPathName);
         AfxMessageBox(msg );
         return FALSE;
      }

      // Create a backup copy of the last good save.
      // Backup filename is orginial filename, except the first
      // letter is a ~.
      int idx = strBackup.ReverseFind( '\\' ); // look for last '\'. 
                                               // This is one character before the 
                                               // beginning of the filename
      ASSERT( idx != -1 ); // '\' wasn't found
      idx++;
      strBackup.SetAt(idx,'~');

      bDidCopy = ::CopyFile( lpszPathName, strBackup, FALSE );
      if ( !bDidCopy && AfxMessageBox(IDS_E_UNSAFESAVE,MB_YESNO) == IDNO )
      {
         return FALSE;
      }
   }

   // Attempt to save the document
   if ( !SaveTheDocument( lpszPathName ) && bDidCopy )
   {
      // Save failed... Restore the backup

      // Delete the bad save
      BOOL bDidDelete = ::DeleteFile( lpszPathName );
      if ( !bDidDelete )
      {
         // Opps... Couldn't delete it.
         // Alter the user so he's not screwed.
         CString msg;
         
         pLog->LogMessage("");
         pLog->LogMessage("An error occured while recovering your last successful save.");
         msg.Format("It is highly likely that the file %s is corrupt.", lpszPathName);
         pLog->LogMessage( msg );
         pLog->LogMessage("To recover from this error,");
         msg.Format("   1. Delete %s", lpszPathName );
         pLog->LogMessage( msg );
         msg.Format("   2. Rename %s to %s", strBackup, lpszPathName );
         pLog->LogMessage( msg );
         pLog->LogMessage("");

         std::string strLogFileName = pLog->GetName();

         AfxFormatString2( msg, IDS_E_SAVERECOVER1, lpszPathName, CString(strLogFileName.c_str()) );
         AfxMessageBox(msg );
      }

      if ( bDidDelete )
      {
         // OK, We were able to delete the bad save.
         // Rename the backup to the original name.
         BOOL bDidMove = ::MoveFile( strBackup, lpszPathName ); // Rename the file
         if ( !bDidMove )
         {
            // Opps... A file with the original name is gone, and we can't
            // rename the backup to the file with the orignal name.
            // Alert the user so he's not screwed.
            CString msg;

            pLog->LogMessage("");
            pLog->LogMessage("An error occured while recovering your last successful save.");
            msg.Format("It is highly likely that the file %s no longer exists.", lpszPathName);
            pLog->LogMessage( msg );
            pLog->LogMessage("To recover from this error,");
            msg.Format("   1. If %s exists, delete it.", lpszPathName );
            pLog->LogMessage( msg );
            msg.Format("   2. Rename %s to %s", strBackup, lpszPathName );
            pLog->LogMessage( msg );
            pLog->LogMessage("");

            std::string strLogFileName = pLog->GetName();

            AfxFormatString2( msg, IDS_E_SAVERECOVER2, lpszPathName, CString(strLogFileName.c_str()) );
            AfxMessageBox( msg );
         }
      }

      return FALSE;
   }
   else
   {
      // Save was successful... Delete the backup if one was created
      if ( bDidCopy )
         ::DeleteFile( strBackup );
      // It's no big deal if this call fails.  The user is simply left
      // with an out of date backup file on their disk drive.
   }

   SetModifiedFlag( FALSE );

   // update title frame
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->UpdateFrameTitle(lpszPathName);

   return TRUE;
}

/*--------------------------------------------------------------------*/
BOOL CPGSuperDoc::SaveTheDocument(LPCTSTR lpszPathName)
{
   HRESULT hr = S_OK;
   CComQIPtr<IBrokerPersist,&IID_IBrokerPersist> pPersist( m_pBroker );
   ASSERT( pPersist );

   CComPtr<IStructuredSave> pStrSave;

   hr = ::CoCreateInstance( CLSID_StructuredSave, NULL, 
	   CLSCTX_INPROC_SERVER, IID_IStructuredSave, (void**)&pStrSave );
   if ( FAILED(hr) )
   {
      // We are not aggregating so we should CoCreateInstance should
      // never fail with this HRESULT
      ASSERT( hr != CLASS_E_NOAGGREGATION );

      HandleSaveDocumentError( hr, lpszPathName );
      return FALSE;
   }

   hr = pStrSave->Open( lpszPathName );
   if ( FAILED(hr) )
   {
      HandleSaveDocumentError( hr, lpszPathName );
      return FALSE;
   }

   // a unit to wrap the whole file with
   pStrSave->BeginUnit("PGSuper", FILE_VERSION);
   pStrSave->put_Property("Version",CComVariant(theApp.GetVersion(true)));

   hr = pPersist->Save( pStrSave );
   if ( FAILED(hr) )
   {
      HandleSaveDocumentError( hr, lpszPathName );
      return FALSE;
   }


   pStrSave->EndUnit(); // PGSuper

   hr = pStrSave->Close();
   if ( FAILED(hr) )
   {
      HandleSaveDocumentError( hr, lpszPathName );
      return FALSE;
   }
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc serialization

void CPGSuperDoc::Serialize(CArchive& ar)
{
   ASSERT(FALSE); // We should never get here!!!

	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc diagnostics

#ifdef _DEBUG
void CPGSuperDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPGSuperDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc commands

// Use this call when the project agent is messed up and
// you can't use its logging features.
void fail_safe_log_message( const char* msg )
{
   std::ofstream ofile("PGSuper.log");
   sysTime now;
   now.PrintDate( true );
   ofile << "Log opened " << now << std::endl;

   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
   CString strVersion = pApp->GetVersionString(true);
   ofile << strVersion.LockBuffer() << std::endl;
   strVersion.UnlockBuffer();

   ofile << msg << std::endl;
   
   ofile << "Log closed " << now << std::endl;

   ofile.close();
}

BOOL CPGSuperDoc::Init()
{
   BOOL bAgentsLoaded = LoadAgents();
   if ( !bAgentsLoaded )
   {
      CString msg, msg1, msg2;

      msg1.LoadString( IDS_E_BADINSTALL );
      AfxFormatString1( msg2, IDS_E_PROBPERSISTS, "PGSuper.log" );
      AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
      AfxMessageBox( msg );

      return FALSE;
   }

   // load up the library manager
   if ( !LoadMasterLibrary() )
      return FALSE;

   // Setup the library manager (same as if it changed)
   OnLibMgrChanged( &m_LibMgr );

   GET_IFACE(IVersionInfo,pVerInfo);
   std::string ver( theApp.GetVersionString(false).GetBuffer(100) );
   pVerInfo->SetVersionString( ver );
   pVerInfo->SetVersion( theApp.GetVersion(false).GetBuffer(100) );

   CPGSuperApp* pApp =(CPGSuperApp*)AfxGetApp();
   CComPtr<IAppUnitSystem> appUnitSystem;
   pApp->GetAppUnitSystem(&appUnitSystem);
   CreateDocUnitSystem(appUnitSystem,&m_DocUnitSystem);

   return TRUE;
}

BOOL CPGSuperDoc::LoadAgents()
{
   HRESULT hr;
   hr = ::CoCreateInstance( CLSID_Broker2, NULL, CLSCTX_INPROC_SERVER, IID_IBroker, (void**)&m_TheRealBroker );
   if ( FAILED(hr) )
   {
      std::ostringstream msg;
      msg << "Failed to create broker. hr = " << hr << std::endl << std::ends;
      fail_safe_log_message( msg.str().c_str() );
      return FALSE;
   }

   AGENT_SET_BROKER(m_TheRealBroker);

   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   if ( pBrokerInit == NULL )
   {
      fail_safe_log_message("Wrong version of Broker installed\nRe-install PGSuper");
      return FALSE;
   }

   pBrokerInit->DelayInit();


   // create component category manager
   CComPtr<ICatRegister> pICatReg = 0;
   hr = ::CoCreateInstance( CLSID_StdComponentCategoriesMgr,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ICatRegister,
                            (void**)&pICatReg );
   if ( FAILED(hr) )
   {
      std::ostringstream msg;
      msg << "Failed to create Component Category Manager. hr = " << hr << std::endl;
      msg << "Is the correct version of Internet Explorer installed?" << std::endl << std::ends;
      fail_safe_log_message( msg.str().c_str() );
      return FALSE;
   }

   CComQIPtr<ICatInformation,&IID_ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID = 0;

   const int nID = 1;
   CATID ID[nID];
   ID[0] = CATID_PGSuperAgent;

   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   // load up to 10 agents at a time
   const int nMaxAgents = 10;
   CLSID clsid[nMaxAgents];

   ULONG nAgentsLoaded = 0;
   bool bSomeAgentsLoaded = false;
   while (SUCCEEDED(pIEnumCLSID->Next(nMaxAgents,clsid,&nAgentsLoaded)) && 0 < nAgentsLoaded )
   {
      if ( !LoadAgents(pBrokerInit, clsid, nAgentsLoaded) )
         return FALSE;

      bSomeAgentsLoaded = true;
   }

   if ( nAgentsLoaded == 0 && !bSomeAgentsLoaded )
   {
      std::ostringstream msg;
      msg << "There are no PGSuper Agents registered." << std::endl;
      msg << "Please re-install PGSuper"<< std::endl;
      fail_safe_log_message( msg.str().c_str() );
      return FALSE;
   }

   // load special agents
   clsid[0] = CLSID_SysAgent;
   clsid[1] = CLSID_ReportManager;
   if ( !LoadAgents( pBrokerInit, clsid, 2) )
      return FALSE;


   // Load up the proxy agent -> This guy does all the UpdateAllViews calls when
   // an event occurs in an agent.
   pgsDocProxyAgent* pProxyAgent = new pgsDocProxyAgent;
   pProxyAgent->AddRef();
   pProxyAgent->SetDocument( this );
   hr = pBrokerInit->AddAgent( pProxyAgent );
   if ( FAILED( hr ) )
   {
      pProxyAgent->Release();
      pProxyAgent = 0;
      return FALSE;
   }
   pProxyAgent->Release();
   pProxyAgent = 0;

   pBrokerInit->InitAgents();

   return TRUE;
}

BOOL CPGSuperDoc::LoadAgents(IBrokerInitEx2* pBrokerInit, CLSID* pClsid, long nClsid)
{
   long lErrIndex = 0;
   HRESULT hr = pBrokerInit->LoadAgents( pClsid, nClsid, &lErrIndex );
   if ( FAILED(hr) )
   {
      LPOLESTR pszCLSID;
      StringFromCLSID( pClsid[lErrIndex], &pszCLSID );
      CString strCLSID(pszCLSID);
      ::CoTaskMemFree( (LPVOID)pszCLSID );

      LPOLESTR pszProgID;
      ProgIDFromCLSID( pClsid[lErrIndex], &pszProgID );
      CString strProgID(pszProgID);
      ::CoTaskMemFree( (LPVOID)pszProgID );

      std::ostringstream msg;
      msg << "Failed to load agent. hr = " << hr << std::endl;
      msg << "CLSID = " << strCLSID.LockBuffer() << std::endl;
      msg << "ProgID = " << strProgID.LockBuffer() << std::endl << std::ends;
      fail_safe_log_message( msg.str().c_str() );

      strCLSID.UnlockBuffer();
      strProgID.UnlockBuffer();

      return FALSE;
   }
   return TRUE;
}

void CPGSuperDoc::OnFileProjectProperties() 
{
   GET_IFACE( IProjectProperties, pProjProp );
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();

   CProjectPropertiesDlg dlg;

   dlg.m_Bridge    = pProjProp->GetBridgeName();
   dlg.m_BridgeID  = pProjProp->GetBridgeId();
   dlg.m_JobNumber = pProjProp->GetJobNumber();
   dlg.m_Engineer  = pProjProp->GetEngineer();
   dlg.m_Company   = pProjProp->GetCompany();
   dlg.m_Comments  = pProjProp->GetComments();
   dlg.m_bShowProjectProperties = pApp->ShowProjectPropertiesOnNewProject();


   if ( dlg.DoModal() == IDOK )
   {
      // Turn off update
      pProjProp->EnableUpdate( false );

      // Make all the changes
      pProjProp->SetBridgeName( dlg.m_Bridge );
      pProjProp->SetBridgeId( dlg.m_BridgeID );
      pProjProp->SetJobNumber( dlg.m_JobNumber );
      pProjProp->SetEngineer( dlg.m_Engineer );
      pProjProp->SetCompany( dlg.m_Company );
      pProjProp->SetComments( dlg.m_Comments );
      pApp->ShowProjectPropertiesOnNewProject(dlg.m_bShowProjectProperties);

      // Turn updates back on.  If something changed, this will cause an
      // event to fire on the doc proxy event sink.
      pProjProp->EnableUpdate( true );
   }
}


void CPGSuperDoc::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   GET_IFACE( IProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format("The following error occured while opening %s",lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredLoad not registered") );
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case STRLOAD_E_CANTOPEN:
      pLog->LogMessage( TEXT("Could not open file") );
      AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      break;

   case STRLOAD_E_FILENOTFOUND:
      pLog->LogMessage( TEXT("File Not Found") );
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRLOAD_E_INVALIDFORMAT:
      pLog->LogMessage( TEXT("File does not have valid PGSuper format") );
      AfxFormatString1( msg1, IDS_E_INVALIDFORMAT, lpszPathName );
      break;

   case STRLOAD_E_BADVERSION:
      pLog->LogMessage( TEXT("This file came from a newer version of PGSuper, please upgrade") );
      AfxFormatString1( msg1, IDS_E_INVALIDVERSION, lpszPathName );
      break;

   case STRLOAD_E_USERDEFINED:
      AfxFormatString1( msg1, IDS_E_USERDEFINED, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format("An unknown error occured while opening the file (hr = %d)",hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::string strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   GET_IFACE( IProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format("The following error occured while saving %s",lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredSave not registered") );
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case STRSAVE_E_CANTOPEN:
      pLog->LogMessage( TEXT("Could not open file") );
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRSAVE_E_BADWRITE:
      pLog->LogMessage( TEXT("Error Writing to File") );
      AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format("An unknown error occured while closing the file (hr = %d)",hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::string strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   GET_IFACE( IProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format("The following error occured while converting %s",lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("File converter is not registered") );
      msg1.LoadString( IDS_E_BADINSTALL );
      break;

   case E_INVALIDARG:
      msg1.Format( "%s could not be opened",lpszPathName);
      pLog->LogMessage( msg1 );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format("An unknown error occured while closing the file (hr = %d)",hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::string strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::OnProjectUnits() 
{
   CUnitsDlg dlg;

   Int32 units = GetUnitMode();
   dlg.m_Units = (units == pgsTypes::umSI ? 0 : 1);
   if ( dlg.DoModal() )
   {
      pgsTypes::UnitMode newunits = dlg.m_Units == 0 ? pgsTypes::umSI : pgsTypes::umUS;
      SetUnitMode( newunits );
   }
	
}

void CPGSuperDoc::SetUnitMode(pgsTypes::UnitMode unitsMode)
{
   GET_IFACE(IProjectSettings,pProjSettings);
   pgsTypes::UnitMode currentUnits = pProjSettings->GetUnitsMode();
   txnChangeUnits* pTxn = new txnChangeUnits(this,currentUnits,unitsMode);
   txnTxnManager::GetInstance()->Execute(pTxn);
}

pgsTypes::UnitMode CPGSuperDoc::GetUnitMode() const
{
   GET_IFACE(IProjectSettings,pProjSettings);
   return pProjSettings->GetUnitsMode();
}

void CPGSuperDoc::OnSiUnits()
{
   SetUnitMode( pgsTypes::umSI );
}

void CPGSuperDoc::OnUpdateSiUnits(CCmdUI* pCmdUI)
{
   pCmdUI->SetRadio( GetUnitMode() == pgsTypes::umSI );
}

void CPGSuperDoc::OnUsUnits()
{
   SetUnitMode( pgsTypes::umUS );
}

void CPGSuperDoc::OnUpdateUsUnits(CCmdUI* pCmdUI)
{
   pCmdUI->SetRadio( GetUnitMode() == pgsTypes::umUS );
}

void CPGSuperDoc::OnProjectEnvironment() 
{
   GET_IFACE( IEnvironment, pEnvironment );

   enumExposureCondition ec = pEnvironment->GetExposureCondition();
   int expCond = ( ec == expNormal ? 0 : 1 );
   
   Float64 relHumidity = pEnvironment->GetRelHumidity();

   CEnvironmentDlg dlg;
   dlg.m_Exposure = expCond;
   dlg.m_RelHumidity = relHumidity;

   if ( dlg.DoModal() )
   {
      if ( expCond != dlg.m_Exposure || relHumidity != dlg.m_RelHumidity )
      {
         txnEditEnvironment* pTxn = new txnEditEnvironment(ec,dlg.m_Exposure == 0 ? expNormal : expSevere,relHumidity,dlg.m_RelHumidity);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnProjectBridgeDesc() 
{
   EditBridgeDescription(0); // open to first page
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnProjectSpec() 
{
   GET_IFACE( ILibraryNames, pLibNames );

   std::vector<std::string> specs;
   pLibNames->EnumSpecNames( &specs );
	CSpecDlg dlg(specs);

   GET_IFACE( ISpecification, pSpec );

   std::string cur_spec = pSpec->GetSpecification();
   dlg.m_Spec = cur_spec;
   if ( dlg.DoModal() )
   {
      if ( dlg.m_Spec != cur_spec )
      {
         txnEditProjectCriteria* pTxn = new txnEditProjectCriteria(cur_spec.c_str(),dlg.m_Spec.c_str());
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnProjectAutoCalc() 
{
	EnableAutoCalc( !IsAutoCalcEnabled() );
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnUpdateProjectAutoCalc(CCmdUI* pCmdUI) 
{
	if ( IsAutoCalcEnabled() )
      pCmdUI->SetText( "Turn AutoCalc Off" );
   else
      pCmdUI->SetText( "Turn AutoCalc On" );
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnExportToTemplateFile() 
{
   // select inital directory to try and save in
   CString default_name = "PGSuper.pgt";
   CString initial_filespec;
   CString initial_dir;
   
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();


   // prompt user to save current project to a template file
   CFileDialog  fildlg(FALSE,"pgt",default_name,OFN_HIDEREADONLY,
                   "PGSuper Template Files (*.pgt)|*.pgt||");

#if defined _DEBUG
   // If this is a debug build, then the developers are probably running
   // the software and they want the workgroup folder most often.
   CString workgroup_folder;
   pApp->GetTemplateFolders(workgroup_folder);
   fildlg.m_ofn.lpstrInitialDir = workgroup_folder;
#else
   fildlg.m_ofn.lpstrInitialDir = initial_dir;
#endif // _DEBUG

   int stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      // try loop with a new file name
      CString file_path = fildlg.GetPathName();

      // check if file exists and prompt user if he wants to overwrite
      if (DoesFileExist(file_path))
      {
         CString msg(" The file: ");
         msg += file_path + " exists. Overwrite it?";
         int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
         if (stm!=IDYES)
            return;
      }

      // write the file.
      SaveTheDocument( file_path );
   }
}

bool CPGSuperDoc::DoTxDotCadReport(const CString& outputFileName, const CString& errorFileName, const CPGSuperCommandLineInfo& txInfo)
{
   // Called from the command line processing in the Application object

   // open the error file
   std::ofstream err_file(errorFileName);
   if ( !err_file )
   {
	   AfxMessageBox ("Could not Create error file");
	   return false;
   }

   // Open/create the specified output file 
   FILE	*fp;
   errno_t result;
   if (txInfo.m_DoAppendToFile)
   {
      result = fopen_s(&fp, LPCTSTR (outputFileName), "a+");
   }
   else
   {
      result = fopen_s(&fp, LPCTSTR (outputFileName), "w+");
   }

   if (result != 0 || fp == NULL)
   {
      err_file<<"Error: Output file could not be Created."<<std::endl;
	   return false;
   }

   // Get starting and ending spans
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   SpanIndexType start_span, end_span;
   if (txInfo.m_TxSpan==ALL_SPANS)
   {
      // looping over all spans
      start_span = 0;
      end_span = nSpans-1;
   }
   else
   {
      if (txInfo.m_TxSpan>=nSpans)
      {
         err_file<<"Span value is out of range for this bridge"<<std::endl;
	      return false;
      }
      else
      {
         start_span = txInfo.m_TxSpan;
         end_span   = txInfo.m_TxSpan;
      }
   }

   // Build list of span/girders to operate on (error out before we do anything of there are problems)
   std::vector<SpanGirderHashType> spn_grd_list;

   for (SpanIndexType is=start_span; is<=end_span; is++)
   {
      // we can have a different number of girders per span
      GirderIndexType nGirders = pBridge->GetGirderCount(is);

      if (txInfo.m_TxGirder==TXALLGIRDERS)
      {
         for (GirderIndexType ig=0; ig<nGirders; ig++)
         {
            SpanGirderHashType key = HashSpanGirder(is,ig);
            spn_grd_list.push_back(key);
         }
      }
      else if (txInfo.m_TxGirder==TXEIGIRDERS)
      {
         // Exterior/Interior option
         SpanGirderHashType key = HashSpanGirder(is,0); // left exterior
         spn_grd_list.push_back(key);

         if (nGirders>2)
         {
            SpanGirderHashType key = HashSpanGirder(is,1); // exterior-most interior
            spn_grd_list.push_back(key);

            if (nGirders>4)
            {
               SpanGirderHashType key = HashSpanGirder(is,nGirders/2); // middle-most interior
               spn_grd_list.push_back(key);
            }
         }
      }

      else if (txInfo.m_TxGirder<nGirders)
      {
         SpanGirderHashType key = HashSpanGirder(is,txInfo.m_TxGirder);
         spn_grd_list.push_back(key);
      }
      else
      {
         err_file<<"Girder value is out of range for this bridge"<<std::endl;
	      return false;
      }
   }

   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   // Loop over all span/girder combos and create results
   for(std::vector<SpanGirderHashType>::iterator it=spn_grd_list.begin(); it!=spn_grd_list.end(); it++)
   {
      SpanGirderHashType key = *it;
      SpanIndexType span;
      GirderIndexType girder;
      UnhashSpanGirder(key, &span, &girder);

      CString strMessage;
      strMessage.Format("Creating TxDOT CAD report for Span %d, Girder %s",LABEL_SPAN(span), LABEL_GIRDER(girder));
      pProgress->UpdateMessage(strMessage);

      // See if we need to run a design
      bool designSucceeded=true;
      if (txInfo.m_TxRunType==CPGSuperCommandLineInfo::txrDesign)
      {
         // get design options from library entry. Do flexure only
         GET_IFACE(ISpecification,pSpecification);
         arDesignOptions des_options = pSpecification->GetDesignOptions(span,girder);

         des_options.doDesignForShear = false; // shear design is off

         GET_IFACE(IArtifact,pIArtifact);
         const pgsDesignArtifact* pArtifact;
         try
         {
            // Design the girder
            pArtifact = pIArtifact->CreateDesignArtifact( span,girder, des_options);
         
            if (pArtifact->GetOutcome() != pgsDesignArtifact::Success)
            {
               err_file <<"Design was unsuccessful"<<std::endl;
               designSucceeded=false;
            }

            // and copy the design to the bridge
            SaveFlexureDesign(span,girder, des_options, pArtifact);
         }
         catch(...)
         {
           err_file <<"Design Failed for span"<<span<<" girder "<<girder<<std::endl;
            return false;
         }
      }

      GET_IFACE(ITxDOTCadExport,pTxDOTCadExport);
      if ( !pTxDOTCadExport )
      {
         AfxMessageBox("The TxDOT Cad Exporter is not currently installed");
         return false;
      }

      if (txInfo.m_TxRunType==CPGSuperCommandLineInfo::TxrDistributionFactors)
      {
         // Write distribution factor data to file
         if (CAD_SUCCESS != pTxDOTCadExport->WriteDistributionFactorsToFile (fp, this->m_pBroker, span, girder))
         {
            err_file <<"Warning: An error occured while writing to File"<<std::endl;
	         return false;
         }
      }
      else
      {
         /* Write CAD data to text file */
         if (CAD_SUCCESS != pTxDOTCadExport->WriteCADDataToFile(fp, this->m_pBroker, span, girder, (TxDOTCadExportFormatType)txInfo.m_TxFType, designSucceeded) )
         {
            err_file <<"Warning: An error occured while writing to File"<<std::endl;
	         return false;
         }
	  }
   }

   /* Close the open text file */
   fclose (fp);

   // ---------------------------------------------
   // Run a 12-50 output if this is a test file
   if (txInfo.m_TxFType==CPGSuperCommandLineInfo::txfTest)
   {
      // file names
      CString resultsfile, poifile, errfile;
      if (create_test_file_names(txInfo.m_strFileName,&resultsfile,&poifile,&errfile))
      {
         GET_IFACE(ITest1250, ptst );

         try
         {
            if (!ptst->RunTestEx(RUN_CADTEST, spn_grd_list, std::string(resultsfile), std::string(poifile)))
            {
               CString msg = CString("Error - Running test on file")+txInfo.m_strFileName;
               ::AfxMessageBox(msg);
            }
         }
         catch(...)
         {
            CString msg = CString("Error - running test for input file:")+txInfo.m_strFileName;
            ::AfxMessageBox(msg);
            return false;
         }
      }
      else
      {
         CString msg = CString("Error - Determining 1250 test file names for")+txInfo.m_strFileName;
         ::AfxMessageBox(msg);
         return false;
      }
   }


   return true;
}

void CPGSuperDoc::OnUpdateStatusCenter(CCmdUI* pCmdUI)
{
   CString str;
   str.Format("%s Status Center",m_StatusCenterDlg.IsWindowVisible() ? "Hide" : "Show");
   pCmdUI->SetText(str);

   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->UpdateToolbarStatusItems(m_StatusCenter.GetSeverity());
   pMainFrame->UpdateStatusBar();
}

void CPGSuperDoc::OnViewStatusCenter(UINT nID)
{
   m_StatusCenterDlg.ShowWindow(m_StatusCenterDlg.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}

bool DoesFolderExist(const CString& dirname)
{
   if (dirname.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      CString nam = dirname + CString("\\*.*");
      is_file = finder.FindFile(nam);
      return (is_file!=0);
   }
}

bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}


void CPGSuperDoc::OnViewsettingsBridgemodelEditor() 
{
   EditBridgeViewSettings(0);
}

void CPGSuperDoc::OnViewsettingsGirderEditor() 
{
   EditGirderViewSettings(0);
}


void CPGSuperDoc::OnLoadsLoadModifiers() 
{
   GET_IFACE(ILoadModifiers,pLoadModifiers);

   ILoadModifiers::Level ductility_level  = pLoadModifiers->GetDuctilityLevel();
   ILoadModifiers::Level importance_level = pLoadModifiers->GetImportanceLevel();
   ILoadModifiers::Level redundancy_level = pLoadModifiers->GetRedundancyLevel();

   CLoadModifiersDlg dlg;
   dlg.SetHelpData( AfxGetApp()->m_pszHelpFilePath, IDH_DIALOG_LOADMODIFIERS, IDH_DIALOG_LOADMODIFIERS, IDH_DIALOG_LOADMODIFIERS);

   dlg.SetLoadModifiers( pLoadModifiers->GetDuctilityFactor(),
                         ((ductility_level == ILoadModifiers::High)  ? 0 : (ductility_level  == ILoadModifiers::Normal ? 1 : 2)),
                         pLoadModifiers->GetRedundancyFactor(),
                         ((redundancy_level == ILoadModifiers::High) ? 0 : (redundancy_level == ILoadModifiers::Normal ? 1 : 2)),
                         pLoadModifiers->GetImportanceFactor(),
                         ((importance_level == ILoadModifiers::High) ? 0 : (importance_level == ILoadModifiers::Normal ? 1 : 2))
                        );

   if ( dlg.DoModal() == IDOK )
   {
      Float64 d,r,i;
      Int16 dl, il, rl;
      dlg.GetLoadModifiers(&d,&dl,&r,&rl,&i,&il);
      pLoadModifiers->SetDuctilityFactor(  dl == 0 ? ILoadModifiers::High : dl == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low, d );
      pLoadModifiers->SetRedundancyFactor( rl == 0 ? ILoadModifiers::High : rl == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low, r );
      pLoadModifiers->SetImportanceFactor( il == 0 ? ILoadModifiers::High : il == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low, i );
   }
}

void CPGSuperDoc::SetModifiedFlag(BOOL bModified)
{
   CDocument::SetModifiedFlag(bModified);
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->EnableModifiedFlag( IsModified() );
}

void CPGSuperDoc::UpdateAnalysisTypeStatusIndicator()
{
   GET_IFACE(ISpecification,pSpec);
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();

   pMainFrame->SetAnalysisTypeStatusIndicator(pSpec->GetAnalysisType());
}

void CPGSuperDoc::OnProjectDesignGirder() 
{
   if ( GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("There are errors that must be corrected before you can design a girder\r\n\r\nSee the Status Center for details.",MB_OK);
      return;
   }

   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);

   // only allow A design if it's allowed in the library, then save setting for future visits.
   // Do not save this in registry because library selection should be default for new documents
   bool enable_so_selection = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;

   CDesignGirderDlg dlg(m_CurrentSpanIdx   == ALL_SPANS   ? 0 : m_CurrentSpanIdx, 
                        m_CurrentGirderIdx == ALL_GIRDERS ? 0 : m_CurrentGirderIdx,
                        enable_so_selection, m_DesignSlabOffset, m_pBroker);

   // Set initial dialog values based on last stored in registry. These may be changed
   // internally by dialog based on girder type, and other library values
   dlg.m_DesignForFlexure = (IsDesignFlexureEnabled()  ? TRUE : FALSE);
   dlg.m_DesignForShear   = (IsDesignShearEnabled()    ? TRUE : FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      arDesignOptions des_options = pSpecification->GetDesignOptions(dlg.m_Span,dlg.m_Girder);

      if (dlg.m_DesignForFlexure==FALSE)
      {
         des_options.doDesignForFlexure = dtNoDesign;
      }

      des_options.doDesignForShear   = dlg.m_DesignForShear==TRUE ? true : false;

      // we can override library setting here
      des_options.doDesignSlabOffset = dlg.m_DesignA && enable_so_selection;
      m_DesignSlabOffset = dlg.m_DesignA; // retain value for current document

      EnableDesignFlexure(dlg.m_DesignForFlexure == TRUE ? true : false);
      EnableDesignShear(  dlg.m_DesignForShear   == TRUE ? true : false);


      DoDesignGirder(dlg.m_Span,dlg.m_Girder,des_options);
   }
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirect(CCmdUI* pCmdUI)
{
   pCmdUI->Enable( m_CurrentSpanIdx != ALL_SPANS && m_CurrentGirderIdx != ALL_GIRDERS );
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI)
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   pCmdUI->Enable( m_CurrentSpanIdx != ALL_SPANS && m_CurrentGirderIdx != ALL_GIRDERS && bDesignSlabOffset );
}

void CPGSuperDoc::OnProjectDesignGirderDirect()
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   DesignGirderDirect(bDesignSlabOffset);
}

void CPGSuperDoc::OnProjectDesignGirderDirectHoldSlabOffset()
{
   DesignGirderDirect(false);
}

void CPGSuperDoc::DesignGirderDirect(bool bDesignSlabOffset)
{
   if ( GetStatusCenter().GetSeverity() == pgsTypes::statusError )
   {
      AfxMessageBox("There are errors that must be corrected before you can design a girder\r\n\r\nSee the Status Center for details.",MB_OK);
      return;
   }


   SpanIndexType spanIdx   = m_CurrentSpanIdx   == ALL_SPANS ? 0 : m_CurrentSpanIdx;
   GirderIndexType gdrIdx  = m_CurrentGirderIdx == ALL_GIRDERS ? 0 : m_CurrentGirderIdx;

   GET_IFACE(ISpecification,pSpecification);
   arDesignOptions des_options = pSpecification->GetDesignOptions(spanIdx,gdrIdx);

   des_options.doDesignSlabOffset = bDesignSlabOffset;

   if ( !IsDesignFlexureEnabled() )
   {
      des_options.doDesignForFlexure = dtNoDesign;
   }

   des_options.doDesignForShear = IsDesignShearEnabled();
   DoDesignGirder(spanIdx,
                  gdrIdx,
                  des_options);
}

void CPGSuperDoc::DoDesignGirder(SpanIndexType span,GirderIndexType gdr,const arDesignOptions& designOptions)
{
   GET_IFACE(IArtifact,pIArtifact);
   const pgsDesignArtifact* pArtifact = pIArtifact->CreateDesignArtifact( span, gdr, designOptions);

   if ( pArtifact->GetOutcome() == pgsDesignArtifact::DesignCancelled )
   {
      AfxMessageBox("Design Cancelled",MB_OK);
      return;
   }

   GET_IFACE(IReportManager,pReportMgr);
   CReportDescription rptDesc = pReportMgr->GetReportDescription("Design Outcome Report");
   boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   boost::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);
   boost::shared_ptr<CSpanGirderReportSpecification> pSGRptSpec = boost::dynamic_pointer_cast<CSpanGirderReportSpecification,CReportSpecification>(pRptSpec);
   pSGRptSpec->SetSpan(span);
   pSGRptSpec->SetGirder(gdr);

   CDesignOutcomeDlg dlg(pSGRptSpec);
   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      pgsTypes::SlabOffsetType slabOffsetType = pIBridgeDesc->GetSlabOffsetType();
      txnDesignGirder* pTxn = new txnDesignGirder(span,gdr,designOptions,*pArtifact,slabOffsetType);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CPGSuperDoc::SaveFlexureDesign(SpanIndexType span,GirderIndexType gdr,const arDesignOptions& designOptions,const pgsDesignArtifact* pArtifact)
{
   GET_IFACE(IGirderData,pGirderData);
   GET_IFACE(IStrandGeometry, pStrandGeometry );

   CGirderData girderData = pGirderData->GetGirderData(span, gdr);

   // Convert Harp offset data
   // offsets are absolute measure in the design artifact
   // convert them to the measurement basis that the CGirderData object is using
   girderData.HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, 
                                                                                  pArtifact->GetNumHarpedStrands(), 
                                                                                  girderData.HsoEndMeasurement, 
                                                                                  pArtifact->GetHarpStrandOffsetEnd());

   girderData.HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr, 
                                                                                pArtifact->GetNumHarpedStrands(), 
                                                                                girderData.HsoHpMeasurement, 
                                                                                pArtifact->GetHarpStrandOffsetHp());



#pragma Reminder("############ - Update with loop after updating Artifact #############")
   // see if strand design data fits in grid
   bool fills_grid=false;
   StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
   if (designOptions.doStrandFillType==ftGridOrder)
   {
      // we asked design to fill using grid, but this may be a non-standard design - let's check
      StrandIndexType ns, nh;
      if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
      {
         if (ns==pArtifact->GetNumStraightStrands() && nh==pArtifact->GetNumHarpedStrands() )
         {
            fills_grid = true;
         }
      }
   }

   if (fills_grid)
   {
      girderData.NumPermStrandsType = NPS_TOTAL_NUMBER;

      girderData.Nstrands[pgsTypes::Permanent]            = num_permanent;
      girderData.Pjack[pgsTypes::Permanent]               = pArtifact->GetPjackStraightStrands() + pArtifact->GetPjackHarpedStrands();
      girderData.bPjackCalculated[pgsTypes::Permanent]    = pArtifact->GetUsedMaxPjackStraightStrands();
   }
   else
   {
      girderData.NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }

   girderData.Nstrands[pgsTypes::Harped]            = pArtifact->GetNumHarpedStrands();
   girderData.Nstrands[pgsTypes::Straight]          = pArtifact->GetNumStraightStrands();
   girderData.Nstrands[pgsTypes::Temporary]         = pArtifact->GetNumTempStrands();
   girderData.Pjack[pgsTypes::Harped]               = pArtifact->GetPjackHarpedStrands();
   girderData.Pjack[pgsTypes::Straight]             = pArtifact->GetPjackStraightStrands();
   girderData.Pjack[pgsTypes::Temporary]            = pArtifact->GetPjackTempStrands();
   girderData.bPjackCalculated[pgsTypes::Harped]    = pArtifact->GetUsedMaxPjackHarpedStrands();
   girderData.bPjackCalculated[pgsTypes::Straight]  = pArtifact->GetUsedMaxPjackStraightStrands();
   girderData.bPjackCalculated[pgsTypes::Temporary] = pArtifact->GetUsedMaxPjackTempStrands();
   girderData.LastUserPjack[pgsTypes::Harped]       = pArtifact->GetPjackHarpedStrands();
   girderData.LastUserPjack[pgsTypes::Straight]     = pArtifact->GetPjackStraightStrands();
   girderData.LastUserPjack[pgsTypes::Temporary]    = pArtifact->GetPjackTempStrands();

   girderData.TempStrandUsage = pArtifact->GetTemporaryStrandUsage();

   // Get debond information from design artifact
   girderData.ClearDebondData();
   girderData.bSymmetricDebond = true;  // design is always symmetric

   DebondInfoCollection dbcoll = pArtifact->GetStraightStrandDebondInfo();
   // TRICKY: Mapping from DEBONDINFO to CDebondInfo is tricky because
   //         former designates individual strands and latter stores symmetric strands
   //         in pairs.
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondInfoIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDINFO& rdbrinfo = *dbit;

      CDebondInfo cdbi;
      cdbi.idxStrand1 = rdbrinfo.strandIdx;

      // if the difference between the current and next number of strands is 2, this is a pair
      StrandIndexType currnum = rdbrinfo.strandIdx;
      StrandIndexType nextnum = pStrandGeometry->GetNextNumStrands(span, gdr, pgsTypes::Straight, currnum);
      if (nextnum-currnum == 2)
      {
         dbit++; // increment counter to account for a pair
         cdbi.idxStrand2 = dbit->strandIdx;

         // some asserts to ensure we got things right
         ATLASSERT(cdbi.idxStrand1+1 == cdbi.idxStrand2);
         ATLASSERT(rdbrinfo.LeftDebondLength == dbit->LeftDebondLength);
         ATLASSERT(rdbrinfo.RightDebondLength == dbit->RightDebondLength);
      }
      else
      {
         // not a pair
         cdbi.idxStrand2 = INVALID_INDEX;
      }
      cdbi.Length1    = rdbrinfo.LeftDebondLength;
      cdbi.Length2    = rdbrinfo.RightDebondLength;

      girderData.Debond[pgsTypes::Straight].push_back(cdbi);
   }
   
   // concrete
   girderData.Material.Fci = pArtifact->GetReleaseStrength();
   girderData.Material.Fc  = pArtifact->GetConcreteStrength();

   pGirderData->SetGirderData( girderData, span, gdr );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   CBridgeDescription bridgeDesc = *pIBridgeDesc->GetBridgeDescription();
   CSpanData* pSpan = bridgeDesc.GetSpan(span);
   CGirderTypes girderTypes = *(pSpan->GetGirderTypes());
   girderTypes.SetSlabOffset(gdr,pgsTypes::metStart,pArtifact->GetSlabOffset(pgsTypes::metStart));
   girderTypes.SetSlabOffset(gdr,pgsTypes::metEnd,  pArtifact->GetSlabOffset(pgsTypes::metEnd));
   pSpan->SetGirderTypes(girderTypes);
   pIBridgeDesc->SetBridgeDescription(bridgeDesc);

   GET_IFACE(IGirderLifting,pLifting);
   pLifting->SetLiftingLoopLocations(span, gdr,pArtifact->GetLeftLiftingLocation(),pArtifact->GetRightLiftingLocation());

   GET_IFACE(IGirderHauling,pHauling);
   pHauling->SetTruckSupportLocations(span, gdr,pArtifact->GetTrailingOverhang(),pArtifact->GetLeadingOverhang());

}


void CPGSuperDoc::OnCloseDocument() 
{
   // Tell main frame to turn off modified flag since there wont be an
   // open document.
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   pMainFrame->EnableModifiedFlag( false );
	
	CDocument::OnCloseDocument();

   txnTxnManager* pTxnMgr = txnTxnManager::GetInstance();
   pTxnMgr->ClearTxnHistory();
   pTxnMgr->ClearUndoHistory();

   pMainFrame->UpdateStatusBar();
}

void CPGSuperDoc::DeleteContents()
{
   m_StatusCenter.UnSinkEvents(this);

   AGENT_CLEAR_INTERFACE_CACHE;

   if ( m_TheRealBroker )
   {
      m_TheRealBroker->ShutDown();
      IBroker* pBroker = m_TheRealBroker.Detach();
      ULONG cRef = pBroker->Release();
      ASSERT( cRef == 0 );
      WATCH("~CPGSuperDoc - Broker Ref Count = " << cRef);
   }

}

bool CPGSuperDoc::LoadMasterLibrary()
{
   WATCH("Loading Master Library");

   // Load the master library
   CPGSuperApp* pApp =(CPGSuperApp*) AfxGetApp();

   CString strMasterLibaryFile = pApp->GetCachedMasterLibraryFile();

   std::string strPublisher = pApp->GetMasterLibraryPublisher();
   std::string strMasterLibFile = pApp->GetMasterLibraryFile();

   m_LibMgr.SetMasterLibraryInfo(strPublisher.c_str(),strMasterLibFile.c_str());

   return DoLoadMasterLibrary(strMasterLibaryFile);
}

bool CPGSuperDoc::DoLoadMasterLibrary(const CString& strMasterLibaryFile)
{
   // loop until a library file is opened or user gives up trying to find it
   CComBSTR bpath(strMasterLibaryFile);
   CString err_msg;
   err_msg.LoadString(IDS_CANT_FIND_LIBRARY_FILE);

   bool do_loop=true;
   while(do_loop)
   {
      FileStream ifile;
      if ( bpath.Length() != 0 && ifile.open(bpath))
      {
         // try to load file
         try
         {
            // clear out library
            m_LibMgr.ClearAllEntries();

            sysStructuredLoadXmlPrs load;
            load.BeginLoad( &ifile );

            // Problem : Library Editor application specific code is in the
            // master library file. We have to read it or get an error.
            load.BeginUnit("LIBRARY_EDITOR");
            std::string str;
            load.Property("EDIT_UNITS", &str);

            if ( !m_LibMgr.LoadMe( &load ) )
               return false;

            load.EndUnit(); //"LIBRARY_EDITOR"
            load.EndLoad();

            // success!
            WATCH("Master Library loaded successfully");
            do_loop=false;
         }
         catch( sysXStructuredLoad& e )
         {
            if ( e.GetExplicitReason() == sysXStructuredLoad::CantInitializeTheParser )
            {
               WATCH("Failed to initialize the parser");
               err_msg.LoadString( IDS_E_PARSER_INIT_FAIL );
            }
            else
            {
               e.Throw(); // Should be handled by catch(...)
            }
         }
         catch(...)
         {
            WATCH("Failed to load master library");
            AfxFormatString1(err_msg, IDS_CORRUPTED_LIBRARY_FILE, strMasterLibaryFile);
         }
      }

      if (do_loop)
      {
         // if we are here, an error occured. Issue the message and give
         // the user a chance to load another library file

         AfxMessageBox(err_msg,MB_OK|MB_ICONSTOP);
         CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
         pApp->OnProgramSettings(TRUE);
         bpath = pApp->GetCachedMasterLibraryFile();
      }
   }

   // make all entries in master library read-only
   m_LibMgr.EnableEditingForAllEntries(false);

   return true; // the only way out alive!
}

void CPGSuperDoc::OnEditGirder() 
{
   CSelectGirderDlg dlg(m_pBroker);
   dlg.m_Span   = m_CurrentSpanIdx   == ALL_SPANS   ? 0 : m_CurrentSpanIdx;
   dlg.m_Girder = m_CurrentGirderIdx == ALL_GIRDERS ? 0 : m_CurrentGirderIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditGirderDescription(dlg.m_Span,dlg.m_Girder,EGD_GENERAL);
   }
}

PierIndexType CPGSuperDoc::GetPierIdx()
{
   return m_CurrentPierIdx;
}

SpanIndexType CPGSuperDoc::GetSpanIdx()
{
   return m_CurrentSpanIdx;
}

GirderIndexType CPGSuperDoc::GetGirderIdx()
{
   return m_CurrentGirderIdx;
}

void CPGSuperDoc::SelectPier(PierIndexType pierIdx)
{
   m_CurrentPierIdx = pierIdx;

   if (m_CurrentPierIdx != ALL_PIERS)
   {
      m_CurrentSpanIdx   = INVALID_INDEX;
      m_CurrentGirderIdx = INVALID_INDEX;
   }

   UpdateAllViews(0,HINT_GIRDERSELECTIONCHANGED,0);
}

void CPGSuperDoc::SelectSpan(SpanIndexType spanIdx)
{
   m_CurrentSpanIdx = spanIdx;

   if ( m_CurrentSpanIdx != ALL_SPANS)
   {
      m_CurrentPierIdx   = INVALID_INDEX;
      m_CurrentGirderIdx = INVALID_INDEX;
   }

   UpdateAllViews(0,HINT_GIRDERSELECTIONCHANGED,0);
}

void CPGSuperDoc::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   m_CurrentSpanIdx   = spanIdx;
   m_CurrentGirderIdx = gdrIdx;

   m_CurrentPierIdx = INVALID_INDEX;

   UpdateAllViews(0,HINT_GIRDERSELECTIONCHANGED,0);
}

void CPGSuperDoc::OnCopyGirderProps() 
{
   CCopyGirderDlg dlg(m_pBroker,this);
   if ( dlg.DoModal() == IDOK )
   {
      OnApplyCopyGirder(dlg.m_FromSpanGirderHashValue,
                        dlg.m_ToSpanGirderHashValues,
                        dlg.m_bCopyGirder,
                        dlg.m_bCopyTransverse,
                        dlg.m_bCopyLongitudinalRebar,
                        dlg.m_bCopyPrestressing,
                        dlg.m_bCopyHandling,
                        dlg.m_bCopyMaterial,
                        dlg.m_bCopySlabOffset);
   }
}

void CPGSuperDoc::OnApplyCopyGirder(SpanGirderHashType fromHash,std::vector<SpanGirderHashType> toHash,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial, BOOL bSlabOffset)
{
   if (!(bGirder || bTransverse || bPrestress || bLongitudinalRebar || bHandling || bMaterial))
      return; //nothing to do

   txnCopyGirder* pTxn = new txnCopyGirder(fromHash,toHash,bGirder,bTransverse,bLongitudinalRebar,bPrestress,bHandling,bMaterial,bSlabOffset);
   txnTxnManager::GetInstance()->Execute(pTxn);
}

void CPGSuperDoc::OnImportProjectLibrary() 
{
	// ask user for file name
   CFileDialog  fildlg(TRUE,"pgs",NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   "PGSuper Project Files (*.pgs)|*.pgs||");
   int stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      CString rPath;
      rPath = fildlg.GetPathName();

      GET_IFACE( IImportProjectLibrary, pImport );

      CString real_file_name; // name of actual file to be read may be different than lpszPathName
      long convert_status = ConvertTheDocument(rPath, &real_file_name);
      // convert document. if file was converted, then we need to delete the converted file at the end
      if ( -1== convert_status)
      {
         HandleOpenDocumentError( STRLOAD_E_INVALIDFORMAT, rPath );
         ASSERT(FALSE);
      }
      else
      {
         // NOTE: Although it looks innocent, this control block is very important!! 
         // This is because the IStructuredLoad must be destroyed before the temp 
         // file can be deleted

         CComPtr<IStructuredLoad> pStrLoad;
         HRESULT hr = ::CoCreateInstance( CLSID_StructuredLoad, NULL, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
         if ( FAILED(hr) )
         {
            // We are not aggregating so we should CoCreateInstance should
            // never fail with this HRESULT
            ASSERT( hr != CLASS_E_NOAGGREGATION );

            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Open( real_file_name );
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         if ( !pImport->ImportProjectLibraries( pStrLoad ) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Close();
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }
      }

      if (convert_status==1)
      {
         // file was converted and written to a temporary file. delete the temp file
         CFile::Remove(real_file_name);
      }


      SetModifiedFlag();
      UpdateAllViews(NULL, HINT_LIBRARYCHANGED);
   }
}

void CPGSuperDoc::OnLoadsLldf() 
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();
                  
   OnLoadsLldf(method,roaAction);
}

void CPGSuperDoc::OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction) 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(ILiveLoads,pLiveLoads);

   CLiveLoadDistFactorsDlg dlg;
   dlg.m_BridgeDesc = *pOldBridgeDesc;
   dlg.m_BridgeDesc.SetDistributionFactorMethod(method);
   dlg.m_LldfRangeOfApplicabilityAction = roaAction;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLLDF* pTxn = new txnEditLLDF(*pOldBridgeDesc,dlg.m_BridgeDesc,
                                          pLiveLoads->GetLldfRangeOfApplicabilityAction(),dlg.m_LldfRangeOfApplicabilityAction);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CPGSuperDoc::OnStatusItemAdded(pgsStatusItem* pItem)
{
   CWnd* pWnd = AfxGetMainWnd();
   CMainFrame* pFrame = (CMainFrame*)(pWnd);
   pFrame->UpdateStatusBar();

}

void CPGSuperDoc::OnStatusItemRemoved(long id)
{
   CWnd* pWnd = AfxGetMainWnd();
   CMainFrame* pFrame = (CMainFrame*)(pWnd);
   pFrame->UpdateStatusBar();
}

void CPGSuperDoc::OnAddPointload() 
{
	CEditPointLoadDlg dlg(CPointLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

/*-------------------------------------------------------------------*/
void CPGSuperDoc::OnAddDistributedLoad() 
{
	CEditDistributedLoadDlg dlg(CDistributedLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
	
}

void CPGSuperDoc::OnAddMomentLoad() 
{
	CEditMomentLoadDlg dlg(CMomentLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CPGSuperDoc::OnProjectAlignment() 
{
   EditAlignmentDescription(0);
}

void CPGSuperDoc::OnLiveLoads() 
{
   GET_IFACE( ILibraryNames, pLibNames );
   GET_IFACE( ILiveLoads, pLiveLoad );

   std::vector<std::string> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );

   std::vector<std::string> design_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::string> fatigue_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::string> permit_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltPermit);

   CLiveLoadSelectDlg dlg(all_names, design_names, fatigue_names, permit_names);

   dlg.m_DesignTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltDesign);
   dlg.m_DesignLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltDesign);

   dlg.m_FatigueTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltFatigue);
   dlg.m_FatigueLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltFatigue);

   dlg.m_PermitLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltPermit);
   dlg.m_PermitTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltPermit);

   txnEditLiveLoadData oldDesign, oldFatigue, oldPermit;
   oldDesign.m_VehicleNames = dlg.m_DesignNames;
   oldDesign.m_TruckImpact  = dlg.m_DesignTruckImpact;
   oldDesign.m_LaneImpact   = dlg.m_DesignLaneImpact;

   oldFatigue.m_VehicleNames = dlg.m_FatigueNames;
   oldFatigue.m_TruckImpact  = dlg.m_FatigueTruckImpact;
   oldFatigue.m_LaneImpact   = dlg.m_FatigueLaneImpact;

   oldPermit.m_VehicleNames = dlg.m_PermitNames;
   oldPermit.m_TruckImpact  = dlg.m_PermitTruckImpact;
   oldPermit.m_LaneImpact   = dlg.m_PermitLaneImpact;

   if ( dlg.DoModal() == IDOK)
   {
      txnEditLiveLoadData newDesign, newFatigue, newPermit;
      newDesign.m_VehicleNames = dlg.m_DesignNames;
      newDesign.m_TruckImpact  = dlg.m_DesignTruckImpact;
      newDesign.m_LaneImpact   = dlg.m_DesignLaneImpact;

      newFatigue.m_VehicleNames = dlg.m_FatigueNames;
      newFatigue.m_TruckImpact  = dlg.m_FatigueTruckImpact;
      newFatigue.m_LaneImpact   = dlg.m_FatigueLaneImpact;

      newPermit.m_VehicleNames = dlg.m_PermitNames;
      newPermit.m_TruckImpact  = dlg.m_PermitTruckImpact;
      newPermit.m_LaneImpact   = dlg.m_PermitLaneImpact;

      txnEditLiveLoad* pTxn = new txnEditLiveLoad(oldDesign,newDesign,oldFatigue,newFatigue,oldPermit,newPermit);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CPGSuperDoc::OnReport(UINT nID)
{
   // User picked a report from a menu.
   // get the report index
   CollectionIndexType rptIdx = GetReportIndex(nID,false);

   CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
   pFrame->CreateReport(rptIdx,true); // true = prompt for repot specification
}

void CPGSuperDoc::OnQuickReport(UINT nID)
{
   // User picked a report from a menu.
   // This is a "quick report" so don't prompt
   if ( m_CurrentSpanIdx != ALL_SPANS && m_CurrentGirderIdx != ALL_GIRDERS  )
   {
      // get the report index
      CollectionIndexType rptIdx = GetReportIndex(nID,true);

      CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
      pFrame->CreateReport(rptIdx,false);// false = don't prompt for repot specification
   }
}

void CPGSuperDoc::OnViewAnalysisResults()
{
   CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
   pFrame->CreateAnalysisResultsView();
}

void CPGSuperDoc::OnProjectAnalysis() 
{
   CStructuralAnalysisMethodDlg dlg;
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType currAnalysisType = pSpec->GetAnalysisType();
   dlg.m_AnalysisType = currAnalysisType;
   if ( dlg.DoModal() == IDOK )
   {
      if ( currAnalysisType != dlg.m_AnalysisType )
      {
         txnEditAnalysisType* pTxn = new txnEditAnalysisType(currAnalysisType,dlg.m_AnalysisType);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}

void CPGSuperDoc::OnEditPier() 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strItems;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CString strItem;
      if ( pierIdx == 0 || pierIdx == nPiers-1 )
         strItem.Format("Abutment %d\n",pierIdx+1);
      else
         strItem.Format("Pier %d\n",pierIdx+1);

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = "Select pier to edit";
   dlg.m_strItems = strItems;
   dlg.m_strLabel = "Select pier to edit";
   dlg.m_ItemIdx = m_CurrentPierIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditPierDescription(dlg.m_ItemIdx,EPD_GENERAL);
   }
}

void CPGSuperDoc::OnEditSpan() 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CString strItems;
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CString strItem;
      strItem.Format("Span %d\n",LABEL_SPAN(spanIdx));

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = "Select span to edit";
   dlg.m_strItems = strItems;
   dlg.m_strLabel = "Select span to edit";
   dlg.m_ItemIdx = m_CurrentSpanIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditSpanDescription(dlg.m_ItemIdx,ESD_GENERAL);
   }
}

void CPGSuperDoc::DeletePier(PierIndexType pierIdx)
{
   // deleting a pier

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strTitle;
   strTitle.Format("Deleting %s %d",(pierIdx == 0 || pierIdx == nPiers-1 ? "Abutment" : "Pier"),pierIdx+1);
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format("%s. Select the span to be deleted with the pier",strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( pierIdx == 0 )
      strItems.Format("%s","Span 1\n");
   else if ( pierIdx == nPiers-1)
      strItems.Format("Span %d\n",pierIdx);
   else
      strItems.Format("Span %d\nSpan %d\n",pierIdx,pierIdx+1);

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      if ( pierIdx == 0 )
         DeletePier(pierIdx,pgsTypes::Ahead);
      else if ( pierIdx == nPiers-1 )
         DeletePier(pierIdx,pgsTypes::Back);
      else
         DeletePier(pierIdx,dlg.m_ItemIdx == 0 ? pgsTypes::Back : pgsTypes::Ahead);
   }
}

void CPGSuperDoc::DeleteSpan(SpanIndexType spanIdx)
{
   // deleting a span

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CString strTitle;
   strTitle.Format("Deleting Span %d",LABEL_SPAN(spanIdx));
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format("%s. Select the pier to be deleted with the span",strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( spanIdx == 0 )
      strItems.Format("%s","Abutment 1\nPier 2\n");
   else if ( spanIdx == nSpans-1)
      strItems.Format("Pier %d\nAbutment %d\n",LABEL_PIER(nSpans-1),LABEL_PIER(nSpans));
   else
      strItems.Format("Pier %d\nPier %d\n",LABEL_PIER(spanIdx),LABEL_PIER(spanIdx+1));

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      DeleteSpan(spanIdx,dlg.m_ItemIdx == 0 ? pgsTypes::PrevPier : pgsTypes::NextPier );
   }
}

void CPGSuperDoc::OnDeleteSelection() 
{
   if (  m_CurrentPierIdx != ALL_PIERS )
   {
      DeletePier(m_CurrentPierIdx);
   }
   else if ( m_CurrentSpanIdx != ALL_SPANS )
   {
      DeleteSpan(m_CurrentSpanIdx);
   }
   else
   {
      ASSERT(FALSE); // should not get here
   }
}

void CPGSuperDoc::OnUpdateDeleteSelection(CCmdUI* pCmdUI) 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   long nSpans = pBridgeDesc->GetSpanCount();
   if ( nSpans == 1 )
   {
      pCmdUI->Enable(FALSE);
      // can't delete the last span
      return;
   }

   if ( m_CurrentPierIdx != ALL_PIERS )
   {
      long nPiers = pBridgeDesc->GetPierCount();
      CString strLabel;
      if ( m_CurrentPierIdx == 0 || m_CurrentPierIdx == nPiers-1 )
         strLabel.Format("Delete Abutment %d",m_CurrentPierIdx+1);
      else
         strLabel.Format("Delete Pier %d",m_CurrentPierIdx+1);

      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else if ( m_CurrentSpanIdx != ALL_SPANS  )
   {
      if ( m_CurrentGirderIdx != ALL_GIRDERS  )
      {
         // girder is selected.. can't delete an individual girder
         pCmdUI->Enable(FALSE);
      }
      else
      {
         // only span is selected
         CString strLabel;
         strLabel.Format("Delete Span %d",LABEL_SPAN(m_CurrentSpanIdx));
         pCmdUI->SetText(strLabel);
         pCmdUI->Enable(TRUE);
      }
   }
   else
   {
      // can't delete alignment, bridge, deck, or section cut tool
      pCmdUI->Enable(FALSE);
   }
}

void CPGSuperDoc::DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType face)
{
   txnDeleteSpan* pTxn = new txnDeleteSpan(pierIdx,face);
   txnTxnManager::GetInstance()->Execute(pTxn);
}

void CPGSuperDoc::DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType)
{
   PierIndexType pierIdx = (pierRemoveType == pgsTypes::PrevPier ? spanIdx : spanIdx+1);
   pgsTypes::PierFaceType pierFace = (pierRemoveType == pgsTypes::PrevPier ? pgsTypes::Ahead : pgsTypes::Back);
   DeletePier(pierIdx,pierFace);
}

void CPGSuperDoc::OnUndo() 
{
   txnTxnManager* pTxnMgr = txnTxnManager::GetInstance();
   pTxnMgr->Undo();
}

void CPGSuperDoc::OnUpdateUndo(CCmdUI* pCmdUI) 
{
   txnTxnManager* pTxnMgr = txnTxnManager::GetInstance();
   
   if ( pTxnMgr->CanUndo() )
   {
      pCmdUI->Enable(TRUE);
      CString strCommand;
      strCommand.Format("Undo %s\tCtrl+Z",pTxnMgr->UndoName().c_str());
      pCmdUI->SetText(strCommand);
   }
   else
   {
      pCmdUI->SetText("Undo\tCtrl+Z");
      pCmdUI->Enable(FALSE);
   }
}

void CPGSuperDoc::OnRedo() 
{
   txnTxnManager* pTxnMgr = txnTxnManager::GetInstance();
   ASSERT( pTxnMgr->CanRedo() );
   pTxnMgr->Redo();
}

void CPGSuperDoc::OnUpdateRedo(CCmdUI* pCmdUI) 
{
   txnTxnManager* pTxnMgr = txnTxnManager::GetInstance();
   
   if ( pTxnMgr->CanRedo() )
   {
      pCmdUI->Enable(TRUE);
      CString strCommand;
      strCommand.Format("Redo %s\tCtrl+Y",pTxnMgr->RedoName().c_str());
      pCmdUI->SetText(strCommand);
   }
   else
   {
      pCmdUI->SetText("Redo\tCtrl+Y");
      pCmdUI->Enable(FALSE);
   }
}

void CPGSuperDoc::OnInsert() 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   std::vector< std::pair<PierIndexType,pgsTypes::PierFaceType> > keys;

   CString strItems;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CString strItem;
      CString strPier;
      if ( pierIdx == 0 || pierIdx == nPiers-1 )
         strPier = "Abutment";
      else 
         strPier = "Pier";

      strItem.Format("Before %s %d\n",strPier,pierIdx+1);
      strItems += strItem;
      keys.push_back( std::make_pair(pierIdx,pgsTypes::Back) );

      strItem.Format("After %s %d\n",strPier,pierIdx+1);
      strItems += strItem;
      keys.push_back( std::make_pair(pierIdx,pgsTypes::Ahead) );
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = "Insert Span";
   dlg.m_strItems = strItems;
   dlg.m_strLabel = "Select location to insert span";
   dlg.m_ItemIdx = 0;

   if ( dlg.DoModal() == IDOK )
   {
      PierIndexType refPierIdx = keys[dlg.m_ItemIdx].first;
      pgsTypes::PierFaceType face = keys[dlg.m_ItemIdx].second;
      InsertSpan(refPierIdx,face);
   }
}

void CPGSuperDoc::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace)
{
   txnInsertSpan* pTxn = new txnInsertSpan(refPierIdx,pierFace);
   txnTxnManager::GetInstance()->Execute(pTxn);
}

void CPGSuperDoc::OnOptionsHints() 
{
   CString strText;
   strText = "Reset all user interface hints";
   int result = AfxMessageBox(strText,MB_YESNO);
   if ( result == IDYES )
   {
      CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
      UINT hintSettings = pApp->GetUIHintSettings();
      hintSettings = UIHINT_ENABLE_ALL;
      pApp->SetUIHintSettings(hintSettings);
   }
}

void CPGSuperDoc::OnOptionsLabels() 
{
   CGirderLabelFormatDlg dlg;
   dlg.m_Format = (pgsGirderLabel::UseAlphaLabel() ? 0 : 1);
   if ( dlg.DoModal() )
   {
      bool bUseAlpha = dlg.m_Format == 0 ? true : false;
      if ( bUseAlpha != pgsGirderLabel::UseAlphaLabel() )
      {
         pgsGirderLabel::UseAlphaLabel(bUseAlpha);
         UpdateAllViews(NULL,HINT_GIRDERLABELFORMATCHANGED);
      }
   }
}

BOOL CPGSuperDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   // Interrupt the normal command processing to handle reports
   // The report menu items are dynamically generated and so are their IDs
   // If the command code is in the range IDM_REPORT to IDM_REPORT+nReports a specific
   // report name was selected from a menu. Send the message on to the OnReport handler
   // and tell MFC that this message has been handled (return TRUE)
   GET_IFACE(IReportManager,pReportMgr);
   CollectionIndexType nReports = pReportMgr->GetReportBuilderCount();
   BOOL bIsReport      = (GetReportCommand(0,false) <= nID && nID <= GetReportCommand(nReports-1,false));
   BOOL bIsQuickReport = (GetReportCommand(0,true)  <= nID && nID <= GetReportCommand(nReports-1,true));

   if ( bIsReport || bIsQuickReport )
   {
      if ( nCode == CN_UPDATE_COMMAND_UI )
      {
         CCmdUI* pCmdUI = (CCmdUI*)(pExtra);
         pCmdUI->Enable(TRUE);
         return TRUE;
      }
      else if ( nCode == CN_COMMAND )
      {
         if ( bIsQuickReport )
            OnQuickReport(nID);
         else
            OnReport(nID);

         return TRUE;
      }
   }
	
	return CDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPGSuperDoc::PopulateReportMenu()
{
   if (m_bIsReportMenuPopulated)
      return;

   // Called from CCountedMultiDocTemplate::InitialUpdateFrame()
   // Fill the report sub menu with the names of all the reports
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   CMenu* pMainMenu = pMainFrame->GetMenu();

   int pos = FindMenuItem(pMainMenu,"&View");
   ASSERT( 0 <= pos );

   CMenu* pViewMenu = pMainMenu->GetSubMenu(pos);
   ASSERT( pViewMenu != NULL );

   pos = FindMenuItem(pViewMenu,"&Reports");
   ASSERT( 0 <= pos );

   // Delete the reports menu
   pViewMenu->DeleteMenu(pos,MF_BYPOSITION);

   // Create a new reports menu
   CMenu mnuReports;
   mnuReports.CreatePopupMenu();
   pViewMenu->InsertMenu(pos,MF_BYPOSITION | MF_POPUP,(UINT_PTR)mnuReports.GetSafeHmenu(),"&Reports");

   GET_IFACE(IReportManager,pReportMgr);
   std::vector<std::string> rptNames = pReportMgr->GetReportNames();

   int i = 0;
   std::vector<std::string>::iterator iter;
   for ( iter = rptNames.begin(); iter != rptNames.end(); iter++ )
   {
      std::string rptName = *iter;
      mnuReports.AppendMenu(MF_STRING,GetReportCommand(i,false),rptName.c_str());
      i++;
   }

   mnuReports.Detach();
   m_bIsReportMenuPopulated = true;
}

int CPGSuperDoc::FindMenuItem(CMenu* pParentMenu,const char* strTargetMenu)
{
   UINT nMenus = pParentMenu->GetMenuItemCount();
   for ( UINT menuPos = 0; menuPos < nMenus; menuPos++ )
   {
      CString strMenu;
      pParentMenu->GetMenuString(menuPos,strMenu,MF_BYPOSITION);

      if ( strMenu.Compare(strTargetMenu) == 0 )
      {
         return menuPos;
      }
   }

   return -1;
}

UINT_PTR CPGSuperDoc::GetReportCommand(CollectionIndexType rptIdx,bool bQuickReport)
{
   UINT baseID = IDM_REPORT;

   if ( !bQuickReport )
   {
      GET_IFACE(IReportManager,pReportMgr);
      Uint32 nReports = pReportMgr->GetReportBuilderCount();

      baseID += nReports + 1;
   }

   return (UINT_PTR)(rptIdx + baseID);
}

CollectionIndexType CPGSuperDoc::GetReportIndex(UINT nID,bool bQuickReport)
{
   UINT baseID = IDM_REPORT;
   if ( !bQuickReport )
   {
      GET_IFACE(IReportManager,pReportMgr);
      Uint32 nReports = pReportMgr->GetReportBuilderCount();

      baseID += nReports + 1;
   }

   return (CollectionIndexType)(nID - baseID);
}