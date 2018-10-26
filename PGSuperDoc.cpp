///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\stdafx.h"

#include "PGSuperAppPlugin\PGSuperApp.h"

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <objbase.h>
#include <initguid.h>

#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperBaseAppPlugin.h"

#include <WBFLCore_i.c>
#include <WBFLReportManagerAgent_i.c>
#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>
#include <WBFLDManip_i.c>
#include <WBFLDManipTools_i.c>

#include <PGSuperAppPlugin.h>
#include <PGSuperProjectImporterAppPlugin.h>

#include <PsgLib\PsgLib.h>

#include <IFace\Test1250.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Artifact.h>
#include <IFace\TxDOTCadExport.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>
#include <IFace\VersionInfo.h>
#include <IFace\GirderHandling.h>
#include <IFace\Project.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\Allowables.h>
#include <IFace\StatusCenter.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>
#include <EAF\EAFUIIntegration.h>
#include "PGSuperDocProxyAgent.h"

#include "SupportDrawStrategy.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategy.h"
#include "DistributedLoadDrawStrategy.h"
#include "MomentLoadDrawStrategy.h"
#include "GevEditLoad.h"


#include "HtmlHelp\HelpTopics.hh"

#include "Convert.h"

#include <ComCat.h>

#include "BridgeLinkCatCom.h"
#include "PGSuperCatCom.h"

#include "Hints.h"

#include "PGSuperException.h"
#include <System\FileStream.h>
#include <System\StructuredLoadXmlPrs.h>

// Helpers
#include <MathEx.h>

// Agents
#include "PGSuperDocProxyAgent.h"

// Dialogs
#include <MfcTools\DocTemplateFinder.h>
#include <MfcTools\LoadModifiersDlg.h>
#include "ProjectPropertiesDlg.h"
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
#include "RatingOptionsDlg.h"
#include "ConstructionLoadDlg.h"

#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <PgsExt\GirderArtifact.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\DesignArtifact.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>

#include <Reporting\ReportStyleHolder.h>

// Transactions
#include "EditAlignment.h"
#include "EditBridge.h"
#include "EditPier.h"
#include "EditSpan.h"
#include "EditGirder.h"
#include "DesignGirder.h"
#include "InsertDeleteSpan.h"
#include "EditLLDF.h"
#include "CopyGirder.h"
#include "EditEnvironment.h"
#include "EditProjectCriteria.h"
#include "EditRatingCriteria.h"
#include "EditLiveLoad.h"
#include "EditAnalysisType.h"
#include "EditConstructionLoad.h"
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

// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define PGSUPER_PLUGIN_COMMAND_BASE 0xC000 // 49152 (this gives us about 8100 plug commands)
#if PGSUPER_PLUGIN_COMMAND_BASE < _APS_NEXT_COMMAND_VALUE
#error "PGSuper Document Plugins and Extension Agents: Command IDs interfere with plug-in commands, change the plugin command base ID"
#endif

static const Float64 FILE_VERSION=2.0;

static bool DoesFolderExist(const CString& dirname);
static bool DoesFileExist(const CString& filname);


/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc

IMPLEMENT_DYNCREATE(CPGSuperDoc, CEAFBrokerDocument)

BEGIN_MESSAGE_MAP(CPGSuperDoc, CEAFBrokerDocument)
	//{{AFX_MSG_MAP(CPGSuperDoc)
	ON_COMMAND(ID_FILE_PROJECT_PROPERTIES, OnFileProjectProperties)
	ON_COMMAND(ID_PROJECT_ENVIRONMENT, OnProjectEnvironment)
	ON_COMMAND(ID_PROJECT_BRIDGEDESC, OnProjectBridgeDesc)
	ON_COMMAND(ID_PROJECT_SPEC, OnProjectSpec)
	ON_COMMAND(ID_RATING_SPEC,  OnRatingSpec)
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
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMomentLoad)
   ON_COMMAND(ID_CONSTRUCTION_LOADS,OnConstructionLoads)
	ON_COMMAND(ID_PROJECT_ALIGNMENT, OnProjectAlignment)
	ON_COMMAND(ID_PROJECT_ANALYSIS, OnProjectAnalysis)
	ON_COMMAND(ID_PROJECT_PIERDESC, OnEditPier)
	ON_COMMAND(ID_PROJECT_SPANDESC, OnEditSpan)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
	ON_COMMAND(ID_LOADS_LLDF, OnLoadsLldf)
   ON_COMMAND(ID_LIVE_LOADS,OnLiveLoads)
	ON_COMMAND(ID_INSERT, OnInsert)
	ON_COMMAND(ID_OPTIONS_HINTS, OnOptionsHints)
	ON_COMMAND(ID_OPTIONS_LABELS, OnOptionsLabels)

   ON_COMMAND(ID_VIEW_BRIDGEMODELEDITOR, OnViewBridgeModelEditor)
   ON_COMMAND(ID_VIEW_GIRDEREDITOR, OnViewGirderEditor)
   ON_COMMAND(ID_VIEW_LIBRARYEDITOR, OnViewLibraryEditor)
   ON_COMMAND(ID_VIEW_ANALYSISRESULTS, OnViewAnalysisResults)
   ON_COMMAND(ID_VIEW_STABILITY,OnViewStability)
	ON_COMMAND(ID_EDIT_USERLOADS, OnEditUserLoads)
   //}}AFX_MSG_MAP
	
   ON_COMMAND(ID_PROJECT_UPDATENOW, OnUpdateNow)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_UPDATENOW, OnUpdateUpdateNow)

	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)


   ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_STATUSCENTER, ID_VIEW_STATUSCENTER3, CEAFBrokerDocument::OnUpdateViewStatusCenter)
	ON_COMMAND_RANGE(ID_VIEW_STATUSCENTER, ID_VIEW_STATUSCENTER3,OnViewStatusCenter)

   ON_UPDATE_COMMAND_UI(ID_VIEW_REPORTS,OnUpdateViewReports)

	ON_UPDATE_COMMAND_UI(FIRST_DATA_IMPORTER_PLUGIN, OnImportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_IMPORTER_PLUGIN,LAST_DATA_IMPORTER_PLUGIN, OnImport)
	ON_UPDATE_COMMAND_UI(FIRST_DATA_EXPORTER_PLUGIN, OnExportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_EXPORTER_PLUGIN,LAST_DATA_EXPORTER_PLUGIN, OnExport)

   // this doesn't work for documents... see OnCmdMsg for handling of WM_NOTIFY
   //ON_NOTIFY(TBN_DROPDOWN,ID_STDTOOLBAR,OnViewReports)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc construction/destruction

CPGSuperDoc::CPGSuperDoc():
m_bDesignSlabOffset(true),
m_bAutoCalcEnabled(true)
{
	EnableAutomation();

	AfxOleLockApp();

   m_Selection.Type      = CSelection::None;
   m_Selection.PierIdx   = INVALID_INDEX;
   m_Selection.SpanIdx   = INVALID_INDEX;
   m_Selection.GirderIdx = INVALID_INDEX;

   m_LibMgr.SetName( _T("PGSuper Library") );

   CEAFAutoCalcDocMixin::SetDocument(this);

   m_bShowProjectProperties = true;

   m_pPGSuperDocProxyAgent = NULL;

   m_PluginMgr.LoadPlugins(); // these are the data importers and exporters

   m_ViewCallbackID = 0;

   // Set the base command ID for EAFDocumentPlugin objects (not currently supported)
   // and extension agents (supported)
   GetPluginCommandManager()->SetBaseCommandID(PGSUPER_PLUGIN_COMMAND_BASE);
}

CPGSuperDoc::~CPGSuperDoc()
{
   m_DocUnitSystem.Release();
   m_PluginMgr.UnloadPlugins();
   AfxOleUnlockApp();
}

// CEAFBrokerDocument overrides
CATID CPGSuperDoc::GetAgentCategoryID()
{
   // all dynamically created agents used with this document type must
   // belong to the CATID_PGSuperAgent category
   return CATID_PGSuperAgent;
}

CATID CPGSuperDoc::GetExtensionAgentCategoryID()
{
   return CATID_PGSuperExtensionAgent;
}

// CEAFAutoCalcDocMixin overrides
bool CPGSuperDoc::IsAutoCalcEnabled() const
{
   return m_bAutoCalcEnabled;
}

void CPGSuperDoc::EnableAutoCalc(bool bEnable)
{
   if ( m_bAutoCalcEnabled != bEnable )
   {
      bool bWasDisabled = !IsAutoCalcEnabled();
      m_bAutoCalcEnabled = bEnable;

      CPGSuperStatusBar* pStatusBar = ((CPGSuperStatusBar*)EAFGetMainFrame()->GetStatusBar());
      pStatusBar->AutoCalcEnabled( m_bAutoCalcEnabled );

      // If AutoCalc was off and now it is on,
      // Update the views.
      if ( bWasDisabled && IsAutoCalcEnabled() )
        OnUpdateNow();
   }
}

void CPGSuperDoc::OnUpdateNow()
{
   CEAFAutoCalcDocMixin::OnUpdateNow();
}

void CPGSuperDoc::OnUpdateUpdateNow(CCmdUI* pCmdUI)
{
   CEAFAutoCalcDocMixin::OnUpdateUpdateNow(pCmdUI);
}

void CPGSuperDoc::OnViewStatusCenter(UINT nID)
{
   CEAFBrokerDocument::OnViewStatusCenter();
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

void CPGSuperDoc::GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem)
{
   (*ppDocUnitSystem) = m_DocUnitSystem;
   (*ppDocUnitSystem)->AddRef();
}

void CPGSuperDoc::EditAlignmentDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IRoadwayData,pAlignment);
   CAlignmentDescriptionDlg dlg(_T("Alignment Description"),m_pBroker);

   dlg.m_AlignmentPage.m_AlignmentData = pAlignment->GetAlignmentData2();
   dlg.m_ProfilePage.m_ProfileData = pAlignment->GetProfileData2();
   dlg.m_CrownSlopePage.m_RoadwaySectionData = pAlignment->GetRoadwaySectionData();

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditAlignment* pTxn = new txnEditAlignment(pAlignment->GetAlignmentData2(),     dlg.m_AlignmentPage.m_AlignmentData,
                                                    pAlignment->GetProfileData2(),       dlg.m_ProfilePage.m_ProfileData,
                                                    pAlignment->GetRoadwaySectionData(), dlg.m_CrownSlopePage.m_RoadwaySectionData );

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDoc::EditBridgeDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDoc::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDoc::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDoc::EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

   GirderIndexType ngrds = pBridge->GetGirderCount(m_Selection.SpanIdx == ALL_SPANS ? 0 : m_Selection.SpanIdx);
   if ( m_Selection.GirderIdx < ngrds)
      gdrIdx = girder;
   else
      gdrIdx = ngrds-1;

   // collect current values for later undo
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   bool bUseSameGirder              = pBridgeDesc->UseSameGirderForEntireBridge();
   std::_tstring strGirderName        = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderName(gdrIdx);
   CGirderData girderData           = pGirderData->GetGirderData(spanIdx,gdrIdx);
   CShearData  shearData            = pShear->GetShearData(spanIdx,gdrIdx);
   CLongitudinalRebarData rebarData = pLongitudinalRebar->GetLongitudinalRebarData( spanIdx, gdrIdx );
   double liftingLocation           = pGirderLifting->GetLeftLiftingLoopLocation( spanIdx, gdrIdx );
   double trailingOverhang          = pGirderHauling->GetTrailingOverhang( spanIdx, gdrIdx );
   double leadingOverhang           = pGirderHauling->GetLeadingOverhang( spanIdx, gdrIdx );

   CGirderDescDlg dlg(spanIdx,gdrIdx);
   dlg.m_strGirderName = strGirderName;
   dlg.m_GirderData    = girderData;

   pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();
   Float64 slabOffset[2];
   pIBridgeDesc->GetSlabOffset(spanIdx,gdrIdx,&slabOffset[pgsTypes::metStart],&slabOffset[pgsTypes::metEnd]);
   dlg.m_General.m_SlabOffsetType = slabOffsetType;
   dlg.m_General.m_SlabOffset[pgsTypes::metStart] = slabOffset[pgsTypes::metStart];
   dlg.m_General.m_SlabOffset[pgsTypes::metEnd]   = slabOffset[pgsTypes::metEnd];



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

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);

      return true;
   }
   //else if (st == IDC_FIX_BRIDGE)
   //{
   //   // The user wants to alter the bridge so that it can accept post-tensioning
   //   GET_IFACE(IBridgeDescription,pBridgeDesc);
   //   pBridgeDesc->ConfigureBridgeForPostTensioning();

   //   // take the user back to the editing interface
   //   // TODO: Make this go back to the Tendons page
   //   SelectGirder(dlg.m_CurrentSpanIdx,dlg.m_CurrentGirderIdx);
   //   EAFGetMainFrame()->PostMessage(WM_COMMAND,ID_EDIT_GIRDER);

   //   return false;
   //}
   else
   {
      return false;
   }
}
   
void CPGSuperDoc::AddPointLoad(const CPointLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditPointLoadDlg dlg(loadData,m_pBroker);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDoc::EditPointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData& loadData = pUserDefinedLoads->GetPointLoad(loadIdx);

   CEditPointLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditPointLoad* pTxn = new txnEditPointLoad(loadIdx,loadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDoc::DeletePointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeletePointLoad* pTxn = new txnDeletePointLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::AddDistributedLoad(const CDistributedLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditDistributedLoadDlg dlg(loadData,m_pBroker);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDoc::EditDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData& loadData = pUserDefinedLoads->GetDistributedLoad(loadIdx);

   CEditDistributedLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditDistributedLoad* pTxn = new txnEditDistributedLoad(loadIdx,loadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDoc::DeleteDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeleteDistributedLoad* pTxn = new txnDeleteDistributedLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::AddMomentLoad(const CMomentLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditMomentLoadDlg dlg(loadData,m_pBroker);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDoc::EditMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData& loadData = pUserDefinedLoads->GetMomentLoad(loadIdx);

   CEditMomentLoadDlg dlg(loadData,m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (loadData != dlg.m_Load)
      {
         txnEditMomentLoad* pTxn = new txnEditMomentLoad(loadIdx,loadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDoc::DeleteMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeleteMomentLoad* pTxn = new txnDeleteMomentLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::EditGirderViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetGirderEditorSettings();

	CGirderEditorSettingsSheet dlg(IDS_GM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      SetGirderEditorSettings(settings);

      // tell the world we've changed settings
      UpdateAllViews( 0, HINT_GIRDERVIEWSETTINGSCHANGED, 0 );
   }
}

void CPGSuperDoc::EditBridgeViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetBridgeEditorSettings();

	CBridgeEditorSettingsSheet dlg(IDS_BM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      settings |= IDB_PV_DRAW_ISOTROPIC;
      SetBridgeEditorSettings(settings);
   }

   // tell the world we've changed settings
   UpdateAllViews( 0, HINT_BRIDGEVIEWSETTINGSCHANGED, 0 );
	
}

BOOL CPGSuperDoc::UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir)
{
   CFileFind dir_finder;
   BOOL bMoreDir = dir_finder.FindFile(CString(lpszDir)+_T("\\*"));

   // recursively go through the directories
   while ( bMoreDir )
   {
      bMoreDir = dir_finder.FindNextFile();
      CString strDir = dir_finder.GetFilePath();

      if ( !dir_finder.IsDots() && dir_finder.IsDirectory() )
         UpdateTemplates(pProgress,strDir);
   }

   // done with the directories below this leave. Process the templates at this level
   CString strMessage;
   strMessage.Format(_T("Updating templates in %s"),lpszDir);
   pProgress->UpdateMessage(strMessage);

   CFileFind template_finder;
   BOOL bMoreTemplates = template_finder.FindFile(CString(lpszDir) + _T("\\*.pgt"));
   while ( bMoreTemplates )
   {
      bMoreTemplates      = template_finder.FindNextFile();
      CString strTemplate = template_finder.GetFilePath();

      strMessage.Format(_T("Updating %s"),template_finder.GetFileTitle());
      pProgress->UpdateMessage(strMessage);

      if ( !OpenTheDocument(strTemplate) )
         return FALSE;

      CEAFBrokerDocument::SaveTheDocument(strTemplate);

      m_pBroker->Reset();

      CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pInit(m_pBroker);
      pInit->InitAgents();
   }

   return TRUE;
}

BOOL CPGSuperDoc::UpdateTemplates()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);

   if  ( !Init() ) // load the agents and other necessary stuff
      return FALSE;

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // we want the template files to be "clean" with only PGSuper data in them
   // Tell the broker to not save data from any missing extension agents
   CComQIPtr<IBrokerPersist2> broker_persist(m_pBroker);
   ATLASSERT(broker_persist);
   VARIANT_BOOL bFlag;
   broker_persist->GetSaveMissingAgentDataFlag(&bFlag);
   broker_persist->SetSaveMissingAgentDataFlag(VARIANT_FALSE);

   UpdateTemplates(pProgress,workgroup_folder);

   // restore the flag to its previous state
   broker_persist->SetSaveMissingAgentDataFlag(bFlag);


   return FALSE; // didn't really open a file
}

Uint32 CPGSuperDoc::RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback)
{
   Uint32 key = m_ViewCallbackID++;
   m_BridgePlanViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDoc::UnregisterBridgePlanViewCallback(Uint32 ID)
{
   std::map<Uint32,IBridgePlanViewEventCallback*>::iterator found = m_BridgePlanViewCallbacks.find(ID);
   if ( found == m_BridgePlanViewCallbacks.end() )
      return false;

   m_BridgePlanViewCallbacks.erase(found);

   return true;
}

std::map<Uint32,IBridgePlanViewEventCallback*> CPGSuperDoc::GetBridgePlanViewCallbacks()
{
   return m_BridgePlanViewCallbacks;
}

Uint32 CPGSuperDoc::RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback)
{
   Uint32 key = m_ViewCallbackID++;
   m_BridgeSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDoc::UnregisterBridgeSectionViewCallback(Uint32 ID)
{
   std::map<Uint32,IBridgeSectionViewEventCallback*>::iterator found = m_BridgeSectionViewCallbacks.find(ID);
   if ( found == m_BridgeSectionViewCallbacks.end() )
      return false;

   m_BridgeSectionViewCallbacks.erase(found);

   return true;
}

std::map<Uint32,IBridgeSectionViewEventCallback*> CPGSuperDoc::GetBridgeSectionViewCallbacks()
{
   return m_BridgeSectionViewCallbacks;
}

Uint32 CPGSuperDoc::RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback)
{
   Uint32 key = m_ViewCallbackID++;
   m_GirderElevationViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDoc::UnregisterGirderElevationViewCallback(Uint32 ID)
{
   std::map<Uint32,IGirderElevationViewEventCallback*>::iterator found = m_GirderElevationViewCallbacks.find(ID);
   if ( found == m_GirderElevationViewCallbacks.end() )
      return false;

   m_GirderElevationViewCallbacks.erase(found);

   return true;
}

std::map<Uint32,IGirderElevationViewEventCallback*> CPGSuperDoc::GetGirderElevationViewCallbacks()
{
   return m_GirderElevationViewCallbacks;
}

Uint32 CPGSuperDoc::RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback)
{
   Uint32 key = m_ViewCallbackID++;
   m_GirderSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDoc::UnregisterGirderSectionViewCallback(Uint32 ID)
{
   std::map<Uint32,IGirderSectionViewEventCallback*>::iterator found = m_GirderSectionViewCallbacks.find(ID);
   if ( found == m_GirderSectionViewCallbacks.end() )
      return false;

   m_GirderSectionViewCallbacks.erase(found);

   return true;
}

std::map<Uint32,IGirderSectionViewEventCallback*> CPGSuperDoc::GetGirderSectionViewCallbacks()
{
   return m_GirderSectionViewCallbacks;
}

BOOL CPGSuperDoc::OnNewDocumentFromTemplate(LPCTSTR lpszPathName)
{
   if ( !CEAFDocument::OnNewDocumentFromTemplate(lpszPathName) )
      return FALSE;

   InitProjectProperties();

   return TRUE;
}

void CPGSuperDoc::OnCloseDocument()
{
   CEAFBrokerDocument::OnCloseDocument();
}

void CPGSuperDoc::InitProjectProperties()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   CString engineer_name = pPGSuper->GetEngineerName();
   CString company       = pPGSuper->GetEngineerCompany();

   GET_IFACE( IProjectProperties, pProjProp );

   pProjProp->SetEngineer(std::_tstring(engineer_name));
   pProjProp->SetCompany(std::_tstring(company));

   if ( ShowProjectPropertiesOnNewProject() )
      OnFileProjectProperties();
}

void CPGSuperDoc::OnCreateInitialize()
{
   // called before any data is loaded/created in the document
   CEAFBrokerDocument::OnCreateInitialize();

   // Cant' hold events here because this is before any document
   // initialization happens. ie., the broker hasn't been
   // created yet
}

void CPGSuperDoc::OnCreateFinalize()
{
   CEAFBrokerDocument::OnCreateFinalize();

   PopulateReportMenu();

   // if user is on Windows Vista or Windows 7, the Send Email feature doesn't work
   // so we will remove it from the menu

   OSVERSIONINFO osInfo;
   osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osInfo);
   bool bRemoveEmailOption = false;
   if ( 6 < osInfo.dwMajorVersion || (osInfo.dwMajorVersion == 6 && 1 <= osInfo.dwMinorVersion) )
   {
      // this is Windows 7 or a future OS... assume feature isn't supported
      bRemoveEmailOption = true;
   }

   if ( bRemoveEmailOption ) 
   {
      // get main menu
      CEAFMenu* pMainMenu = GetMainMenu();

      // find position of file menu
      UINT filePos = pMainMenu->FindMenuItem(_T("&File"));

      // get the file menu
      CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

      // get the text string from the email command
      CString strEmail;
      pFileMenu->GetMenuString(ID_FILE_SEND_MAIL,strEmail,MF_BYCOMMAND);

      // find the position of the email command
      UINT emailPos = pFileMenu->FindMenuItem(strEmail);

      // remove the email command and the adjacent separator
      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,NULL);
      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,NULL);
   }

   // Set the autocalc state on the status bar
   CPGSuperStatusBar* pStatusBar = ((CPGSuperStatusBar*)EAFGetMainFrame()->GetStatusBar());
   pStatusBar->AutoCalcEnabled( IsAutoCalcEnabled() );

   // views have been initilized so fire any pending events
   GET_IFACE(IEvents,pEvents);
   GET_IFACE(IUIEvents,pUIEvents);
   pEvents->FirePendingEvents(); 
   pUIEvents->HoldEvents(false);
}

BOOL CPGSuperDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
   CString file_ext;
   CString file_name(lpszPathName);
	int charpos = file_name.ReverseFind(_T('.'));
	if (0 <= charpos)
   {
		file_ext = file_name.Right(file_name.GetLength() - charpos);
   }

   if ( file_ext.CompareNoCase(_T(".pgt")) == 0 )
   {
      // this is a template file
      return OnNewDocumentFromTemplate(lpszPathName);
   }
   else
   {
      return CEAFBrokerDocument::OnOpenDocument(lpszPathName);
   }
}

BOOL CPGSuperDoc::OpenTheDocument(LPCTSTR lpszPathName)
{
   // don't fire UI events as the UI isn't completely built when the document is created
   // (view classes haven't been initialized)
   m_pPGSuperDocProxyAgent->HoldEvents();
   // Events are released in OnCreateFinalize()

   if ( !CEAFBrokerDocument::OpenTheDocument(lpszPathName) )
      return FALSE;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   m_DocUnitSystem->put_UnitMode( IS_US_UNITS(pDisplayUnits) ? umUS : umSI );
  
   return TRUE;
}

HRESULT CPGSuperDoc::ConvertTheDocument(LPCTSTR lpszPathName, CString* prealFileName)
{
   // Open the document and look at the second line
   // If the version tag is 0.80, then the document needs to be converted
   std::ifstream ifile(lpszPathName);
   if ( !ifile )
   {
      return E_INVALIDARG;
   }

   char line[50];
   ifile.getline(line,50);
   ifile.getline(line,50);
   CString strLine(line);
   strLine.TrimLeft();
   int loc = strLine.Find(_T(">"),0);
   if (loc!=-1)
   {
      strLine = strLine.Left(loc+1);
      if ( strLine == CString("<PGSuper version=\"0.8\">") ||
           strLine == CString("<PGSuper version=\"0.83\">")  )
      {
         ifile.close();

         _PgsFileConvert1 convert;
         if ( !convert.CreateDispatch(_T("convert.PgsFileConvert1")) )
         {
            return REGDB_E_CLASSNOTREG;
         }

         BSTR bstrPathName = CString(lpszPathName).AllocSysString();
         BSTR bstrRealName=0;
         long convert_status = convert.Convert( &bstrPathName, &bstrRealName );
         if ( convert_status == -1  )
         {
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
            return E_FAIL;
         }
         else if (convert_status == 1)
         {
            // file was converted
            ::SysFreeString(bstrPathName);
            *prealFileName = CString(bstrRealName);
            ::SysFreeString(bstrRealName);
            return S_OK;
         }
         else
         {
            // converter did not convert file - this should be impossible becuase
            // we are checking versions above. must be wrong version of the converter
            ATLASSERT(false);
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
         }
      }
      else
      {
         // no file conversion needed - give realFile same name as MFC's file
         *prealFileName = CString(lpszPathName);
         return S_FALSE; // succeeded, but not converted
      }
   }

   return E_FAIL;
}

CString CPGSuperDoc::GetRootNodeName()
{
   return _T("PGSuper");
}

Float64 CPGSuperDoc::GetRootNodeVersion()
{
   return FILE_VERSION;
}

HRESULT CPGSuperDoc::OpenDocumentRootNode(IStructuredSave* pStrSave)
{
  HRESULT hr = CEAFDocument::OpenDocumentRootNode(pStrSave);
  if ( FAILED(hr) )
     return hr;

  hr = pStrSave->put_Property(_T("Version"),CComVariant(theApp.GetVersion(true)));
  if ( FAILED(hr) )
     return hr;

  return S_OK;
}

HRESULT CPGSuperDoc::OpenDocumentRootNode(IStructuredLoad* pStrLoad)
{
   HRESULT hr = CEAFDocument::OpenDocumentRootNode(pStrLoad);
   if ( FAILED(hr) )
      return hr;

   Float64 version;
   hr = pStrLoad->get_Version(&version);
   if ( FAILED(hr) )
      return hr;

   if ( 1.0 < version )
   {
      CComVariant var;
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Version"),&var);
      if ( FAILED(hr) )
         return hr;

   #if defined _DEBUG
      TRACE(_T("Loading data saved with PGSuper Version %s\n"),CComBSTR(var.bstrVal));
   }
   else
   {
      TRACE(_T("Loading data saved with PGSuper Version 2.1 or earlier\n"));
   #endif
   } // clses the bracket for if ( 1.0 < version )

   return S_OK;
}

void CPGSuperDoc::OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
{
   CString msg;

   GET_IFACE(IEAFProjectLog,pLog);

   pLog->LogMessage(_T(""));
   pLog->LogMessage(_T("An error occured while recovering your last successful save."));
   msg.Format(_T("It is highly likely that the file %s is corrupt."), lpszPathName);
   pLog->LogMessage( msg );
   pLog->LogMessage(_T("To recover from this error,"));
   msg.Format(_T("   1. Delete %s"), lpszPathName );
   pLog->LogMessage( msg );
   msg.Format(_T("   2. Rename %s to %s"), lpszBackup, lpszPathName );
   pLog->LogMessage( msg );
   pLog->LogMessage(_T(""));

   std::_tstring strLogFileName = pLog->GetName();

   AfxFormatString2( msg, IDS_E_SAVERECOVER1, lpszPathName, CString(strLogFileName.c_str()) );
   AfxMessageBox(msg );
}

void CPGSuperDoc::OnErrorRemaningSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
{
   CString msg;

   GET_IFACE(IEAFProjectLog,pLog);

   pLog->LogMessage(_T(""));
   pLog->LogMessage(_T("An error occured while recovering your last successful save."));
   msg.Format(_T("It is highly likely that the file %s no longer exists."), lpszPathName);
   pLog->LogMessage( msg );
   pLog->LogMessage(_T("To recover from this error,"));
   msg.Format(_T("   1. If %s exists, delete it."), lpszPathName );
   pLog->LogMessage( msg );
   msg.Format(_T("   2. Rename %s to %s"), lpszBackup, lpszPathName );
   pLog->LogMessage( msg );
   pLog->LogMessage(_T(""));

   std::_tstring strLogFileName = pLog->GetName();

   AfxFormatString2( msg, IDS_E_SAVERECOVER2, lpszPathName, CString(strLogFileName.c_str()) );
   AfxMessageBox( msg );
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc diagnostics

#ifdef _DEBUG
void CPGSuperDoc::AssertValid() const
{
	CEAFBrokerDocument::AssertValid();
}

void CPGSuperDoc::Dump(CDumpContext& dc) const
{
	CEAFBrokerDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc commands

BOOL CPGSuperDoc::Init()
{
   if ( !CEAFBrokerDocument::Init() )
      return FALSE;

   // extend the default initialization

   // load up the library manager
   if ( !LoadMasterLibrary() )
      return FALSE;

   // Setup the library manager (same as if it changed)
   OnLibMgrChanged( &m_LibMgr );

   // Set up the document unit system
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);
   CComPtr<IAppUnitSystem> appUnitSystem;
   pPGSuper->GetAppUnitSystem(&appUnitSystem);
   CreateDocUnitSystem(appUnitSystem,&m_DocUnitSystem);

   return TRUE;
}

BOOL CPGSuperDoc::LoadSpecialAgents(IBrokerInitEx2* pBrokerInit)
{
   if ( !CEAFBrokerDocument::LoadSpecialAgents(pBrokerInit) )
      return FALSE;

   CComObject<CPGSuperDocProxyAgent>* pDocProxyAgent;
   CComObject<CPGSuperDocProxyAgent>::CreateInstance(&pDocProxyAgent);
   m_pPGSuperDocProxyAgent = dynamic_cast<CPGSuperDocProxyAgent*>(pDocProxyAgent);
   m_pPGSuperDocProxyAgent->SetDocument( this );

   CComPtr<IAgentEx> pAgent(m_pPGSuperDocProxyAgent);
   
   HRESULT hr = pBrokerInit->AddAgent( pAgent );
   if ( FAILED(hr) )
      return hr;

   // we want to use some special agents
   CLSID clsid[] = {CLSID_SysAgent,CLSID_ReportManager};
   if ( !LoadAgents(pBrokerInit, clsid, sizeof(clsid)/sizeof(CLSID) ) )
      return FALSE;

   return TRUE;
}

void CPGSuperDoc::OnFileProjectProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IProjectProperties, pProjProp );

   CProjectPropertiesDlg dlg;

   dlg.m_Bridge    = pProjProp->GetBridgeName();
   dlg.m_BridgeID  = pProjProp->GetBridgeId();
   dlg.m_JobNumber = pProjProp->GetJobNumber();
   dlg.m_Engineer  = pProjProp->GetEngineer();
   dlg.m_Company   = pProjProp->GetCompany();
   dlg.m_Comments  = pProjProp->GetComments();
   dlg.m_bShowProjectProperties = ShowProjectPropertiesOnNewProject();


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
      ShowProjectPropertiesOnNewProject(dlg.m_bShowProjectProperties);

      // Turn updates back on.  If something changed, this will cause an
      // event to fire on the doc proxy event sink.
      pProjProp->EnableUpdate( true );
   }
}


void CPGSuperDoc::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleOpenDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while opening %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredLoad not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, ::AfxGetAppName() );
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
         log_msg.Format(_T("An unknown error occured while opening the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleSaveDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while saving %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredSave not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, ::AfxGetAppName() );
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
         log_msg.Format(_T("An unknown error occured while closing the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleConvertDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while converting %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("File converter is not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, ::AfxGetAppName() );
      break;

   case E_INVALIDARG:
      msg1.Format( _T("%s could not be opened"),lpszPathName);
      pLog->LogMessage( msg1 );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while converting the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSuperDoc::OnProjectEnvironment() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
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
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );

   std::vector<std::_tstring> specs;
   pLibNames->EnumSpecNames( &specs );
	CSpecDlg dlg(specs);

   GET_IFACE( ISpecification, pSpec );

   std::_tstring cur_spec = pSpec->GetSpecification();
   dlg.m_Spec = cur_spec;
   if ( dlg.DoModal() )
   {
      if ( dlg.m_Spec != cur_spec )
      {
         txnEditProjectCriteria* pTxn = new txnEditProjectCriteria(cur_spec.c_str(),dlg.m_Spec.c_str());
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSuperDoc::OnRatingSpec()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );

   CRatingOptionsDlg dlg;
   txnRatingCriteriaData oldData;

   std::vector<std::_tstring> specs;
   pLibNames->EnumRatingCriteriaNames( &specs );
   dlg.m_GeneralPage.m_RatingSpecs = specs;

   std::vector<std::_tstring> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );
   dlg.m_LegalPage.m_AllNames  = all_names;
   dlg.m_PermitPage.m_AllNames = all_names;

   GET_IFACE( IRatingSpecification, pSpec );
   std::_tstring cur_spec = pSpec->GetRatingSpecification();
   oldData.m_General.CriteriaName  = cur_spec;
   oldData.m_General.bIncludePedestrianLiveLoad = pSpec->IncludePedestrianLiveLoad();

   oldData.m_General.SystemFactorFlexure = pSpec->GetSystemFactorFlexure();
   oldData.m_General.SystemFactorShear   = pSpec->GetSystemFactorShear();

   oldData.m_General.ADTT = pSpec->GetADTT();

   oldData.m_General.bDesignRating = pSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory);
   oldData.m_General.bLegalRating  = (pSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine)  || pSpec->IsRatingEnabled(pgsTypes::lrLegal_Special));
   oldData.m_General.bPermitRating = (pSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pSpec->IsRatingEnabled(pgsTypes::lrPermit_Special));

   oldData.m_Design.StrengthI_DC           = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_DW           = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_LL_Inventory = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_LL_Operating = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_Operating);

   oldData.m_Design.ServiceIII_DC          = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_DW          = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_LL          = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_Inventory);

   oldData.m_Design.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrDesign_Inventory);
   oldData.m_Design.bRateForShear = pSpec->RateForShear(pgsTypes::lrDesign_Inventory);

   GET_IFACE(ILiveLoads,pLiveLoads);
   oldData.m_Legal.IM_Truck_Routine = pLiveLoads->GetTruckImpact(pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.IM_Lane_Routine  = pLiveLoads->GetLaneImpact( pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.IM_Truck_Special = pLiveLoads->GetTruckImpact(pgsTypes::lltLegalRating_Special);
   oldData.m_Legal.IM_Lane_Special  = pLiveLoads->GetLaneImpact( pgsTypes::lltLegalRating_Special);
   oldData.m_Legal.RoutineNames     = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.SpecialNames     = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   
   oldData.m_Legal.StrengthI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_LegalSpecial);

   oldData.m_Legal.ServiceIII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_LegalSpecial);

   oldData.m_Legal.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bRateForShear    = pSpec->RateForShear(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bExcludeLaneLoad = pSpec->ExcludeLegalLoadLaneLoading();

   oldData.m_Permit.RoutinePermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   oldData.m_Permit.SpecialPermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitRoutine);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitSpecial);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitRoutine);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitSpecial);

   oldData.m_Permit.IM_Truck_Routine = pLiveLoads->GetTruckImpact(pgsTypes::lltPermitRating_Routine);
   oldData.m_Permit.IM_Lane_Routine  = pLiveLoads->GetLaneImpact( pgsTypes::lltPermitRating_Routine);

   oldData.m_Permit.IM_Truck_Special = pLiveLoads->GetTruckImpact(pgsTypes::lltPermitRating_Special);
   oldData.m_Permit.IM_Lane_Special  = pLiveLoads->GetLaneImpact( pgsTypes::lltPermitRating_Special);

   oldData.m_Permit.bRateForShear = pSpec->RateForShear(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.bCheckReinforcementYielding = pSpec->RateForStress(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.YieldStressCoefficient = pSpec->GetYieldStressLimitCoefficient();
   oldData.m_Permit.SpecialPermitType = pSpec->GetSpecialPermitType();

   dlg.m_GeneralPage.m_Data = oldData.m_General;
   dlg.m_DesignPage.m_Data  = oldData.m_Design;
   dlg.m_LegalPage.m_Data   = oldData.m_Legal;
   dlg.m_PermitPage.m_Data  = oldData.m_Permit;


   if ( dlg.DoModal() == IDOK )
   {
      txnRatingCriteriaData newData;

      newData.m_General = dlg.m_GeneralPage.m_Data;
      newData.m_Design  = dlg.m_DesignPage.m_Data;
      newData.m_Legal   = dlg.m_LegalPage.m_Data;
      newData.m_Permit  = dlg.m_PermitPage.m_Data;

      if ( oldData != newData )
      {
         txnEditRatingCriteria* pTxn = new txnEditRatingCriteria(oldData,newData);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
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
      pCmdUI->SetText( _T("Turn AutoCalc Off") );
   else
      pCmdUI->SetText( _T("Turn AutoCalc On") );
}

/*--------------------------------------------------------------------*/
void CPGSuperDoc::OnExportToTemplateFile() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // select inital directory to try and save in
   CString default_name = _T("PGSuper.pgt");
   CString initial_filespec;
   CString initial_dir;
   
   // prompt user to save current project to a template file
   CFileDialog  fildlg(FALSE,_T("pgt"),default_name,OFN_HIDEREADONLY,
                   _T("PGSuper Template Files (*.pgt)|*.pgt||"));

#if defined _DEBUG
   // If this is a debug build, then the developers are probably running
   // the software and they want the workgroup folder most often.
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);
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
         CString msg(_T(" The file: "));
         msg += file_path + _T(" exists. Overwrite it?");
         int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
         if (stm!=IDYES)
            return;
      }

      // write the file.
      SaveTheDocument( file_path );
   }
}

bool DoesFolderExist(const CString& dirname)
{
   if (dirname.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      CString nam = dirname + CString(_T("\\*.*"));
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

void CPGSuperDoc::UpdateAnalysisTypeStatusIndicator()
{
   CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)(EAFGetMainFrame()->GetStatusBar());

   GET_IFACE(ISpecification,pSpec);
   pStatusBar->SetAnalysisTypeStatusIndicator(pSpec->GetAnalysisType());
}

void CPGSuperDoc::DesignGirder(bool bPrompt,bool bDesignSlabOffset,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("There are errors that must be corrected before you can design a girder\r\n\r\nSee the Status Center for details."),MB_OK);
      return;
   }

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx);
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx);

   // set up default design options
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(IBridge,pBridge);
   bool can_design_Adim    = pSpec->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;

   bDesignSlabOffset = bDesignSlabOffset && can_design_Adim; // only design A if it's possible

   std::vector<SpanGirderHashType> gdr_list;
   if ( bPrompt )
   {
      GET_IFACE(ISpecification,pSpecification);

      // only show A design option if it's allowed in the library
      // Do not save this in registry because library selection should be default for new documents

      CDesignGirderDlg dlg(spanIdx, 
                           gdrIdx,
                           can_design_Adim, bDesignSlabOffset, m_pBroker);

      // Set initial dialog values based on last stored in registry. These may be changed
      // internally by dialog based on girder type, and other library values
      dlg.m_DesignForFlexure = (IsDesignFlexureEnabled() ? TRUE : FALSE);
      dlg.m_DesignForShear   = (IsDesignShearEnabled()   ? TRUE : FALSE);

      if ( dlg.DoModal() == IDOK )
      {
         bDesignSlabOffset    = dlg.m_DesignA != FALSE;    // we can override library setting here

         EnableDesignFlexure(dlg.m_DesignForFlexure == TRUE ? true : false);
         EnableDesignShear(  dlg.m_DesignForShear   == TRUE ? true : false);
         m_bDesignSlabOffset = bDesignSlabOffset; // retain value for current document

         gdr_list = dlg.m_GirderList;

         if (gdr_list.empty())
         {
            ATLASSERT(0); // dialog should handle this
            return; 
         }
      }
      else
      {
         return;
      }
   }
   else
   {
      // only one girder - spec'd from command line
      SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);
      gdr_list.push_back(hash);
   }

   DoDesignGirder(gdr_list, bDesignSlabOffset);
}

void CPGSuperDoc::OnProjectDesignGirder() 
{
   DesignGirder(true,m_bDesignSlabOffset,m_Selection.SpanIdx,m_Selection.GirderIdx);
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirect(CCmdUI* pCmdUI)
{
   pCmdUI->Enable( m_Selection.SpanIdx != ALL_SPANS && m_Selection.GirderIdx != ALL_GIRDERS );
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI)
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   pCmdUI->Enable( m_Selection.SpanIdx != ALL_SPANS && m_Selection.GirderIdx != ALL_GIRDERS && bDesignSlabOffset );
}

void CPGSuperDoc::OnProjectDesignGirderDirect()
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   m_bDesignSlabOffset = bDesignSlabOffset; // retain setting in document

   DesignGirder(false,bDesignSlabOffset,m_Selection.SpanIdx,m_Selection.GirderIdx);
}

void CPGSuperDoc::OnProjectDesignGirderDirectHoldSlabOffset()
{
   m_bDesignSlabOffset = false; // retain setting in document
   DesignGirder(false,false,m_Selection.SpanIdx,m_Selection.GirderIdx);
}

void CPGSuperDoc::DoDesignGirder(const std::vector<SpanGirderHashType>& girderList, bool doDesignADim)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IArtifact,pIArtifact);

   std::vector<const pgsDesignArtifact*> pArtifacts;

   // Need to scope the following block, otherwise the progress window will carry the cancel
   // and progress buttons past the design outcome and into any reports that need to be generated.
   {
      bool multi = girderList.size()>1;

      GET_IFACE(IProgress,pProgress);
      DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (multi)
         pProgress->Init(0,girderList.size(),1);  // and for multi-girders, a gauge.

      // Design all girders in list
      for (std::vector<SpanGirderHashType>::const_iterator it=girderList.begin(); it!=girderList.end(); it++)
      {
         SpanIndexType span;
         GirderIndexType gdr;
         UnhashSpanGirder(*it, &span, &gdr);

         arDesignOptions des_options = pSpecification->GetDesignOptions(span,gdr);
         des_options.doDesignSlabOffset = doDesignADim;

         if(!this->IsDesignFlexureEnabled())
         {
            des_options.doDesignForFlexure = dtNoDesign;
         }

         des_options.doDesignForShear = this->IsDesignShearEnabled();

         const pgsDesignArtifact* pArtifact = pIArtifact->CreateDesignArtifact( span, gdr, des_options);

         pProgress->Increment();

         if ( pArtifact == NULL )
         {
            AfxMessageBox(_T("Design Cancelled"),MB_OK);
            return;
         }
         
         pArtifacts.push_back(pArtifact);
      }
   }

   GET_IFACE(IReportManager,pReportMgr);
   CReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Design Outcome Report"));
   boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   boost::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

   boost::shared_ptr<CMultiGirderReportSpecification> pMGRptSpec = boost::dynamic_pointer_cast<CMultiGirderReportSpecification,CReportSpecification>(pRptSpec);

   pMGRptSpec->SetGirderList(girderList);

   CDesignOutcomeDlg dlg(pMGRptSpec);
   if ( dlg.DoModal() == IDOK )
   {
      // Create our transaction and execute
      GET_IFACE(IEAFTransactions,pTransactions);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);

      pgsTypes::SlabOffsetType slabOffsetType = pIBridgeDesc->GetSlabOffsetType();

      txnDesignGirder* pTxn = new txnDesignGirder(pArtifacts,slabOffsetType);

      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDoc::LoadMasterLibrary()
{
   WATCH(_T("Loading Master Library"));

   // Load the master library
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   CString strMasterLibaryFile = pPGSuper->GetCachedMasterLibraryFile();

   std::_tstring strPublisher = pPGSuper->GetMasterLibraryPublisher();
   std::_tstring strMasterLibFile = pPGSuper->GetMasterLibraryFile();

   m_LibMgr.SetMasterLibraryInfo(strPublisher.c_str(),strMasterLibFile.c_str());

   return DoLoadMasterLibrary(strMasterLibaryFile);
}

bool CPGSuperDoc::DoLoadMasterLibrary(const CString& strMasterLibraryFile)
{
   if ( strMasterLibraryFile.GetLength() == 0 )
      return true;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // loop until a library file is opened or user gives up trying to find it
   CString strFile = strMasterLibraryFile;
   CString err_msg;

   bool bSuccess = false;
   while(!bSuccess)
   {
      eafTypes::UnitMode unitMode;
      HRESULT hr = pgslibLoadLibrary(strFile,&m_LibMgr,&unitMode);
      if ( FAILED(hr) )
      {
         WATCH(_T("Failed to load master library"));
         AfxFormatString1(err_msg, IDS_CORRUPTED_LIBRARY_FILE, strFile);

         // if we are here, an error occured. Issue the message and give
         // the user a chance to load another library file

         if ( AfxMessageBox(err_msg,MB_YESNO|MB_ICONSTOP) == IDYES )
         {
            CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
            CComPtr<IEAFAppPlugin> pAppPlugin;
            pTemplate->GetPlugin(&pAppPlugin);
            CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

            pPGSuper->UpdateProgramSettings(TRUE);
            strFile = pPGSuper->GetCachedMasterLibraryFile();
         }
         else
         {
            bSuccess = true;
         }
      }
      else
      {
         bSuccess = true;
      }
   }

   // make all entries in master library read-only
   m_LibMgr.EnableEditingForAllEntries(false);

   return true; // the only way out alive!
}

void CPGSuperDoc::OnEditGirder() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSelectGirderDlg dlg(m_pBroker);
   dlg.m_Span   = m_Selection.SpanIdx   == ALL_SPANS   ? 0 : m_Selection.SpanIdx;
   dlg.m_Girder = m_Selection.GirderIdx == ALL_GIRDERS ? 0 : m_Selection.GirderIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditGirderDescription(dlg.m_Span,dlg.m_Girder,EGD_GENERAL);
   }
}

CSelection CPGSuperDoc::GetSelection()
{
   return m_Selection;
}

void CPGSuperDoc::SelectPier(PierIndexType pierIdx)
{
   if ( m_Selection.Type == CSelection::Pier && m_Selection.PierIdx == pierIdx )
      return; // the selection isn't changing

   m_Selection.Type      = CSelection::Pier;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.SpanIdx   = INVALID_INDEX;
   m_Selection.PierIdx   = pierIdx;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDoc::SelectSpan(SpanIndexType spanIdx)
{
   if ( m_Selection.Type == CSelection::Span && m_Selection.SpanIdx == spanIdx )
      return; // the selection isn't changing

   m_Selection.Type      = CSelection::Span;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.SpanIdx   = spanIdx;
   m_Selection.PierIdx   = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDoc::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   if ( m_Selection.Type == CSelection::Girder && m_Selection.SpanIdx == spanIdx && m_Selection.GirderIdx == gdrIdx )
      return; // the selection isn't changing

   m_Selection.Type      = CSelection::Girder;
   m_Selection.GirderIdx = gdrIdx;
   m_Selection.SpanIdx   = spanIdx;
   m_Selection.PierIdx   = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDoc::SelectDeck()
{
   if ( m_Selection.Type == CSelection::Deck )
      return;

   m_Selection.Type      = CSelection::Deck;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.SpanIdx   = INVALID_INDEX;
   m_Selection.PierIdx   = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDoc::SelectAlignment()
{
   if ( m_Selection.Type == CSelection::Alignment )
      return;

   m_Selection.Type      = CSelection::Alignment;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.SpanIdx   = INVALID_INDEX;
   m_Selection.PierIdx   = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDoc::ClearSelection()
{
   if ( m_Selection.Type == CSelection::None )
      return;

   m_Selection.Type      = CSelection::None;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.SpanIdx   = INVALID_INDEX;
   m_Selection.PierIdx   = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDoc::OnCopyGirderProps() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::OnImportProjectLibrary() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// ask user for file name
   CFileDialog  fildlg(TRUE,_T("pgs"),NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   _T("PGSuper Project File (*.pgs)|*.pgs||"));
   int stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      CString rPath;
      rPath = fildlg.GetPathName();

      GET_IFACE( IImportProjectLibrary, pImport );

      CString real_file_name; // name of actual file to be read may be different than lpszPathName
      HRESULT convert_status = ConvertTheDocument(rPath, &real_file_name);
      // convert document. if file was converted, then we need to delete the converted file at the end
      if ( FAILED(convert_status) )
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


         // advance the structured load pointer to the correct point for agent
         hr = pgslibPGSuperDocHeader(pStrLoad);
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError(hr,rPath);
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

      if (convert_status == S_OK)
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
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE(ILiveLoads,pLiveLoads);

   CLiveLoadDistFactorsDlg dlg;
   dlg.m_BridgeDesc = *pOldBridgeDesc;
   dlg.m_BridgeDesc.SetDistributionFactorMethod(method);
   dlg.m_LldfRangeOfApplicabilityAction = roaAction;
   dlg.m_pBroker = m_pBroker;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLLDF* pTxn = new txnEditLLDF(*pOldBridgeDesc,dlg.m_BridgeDesc,
                                          pLiveLoads->GetLldfRangeOfApplicabilityAction(),dlg.m_LldfRangeOfApplicabilityAction);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDoc::OnAddPointload() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CEditPointLoadDlg dlg(CPointLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

/*-------------------------------------------------------------------*/
void CPGSuperDoc::OnAddDistributedLoad() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CEditDistributedLoadDlg dlg(CDistributedLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
	
}

void CPGSuperDoc::OnAddMomentLoad() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CEditMomentLoadDlg dlg(CMomentLoadData(),m_pBroker);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDoc::OnConstructionLoads()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConstructionLoadDlg dlg;
   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 load = pLoads->GetConstructionLoad();
   dlg.m_Load = load;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditConstructionLoad* pTxn = new txnEditConstructionLoad(load,dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDoc::OnProjectAlignment() 
{
   EditAlignmentDescription(0);
}

void CPGSuperDoc::OnLiveLoads() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );
   GET_IFACE( ILiveLoads, pLiveLoad );

   std::vector<std::_tstring> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );

   std::vector<std::_tstring> design_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> fatigue_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> permit_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltPermit);

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
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

BOOL CPGSuperDoc::GetStatusBarMessageString(UINT nID,CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetStatusBarMessageString(nID,rMessage) )
      return TRUE;

   CPGSuperDoc* pThis = const_cast<CPGSuperDoc*>(this);
   
   CComPtr<IPGSuperDataExporter> exporter;
   pThis->m_PluginMgr.GetPGSuperExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      rMessage = OLE2T(bstr);
      rMessage.Replace('\n','\0');

      return TRUE;
   }
   
   CComPtr<IPGSuperDataImporter> importer;
   pThis->m_PluginMgr.GetPGSuperImporter(nID,false,&importer);
   if ( importer )
   {
      CComBSTR bstr;
      importer->GetCommandHintText(&bstr);
      rMessage = OLE2T(bstr);
      rMessage.Replace('\n','\0');

      return TRUE;
   }

   return FALSE;
}

BOOL CPGSuperDoc::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetToolTipMessageString(nID,rMessage) )
      return TRUE;

   CPGSuperDoc* pThis = const_cast<CPGSuperDoc*>(this);
   
   CComPtr<IPGSuperDataExporter> exporter;
   pThis->m_PluginMgr.GetPGSuperExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);

      return TRUE;
   }
   
   CComPtr<IPGSuperDataImporter> importer;
   pThis->m_PluginMgr.GetPGSuperImporter(nID,false,&importer);
   if ( importer )
   {
      CComBSTR bstr;
      importer->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);

      return TRUE;
   }

   return FALSE;
}

void CPGSuperDoc::CreateReportView(CollectionIndexType rptIdx,bool bPrompt)
{
   if ( !bPrompt )
   {
      // this is a quick report... make sure there is a current span and girder
      m_Selection.SpanIdx   = (m_Selection.SpanIdx   == INVALID_INDEX ? 0 : m_Selection.SpanIdx);
      m_Selection.GirderIdx = (m_Selection.GirderIdx == INVALID_INDEX ? 0 : m_Selection.GirderIdx);
   }

   m_pPGSuperDocProxyAgent->CreateReportView(rptIdx,bPrompt);

   // the base class does nothing so we won't bother calling it
}

void CPGSuperDoc::OnViewBridgeModelEditor()
{
   m_pPGSuperDocProxyAgent->CreateBridgeModelView();
}

void CPGSuperDoc::OnViewGirderEditor()
{
   SpanIndexType spanIdx  = m_Selection.SpanIdx;
   GirderIndexType gdrIdx = m_Selection.GirderIdx;
   m_pPGSuperDocProxyAgent->CreateGirderView(spanIdx,gdrIdx);
}

void CPGSuperDoc::OnViewAnalysisResults()
{
   SpanIndexType spanIdx  = m_Selection.SpanIdx;
   GirderIndexType gdrIdx = m_Selection.GirderIdx;
   m_pPGSuperDocProxyAgent->CreateAnalysisResultsView(spanIdx,gdrIdx);
}

void CPGSuperDoc::OnViewStability()
{
   SpanIndexType spanIdx  = m_Selection.SpanIdx;
   GirderIndexType gdrIdx = m_Selection.GirderIdx;
   m_pPGSuperDocProxyAgent->CreateStabilityView(spanIdx,gdrIdx);
}

void CPGSuperDoc::OnEditUserLoads()
{
   m_pPGSuperDocProxyAgent->CreateLoadsView();
}

void CPGSuperDoc::OnViewLibraryEditor()
{
   m_pPGSuperDocProxyAgent->CreateLibraryEditorView();
}

void CPGSuperDoc::OnProjectAnalysis() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CStructuralAnalysisMethodDlg dlg;
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType currAnalysisType = pSpec->GetAnalysisType();
   dlg.m_AnalysisType = currAnalysisType;
   if ( dlg.DoModal() == IDOK )
   {
      if ( currAnalysisType != dlg.m_AnalysisType )
      {
         txnEditAnalysisType* pTxn = new txnEditAnalysisType(currAnalysisType,dlg.m_AnalysisType);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSuperDoc::OnEditPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strItems;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CString strItem;
      if ( pierIdx == 0 || pierIdx == nPiers-1 )
         strItem.Format(_T("Abutment %d\n"),pierIdx+1);
      else
         strItem.Format(_T("Pier %d\n"),pierIdx+1);

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = _T("Select pier to edit");
   dlg.m_strItems = strItems;
   dlg.m_strLabel = _T("Select pier to edit");
   dlg.m_ItemIdx = m_Selection.PierIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditPierDescription(dlg.m_ItemIdx,EPD_GENERAL);
   }
}

void CPGSuperDoc::OnEditSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CString strItems;
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CString strItem;
      strItem.Format(_T("Span %d\n"),LABEL_SPAN(spanIdx));

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = _T("Select span to edit");
   dlg.m_strItems = strItems;
   dlg.m_strLabel = _T("Select span to edit");
   dlg.m_ItemIdx = m_Selection.SpanIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditSpanDescription(dlg.m_ItemIdx,ESD_GENERAL);
   }
}

void CPGSuperDoc::DeletePier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a pier

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strTitle;
   strTitle.Format(_T("Deleting %s %d"),(pierIdx == 0 || pierIdx == nPiers-1 ? _T("Abutment") : _T("Pier")),pierIdx+1);
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format(_T("%s. Select the span to be deleted with the pier"),strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( pierIdx == 0 )
      strItems.Format(_T("%s"),_T("Span 1\n"));
   else if ( pierIdx == nPiers-1)
      strItems.Format(_T("Span %d\n"),LABEL_SPAN(pierIdx-1));
   else
      strItems.Format(_T("Span %d\nSpan %d\n"),LABEL_SPAN(pierIdx-1),LABEL_SPAN(pierIdx));

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
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a span

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CString strTitle;
   strTitle.Format(_T("Deleting Span %d"),LABEL_SPAN(spanIdx));
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format(_T("%s. Select the pier to be deleted with the span"),strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( spanIdx == 0 )
      strItems.Format(_T("%s"),_T("Abutment 1\nPier 2\n"));
   else if ( spanIdx == nSpans-1)
      strItems.Format(_T("Pier %d\nAbutment %d\n"),LABEL_PIER(nSpans-1),LABEL_PIER(nSpans));
   else
      strItems.Format(_T("Pier %d\nPier %d\n"),LABEL_PIER(spanIdx),LABEL_PIER(spanIdx+1));

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      DeleteSpan(spanIdx,dlg.m_ItemIdx == 0 ? pgsTypes::PrevPier : pgsTypes::NextPier );
   }
}

void CPGSuperDoc::OnDeleteSelection() 
{
   if (  m_Selection.PierIdx != ALL_PIERS )
   {
      DeletePier(m_Selection.PierIdx);
   }
   else if ( m_Selection.SpanIdx != ALL_SPANS )
   {
      DeleteSpan(m_Selection.SpanIdx);
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

   if ( m_Selection.PierIdx != ALL_PIERS )
   {
      long nPiers = pBridgeDesc->GetPierCount();
      CString strLabel;
      if ( m_Selection.PierIdx == 0 || m_Selection.PierIdx == nPiers-1 )
         strLabel.Format(_T("Delete Abutment %d"),m_Selection.PierIdx+1);
      else
         strLabel.Format(_T("Delete Pier %d"),m_Selection.PierIdx+1);

      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else if ( m_Selection.SpanIdx != ALL_SPANS  )
   {
      if ( m_Selection.GirderIdx != ALL_GIRDERS  )
      {
         // girder is selected.. can't delete an individual girder
         pCmdUI->Enable(FALSE);
      }
      else
      {
         // only span is selected
         CString strLabel;
         strLabel.Format(_T("Delete Span %d"),LABEL_SPAN(m_Selection.SpanIdx));
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
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType)
{
   PierIndexType pierIdx = (pierRemoveType == pgsTypes::PrevPier ? spanIdx : spanIdx+1);
   pgsTypes::PierFaceType pierFace = (pierRemoveType == pgsTypes::PrevPier ? pgsTypes::Ahead : pgsTypes::Back);
   DeletePier(pierIdx,pierFace);
}

void CPGSuperDoc::OnInsert() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
         strPier = _T("Abutment");
      else 
         strPier = _T("Pier");

      strItem.Format(_T("Before %s %d\n"),strPier,pierIdx+1);
      strItems += strItem;
      keys.push_back( std::make_pair(pierIdx,pgsTypes::Back) );

      strItem.Format(_T("After %s %d\n"),strPier,pierIdx+1);
      strItems += strItem;
      keys.push_back( std::make_pair(pierIdx,pgsTypes::Ahead) );
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = _T("Insert Span");
   dlg.m_strItems = strItems;
   dlg.m_strLabel = _T("Select location to insert span");
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
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDoc::OnOptionsHints() 
{
   CString strText;
   strText = _T("Reset all user interface hints");
   int result = AfxMessageBox(strText,MB_YESNO);
   if ( result == IDYES )
   {
      UINT hintSettings = GetUIHintSettings();
      hintSettings = UIHINT_ENABLE_ALL;
      SetUIHintSettings(hintSettings);
   }
}

void CPGSuperDoc::OnOptionsLabels() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
    // document classes can't process ON_NOTIFY
    // see http://www.codeproject.com/KB/docview/NotifierApp.aspx for details
    if ( HIWORD(nCode) == WM_NOTIFY )
    {
       // verify that this is a WM_NOTIFY message
        WORD wCode = LOWORD(nCode) ;

        AFX_NOTIFY * notify = reinterpret_cast<AFX_NOTIFY*>(pExtra) ;
        if ( notify->pNMHDR->code == TBN_DROPDOWN )
        {
           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() )
              return OnViewReports(notify->pNMHDR,notify->pResult); 

           return TRUE; // message was handled
        }
    }
	
	return CEAFBrokerDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPGSuperDoc::PopulateReportMenu()
{
   CEAFMenu* pMainMenu = GetMainMenu();

   UINT viewPos = pMainMenu->FindMenuItem(_T("&View"));
   ASSERT( 0 <= viewPos );

   CEAFMenu* pViewMenu = pMainMenu->GetSubMenu(viewPos);
   ASSERT( pViewMenu != NULL );

   UINT reportsPos = pViewMenu->FindMenuItem(_T("&Reports"));
   ASSERT( 0 <= reportsPos );

   // Get the reports menu
   CEAFMenu* pReportsMenu = pViewMenu->GetSubMenu(reportsPos);
   ASSERT(pReportsMenu != NULL);

   CEAFBrokerDocument::PopulateReportMenu(pReportsMenu);
}

void CPGSuperDoc::LoadDocumentSettings()
{
   CEAFBrokerDocument::LoadDocumentSettings();

   CEAFApp* pApp = EAFGetApp();
   CString strAutoCalcDefault = pApp->GetLocalMachineString(_T("Settings"),_T("AutoCalc"), _T("On"));
   CString strAutoCalc = pApp->GetProfileString(_T("Settings"),_T("AutoCalc"),strAutoCalcDefault);
   if ( strAutoCalc.CompareNoCase(_T("Off")) == 0 )
      m_bAutoCalcEnabled = false;
   else
      m_bAutoCalcEnabled = true;

   // bridge model editor settings
   // turn on all settings for default
   UINT def_bm = IDB_PV_LABEL_PIERS     |
                 IDB_PV_LABEL_ALIGNMENT |
                 IDB_PV_LABEL_GIRDERS   |
                 IDB_PV_LABEL_BEARINGS  |
                 IDB_PV_LABEL_TICKMARKS |
                 IDB_PV_SHOW_TICKMARKS  |
                 IDB_PV_DRAW_ISOTROPIC  |
                 IDB_CS_LABEL_GIRDERS   |
                 IDB_CS_SHOW_DIMENSIONS |
                 IDB_CS_DRAW_ISOTROPIC;

   m_BridgeModelEditorSettings = pApp->GetProfileInt(_T("Settings"),_T("BridgeEditor"),def_bm);

   m_GirderModelEditorSettings = pApp->GetProfileInt(_T("Settings"),_T("GirderEditor"),def_bm);

   m_UIHintSettings = pApp->GetProfileInt(_T("Settings"),_T("UIHints"),0); // default, all hints enabled

   CString strDefaultGirderLabelFormat = pApp->GetLocalMachineString(_T("Settings"),_T("GirderLabelFormat"),     _T("Alpha"));
   CString strGirderLabelFormat = pApp->GetProfileString(_T("Settings"),_T("GirderLabelFormat"),strDefaultGirderLabelFormat);
   if ( strGirderLabelFormat.CompareNoCase(_T("Alpha")) == 0 )
      pgsGirderLabel::UseAlphaLabel(true);
   else
      pgsGirderLabel::UseAlphaLabel(false);


   // Flexure and stirrup design defaults for design dialog.
   // Default is to design flexure and not shear.
   // If the string is not Off, then assume it is on.
   CString strDefaultDesignFlexure = pApp->GetLocalMachineString(_T("Settings"),_T("DesignFlexure"),_T("On"));
   CString strDesignFlexure = pApp->GetProfileString(_T("Settings"),_T("DesignFlexure"),strDefaultDesignFlexure);
   if ( strDesignFlexure.CompareNoCase(_T("Off")) == 0 )
      m_bDesignFlexureEnabled = false;
   else
      m_bDesignFlexureEnabled = true;

   CString strDefaultDesignShear = pApp->GetLocalMachineString(_T("Settings"),_T("DesignShear"),_T("Off"));
   CString strDesignShear = pApp->GetProfileString(_T("Settings"),_T("DesignShear"),strDefaultDesignShear);
   if ( strDesignShear.CompareNoCase(_T("Off")) == 0 )
      m_bDesignShearEnabled = false;
   else
      m_bDesignShearEnabled = true;

   CString strShowProjectProperties = pApp->GetLocalMachineString(_T("Settings"),_T("ShowProjectProperties"), _T("On"));
   CString strProjectProperties = pApp->GetProfileString(_T("Settings"),_T("ShowProjectProperties"),strShowProjectProperties);
   if ( strProjectProperties.CompareNoCase(_T("Off")) == 0 )
      m_bShowProjectProperties = false;
   else
      m_bShowProjectProperties = true;


   CString strDefaultReportCoverImage = pApp->GetLocalMachineString(_T("Settings"),_T("ReportCoverImage"),_T(""));
   pgsReportStyleHolder::SetReportCoverImage(pApp->GetProfileString(_T("Settings"),_T("ReportCoverImage"),strDefaultReportCoverImage));
}

void CPGSuperDoc::SaveDocumentSettings()
{
   CEAFBrokerDocument::SaveDocumentSettings();

   CEAFApp* pApp = EAFGetApp();

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("AutoCalc"),m_bAutoCalcEnabled ? _T("On") : _T("Off") ));

   // bridge editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("BridgeEditor"),m_BridgeModelEditorSettings));

   // girder editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("GirderEditor"),m_GirderModelEditorSettings));

   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("UIHints"),m_UIHintSettings));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("GirderLabelFormat"),pgsGirderLabel::UseAlphaLabel() ? _T("Alpha") : _T("Numeric") ));

   // Save the design mode settings
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("DesignFlexure"),m_bDesignFlexureEnabled ? _T("On") : _T("Off") ));
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("DesignShear"),  m_bDesignShearEnabled   ? _T("On") : _T("Off") ));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("ShowProjectProperties"),m_bShowProjectProperties ? _T("On") : _T("Off") ));
}

void CPGSuperDoc::OnLogFileOpened()
{
   CEAFBrokerDocument::OnLogFileOpened();

   GET_IFACE(IEAFProjectLog,pLog);
   CString strMsg;
   strMsg.Format(_T("PGSuper version %s"),theApp.GetVersion(false).GetBuffer(100));
   pLog->LogMessage(strMsg);
}

void CPGSuperDoc::BrokerShutDown()
{
   CEAFBrokerDocument::BrokerShutDown();

   m_pPGSuperDocProxyAgent = NULL;
}

void CPGSuperDoc::OnStatusChanged()
{
   CEAFBrokerDocument::OnStatusChanged();
   if ( m_pPGSuperDocProxyAgent )
      m_pPGSuperDocProxyAgent->OnStatusChanged();
}

CString CPGSuperDoc::GetToolbarSectionName()
{
   return CString(_T("PGSuper Toolbars"));
}

void CPGSuperDoc::OnUpdateViewReports(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}

BOOL CPGSuperDoc::OnViewReports(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_REPORTS )
      return FALSE; // not our button

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_REPORTS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());


   BuildReportMenu(&contextMenu,false);

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_VIEW_REPORTS,NULL);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
}


void CPGSuperDoc::OnImportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
      return;

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   Uint32 nImporters = m_PluginMgr.GetImporterCount();
   if ( nImporters == 0 )
   {
      pCmdUI->SetText(_T("Custom importers not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      Uint32 idx;
      // clean up the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+idx,MF_BYCOMMAND);
      }

      // populate the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         CComPtr<IPGSuperDataImporter> importer;
         m_PluginMgr.GetPGSuperImporter(idx,true,&importer);

         UINT cmdID = m_PluginMgr.GetPGSuperImporterCommand(idx);

         CComBSTR bstrMenuText;
         importer->GetMenuText(&bstrMenuText);
         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_PluginMgr.GetPGSuperImporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,NULL);

   	   pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();

         pCmdUI->m_nIndex++;
      }
   }

   pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperDoc::OnExportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
      return;

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   Uint32 nExporters = m_PluginMgr.GetExporterCount();
   if ( nExporters == 0 )
   {
      pCmdUI->SetText(_T("Custom exporters not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      Uint32 idx;
      for ( idx = 0; idx < nExporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+idx,MF_BYCOMMAND);
      }

      for ( idx = 0; idx < nExporters; idx++ )
      {
         CComPtr<IPGSuperDataExporter> exporter;
         m_PluginMgr.GetPGSuperExporter(idx,true,&exporter);

         UINT cmdID = m_PluginMgr.GetPGSuperExporterCommand(idx);

         CComBSTR bstrMenuText;
         exporter->GetMenuText(&bstrMenuText);

         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_PluginMgr.GetPGSuperExporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,NULL);

         pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();
         pCmdUI->m_nIndex++;
      }
   }

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperDoc::OnImport(UINT nID)
{
   CComPtr<IPGSuperDataImporter> importer;
   m_PluginMgr.GetPGSuperImporter(nID,false,&importer);

   if ( importer )
   {
      importer->Import(m_pBroker);
   }
}

void CPGSuperDoc::OnExport(UINT nID)
{
   CComPtr<IPGSuperDataExporter> exporter;
   m_PluginMgr.GetPGSuperExporter(nID,false,&exporter);

   if ( exporter )
   {
      exporter->Export(m_pBroker);
   }
}

UINT CPGSuperDoc::GetBridgeEditorSettings() const
{
   return m_BridgeModelEditorSettings;
}

void CPGSuperDoc::SetBridgeEditorSettings(UINT settings)
{
   m_BridgeModelEditorSettings = settings;
}

UINT CPGSuperDoc::GetGirderEditorSettings() const
{
   return m_GirderModelEditorSettings;
}

void CPGSuperDoc::SetGirderEditorSettings(UINT settings)
{
   m_GirderModelEditorSettings = settings;
}

UINT CPGSuperDoc::GetUIHintSettings() const
{
   return m_UIHintSettings;
}

void CPGSuperDoc::SetUIHintSettings(UINT settings)
{
   m_UIHintSettings = settings;
}

bool CPGSuperDoc::IsDesignFlexureEnabled() const
{
   return m_bDesignFlexureEnabled;
}

void CPGSuperDoc::EnableDesignFlexure( bool bEnable )
{
   m_bDesignFlexureEnabled = bEnable;
}

bool CPGSuperDoc::IsDesignShearEnabled() const
{
   return m_bDesignShearEnabled;
}

void CPGSuperDoc::EnableDesignShear( bool bEnable )
{
   m_bDesignShearEnabled = bEnable;
}

bool CPGSuperDoc::ShowProjectPropertiesOnNewProject()
{
   return m_bShowProjectProperties;
}

void CPGSuperDoc::ShowProjectPropertiesOnNewProject(bool bShow)
{
   m_bShowProjectProperties = bShow;
}
