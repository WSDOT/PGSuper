///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// PGSuperDocBase.cpp : implementation of the CPGSuperDocBase class
//
#include "PGSuperAppPlugin\stdafx.h"

#include "PGSuperAppPlugin\PGSuperApp.h"

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <objbase.h>
#include <initguid.h>

#include "PGSuperDocBase.h"
#include "PGSuperUnits.h"
#include "PGSuperBaseAppPlugin.h"

#include "BridgeSectionView.h"
#include "BridgePlanView.h"

#include <WBFLCore_i.c>
#include <WBFLReportManagerAgent_i.c>
#include <WBFLGraphManagerAgent_i.c>
#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>
#include <WBFLDManip_i.c>
#include <WBFLDManipTools_i.c>

#include <PGSuperAppPlugin.h>
#include <PGSuperProjectImporterAppPlugin.h>

#include <PsgLib\PsgLib.h>
#include <PsgLib\BeamFamilyManager.h>

#include <IFace\Test1250.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Artifact.h>
#include <IFace\TxDOTCadExport.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>
#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Allowables.h>
#include <IFace\StatusCenter.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Intervals.h>
#include <EAF\EAFUIIntegration.h>
#include "PGSuperDocProxyAgent.h"

#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>


#include "SupportDrawStrategy.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategy.h"
#include "DistributedLoadDrawStrategy.h"
#include "MomentLoadDrawStrategy.h"
#include "GevEditLoad.h"


#include "HtmlHelp\HelpTopics.hh"

#include "Convert.h"

#include <ComCat.h>

#include "BridgeLinkCATID.h"

#include "Hints.h"
#include "UIHintsDlg.h"

#include "PGSuperException.h"
#include <System\FileStream.h>
#include <System\StructuredLoadXmlPrs.h>

// Helpers
#include <MathEx.h>

// Agents
#include "PGSuperDocProxyAgent.h"

// Dialogs
#include "PGSuperAppPlugin\AboutDlg.h"
#include "ProjectPropertiesDlg.h"
#include "EnvironmentDlg.h"
#include "BridgeDescDlg.h"
#include "SpecDlg.h"
#include "BridgeEditorSettingsSheet.h"
#include "GirderEditorSettingsSheet.h"
#include "CopyGirderDlg.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LiveLoadSelectDlg.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include "AlignmentDescriptionDlg.h"
#include "SelectItemDlg.h"
#include "SpanDetailsDlg.h"
#include "PierDetailsDlg.h"
#include "GirderLabelFormatDlg.h"
#include "RatingOptionsDlg.h"
#include "ConstructionLoadDlg.h"
#include "GirderSelectStrandsDlg.h"
#include "PGSuperAppPlugin\InsertSpanDlg.h"
#include "PGSuperAppPlugin\LoadFactorsDlg.h"
#include "PGSuperAppPlugin\LossParametersDlg.h"

#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>

#include <PgsExt\ReportStyleHolder.h>

// Transactions
#include <PgsExt\EditBridge.h>
#include "EditAlignment.h"
#include "EditPier.h"
#include "EditSpan.h"
#include "InsertDeleteSpan.h"
#include "EditLLDF.h"
#include "EditEnvironment.h"
#include "EditProjectCriteria.h"
#include "EditRatingCriteria.h"
#include "EditLiveLoad.h"
#include "EditConstructionLoad.h"
#include <PgsExt\InsertDeleteLoad.h>
#include "EditEffectiveFlangeWidth.h"
#include "EditLoadFactors.h"
#include "EditLoadModifiers.h"
#include "PGSuperAppPlugin\EditLossParameters.h"
#include "PGSuperAppPlugin\EditPrecastSegment.h"
#include "EditProjectProperties.h"

// Logging
#include <iostream>
#include <fstream>
#include <System\Time.h>

#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\MacroTxn.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define PGSUPER_PLUGIN_COMMAND_COUNT 256

static const Float64 FILE_VERSION = 3.0;

static bool DoesFolderExist(const CString& dirname);
static bool DoesFileExist(const CString& filname);

#pragma Reminder("UPDATE: UpdatePrestressForce should be part of the strand editing dialogs")
// Function to update prestress force after editing strands
static void UpdatePrestressForce(pgsTypes::StrandType type, const CSegmentKey& segmentKey,
                                 CPrecastSegmentData& newSegmentData,const CPrecastSegmentData& oldSegmentData, 
                                 IPretensionForce* pPrestress)
{

      // If going from no strands - always compute pjack automatically
      if(newSegmentData.Strands.IsPjackCalculated(type) ||
         (0 == oldSegmentData.Strands.GetStrandCount(type) &&
          0 < newSegmentData.Strands.GetStrandCount(type)))
      {
         newSegmentData.Strands.IsPjackCalculated(type,true);
         newSegmentData.Strands.SetPjack(type, pPrestress->GetPjackMax(segmentKey, 
                                                                 *(newSegmentData.Strands.GetStrandMaterial(type)),
                                                                 newSegmentData.Strands.GetStrandCount(type)));
      }
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDocBase

BEGIN_MESSAGE_MAP(CPGSuperDocBase, CEAFBrokerDocument)
	//{{AFX_MSG_MAP(CPGSuperDocBase)
	ON_COMMAND(ID_FILE_PROJECT_PROPERTIES, OnFileProjectProperties)
	ON_COMMAND(ID_PROJECT_ENVIRONMENT, OnProjectEnvironment)
   ON_COMMAND(ID_PROJECT_EFFECTIVEFLANGEWIDTH, OnEffectiveFlangeWidth)
	ON_COMMAND(ID_PROJECT_SPEC, OnProjectSpec)
	ON_COMMAND(ID_RATING_SPEC,  OnRatingSpec)
	ON_COMMAND(ID_PROJECT_AUTOCALC, OnProjectAutoCalc)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_AUTOCALC, OnUpdateProjectAutoCalc)
	ON_COMMAND(IDM_EXPORT_TEMPLATE, OnExportToTemplateFile)
	ON_COMMAND(ID_VIEWSETTINGS_BRIDGEMODELEDITOR, OnViewsettingsBridgemodelEditor)
	ON_COMMAND(ID_LOADS_LOADMODIFIERS, OnLoadsLoadModifiers)
   ON_COMMAND(ID_LOADS_LOADFACTORS, OnLoadsLoadFactors)
	ON_COMMAND(ID_VIEWSETTINGS_GIRDEREDITOR, OnViewsettingsGirderEditor)
	ON_COMMAND(IDM_COPY_GIRDER_PROPS, OnCopyGirderProps)
	ON_COMMAND(IDM_IMPORT_PROJECT_LIBRARY, OnImportProjectLibrary)
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMomentLoad)
   ON_COMMAND(ID_CONSTRUCTION_LOADS,OnConstructionLoads)
	ON_COMMAND(ID_PROJECT_ALIGNMENT, OnProjectAlignment)
	ON_COMMAND(ID_PROJECT_PIERDESC, OnEditPier)
	ON_COMMAND(ID_PROJECT_SPANDESC, OnEditSpan)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
	ON_COMMAND(ID_LOADS_LLDF, OnLoadsLldf)
   ON_COMMAND(ID_LIVE_LOADS,OnLiveLoads)
	ON_COMMAND(ID_INSERT, OnInsert)
	ON_COMMAND(ID_OPTIONS_HINTS, OnOptionsHints)
	ON_COMMAND(ID_OPTIONS_LABELS, OnOptionsLabels)
   ON_COMMAND(ID_PROJECT_LOSSES,OnLosses)

   ON_COMMAND(ID_VIEW_BRIDGEMODELEDITOR, OnViewBridgeModelEditor)
   ON_COMMAND(ID_VIEW_GIRDEREDITOR, OnViewGirderEditor)
   ON_COMMAND(ID_VIEW_LIBRARYEDITOR, OnViewLibraryEditor)

	ON_COMMAND(ID_EDIT_USERLOADS, OnEditUserLoads)
   //}}AFX_MSG_MAP
	
   ON_COMMAND(ID_PROJECT_UPDATENOW, OnUpdateNow)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_UPDATENOW, OnUpdateUpdateNow)

	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)


   ON_UPDATE_COMMAND_UI_RANGE(EAFID_VIEW_STATUSCENTER, EAFID_VIEW_STATUSCENTER3, CEAFBrokerDocument::OnUpdateViewStatusCenter)
	ON_COMMAND_RANGE(EAFID_VIEW_STATUSCENTER, EAFID_VIEW_STATUSCENTER3,OnViewStatusCenter)

   ON_UPDATE_COMMAND_UI(ID_VIEW_REPORTS,OnUpdateViewReports)
   ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHS,OnUpdateViewGraphs)

	ON_UPDATE_COMMAND_UI(FIRST_DATA_IMPORTER_PLUGIN, OnImportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_IMPORTER_PLUGIN,LAST_DATA_IMPORTER_PLUGIN, OnImport)
	ON_UPDATE_COMMAND_UI(FIRST_DATA_EXPORTER_PLUGIN, OnExportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_EXPORTER_PLUGIN,LAST_DATA_EXPORTER_PLUGIN, OnExport)

   ON_COMMAND(ID_HELP_ABOUT, OnAbout)

   // this doesn't work for documents... see OnCmdMsg for handling of WM_NOTIFY
   //ON_NOTIFY(TBN_DROPDOWN,ID_STDTOOLBAR,OnViewReports)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDocBase construction/destruction

CPGSuperDocBase::CPGSuperDocBase():
m_bDesignSlabOffset(true),
m_bAutoCalcEnabled(true)
{
	EnableAutomation();

	AfxOleLockApp();

   m_Selection.Type       = CSelection::None;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   m_LibMgr.SetName( _T("PGSuper Library") );

   CEAFAutoCalcDocMixin::SetDocument(this);

   m_bShowProjectProperties = true;

   m_pPGSuperDocProxyAgent = NULL;

   m_CallbackID = 0;

   // Reserve a range of command IDs for extension agent commands (which are current supported)
   // and EAFDocumentPlugin objects (which are not currently supported in PGSuper)
   GetPluginCommandManager()->ReserveCommandIDRange(PGSUPER_PLUGIN_COMMAND_COUNT);
}

CPGSuperDocBase::~CPGSuperDocBase()
{
   m_DocUnitSystem.Release();
   m_pPluginMgr->UnloadPlugins();
   delete m_pPluginMgr;
   m_pPluginMgr = NULL;
   AfxOleUnlockApp();
}

// CEAFAutoCalcDocMixin overrides
bool CPGSuperDocBase::IsAutoCalcEnabled() const
{
   return m_bAutoCalcEnabled;
}

void CPGSuperDocBase::EnableAutoCalc(bool bEnable)
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
      {
        OnUpdateNow();
      }
   }
}

void CPGSuperDocBase::OnUpdateNow()
{
   CEAFAutoCalcDocMixin::OnUpdateNow();
}

void CPGSuperDocBase::OnUpdateUpdateNow(CCmdUI* pCmdUI)
{
   CEAFAutoCalcDocMixin::OnUpdateUpdateNow(pCmdUI);
}

void CPGSuperDocBase::OnViewStatusCenter(UINT nID)
{
   CEAFBrokerDocument::OnViewStatusCenter();
}

void CPGSuperDocBase::OnLibMgrChanged(psgLibraryManager* pNewLibMgr)
{
   GET_IFACE( ILibrary, pLib );
   pLib->SetLibraryManager( pNewLibMgr );
}

// libISupportLibraryManager implementation
CollectionIndexType CPGSuperDocBase::GetNumberOfLibraryManagers() const
{
   return 1;
}

libLibraryManager* CPGSuperDocBase::GetLibraryManager(CollectionIndexType num)
{
   PRECONDITION( num == 0 );
   return &m_LibMgr;
}

libLibraryManager* CPGSuperDocBase::GetTargetLibraryManager()
{
   return &m_LibMgr;
}

void CPGSuperDocBase::GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem)
{
   (*ppDocUnitSystem) = m_DocUnitSystem;
   (*ppDocUnitSystem)->AddRef();
}

void CPGSuperDocBase::EditAlignmentDescription(int nPage)
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

void CPGSuperDocBase::EditBridgeDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IEnvironment, pEnvironment );

   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
   Float64 oldRelHumidity = pEnvironment->GetRelHumidity();

   CBridgeDescDlg dlg(*pOldBridgeDesc);

   dlg.m_EnvironmentalPage.m_Exposure    = oldExposureCondition == expNormal ? 0 : 1;
   dlg.m_EnvironmentalPage.m_RelHumidity = oldRelHumidity;

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {

      txnTransaction* pTxn = new txnEditBridge(*pOldBridgeDesc,      dlg.GetBridgeDescription(),
                                              oldExposureCondition, dlg.m_EnvironmentalPage.m_Exposure == 0 ? expNormal : expSevere,
                                              oldRelHumidity,       dlg.m_EnvironmentalPage.m_RelHumidity);


      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDocBase::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CPierDetailsDlg dlg(pBridgeDesc,pierIdx);
   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnEditPier(pierIdx,*pBridgeDesc,*dlg.GetBridgeDescription());
      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn != NULL )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDocBase::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CSpanDetailsDlg dlg(pBridgeDesc,spanIdx);
   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnEditSpan(spanIdx,*pBridgeDesc,*dlg.GetBridgeDescription());

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSuperDocBase::EditDirectInputPrestressing(const CSegmentKey& segmentKey)
{
#pragma Reminder("UPDATE: move this to the CPGSuperDoc class... it doesn't belong in the common base class")
   // it doesn't apply to PGSplice
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc  = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager*    pTimelineMgr = pBridgeDesc->GetTimelineManager();
   const CPrecastSegmentData* pSegment     = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   txnEditPrecastSegmentData oldSegmentData;
   oldSegmentData.m_SegmentKey           = segmentKey;
   oldSegmentData.m_SegmentData          = *pSegment;
   oldSegmentData.m_ConstructionEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(pSegment->GetID());
   oldSegmentData.m_ErectionEventIdx     = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());

   if (pSegment->Strands.GetStrandDefinitionType() != CStrandData::npsDirectSelection )
   {
      // We can go no further
      ::AfxMessageBox(_T("Programmer Error: EditDirectInputPrestressing - can only be called for Direct Select strand fill"),MB_OK | MB_ICONWARNING);
      return false;
   }

   std::_tstring strGirderName = pSegment->GetGirder()->GetGirderName();
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName.c_str());

   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // Get current offset input values - dialog will force in bounds if needed
   HarpedStrandOffsetType endMeasureType = pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd();
   HarpedStrandOffsetType hpMeasureType  = pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint();

   Float64 hpOffsetAtEnd = pSegment->Strands.GetHarpStrandOffsetAtEnd();
   Float64 hpOffsetAtHp  = pSegment->Strands.GetHarpStrandOffsetAtHarpPoint();

   bool allowEndAdjustment = pGdrEntry->IsVerticalAdjustmentAllowedEnd();
   bool allowHpAdjustment  = pGdrEntry->IsVerticalAdjustmentAllowedHP();

   // Max debond length is 1/2 girder length
   Float64 maxDebondLength = pBridge->GetSegmentLength(segmentKey)/2.0;

   // Fire up dialog
   txnEditPrecastSegmentData newSegmentData = oldSegmentData;

   CGirderSelectStrandsDlg dlg;
#pragma Reminder("UPDATE: clean up this initialization... the same code is in multiple locations")
   // this initialization is here, in BridgeDescPrestressPage.cpp and in GirderSegmentDlg.cpp
   // make the page more self-sufficient
   dlg.m_SelectStrandsPage.InitializeData(segmentKey, &newSegmentData.m_SegmentData.Strands, pSpecEntry, pGdrEntry,
                      allowEndAdjustment, allowHpAdjustment, endMeasureType, hpMeasureType, hpOffsetAtEnd, hpOffsetAtHp, maxDebondLength);

   if ( dlg.DoModal() == IDOK )
   {
      // The dialog does not deal with Pjack. Update pjack here
#pragma Reminder("UPDATE: dialog should deal with Pjack")
      GET_IFACE(IPretensionForce, pPrestress );

      UpdatePrestressForce(pgsTypes::Straight,  segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Harped,    segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Temporary, segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);

      // Fire our transaction
      txnEditPrecastSegment* pTxn = new txnEditPrecastSegment(segmentKey,newSegmentData);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);

      return true;
   }
   else
   {
     return true;
   }
}

   
void CPGSuperDocBase::AddPointLoad(const CPointLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditPointLoadDlg dlg(loadData);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDocBase::EditPointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData* pLoadData = pUserDefinedLoads->GetPointLoad(loadIdx);

   CEditPointLoadDlg dlg(*pLoadData);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditPointLoad* pTxn = new txnEditPointLoad(loadIdx,*pLoadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDocBase::DeletePointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeletePointLoad* pTxn = new txnDeletePointLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDocBase::AddDistributedLoad(const CDistributedLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditDistributedLoadDlg dlg(loadData);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDocBase::EditDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData* pLoadData = pUserDefinedLoads->GetDistributedLoad(loadIdx);

   CEditDistributedLoadDlg dlg(*pLoadData);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditDistributedLoad* pTxn = new txnEditDistributedLoad(loadIdx,*pLoadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDocBase::DeleteDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeleteDistributedLoad* pTxn = new txnDeleteDistributedLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDocBase::AddMomentLoad(const CMomentLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEditMomentLoadDlg dlg(loadData);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSuperDocBase::EditMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData* pLoadData = pUserDefinedLoads->GetMomentLoad(loadIdx);

   CEditMomentLoadDlg dlg(*pLoadData);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditMomentLoad* pTxn = new txnEditMomentLoad(loadIdx,*pLoadData,dlg.m_Load);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSuperDocBase::DeleteMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   txnDeleteMomentLoad* pTxn = new txnDeleteMomentLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDocBase::EditGirderViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetGirderEditorSettings();

	CGirderEditorSettingsSheet dlg(IDS_GM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      SetGirderEditorSettings(settings);

      // tell the world we've changed settings
      UpdateAllViews( 0, HINT_GIRDERVIEWSETTINGSCHANGED, 0 );
   }
}

void CPGSuperDocBase::EditBridgeViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetBridgeEditorSettings();

	CBridgeEditorSettingsSheet dlg(IDS_BM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      settings |= IDB_PV_DRAW_ISOTROPIC;
      SetBridgeEditorSettings(settings);
   }

   // tell the world we've changed settings
   UpdateAllViews( 0, HINT_BRIDGEVIEWSETTINGSCHANGED, 0 );
	
}

BOOL CPGSuperDocBase::UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir)
{
   CFileFind dir_finder;
   BOOL bMoreDir = dir_finder.FindFile(CString(lpszDir)+_T("\\*"));

   // recursively go through the directories
   while ( bMoreDir )
   {
      bMoreDir = dir_finder.FindNextFile();
      CString strDir = dir_finder.GetFilePath();

      if ( !dir_finder.IsDots() && dir_finder.IsDirectory() )
      {
         UpdateTemplates(pProgress,strDir);
      }
   }

   // done with the directories below this leave. Process the templates at this level
   CString strMessage;
   strMessage.Format(_T("Updating templates in %s"),lpszDir);
   pProgress->UpdateMessage(strMessage);

   CFileFind template_finder;
   BOOL bMoreTemplates = template_finder.FindFile(CString(lpszDir) + _T("\\*") + CString(GetTemplateExtension()));
   while ( bMoreTemplates )
   {
      bMoreTemplates      = template_finder.FindNextFile();
      CString strTemplate = template_finder.GetFilePath();

      strMessage.Format(_T("Updating %s"),template_finder.GetFileTitle());
      pProgress->UpdateMessage(strMessage);

      if ( !OpenTheDocument(strTemplate) )
      {
         return FALSE;
      }

      CEAFBrokerDocument::SaveTheDocument(strTemplate);

      m_pBroker->Reset();

      CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pInit(m_pBroker);
      pInit->InitAgents();
   }

   return TRUE;
}

BOOL CPGSuperDocBase::UpdateTemplates()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);

   if  ( !Init() ) // load the agents and other necessary stuff
   {
      return FALSE;
   }

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

IDType CPGSuperDocBase::RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_BridgePlanViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterBridgePlanViewCallback(IDType ID)
{
   std::map<IDType,IBridgePlanViewEventCallback*>::iterator found = m_BridgePlanViewCallbacks.find(ID);
   if ( found == m_BridgePlanViewCallbacks.end() )
   {
      return false;
   }

   m_BridgePlanViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IBridgePlanViewEventCallback*>& CPGSuperDocBase::GetBridgePlanViewCallbacks()
{
   return m_BridgePlanViewCallbacks;
}

IDType CPGSuperDocBase::RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_BridgeSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterBridgeSectionViewCallback(IDType ID)
{
   std::map<IDType,IBridgeSectionViewEventCallback*>::iterator found = m_BridgeSectionViewCallbacks.find(ID);
   if ( found == m_BridgeSectionViewCallbacks.end() )
   {
      return false;
   }

   m_BridgeSectionViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IBridgeSectionViewEventCallback*>& CPGSuperDocBase::GetBridgeSectionViewCallbacks()
{
   return m_BridgeSectionViewCallbacks;
}

IDType CPGSuperDocBase::RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_GirderElevationViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterGirderElevationViewCallback(IDType ID)
{
   std::map<IDType,IGirderElevationViewEventCallback*>::iterator found = m_GirderElevationViewCallbacks.find(ID);
   if ( found == m_GirderElevationViewCallbacks.end() )
   {
      return false;
   }

   m_GirderElevationViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IGirderElevationViewEventCallback*>& CPGSuperDocBase::GetGirderElevationViewCallbacks()
{
   return m_GirderElevationViewCallbacks;
}

IDType CPGSuperDocBase::RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_GirderSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterGirderSectionViewCallback(IDType ID)
{
   std::map<IDType,IGirderSectionViewEventCallback*>::iterator found = m_GirderSectionViewCallbacks.find(ID);
   if ( found == m_GirderSectionViewCallbacks.end() )
   {
      return false;
   }

   m_GirderSectionViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IGirderSectionViewEventCallback*>& CPGSuperDocBase::GetGirderSectionViewCallbacks()
{
   return m_GirderSectionViewCallbacks;
}

IDType CPGSuperDocBase::RegisterEditPierCallback(IEditPierCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditPierCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditPierCallback(IDType ID)
{
   std::map<IDType,IEditPierCallback*>::iterator found = m_EditPierCallbacks.find(ID);
   if ( found == m_EditPierCallbacks.end() )
   {
      return false;
   }

   m_EditPierCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditPierCallback*>& CPGSuperDocBase::GetEditPierCallbacks()
{
   return m_EditPierCallbacks;
}

IDType CPGSuperDocBase::RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditTemporarySupportCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditTemporarySupportCallback(IDType ID)
{
   std::map<IDType,IEditTemporarySupportCallback*>::iterator found = m_EditTemporarySupportCallbacks.find(ID);
   if ( found == m_EditTemporarySupportCallbacks.end() )
   {
      return false;
   }

   m_EditTemporarySupportCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditTemporarySupportCallback*>& CPGSuperDocBase::GetEditTemporarySupportCallbacks()
{
   return m_EditTemporarySupportCallbacks;
}

IDType CPGSuperDocBase::RegisterEditSpanCallback(IEditSpanCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditSpanCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditSpanCallback(IDType ID)
{
   std::map<IDType,IEditSpanCallback*>::iterator found = m_EditSpanCallbacks.find(ID);
   if ( found == m_EditSpanCallbacks.end() )
   {
      return false;
   }

   m_EditSpanCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditSpanCallback*>& CPGSuperDocBase::GetEditSpanCallbacks()
{
   return m_EditSpanCallbacks;
}

IDType CPGSuperDocBase::RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback * pCopyCallback)
{
   IDType key = m_CallbackID++;
   m_EditGirderCallbacks.insert(std::make_pair(key,pCallback));

   if ( pCopyCallback )
   {
      m_CopyGirderPropertiesCallbacks.insert(std::make_pair(key,pCopyCallback));
   }

   return key;
}

bool CPGSuperDocBase::UnregisterEditGirderCallback(IDType ID)
{
   std::map<IDType,IEditGirderCallback*>::iterator foundCallback = m_EditGirderCallbacks.find(ID);
   if ( foundCallback == m_EditGirderCallbacks.end() )
   {
      return false;
   }

   m_EditGirderCallbacks.erase(foundCallback);

   std::map<IDType,ICopyGirderPropertiesCallback*>::iterator foundCopyCallback = m_CopyGirderPropertiesCallbacks.find(ID);
   if ( foundCopyCallback != m_CopyGirderPropertiesCallbacks.end() )
   {
      m_CopyGirderPropertiesCallbacks.erase(foundCopyCallback);
   }

   return true;
}

const std::map<IDType,IEditGirderCallback*>& CPGSuperDocBase::GetEditGirderCallbacks()
{
   return m_EditGirderCallbacks;
}

const std::map<IDType,ICopyGirderPropertiesCallback*>& CPGSuperDocBase::GetCopyGirderPropertiesCallbacks()
{
   return m_CopyGirderPropertiesCallbacks;
}

IDType CPGSuperDocBase::RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback)
{
   IDType key = m_CallbackID++;
   m_EditSplicedGirderCallbacks.insert(std::make_pair(key,pCallback));

   if ( pCopyCallback )
   {
      m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(key,pCopyCallback));
   }

   return key;
}

bool CPGSuperDocBase::UnregisterEditSplicedGirderCallback(IDType ID)
{
   std::map<IDType,IEditSplicedGirderCallback*>::iterator found = m_EditSplicedGirderCallbacks.find(ID);
   if ( found == m_EditSplicedGirderCallbacks.end() )
   {
      return false;
   }

   m_EditSplicedGirderCallbacks.erase(found);

   std::map<IDType,ICopyGirderPropertiesCallback*>::iterator foundCopyCallback = m_CopySplicedGirderPropertiesCallbacks.find(ID);
   if ( foundCopyCallback != m_CopySplicedGirderPropertiesCallbacks.end() )
   {
      m_CopySplicedGirderPropertiesCallbacks.erase(foundCopyCallback);
   }

   return true;
}

const std::map<IDType,IEditSplicedGirderCallback*>& CPGSuperDocBase::GetEditSplicedGirderCallbacks()
{
   return m_EditSplicedGirderCallbacks;
}

const std::map<IDType,ICopyGirderPropertiesCallback*>& CPGSuperDocBase::GetCopySplicedGirderPropertiesCallbacks()
{
   return m_CopySplicedGirderPropertiesCallbacks;
}

IDType CPGSuperDocBase::RegisterEditSegmentCallback(IEditSegmentCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditSegmentCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditSegmentCallback(IDType ID)
{
   std::map<IDType,IEditSegmentCallback*>::iterator found = m_EditSegmentCallbacks.find(ID);
   if ( found == m_EditSegmentCallbacks.end() )
   {
      return false;
   }

   m_EditSegmentCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditSegmentCallback*>& CPGSuperDocBase::GetEditSegmentCallbacks()
{
   return m_EditSegmentCallbacks;
}

IDType CPGSuperDocBase::RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditClosureJointCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditClosureJointCallback(IDType ID)
{
   std::map<IDType,IEditClosureJointCallback*>::iterator found = m_EditClosureJointCallbacks.find(ID);
   if ( found == m_EditClosureJointCallbacks.end() )
   {
      return false;
   }

   m_EditClosureJointCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditClosureJointCallback*>& CPGSuperDocBase::GetEditClosureJointCallbacks()
{
   return m_EditClosureJointCallbacks;
}

IDType CPGSuperDocBase::RegisterEditBridgeCallback(IEditBridgeCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditBridgeCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSuperDocBase::UnregisterEditBridgeCallback(IDType ID)
{
   std::map<IDType,IEditBridgeCallback*>::iterator found = m_EditBridgeCallbacks.find(ID);
   if ( found == m_EditBridgeCallbacks.end() )
   {
      return false;
   }

   m_EditBridgeCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditBridgeCallback*>& CPGSuperDocBase::GetEditBridgeCallbacks()
{
   return m_EditBridgeCallbacks;
}

BOOL CPGSuperDocBase::OnNewDocumentFromTemplate(LPCTSTR lpszPathName)
{
   if ( !CEAFDocument::OnNewDocumentFromTemplate(lpszPathName) )
   {
      return FALSE;
   }

   InitProjectProperties();
   return TRUE;
}

void CPGSuperDocBase::OnCloseDocument()
{
   // Put report favorites options back into CPGSuperBaseAppPlugin
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuperAppPlugin = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   bool doDisplayFavorites = GetDoDisplayFavoriteReports();
   std::vector<std::_tstring> Favorites = GetFavoriteReports();

   pPGSuperAppPlugin->SetDoDisplayFavoriteReports(doDisplayFavorites);
   pPGSuperAppPlugin->SetFavoriteReports(Favorites);

   // user-defined custom reports
   CEAFCustomReports reports = GetCustomReports();
   pPGSuperAppPlugin->SetCustomReports(reports);

   // Put the main frame icon back the way it was
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   pFrame->SetIcon(m_hMainFrameBigIcon,TRUE);
   pFrame->SetIcon(m_hMainFrameSmallIcon,FALSE);

   CEAFBrokerDocument::OnCloseDocument();

   CBeamFamilyManager::Reset();
}

void CPGSuperDocBase::InitProjectProperties()
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
   {
      OnFileProjectProperties();
   }
}

void CPGSuperDocBase::OnCreateInitialize()
{
   // called before any data is loaded/created in the document
   CEAFBrokerDocument::OnCreateInitialize();

   // Cant' hold events here because this is before any document
   // initialization happens. ie., the broker hasn't been
   // created yet
}

void CPGSuperDocBase::OnCreateFinalize()
{
   // Register callbacks for status items
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidInformationalError  = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   m_StatusGroupID = pStatusCenter->CreateStatusGroupID();

   CEAFBrokerDocument::OnCreateFinalize();

   PopulateReportMenu();
   PopulateGraphMenu();

#pragma Reminder("REVIEW")
/* This option works if Outlook and PGSuper are running at the same UAC level

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
      if ( pFileMenu->GetMenuString(ID_FILE_SEND_MAIL,strEmail,MF_BYCOMMAND) )
      {
	      // find the position of the email command
	      UINT emailPos = pFileMenu->FindMenuItem(strEmail);
	
	      // remove the email command and the adjacent separator
	      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,NULL);
	      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,NULL);
      }
   }
*/
   // Set the AutoCalc state on the status bar
   CPGSuperStatusBar* pStatusBar = ((CPGSuperStatusBar*)EAFGetMainFrame()->GetStatusBar());
   pStatusBar->AutoCalcEnabled( IsAutoCalcEnabled() );

   // views have been initilized so fire any pending events
   GET_IFACE(IEvents,pEvents);
   GET_IFACE(IUIEvents,pUIEvents);
   pEvents->FirePendingEvents(); 
   pUIEvents->HoldEvents(false);
}

BOOL CPGSuperDocBase::CreateBroker()
{
   if ( !CEAFBrokerDocument::CreateBroker() )
   {
      return FALSE;
   }

   // map old PGSuper (pre version 3.0) CLSID to current CLSID
   // CLSID's where changed so that pre version 3.0 installations could co-exist with 3.0 and later installations
   CComQIPtr<ICLSIDMap> clsidMap(m_pBroker);
   clsidMap->AddCLSID(CComBSTR("{BE55D0A2-68EC-11D2-883C-006097C68A9C}"),CComBSTR("{DD1ECB24-F46E-4933-8EE4-1DC0BC67410D}")); // Analysis Agent
   clsidMap->AddCLSID(CComBSTR("{59753CA0-3B7B-11D2-8EC5-006097DF3C68}"),CComBSTR("{3FD393DD-8AF4-4CB2-A1C5-71E46C436BA0}")); // Bridge Agent
   clsidMap->AddCLSID(CComBSTR("{B455A760-6DAF-11D2-8EE9-006097DF3C68}"),CComBSTR("{73922319-9243-4974-BA54-CF22593EC9C4}")); // Eng Agent
   clsidMap->AddCLSID(CComBSTR("{3DA9045D-7C49-4591-AD14-D560E7D95581}"),CComBSTR("{B4639189-ED38-4A68-8A18-38026202E9DE}")); // Graph Agent
   clsidMap->AddCLSID(CComBSTR("{59D50426-265C-11D2-8EB0-006097DF3C68}"),CComBSTR("{256B5B5B-762C-4693-8802-6B0351290FEA}")); // Project Agent
   clsidMap->AddCLSID(CComBSTR("{3D5066F2-27BE-11D2-8EB2-006097DF3C68}"),CComBSTR("{1FFED5EC-7A32-4837-A1F1-99481AFF2825}")); // PGSuper Report Agent
   clsidMap->AddCLSID(CComBSTR("{EC915470-6E76-11D2-8EEB-006097DF3C68}"),CComBSTR("{F510647E-1F4F-4FEF-8257-6914DE7B07C8}")); // Spec Agent
   clsidMap->AddCLSID(CComBSTR("{433B5860-71BF-11D3-ADC5-00105A9AF985}"),CComBSTR("{7D692AAD-39D0-4E73-842C-854457EA0EE6}")); // Test Agent

   return TRUE;
}

BOOL CPGSuperDocBase::OnOpenDocument(LPCTSTR lpszPathName)
{
   CString file_ext;
   CString file_name(lpszPathName);
	int charpos = file_name.ReverseFind('.');
	if (0 <= charpos)
   {
		file_ext = file_name.Right(file_name.GetLength() - charpos);
   }

   if ( file_ext.CompareNoCase(GetTemplateExtension()) == 0 )
   {
      // this is a template file
      return OnNewDocumentFromTemplate(lpszPathName);
   }
   else
   {
      return CEAFBrokerDocument::OnOpenDocument(lpszPathName);
   }
}

BOOL CPGSuperDocBase::OpenTheDocument(LPCTSTR lpszPathName)
{
   // don't fire UI events as the UI isn't completely built when the document is created
   // (view classes haven't been initialized)
   m_pPGSuperDocProxyAgent->HoldEvents();
   // Events are released in OnCreateFinalize()

   if ( !CEAFBrokerDocument::OpenTheDocument(lpszPathName) )
   {
      return FALSE;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   m_DocUnitSystem->put_UnitMode( IS_US_UNITS(pDisplayUnits) ? umUS : umSI );
  
   return TRUE;
}


HRESULT CPGSuperDocBase::ConvertTheDocument(LPCTSTR lpszPathName, CString* prealFileName)
{
   // Open the document and look at the second line
   // If the version tag is 0.80, then the document needs to be converted
   std::_tifstream ifile(lpszPathName);
   if ( !ifile )
   {
      return E_INVALIDARG;
   }

   TCHAR line[50];
   ifile.getline(line,50);
   ifile.getline(line,50);
   CString strLine(line);
   strLine.TrimLeft();
   int loc = strLine.Find(_T(">"),0);
   if (loc!=-1)
   {
      strLine = strLine.Left(loc+1);
      if ( strLine == CString(_T("<PGSuper version=\"0.8\">")) ||
           strLine == CString(_T("<PGSuper version=\"0.83\">"))  )
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
            return -1;
         }
         else if (convert_status==1)
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

CString CPGSuperDocBase::GetRootNodeName()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);
   return pPGSuper->GetAppName();
}

Float64 CPGSuperDocBase::GetRootNodeVersion()
{
   return FILE_VERSION;
}

HRESULT CPGSuperDocBase::OpenDocumentRootNode(IStructuredSave* pStrSave)
{
  HRESULT hr = CEAFDocument::OpenDocumentRootNode(pStrSave);
  if ( FAILED(hr) )
  {
     return hr;
  }

  hr = pStrSave->put_Property(_T("Version"),CComVariant(theApp.GetVersion(true)));
  if ( FAILED(hr) )
  {
     return hr;
  }

  return S_OK;
}

HRESULT CPGSuperDocBase::OpenDocumentRootNode(IStructuredLoad* pStrLoad)
{
   HRESULT hr = CEAFDocument::OpenDocumentRootNode(pStrLoad);
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 version;
   hr = pStrLoad->get_Version(&version);
   if ( FAILED(hr) )
   {
      return hr;
   }

   if ( 1.0 < version )
   {
      CComVariant var;
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Version"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

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

void CPGSuperDocBase::OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
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

void CPGSuperDocBase::OnErrorRemaningSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
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
// CPGSuperDocBase diagnostics

#ifdef _DEBUG
void CPGSuperDocBase::AssertValid() const
{
	CEAFBrokerDocument::AssertValid();
}

void CPGSuperDocBase::Dump(CDumpContext& dc) const
{
	CEAFBrokerDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDocBase commands

BOOL CPGSuperDocBase::Init()
{
   // set the profile name so we read data from the registry correctly
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp = AfxGetApp();

   m_strAppProfileName = pMyApp->m_pszProfileName;
   free((void*)pMyApp->m_pszProfileName);

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);
   pMyApp->m_pszProfileName = _tcsdup(pPGSuper->GetAppName());

   if ( !CEAFBrokerDocument::Init() )
   {
      return FALSE;
   }

   // Application start up can be improved if this call
   // is executed in its own thread... Need to add some
   // code that indicates if the call fails.. then throw
   // a shut down exception
   if ( FAILED(CBeamFamilyManager::Init(GetBeamFamilyCategoryID())) )
   {
      return FALSE;
   }

   m_pPluginMgr = CreatePluginManager();
   m_pPluginMgr->LoadPlugins(); // these are the data importers and exporters

   // load up the library manager
   if ( !LoadMasterLibrary() )
   {
      return FALSE;
   }

   // Setup the library manager (same as if it changed)
   OnLibMgrChanged( &m_LibMgr );

   // Set up the document unit system
   CComPtr<IAppUnitSystem> appUnitSystem;
   pPGSuper->GetAppUnitSystem(&appUnitSystem);
   CreateDocUnitSystem(appUnitSystem,&m_DocUnitSystem);

   // Transfer report favorites and custom reports data from CPGSuperBaseAppPlugin to CEAFBrokerDocument (this)
   bool doDisplayFavorites = pPGSuper->GetDoDisplayFavoriteReports();
   std::vector<std::_tstring> Favorites = pPGSuper->GetFavoriteReports();

   SetDoDisplayFavoriteReports(doDisplayFavorites);
   SetFavoriteReports(Favorites);

   CEAFCustomReports customs = pPGSuper->GetCustomReports();
   SetCustomReports(customs);

   // register the standard copy girder callback objects
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderType));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderMaterials));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderPrestressing));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderRebar));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderStirrups));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderHandling));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderSlabOffset));

#pragma Reminder("REVIEW: is this correct for spliced girders?")
   // the copy girder properties features really hasn't been tested or made to work for spliced
   // girders yet.
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderType));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderMaterials));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderPrestressing));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderRebar));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderStirrups));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderHandling));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderSlabOffset));

   // Put our icon on the main frame window
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   m_hMainFrameBigIcon = pFrame->GetIcon(TRUE);
   m_hMainFrameSmallIcon = pFrame->GetIcon(FALSE);
   HICON hIcon = AfxGetApp()->LoadIcon(pTemplate->GetResourceID());
   pFrame->SetIcon(hIcon,TRUE);
   pFrame->SetIcon(hIcon,FALSE);

   return TRUE;
}

BOOL CPGSuperDocBase::LoadSpecialAgents(IBrokerInitEx2* pBrokerInit)
{
   if ( !CEAFBrokerDocument::LoadSpecialAgents(pBrokerInit) )
   {
      return FALSE;
   }

   CComObject<CPGSuperDocProxyAgent>* pDocProxyAgent;
   CComObject<CPGSuperDocProxyAgent>::CreateInstance(&pDocProxyAgent);
   m_pPGSuperDocProxyAgent = dynamic_cast<CPGSuperDocProxyAgent*>(pDocProxyAgent);
   m_pPGSuperDocProxyAgent->SetDocument( this );

   CComPtr<IAgentEx> pAgent(m_pPGSuperDocProxyAgent);
   
   HRESULT hr = pBrokerInit->AddAgent( pAgent );
   if ( FAILED(hr) )
   {
      return hr;
   }

   // we want to use some special agents
   CLSID clsid[] = {CLSID_SysAgent,CLSID_ReportManagerAgent,CLSID_GraphManagerAgent};
   if ( !LoadAgents(pBrokerInit, clsid, sizeof(clsid)/sizeof(CLSID) ) )
   {
      return FALSE;
   }

   return TRUE;
}

void CPGSuperDocBase::OnFileProjectProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IProjectProperties, pProjProp );

   CProjectPropertiesDlg dlg;

   dlg.m_Bridge    = pProjProp->GetBridgeName();
   dlg.m_BridgeID  = pProjProp->GetBridgeID();
   dlg.m_JobNumber = pProjProp->GetJobNumber();
   dlg.m_Engineer  = pProjProp->GetEngineer();
   dlg.m_Company   = pProjProp->GetCompany();
   dlg.m_Comments  = pProjProp->GetComments();
   dlg.m_bShowProjectProperties = ShowProjectPropertiesOnNewProject();


   if ( dlg.DoModal() == IDOK )
   {
      txnEditProjectProperties* pTxn = new txnEditProjectProperties( pProjProp->GetBridgeName(), dlg.m_Bridge,
                                                                     pProjProp->GetBridgeID(),   dlg.m_BridgeID,
                                                                     pProjProp->GetJobNumber(),  dlg.m_JobNumber,
                                                                     pProjProp->GetEngineer(),   dlg.m_Engineer,
                                                                     pProjProp->GetCompany(),    dlg.m_Company,
                                                                     pProjProp->GetComments(),   dlg.m_Comments );

         
      ShowProjectPropertiesOnNewProject(dlg.m_bShowProjectProperties);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}


void CPGSuperDocBase::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleOpenDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while opening %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   if ( hr == STRLOAD_E_USERDEFINED )
   {
      // a user defined error occured. an error message should have been displayed
      // at the point where the error occured. 
      // Do nothing here! return because we are done
      return;
   }

   CString strLog;
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
      strLog.Format(_T("File does not have a valid %s format"),::AfxGetAppName());
      pLog->LogMessage(strLog);
      AfxFormatString1( msg1, IDS_E_INVALIDFORMAT, lpszPathName );
      break;

   case STRLOAD_E_BADVERSION:
      strLog.Format(_T("This file came from a newer version of %s, please upgrade"),::AfxGetAppName());
      pLog->LogMessage(strLog);
      AfxFormatString1( msg1, IDS_E_INVALIDVERSION, lpszPathName );
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

void CPGSuperDocBase::HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName )
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

void CPGSuperDocBase::HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName )
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

void CPGSuperDocBase::OnProjectEnvironment() 
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

void CPGSuperDocBase::OnEffectiveFlangeWidth()
{
   GET_IFACE(IEffectiveFlangeWidth,pEFW);
   CString strQuestion(_T("The LRFD General Effective Flange Width provisions (4.6.2.6.1) are consider applicable for skew angles less than 75 degress, L/S less than or equal to 2.0 and overhang widths less than or equal to 0.5S. In unusual cases where these limits are violated, a refined analysis should be used."));
   CString strResponses(_T("Stop analysis if structure violates these limits\nIgnore these limits"));

   int choice = pEFW->IgnoreEffectiveFlangeWidthLimits() ? 1 : 0;
   int new_choice = AfxChoose(_T("Effective Flange Width"),strQuestion,strResponses,choice,TRUE);
   if ( choice != new_choice && 0 <= new_choice )
   {
      txnEditEffectiveFlangeWidth* pTxn = new txnEditEffectiveFlangeWidth(pEFW->IgnoreEffectiveFlangeWidthLimits(),new_choice == 0 ? false : true);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

/*--------------------------------------------------------------------*/
void CPGSuperDocBase::OnProjectSpec() 
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
         GET_IFACE(ILibrary,pLib);
         const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( dlg.m_Spec.c_str() );

         pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
         pgsTypes::AnalysisType newAnalysisType = analysisType;
         if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP && analysisType != pgsTypes::Continuous )
         {
            newAnalysisType = pgsTypes::Continuous;
            AfxMessageBox(_T("The selected project criteria uses the time-step loss method. The analysis mode must be Continuous for the time-step loss method."));
         }

         txnEditProjectCriteria* pTxn = new txnEditProjectCriteria(cur_spec.c_str(),dlg.m_Spec.c_str(),analysisType,newAnalysisType);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSuperDocBase::OnRatingSpec()
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
   oldData.m_Design.StrengthI_CR           = pSpec->GetCreepFactor(         pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_SH           = pSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_PS           = pSpec->GetPrestressFactor(     pgsTypes::StrengthI_Inventory);

   oldData.m_Design.ServiceIII_DC          = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_DW          = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_LL          = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_CR          = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_SH          = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_PS          = pSpec->GetPrestressFactor(     pgsTypes::ServiceIII_Inventory);

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
   oldData.m_Legal.StrengthI_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalSpecial);
   oldData.m_Legal.StrengthI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalSpecial);
   oldData.m_Legal.StrengthI_PS         = pSpec->GetPrestressFactor(     pgsTypes::StrengthI_LegalSpecial);

   oldData.m_Legal.ServiceIII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_PS         = pSpec->GetPrestressFactor(     pgsTypes::ServiceIII_LegalSpecial);

   oldData.m_Legal.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bRateForShear    = pSpec->RateForShear(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bExcludeLaneLoad = pSpec->ExcludeLegalLoadLaneLoading();

   oldData.m_Permit.RoutinePermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   oldData.m_Permit.SpecialPermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_PS         = pSpec->GetPrestressFactor(     pgsTypes::StrengthII_PermitRoutine);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_PS         = pSpec->GetPrestressFactor(     pgsTypes::StrengthII_PermitSpecial);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_PS         = pSpec->GetPrestressFactor(     pgsTypes::ServiceI_PermitRoutine);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_PS         = pSpec->GetPrestressFactor(     pgsTypes::ServiceI_PermitSpecial);

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
void CPGSuperDocBase::OnProjectAutoCalc() 
{
	EnableAutoCalc( !IsAutoCalcEnabled() );
}

/*--------------------------------------------------------------------*/
void CPGSuperDocBase::OnUpdateProjectAutoCalc(CCmdUI* pCmdUI) 
{
	if ( IsAutoCalcEnabled() )
   {
      pCmdUI->SetText( _T("Turn AutoCalc Off") );
   }
   else
   {
      pCmdUI->SetText( _T("Turn AutoCalc On") );
   }
}

/*--------------------------------------------------------------------*/
void CPGSuperDocBase::OnExportToTemplateFile() 
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSuperBaseAppPlugin* pPGSuper = dynamic_cast<CPGSuperBaseAppPlugin*>(pAppPlugin.p);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // select inital directory to try and save in
   CString extension;
   pTemplate->GetDocString(extension,CDocTemplate::filterExt);
   int pos = 0;
   CString ext = extension.Tokenize(_T(";"),pos); // extension of project file
   ext = extension.Tokenize(_T(";"),pos); // extension of template file

   CString default_name;
   default_name.Format(_T("%s%s"),pPGSuper->GetAppName(),ext);
   CString initial_filespec;
   CString initial_dir;
   
   // prompt user to save current project to a template file
   CString strFilter;
   strFilter.Format(_T("%s Template Files (*%s)|*%s||"),pPGSuper->GetAppName(),ext,ext);
   CFileDialog  fildlg(FALSE,extension,default_name,OFN_HIDEREADONLY,strFilter);

#if defined _DEBUG
   // If this is a debug build, then the developers are probably running
   // the software and they want the workgroup folder most often.
   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);
   fildlg.m_ofn.lpstrInitialDir = workgroup_folder;
#else
   fildlg.m_ofn.lpstrInitialDir = initial_dir;
#endif // _DEBUG

   INT_PTR stf = fildlg.DoModal();
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
         {
            return;
         }
      }

      // write the file.
      SaveTheDocument( file_path );
   }
}

bool DoesFolderExist(const CString& dirname)
{
   if (dirname.IsEmpty())
   {
      return false;
   }
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
   {
      return false;
   }
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}


void CPGSuperDocBase::OnViewsettingsBridgemodelEditor() 
{
   EditBridgeViewSettings(0);
}

void CPGSuperDocBase::OnViewsettingsGirderEditor() 
{
   EditGirderViewSettings(0);
}


void CPGSuperDocBase::OnLoadsLoadModifiers() 
{
   GET_IFACE(ILoadModifiers,pLoadModifiers);

   txnEditLoadModifiers::LoadModifiers loadModifiers;
   loadModifiers.DuctilityLevel    = pLoadModifiers->GetDuctilityLevel();
   loadModifiers.DuctilityFactor   = pLoadModifiers->GetDuctilityFactor();
   loadModifiers.ImportanceLevel   = pLoadModifiers->GetImportanceLevel();
   loadModifiers.ImportanceFactor  = pLoadModifiers->GetImportanceFactor();
   loadModifiers.RedundancyLevel   = pLoadModifiers->GetRedundancyLevel();
   loadModifiers.RedundancyFactor  = pLoadModifiers->GetRedundancyFactor();

   CLoadModifiersDlg dlg;
   dlg.SetHelpData( AfxGetApp()->m_pszHelpFilePath, IDH_DIALOG_LOADMODIFIERS, IDH_DIALOG_LOADMODIFIERS, IDH_DIALOG_LOADMODIFIERS);

   dlg.SetLoadModifiers( loadModifiers.DuctilityFactor,
                         ((loadModifiers.DuctilityLevel == ILoadModifiers::High)  ? 0 : (loadModifiers.DuctilityLevel  == ILoadModifiers::Normal ? 1 : 2)),
                         loadModifiers.RedundancyFactor,
                         ((loadModifiers.RedundancyLevel == ILoadModifiers::High) ? 0 : (loadModifiers.RedundancyLevel == ILoadModifiers::Normal ? 1 : 2)),
                         loadModifiers.ImportanceFactor,
                         ((loadModifiers.ImportanceLevel == ILoadModifiers::High) ? 0 : (loadModifiers.ImportanceLevel == ILoadModifiers::Normal ? 1 : 2))
                        );

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLoadModifiers::LoadModifiers newLoadModifiers;

      Int16 d,r,i;
      dlg.GetLoadModifiers(&newLoadModifiers.DuctilityFactor, &d,
                           &newLoadModifiers.RedundancyFactor,&r,
                           &newLoadModifiers.ImportanceFactor,&i);

      newLoadModifiers.DuctilityLevel  = (d == 0 ? ILoadModifiers::High : (d == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));
      newLoadModifiers.RedundancyLevel = (d == 0 ? ILoadModifiers::High : (d == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));
      newLoadModifiers.ImportanceLevel = (d == 0 ? ILoadModifiers::High : (d == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));

      txnEditLoadModifiers* pTxn = new txnEditLoadModifiers(loadModifiers,newLoadModifiers);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDocBase::OnLoadsLoadFactors()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(ILoadFactors,pLoadFactors);

   CLoadFactors loadFactors = *pLoadFactors->GetLoadFactors();
   CLoadFactorsDlg dlg;
   dlg.m_LoadFactors = loadFactors;
   if ( dlg.DoModal() == IDOK )
   {
      txnEditLoadFactors* pTxn = new txnEditLoadFactors(loadFactors,dlg.m_LoadFactors);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDocBase::UpdateAnalysisTypeStatusIndicator()
{
   CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)(EAFGetMainFrame()->GetStatusBar());

   GET_IFACE(ISpecification,pSpec);
   pStatusBar->SetAnalysisTypeStatusIndicator(pSpec->GetAnalysisType());
}

bool CPGSuperDocBase::LoadMasterLibrary()
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

bool CPGSuperDocBase::DoLoadMasterLibrary(const CString& strMasterLibraryFile)
{
   if ( strMasterLibraryFile.GetLength() == 0 )
   {
      return true;
   }

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


   // Remove all girder entries that are not associated with
   // the beam family ID (basically removing PGSuper girders
   // for PGSplice and visa-versa)
   CATID catid = GetBeamFamilyCategoryID();
   CComPtr<ICatRegister> pICatReg = 0;
   HRESULT hr;
   hr = ::CoCreateInstance( CLSID_StdComponentCategoriesMgr,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ICatRegister,
                            (void**)&pICatReg );

   CComPtr<ICatInformation> pICatInfo;
   pICatReg->QueryInterface(IID_ICatInformation,(void**)&pICatInfo);

   GirderLibrary& gdrLib = m_LibMgr.GetGirderLibrary();
   libKeyListType keyList;
   gdrLib.KeyList(keyList);
   CollectionIndexType nEntries = gdrLib.GetCount();
   for ( CollectionIndexType i = 0; i < nEntries; i++ )
   {
      std::_tstring strName = keyList[i];
      const libLibraryEntry* pEntry = gdrLib.GetEntry(strName.c_str());
      const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pEntry);
      CComPtr<IBeamFactory> beamFactory;
      pGdrEntry->GetBeamFactory(&beamFactory);
      CLSID clsid = beamFactory->GetFamilyCLSID();

      HRESULT result = pICatInfo->IsClassOfCategories(clsid,1,&catid,0,NULL);
      if ( result == S_FALSE )
      { 
         gdrLib.RemoveEntry(strName.c_str());
      }
   }

   return true; // the only way out alive!
}

CSelection CPGSuperDocBase::GetSelection()
{
   return m_Selection;
}

void CPGSuperDocBase::SetSelection(const CSelection& selection)
{
   if ( m_Selection != selection )
   {
      m_Selection = selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&m_Selection);
   }
}

void CPGSuperDocBase::SelectPier(PierIndexType pierIdx)
{
   if ( m_Selection.Type == CSelection::Pier && m_Selection.PierIdx == pierIdx )
   {
      return;
   }

   m_Selection.Type       = CSelection::Pier;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = pierIdx;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDocBase::SelectSpan(SpanIndexType spanIdx)
{
   if ( m_Selection.Type == CSelection::Span && m_Selection.SpanIdx == spanIdx )
   {
      return;
   }

   m_Selection.Type       = CSelection::Span;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SpanIdx    = spanIdx;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDocBase::SelectGirder(const CGirderKey& girderKey)
{
   if ( m_Selection.Type == CSelection::Girder && m_Selection.GroupIdx == girderKey.groupIndex && m_Selection.GirderIdx == girderKey.girderIndex )
   {
      return;
   }

   m_Selection.Type       = CSelection::Girder;
   m_Selection.GroupIdx   = girderKey.groupIndex;
   m_Selection.GirderIdx  = girderKey.girderIndex;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDocBase::SelectSegment(const CSegmentKey& segmentKey)
{
   if ( m_Selection.Type == CSelection::Segment && m_Selection.GroupIdx == segmentKey.groupIndex && m_Selection.GirderIdx == segmentKey.girderIndex && m_Selection.SegmentIdx == segmentKey.segmentIndex )
   {
      return;
   }

   m_Selection.Type       = CSelection::Segment;
   m_Selection.GroupIdx   = segmentKey.groupIndex;
   m_Selection.GirderIdx  = segmentKey.girderIndex;
   m_Selection.SegmentIdx = segmentKey.segmentIndex;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDocBase::SelectClosureJoint(const CClosureKey& closureKey)
{
   if ( m_Selection.Type == CSelection::ClosureJoint && m_Selection.GroupIdx == closureKey.groupIndex && m_Selection.GirderIdx == closureKey.girderIndex && m_Selection.SegmentIdx == closureKey.segmentIndex )
   {
      return;
   }

   m_Selection.Type       = CSelection::ClosureJoint;
   m_Selection.GroupIdx   = closureKey.groupIndex;
   m_Selection.GirderIdx  = closureKey.girderIndex;
   m_Selection.SegmentIdx = closureKey.segmentIndex;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDocBase::SelectTemporarySupport(SupportIDType tsID)
{
   if ( m_Selection.Type == CSelection::TemporarySupport && m_Selection.tsID == tsID )
   {
      return;
   }

   m_Selection.Type       = CSelection::TemporarySupport;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = tsID;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDocBase::SelectDeck()
{
   if ( m_Selection.Type == CSelection::Deck )
   {
      return;
   }

   m_Selection.Type       = CSelection::Deck;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDocBase::SelectAlignment()
{
   if ( m_Selection.Type == CSelection::Alignment )
   {
      return;
   }

   m_Selection.Type       = CSelection::Alignment;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
}

void CPGSuperDocBase::ClearSelection()
{
   if ( m_Selection.Type == CSelection::None )
   {
      return;
   }

   m_Selection.Type       = CSelection::None;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      bProcessingSelectionChanged = false;
   }
}

void CPGSuperDocBase::OnCopyGirderProps() 
{
#pragma Reminder("UPDATE: need to make OnCopyGirderProps work for spliced girders")
   // This may be a case when OnCopyGirderProps needs to move to the CPGSuperDoc and CPGSpliceDoc
   // classes.

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CCopyGirderDlg dlg(m_pBroker);
   if ( dlg.DoModal() == IDOK )
   {
      pgsMacroTxn* pMacro = new pgsMacroTxn;
      pMacro->Name(_T("Copy Girder Properties"));

      std::vector<IDType> callbackIDs = dlg.GetCallbackIDs();
      std::vector<IDType>::iterator iter(callbackIDs.begin());
      std::vector<IDType>::iterator end(callbackIDs.end());
      for ( ; iter != end; iter++ )
      {
         IDType callbackID = *iter;
         std::map<IDType,ICopyGirderPropertiesCallback*>::iterator found(m_CopyGirderPropertiesCallbacks.find(callbackID));
         ATLASSERT(found != m_CopyGirderPropertiesCallbacks.end());
         ICopyGirderPropertiesCallback* pCallback = found->second;

         txnTransaction* pTxn = pCallback->CreateCopyTransaction(dlg.m_FromGirderKey,dlg.m_ToGirderKeys);
         if ( pTxn )
         {
            pMacro->AddTransaction(pTxn);
         }
      }

      if ( 0 < pMacro->GetTxnCount() )
      {
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pMacro);
      }
   }
}

void CPGSuperDocBase::OnImportProjectLibrary() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDocTemplate* pTemplate = GetDocTemplate();
   CString strFilterExt;
   pTemplate->GetDocString(strFilterExt,CDocTemplate::filterExt);
   strFilterExt.Replace(_T("."),_T("*."));

   CString strFilter;
   pTemplate->GetDocString(strFilter,CDocTemplate::filterName);

   CString strFilter2;
   strFilter2.Format(_T("%s|%s||"),strFilter,strFilterExt);

	// ask user for file name
   CFileDialog  fileDlg(TRUE,NULL,NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,strFilter2);
   if (fileDlg.DoModal() == IDOK)
   {
      CString rPath;
      rPath = fileDlg.GetPathName();

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
         hr = pgslibReadProjectDocHeader(GetRootNodeName(),pStrLoad);
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

      AfxMessageBox(_T("Done getting library entries"));
   }
}

void CPGSuperDocBase::OnLoadsLldf() 
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();
                  
   OnLoadsLldf(method,roaAction);
}

void CPGSuperDocBase::OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CLiveLoadDistFactorsDlg dlg;
   dlg.m_BridgeDesc = *pOldBridgeDesc;
   dlg.m_BridgeDesc.SetDistributionFactorMethod(method);
   dlg.m_LldfRangeOfApplicabilityAction = roaAction;
   dlg.m_pBroker = m_pBroker;

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(ILiveLoads,pLiveLoads);

      txnEditLLDF* pTxn = new txnEditLLDF(*pOldBridgeDesc,dlg.m_BridgeDesc,
                                          pLiveLoads->GetLldfRangeOfApplicabilityAction(),dlg.m_LldfRangeOfApplicabilityAction);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDocBase::OnAddPointload() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPointLoadData load;
   CEditPointLoadDlg dlg( load );
   if (dlg.DoModal() == IDOK)
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

/*-------------------------------------------------------------------*/
void CPGSuperDocBase::OnAddDistributedLoad() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDistributedLoadData load;
	CEditDistributedLoadDlg dlg(load);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
	
}

void CPGSuperDocBase::OnAddMomentLoad() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CMomentLoadData load;
	CEditMomentLoadDlg dlg(load);
   if (dlg.DoModal() == IDOK)
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDocBase::OnConstructionLoads()
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

void CPGSuperDocBase::OnProjectAlignment() 
{
   EditAlignmentDescription(0);
}

void CPGSuperDocBase::OnLiveLoads() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );
   GET_IFACE( ILiveLoads, pLiveLoad );
   GET_IFACE( IProductLoads,pProductLoads);
   GET_IFACE( IBridgeDescription,pIBridgeDesc);

   std::vector<std::_tstring> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );

   std::vector<std::_tstring> design_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> fatigue_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> permit_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltPermit);

   CLiveLoadSelectDlg dlg(all_names, design_names, fatigue_names, permit_names);

   dlg.m_bHasPedestrianLoad = (pProductLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft)>0.0 ||
                               pProductLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboRight)>0.0);

   EventIndexType oldLiveLoadEvent = pIBridgeDesc->GetTimelineManager()->GetLiveLoadEventIndex();
   dlg.m_LiveLoadEvent = oldLiveLoadEvent;

   dlg.m_DesignTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltDesign);
   dlg.m_DesignLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltDesign);
   dlg.m_DesignPedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltDesign);

   dlg.m_FatigueTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltFatigue);
   dlg.m_FatigueLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltFatigue);
   dlg.m_FatiguePedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltFatigue);

   dlg.m_PermitLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltPermit);
   dlg.m_PermitTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltPermit);
   dlg.m_PermitPedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltPermit);

   txnEditLiveLoadData oldDesign, oldFatigue, oldPermit;
   oldDesign.m_VehicleNames = dlg.m_DesignNames;
   oldDesign.m_TruckImpact  = dlg.m_DesignTruckImpact;
   oldDesign.m_LaneImpact   = dlg.m_DesignLaneImpact;
   oldDesign.m_PedestrianLoadApplicationType = dlg.m_DesignPedesType;

   oldFatigue.m_VehicleNames = dlg.m_FatigueNames;
   oldFatigue.m_TruckImpact  = dlg.m_FatigueTruckImpact;
   oldFatigue.m_LaneImpact   = dlg.m_FatigueLaneImpact;
   oldFatigue.m_PedestrianLoadApplicationType = dlg.m_FatiguePedesType;

   oldPermit.m_VehicleNames = dlg.m_PermitNames;
   oldPermit.m_TruckImpact  = dlg.m_PermitTruckImpact;
   oldPermit.m_LaneImpact   = dlg.m_PermitLaneImpact;
   oldPermit.m_PedestrianLoadApplicationType = dlg.m_PermitPedesType;

   if ( dlg.DoModal() == IDOK)
   {
      txnEditLiveLoadData newDesign, newFatigue, newPermit;
      newDesign.m_VehicleNames                  = dlg.m_DesignNames;
      newDesign.m_TruckImpact                   = dlg.m_DesignTruckImpact;
      newDesign.m_LaneImpact                    = dlg.m_DesignLaneImpact;
      newDesign.m_PedestrianLoadApplicationType = dlg.m_DesignPedesType;

      newFatigue.m_VehicleNames                  = dlg.m_FatigueNames;
      newFatigue.m_TruckImpact                   = dlg.m_FatigueTruckImpact;
      newFatigue.m_LaneImpact                    = dlg.m_FatigueLaneImpact;
      newFatigue.m_PedestrianLoadApplicationType = dlg.m_FatiguePedesType;

      newPermit.m_VehicleNames                  = dlg.m_PermitNames;
      newPermit.m_TruckImpact                   = dlg.m_PermitTruckImpact;
      newPermit.m_LaneImpact                    = dlg.m_PermitLaneImpact;
      newPermit.m_PedestrianLoadApplicationType = dlg.m_PermitPedesType;

      txnEditLiveLoad* pTxn = new txnEditLiveLoad(oldDesign,newDesign,oldFatigue,newFatigue,oldPermit,newPermit,oldLiveLoadEvent,dlg.m_LiveLoadEvent);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

BOOL CPGSuperDocBase::GetStatusBarMessageString(UINT nID,CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetStatusBarMessageString(nID,rMessage) )
   {
      return TRUE;
   }

   CPGSuperDocBase* pThis = const_cast<CPGSuperDocBase*>(this);
   
   CComPtr<IPGSuperDataExporter> exporter;
   pThis->m_pPluginMgr->GetPGSuperExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      rMessage = OLE2T(bstr);
      rMessage.Replace('\n','\0');

      return TRUE;
   }
   
   CComPtr<IPGSuperDataImporter> importer;
   pThis->m_pPluginMgr->GetPGSuperImporter(nID,false,&importer);
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

BOOL CPGSuperDocBase::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetToolTipMessageString(nID,rMessage) )
   {
      return TRUE;
   }

   CPGSuperDocBase* pThis = const_cast<CPGSuperDocBase*>(this);
   
   CComPtr<IPGSuperDataExporter> exporter;
   pThis->m_pPluginMgr->GetPGSuperExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
      {
         rMessage = string.Mid(pos+1);
      }

      return TRUE;
   }
   
   CComPtr<IPGSuperDataImporter> importer;
   pThis->m_pPluginMgr->GetPGSuperImporter(nID,false,&importer);
   if ( importer )
   {
      CComBSTR bstr;
      importer->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
      {
         rMessage = string.Mid(pos+1);
      }

      return TRUE;
   }

   return FALSE;
}

void CPGSuperDocBase::CreateReportView(CollectionIndexType rptIdx,bool bPrompt)
{
   if ( !bPrompt && m_Selection.Type == CSelection::None)
   {
      // this is a quick report... make sure there is a current span and girder
      m_Selection.Type       = CSelection::Girder;
      m_Selection.GroupIdx   = 0;
      m_Selection.GirderIdx  = 0;
      m_Selection.SpanIdx    = INVALID_INDEX;
      m_Selection.PierIdx    = INVALID_INDEX;
      m_Selection.SegmentIdx = INVALID_INDEX;
      m_Selection.tsID       = INVALID_ID;
   }

   m_pPGSuperDocProxyAgent->CreateReportView(rptIdx,bPrompt);

   // the base class does nothing so we won't bother calling it
}

void CPGSuperDocBase::CreateGraphView(CollectionIndexType graphIdx)
{
   m_pPGSuperDocProxyAgent->CreateGraphView(graphIdx);

   // the base class does nothing so we won't bother calling it
}

void CPGSuperDocBase::OnViewBridgeModelEditor()
{
   m_pPGSuperDocProxyAgent->CreateBridgeModelView();
}

void CPGSuperDocBase::OnViewGirderEditor()
{
   CGirderKey girderKey(m_Selection.GroupIdx,m_Selection.GirderIdx);
   m_pPGSuperDocProxyAgent->CreateGirderView(girderKey);
}

void CPGSuperDocBase::OnEditUserLoads()
{
   m_pPGSuperDocProxyAgent->CreateLoadsView();
}

void CPGSuperDocBase::OnViewLibraryEditor()
{
   m_pPGSuperDocProxyAgent->CreateLibraryEditorView();
}

void CPGSuperDocBase::OnEditPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strItems;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CString strItem;
      strItem.Format(_T("%s %d\n"),(pierIdx == 0 || pierIdx == nPiers-1 ? _T("Abutment") : _T("Pier")),LABEL_PIER(pierIdx));

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = _T("Select Abutment/Pier");
   dlg.m_strItems = strItems;
   dlg.m_strLabel = _T("Select an abutment or pier to edit");
   dlg.m_ItemIdx = m_Selection.PierIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditPierDescription(dlg.m_ItemIdx,EPD_GENERAL);
   }
}

void CPGSuperDocBase::OnEditSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
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

void CPGSuperDocBase::DeletePier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a pier

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strTitle;
   strTitle.Format(_T("Deleting Pier %d"),LABEL_PIER(pierIdx));
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format(_T("%s. Select the span to be deleted with the pier"),strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( pierIdx == 0 )
   {
      strItems.Format(_T("%s"),_T("Span 1\n"));
   }
   else if ( pierIdx == nPiers-1)
   {
      strItems.Format(_T("Span %d\n"),LABEL_SPAN(pierIdx-1));
   }
   else
   {
      strItems.Format(_T("Span %d\nSpan %d\n"),LABEL_SPAN(pierIdx-1),LABEL_SPAN(pierIdx));
   }

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      if ( pierIdx == 0 )
      {
         DeletePier(pierIdx,pgsTypes::Ahead);
      }
      else if ( pierIdx == nPiers-1 )
      {
         DeletePier(pierIdx,pgsTypes::Back);
      }
      else
      {
         DeletePier(pierIdx,dlg.m_ItemIdx == 0 ? pgsTypes::Back : pgsTypes::Ahead);
      }
   }
}

void CPGSuperDocBase::DeleteSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a span

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

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
   {
      strItems.Format(_T("%s"),_T("Pier 1\nPier 2\n"));
   }
   else if ( spanIdx == nSpans-1)
   {
      strItems.Format(_T("Pier %d\nPier %d\n"),LABEL_PIER(nSpans-1),LABEL_PIER(nSpans));
   }
   else
   {
      strItems.Format(_T("Pier %d\nPier %d\n"),LABEL_PIER(spanIdx),LABEL_PIER(spanIdx+1));
   }

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      DeleteSpan(spanIdx,dlg.m_ItemIdx == 0 ? pgsTypes::PrevPier : pgsTypes::NextPier );
   }
}

void CPGSuperDocBase::OnDeleteSelection() 
{
   if (  m_Selection.Type == CSelection::Pier )
   {
      DeletePier(m_Selection.PierIdx);
   }
   else if ( m_Selection.Type == CSelection::Span )
   {
      DeleteSpan(m_Selection.SpanIdx);
   }
   else
   {
      ASSERT(FALSE); // should not get here
   }
}

void CPGSuperDocBase::OnUpdateDeleteSelection(CCmdUI* pCmdUI) 
{
   CView* pView = EAFGetActiveView();

   // command doesn't apply if there isn't an active view
   if ( pView == NULL )
   {
      pCmdUI->Enable(FALSE);
      return;
   }

   // command only applies to Bridge Section and Plan views
   if ( !pView->IsKindOf(RUNTIME_CLASS(CBridgeSectionView)) && !pView->IsKindOf(RUNTIME_CLASS(CBridgePlanView)))
   {
      pCmdUI->Enable(FALSE);
      return;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   if ( nSpans == 1 )
   {
      pCmdUI->Enable(FALSE);
      // can't delete the last span
      return;
   }

   if ( m_Selection.Type == CSelection::Pier )
   {
      PierIndexType nPiers = pBridgeDesc->GetPierCount();
      CString strLabel;
      strLabel.Format(_T("Delete Pier %d"),LABEL_PIER(m_Selection.PierIdx));

      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else if ( m_Selection.Type == CSelection::Span  )
   {
      // only span is selected
      CString strLabel;
      strLabel.Format(_T("Delete Span %d"),LABEL_SPAN(m_Selection.SpanIdx));
      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      // can't delete alignment, bridge, deck, or section cut tool
      pCmdUI->SetText(_T("Delete"));
      pCmdUI->Enable(FALSE);
   }
}

void CPGSuperDocBase::DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType face)
{
   txnDeleteSpan* pTxn = new txnDeleteSpan(pierIdx,face);
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDocBase::DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType)
{
   PierIndexType pierIdx = (pierRemoveType == pgsTypes::PrevPier ? spanIdx : spanIdx+1);
   pgsTypes::PierFaceType pierFace = (pierRemoveType == pgsTypes::PrevPier ? pgsTypes::Ahead : pgsTypes::Back);
   DeletePier(pierIdx,pierFace);
}

void CPGSuperDocBase::OnInsert() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
   if ( dlg.DoModal() == IDOK )
   {
      Float64 span_length         = dlg.m_SpanLength;
      PierIndexType refPierIdx    = dlg.m_RefPierIdx;
      pgsTypes::PierFaceType face = dlg.m_PierFace;
      bool bCreateNewGroup        = dlg.m_bCreateNewGroup;
      EventIndexType eventIdx     = dlg.m_EventIndex;
      InsertSpan(refPierIdx,face,span_length,bCreateNewGroup,eventIdx);
   }
}

void CPGSuperDocBase::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,EventIndexType eventIdx)
{
   txnInsertSpan* pTxn = new txnInsertSpan(refPierIdx,pierFace,spanLength,bCreateNewGroup,eventIdx);
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSuperDocBase::OnOptionsHints() 
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

void CPGSuperDocBase::OnOptionsLabels() 
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

void CPGSuperDocBase::OnLosses()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CLossParametersDlg dlg;
   GET_IFACE(ILossParameters,pLossParameters);
   txnEditLossParametersData oldData;
   pLossParameters->GetTendonPostTensionParameters(&oldData.Dset_PT,&oldData.WobbleFriction_PT,&oldData.FrictionCoefficient_PT);
   pLossParameters->GetTemporaryStrandPostTensionParameters(&oldData.Dset_TTS,&oldData.WobbleFriction_TTS,&oldData.FrictionCoefficient_TTS);

   oldData.bIgnoreTimeDependentEffects = pLossParameters->IgnoreTimeDependentEffects();

   oldData.bUseLumpSumLosses             = pLossParameters->UseGeneralLumpSumLosses();
   oldData.BeforeXferLosses              = pLossParameters->GetBeforeXferLosses();
   oldData.AfterXferLosses               = pLossParameters->GetAfterXferLosses();
   oldData.LiftingLosses                 = pLossParameters->GetLiftingLosses();
   oldData.ShippingLosses                = pLossParameters->GetShippingLosses();
   oldData.BeforeTempStrandRemovalLosses = pLossParameters->GetBeforeTempStrandRemovalLosses();
   oldData.AfterTempStrandRemovalLosses  = pLossParameters->GetAfterTempStrandRemovalLosses();
   oldData.AfterDeckPlacementLosses      = pLossParameters->GetAfterDeckPlacementLosses();
   oldData.AfterSIDLLosses               = pLossParameters->GetAfterSIDLLosses();
   oldData.FinalLosses                   = pLossParameters->GetFinalLosses();

   dlg.m_TimeStepProperties.m_bIgnoreTimeDependentEffects = oldData.bIgnoreTimeDependentEffects;

   dlg.m_PostTensioning.Dset_PT                = oldData.Dset_PT;
   dlg.m_PostTensioning.WobbleFriction_PT      = oldData.WobbleFriction_PT;
   dlg.m_PostTensioning.FrictionCoefficient_PT = oldData.FrictionCoefficient_PT;

   dlg.m_PostTensioning.Dset_TTS                = oldData.Dset_TTS;
   dlg.m_PostTensioning.WobbleFriction_TTS      = oldData.WobbleFriction_TTS;
   dlg.m_PostTensioning.FrictionCoefficient_TTS = oldData.FrictionCoefficient_TTS;

   dlg.m_Pretensioning.bUseLumpSumLosses             = oldData.bUseLumpSumLosses;
   dlg.m_Pretensioning.BeforeXferLosses              = oldData.BeforeXferLosses;
   dlg.m_Pretensioning.AfterXferLosses               = oldData.AfterXferLosses;
   dlg.m_Pretensioning.LiftingLosses                 = oldData.LiftingLosses;
   dlg.m_Pretensioning.ShippingLosses                = oldData.ShippingLosses;
   dlg.m_Pretensioning.BeforeTempStrandRemovalLosses = oldData.BeforeTempStrandRemovalLosses;
   dlg.m_Pretensioning.AfterTempStrandRemovalLosses  = oldData.AfterTempStrandRemovalLosses;
   dlg.m_Pretensioning.AfterDeckPlacementLosses      = oldData.AfterDeckPlacementLosses;
   dlg.m_Pretensioning.AfterSIDLLosses               = oldData.AfterSIDLLosses;
   dlg.m_Pretensioning.FinalLosses                   = oldData.FinalLosses;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLossParametersData newData;
      newData.bIgnoreTimeDependentEffects = dlg.m_TimeStepProperties.m_bIgnoreTimeDependentEffects;

      newData.Dset_PT                = dlg.m_PostTensioning.Dset_PT;
      newData.WobbleFriction_PT      = dlg.m_PostTensioning.WobbleFriction_PT;
      newData.FrictionCoefficient_PT = dlg.m_PostTensioning.FrictionCoefficient_PT;

      newData.Dset_TTS                = dlg.m_PostTensioning.Dset_TTS;
      newData.WobbleFriction_TTS      = dlg.m_PostTensioning.WobbleFriction_TTS;
      newData.FrictionCoefficient_TTS = dlg.m_PostTensioning.FrictionCoefficient_TTS;

      newData.bUseLumpSumLosses             = dlg.m_Pretensioning.bUseLumpSumLosses;
      newData.BeforeXferLosses              = dlg.m_Pretensioning.BeforeXferLosses;
      newData.AfterXferLosses               = dlg.m_Pretensioning.AfterXferLosses;
      newData.LiftingLosses                 = dlg.m_Pretensioning.LiftingLosses;
      newData.ShippingLosses                = dlg.m_Pretensioning.ShippingLosses;
      newData.BeforeTempStrandRemovalLosses = dlg.m_Pretensioning.BeforeTempStrandRemovalLosses;
      newData.AfterTempStrandRemovalLosses  = dlg.m_Pretensioning.AfterTempStrandRemovalLosses;
      newData.AfterDeckPlacementLosses      = dlg.m_Pretensioning.AfterDeckPlacementLosses;
      newData.AfterSIDLLosses               = dlg.m_Pretensioning.AfterSIDLLosses;
      newData.FinalLosses                   = dlg.m_Pretensioning.FinalLosses;

      txnEditLossParameters* pTxn = new txnEditLossParameters(oldData,newData);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

BOOL CPGSuperDocBase::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
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
           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_VIEW_REPORTS )
           {
              return OnViewReports(notify->pNMHDR,notify->pResult); 
           }

           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_VIEW_GRAPHS )
           {
              return OnViewGraphs(notify->pNMHDR,notify->pResult); 
           }
        }
    }
	
	return CEAFBrokerDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPGSuperDocBase::PopulateReportMenu()
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

void CPGSuperDocBase::PopulateGraphMenu()
{
   CEAFMenu* pMainMenu = GetMainMenu();

   UINT viewPos = pMainMenu->FindMenuItem(_T("&View"));
   ASSERT( 0 <= viewPos );

   CEAFMenu* pViewMenu = pMainMenu->GetSubMenu(viewPos);
   ASSERT( pViewMenu != NULL );

   UINT graphsPos = pViewMenu->FindMenuItem(_T("Gr&aphs"));
   ASSERT( 0 <= graphsPos );

   // Get the graphs menu
   CEAFMenu* pGraphMenu = pViewMenu->GetSubMenu(graphsPos);
   ASSERT(pGraphMenu != NULL);

   CEAFBrokerDocument::PopulateGraphMenu(pGraphMenu);
}

void CPGSuperDocBase::LoadDocumentSettings()
{
   CEAFBrokerDocument::LoadDocumentSettings();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CPGSuperAppPluginApp* pApp = (CPGSuperAppPluginApp*)AfxGetApp();

   CString strAutoCalcDefault = pApp->GetLocalMachineString(_T("Settings"),_T("AutoCalc"), _T("On"));
   CString strAutoCalc = pApp->GetProfileString(_T("Settings"),_T("AutoCalc"),strAutoCalcDefault);
   if ( strAutoCalc.CompareNoCase(_T("Off")) == 0 )
   {
      m_bAutoCalcEnabled = false;
   }
   else
   {
      m_bAutoCalcEnabled = true;
   }

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
   {
      pgsGirderLabel::UseAlphaLabel(true);
   }
   else
   {
      pgsGirderLabel::UseAlphaLabel(false);
   }


   CString strShowProjectProperties = pApp->GetLocalMachineString(_T("Settings"),_T("ShowProjectProperties"), _T("On"));
   CString strProjectProperties = pApp->GetProfileString(_T("Settings"),_T("ShowProjectProperties"),strShowProjectProperties);
   if ( strProjectProperties.CompareNoCase(_T("Off")) == 0 )
   {
      m_bShowProjectProperties = false;
   }
   else
   {
      m_bShowProjectProperties = true;
   }


   CString strDefaultReportCoverImage = pApp->GetLocalMachineString(_T("Settings"),_T("ReportCoverImage"),_T(""));
   pgsReportStyleHolder::SetReportCoverImage(pApp->GetProfileString(_T("Settings"),_T("ReportCoverImage"),strDefaultReportCoverImage));
}

void CPGSuperDocBase::SaveDocumentSettings()
{
   CEAFBrokerDocument::SaveDocumentSettings();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("AutoCalc"),m_bAutoCalcEnabled ? _T("On") : _T("Off") ));

   // bridge editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("BridgeEditor"),m_BridgeModelEditorSettings));

   // girder editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("GirderEditor"),m_GirderModelEditorSettings));

   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("UIHints"),m_UIHintSettings));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("GirderLabelFormat"),pgsGirderLabel::UseAlphaLabel() ? _T("Alpha") : _T("Numeric") ));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("ShowProjectProperties"),m_bShowProjectProperties ? _T("On") : _T("Off") ));
}

void CPGSuperDocBase::OnLogFileOpened()
{
   CEAFBrokerDocument::OnLogFileOpened();

   GET_IFACE(IEAFProjectLog,pLog);
   CString strMsg;
   strMsg.Format(_T("PGSuper version %s"),theApp.GetVersion(false).GetBuffer(100));
   pLog->LogMessage(strMsg);
}

void CPGSuperDocBase::BrokerShutDown()
{
   CEAFBrokerDocument::BrokerShutDown();

   m_pPGSuperDocProxyAgent = NULL;
}

void CPGSuperDocBase::OnStatusChanged()
{
   CEAFBrokerDocument::OnStatusChanged();
   if ( m_pPGSuperDocProxyAgent )
   {
      m_pPGSuperDocProxyAgent->OnStatusChanged();
   }
}

void CPGSuperDocBase::LoadToolbarState()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::LoadToolbarState();
}

void CPGSuperDocBase::SaveToolbarState()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::SaveToolbarState();
}

CString CPGSuperDocBase::GetToolbarSectionName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   CString strToolbarSection;
   strToolbarSection.Format(_T("%s"),pApp->m_pszProfileName);

   return strToolbarSection;
}

void CPGSuperDocBase::OnUpdateViewGraphs(CCmdUI* pCmdUI)
{
   GET_IFACE(IGraphManager,pGraphMgr);
   pCmdUI->Enable( 0 < pGraphMgr->GetGraphBuilderCount() );
}

BOOL CPGSuperDocBase::OnViewGraphs(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_GRAPHS )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_REPORTS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());


   BuildGraphMenu(&contextMenu);

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_VIEW_GRAPHS,NULL);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
}

void CPGSuperDocBase::OnUpdateViewReports(CCmdUI* pCmdUI)
{
   GET_IFACE(IReportManager,pReportMgr);
   pCmdUI->Enable( 0 < pReportMgr->GetReportBuilderCount() );
}

BOOL CPGSuperDocBase::OnViewReports(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_REPORTS )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_REPORTS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());

   CEAFBrokerDocument::PopulateReportMenu(&contextMenu);

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


void CPGSuperDocBase::OnImportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
   {
      return;
   }

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   CollectionIndexType nImporters = m_pPluginMgr->GetImporterCount();
   if ( nImporters == 0 )
   {
      pCmdUI->SetText(_T("Custom importers not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      CollectionIndexType idx;
      // clean up the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+(UINT)idx,MF_BYCOMMAND);
      }

      // populate the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         CComPtr<IPGSuperDataImporter> importer;
         m_pPluginMgr->GetPGSuperImporter(idx,true,&importer);

         UINT cmdID = m_pPluginMgr->GetPGSuperImporterCommand(idx);

         CComBSTR bstrMenuText;
         importer->GetMenuText(&bstrMenuText);
         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_pPluginMgr->GetPGSuperImporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,NULL);

   	   pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();

         pCmdUI->m_nIndex++;
      }
   }

   pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperDocBase::OnExportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == NULL && pCmdUI->m_pSubMenu == NULL )
   {
      return;
   }

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   CollectionIndexType nExporters = m_pPluginMgr->GetExporterCount();
   if ( nExporters == 0 )
   {
      pCmdUI->SetText(_T("Custom exporters not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      CollectionIndexType idx;
      for ( idx = 0; idx < nExporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+(UINT)idx,MF_BYCOMMAND);
      }

      for ( idx = 0; idx < nExporters; idx++ )
      {
         CComPtr<IPGSuperDataExporter> exporter;
         m_pPluginMgr->GetPGSuperExporter(idx,true,&exporter);

         UINT cmdID = m_pPluginMgr->GetPGSuperExporterCommand(idx);

         CComBSTR bstrMenuText;
         exporter->GetMenuText(&bstrMenuText);

         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_pPluginMgr->GetPGSuperExporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,NULL);

         pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();
         pCmdUI->m_nIndex++;
      }
   }

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSuperDocBase::OnImport(UINT nID)
{
   CComPtr<IPGSuperDataImporter> importer;
   m_pPluginMgr->GetPGSuperImporter(nID,false,&importer);

   if ( importer )
   {
      importer->Import(m_pBroker);
   }
}

void CPGSuperDocBase::OnExport(UINT nID)
{
   CComPtr<IPGSuperDataExporter> exporter;
   m_pPluginMgr->GetPGSuperExporter(nID,false,&exporter);

   if ( exporter )
   {
      exporter->Export(m_pBroker);
   }
}

void CPGSuperDocBase::OnAbout()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   UINT resourceID = pTemplate->GetResourceID();

   CAboutDlg dlg(resourceID);
   dlg.DoModal();
}

UINT CPGSuperDocBase::GetBridgeEditorSettings() const
{
   return m_BridgeModelEditorSettings;
}

void CPGSuperDocBase::SetBridgeEditorSettings(UINT settings)
{
   m_BridgeModelEditorSettings = settings;
}

UINT CPGSuperDocBase::GetGirderEditorSettings() const
{
   return m_GirderModelEditorSettings;
}

void CPGSuperDocBase::SetGirderEditorSettings(UINT settings)
{
   m_GirderModelEditorSettings = settings;
}

UINT CPGSuperDocBase::GetUIHintSettings() const
{
   return m_UIHintSettings;
}

void CPGSuperDocBase::SetUIHintSettings(UINT settings)
{
   m_UIHintSettings = settings;
   if ( m_UIHintSettings == UIHINT_ENABLE_ALL )
   {
      m_pPGSuperDocProxyAgent->OnResetHints();
   }
}

bool CPGSuperDocBase::ShowProjectPropertiesOnNewProject()
{
   return m_bShowProjectProperties;
}

void CPGSuperDocBase::ShowProjectPropertiesOnNewProject(bool bShow)
{
   m_bShowProjectProperties = bShow;
}

void CPGSuperDocBase::DeleteContents()
{
   if ( m_strAppProfileName != _T("") )
   {
      // this method is called from OnNewDocument... don't want to mess with the profile name
      // if it hasn't been changed
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CWinApp* pMyApp = AfxGetApp();

      free((void*)pMyApp->m_pszProfileName);
      pMyApp->m_pszProfileName = _tcsdup(m_strAppProfileName);
   }

   __super::DeleteContents();
}

long CPGSuperDocBase::GetReportViewKey()
{
   return m_pPGSuperDocProxyAgent->GetReportViewKey();
}


void CPGSuperDocBase::OnChangedFavoriteReports(bool isFavorites,bool fromMenu)
{
   // Prompt user with hint about how this menu item works
   if (fromMenu)
   {
      int mask = UIHINT_FAVORITES_MENU;
      Uint32 hintSettings = GetUIHintSettings();
      if ( sysFlags<Uint32>::IsClear(hintSettings,mask) )
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState());

         CUIHintsDlg dlg;
         dlg.m_strTitle = _T("Hint");
         dlg.m_strText = _T("This menu item allows you to display only your favorite reports in the Reports menus, or display all available reports. The change will occur the next time you open a Report menu.");
         dlg.DoModal();
         if ( dlg.m_bDontShowAgain )
         {
            sysFlags<Uint32>::Set(&hintSettings,mask);
            SetUIHintSettings(hintSettings);
         }
      }
   }

   // update main menu submenu
   PopulateReportMenu();
}

void CPGSuperDocBase::OnCustomReportError(custReportErrorType error, const std::_tstring& reportName, const std::_tstring& otherName)
{
   std::_tostringstream os;

   switch(error)
   {
      case creParentMissingAtLoad:
         os << _T("For custom report \"")<<reportName<<_T("\": the parent report ")<<otherName<<_T(" could not be found at program load time. The custom report was deleted.");
         break;
      case creParentMissingAtImport:
         os << _T("For custom report \"")<<reportName<<_T("\": the parent report ")<<otherName<<_T(" could not be found. The report may have depended on one of PGSuper's plug-ins. The custom report was deleted.");
         break;
      case creChapterMissingAtLoad:
      case creChapterMissingAtImport:
         os << _T("For custom report \"")<<reportName<<_T("\": the following chapter ")<<otherName<<_T(" does not exist in the pareent report. The chapter was removed. Perhaps the chapter name changed? You may want to edit the report.");
         break;
      default:
         ATLASSERT(false);
   };

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,os.str().c_str());
   pStatusCenter->Add(pStatusItem);
}

void CPGSuperDocBase::OnCustomReportHelp(custRepportHelpType helpType)
{
   UINT helpID = helpType==crhCustomReport ? IDH_CUSTOM_REPORT : IDH_FAVORITE_REPORT;

   ::HtmlHelp( NULL, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, helpID );
}