///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include "PGSuperDoc.h"
#include "PGSuperAppPlugin\PGSuperApp.h"

#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFAutoProgress.h>

// Dialogs
#include "SelectGirderDlg.h"
#include "GirderDescDlg.h"
#include "DesignGirderDlg.h"
#include "DesignOutcomeDlg.h"
#include "StructuralAnalysisMethodDlg.h"
#include "PGSuperAppPlugin\EditHaunchDlg.h"

// Interfaces
#include <IFace\EditByUI.h> // for EDG_GENERAL
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Transactions.h>
#include <IFace\StatusCenter.h>

// Transactions
#include "EditGirder.h"
#include "DesignGirder.h"
#include "EditAnalysisType.h"
#include <PgsExt\MacroTxn.h>
#include <PgsExt\EditBridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc

IMPLEMENT_DYNCREATE(CPGSuperDoc, CPGSDocBase)

BEGIN_MESSAGE_MAP(CPGSuperDoc, CPGSDocBase)
	//{{AFX_MSG_MAP(CPGSuperDoc)
	ON_COMMAND(ID_PROJECT_BRIDGEDESC, OnEditBridgeDescription)
	ON_COMMAND(ID_EDIT_SEGMENT, OnEditGirder)
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(ID_PROJECT_DESIGNGIRDER, OnProjectDesignGirder)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDER, OnUpdateProjectDesignGirderDirect)
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECT, OnProjectDesignGirderDirect)
   ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDERDIRECT, OnUpdateProjectDesignGirderDirect)
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnProjectDesignGirderDirectHoldSlabOffset)
   ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnUpdateProjectDesignGirderDirectHoldSlabOffset)
	ON_COMMAND(ID_PROJECT_ANALYSIS, OnProjectAnalysis)
	ON_COMMAND(ID_EDIT_HAUNCH, OnEditHaunch)
   ON_UPDATE_COMMAND_UI(ID_EDIT_HAUNCH,OnUpdateEditHaunch)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDoc construction/destruction

CPGSuperDoc::CPGSuperDoc()
{
}

CPGSuperDoc::~CPGSuperDoc()
{
}

BOOL CPGSuperDoc::Init()
{
   GetComponentInfoManager()->SetParent(this);
   GetComponentInfoManager()->SetCATID(GetComponentInfoCategoryID());
   GetComponentInfoManager()->LoadPlugins();

   return CPGSDocBase::Init();
}

#ifdef _DEBUG
void CPGSuperDoc::AssertValid() const
{
   __super::AssertValid();
}

void CPGSuperDoc::Dump(CDumpContext& dc) const
{
   __super::Dump(dc);
}
#endif // _DEBUG


void CPGSuperDoc::OnEditBridgeDescription() 
{
   EditBridgeDescription(0); // open to first page
}

void CPGSuperDoc::OnEditGirder() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSelection selection = GetSelection();
   BOOL bEdit = TRUE;
   if ( 
      ( (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment) && 
        (selection.GroupIdx == INVALID_INDEX || selection.GirderIdx == INVALID_INDEX)) 
        ||
      (selection.Type == CSelection::None || 
       selection.Type == CSelection::Pier || 
       selection.Type == CSelection::Span || 
       selection.Type == CSelection::TemporarySupport || 
       selection.Type == CSelection::Deck || 
       selection.Type == CSelection::Alignment) 
      )
   {
      CSelectGirderDlg dlg(m_pBroker);
      dlg.m_Group  = m_Selection.GroupIdx  == ALL_GROUPS  ? 0 : m_Selection.GroupIdx;
      dlg.m_Girder = m_Selection.GirderIdx == ALL_GIRDERS ? 0 : m_Selection.GirderIdx;

      if ( dlg.DoModal() == IDOK )
      {
         selection.GroupIdx  = dlg.m_Group;
         selection.GirderIdx = dlg.m_Girder;
      }
      else
      {
         bEdit = FALSE;
      }
   }

   if ( bEdit )
   {
      EditGirderSegmentDescription(CSegmentKey(selection.GroupIdx,selection.GirderIdx,0),EGD_GENERAL);
   }
}

bool CPGSuperDoc::EditGirderDescription(const CGirderKey& girderKey,int nPage)
{
   return EditGirderSegmentDescription(CSegmentKey(girderKey,0),nPage);
}

bool CPGSuperDoc::EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // For precast girder bridges, a girder has exactly one precast segment. This edit modifies
   // both the CSplicedGirderData and the CPrecastSegmentData objects.
   ATLASSERT(segmentKey.segmentIndex == 0);

   // collect current values for later undo
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // resequence page if no debonding
   bool bExtraPage = pSegment->Strands.IsSymmetricDebond() || pSpecEntry->AllowStraightStrandExtensions();
   if (EGD_DEBONDING <= nPage  && !bExtraPage)
   {
      nPage--;
   }


   CGirderDescDlg dlg(pBridgeDesc,segmentKey);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if ( st == IDOK )
   {
      CGirderKey girderKey(segmentKey);
      if ( dlg.m_bApplyToAll )
      {
         // apply the changes to all girders in this group
         girderKey.girderIndex = ALL_GIRDERS;
      }

      // collect up the new data
      txnEditGirderData newGirderData;
      newGirderData.m_bUseSameGirder = dlg.m_General.m_bUseSameGirderType;

      // copy original girder and then modify the segment that changed
      newGirderData.m_Girder = *pGirder;
      *newGirderData.m_Girder.GetSegment(segmentKey.segmentIndex) = *dlg.GetSegment();

      newGirderData.m_SlabOffsetType = dlg.m_General.m_SlabOffsetType;
      newGirderData.m_SlabOffset[pgsTypes::metStart] = dlg.m_General.m_SlabOffset[pgsTypes::metStart];
      newGirderData.m_SlabOffset[pgsTypes::metEnd]   = dlg.m_General.m_SlabOffset[pgsTypes::metEnd];

      newGirderData.m_strGirderName = dlg.m_strGirderName;

      newGirderData.m_Girder.SetConditionFactorType(dlg.GetConditionFactorType());
      newGirderData.m_Girder.SetConditionFactor(dlg.GetConditionFactor());

      txnTransaction* pTxn = new txnEditGirder(girderKey,newGirderData);

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

      return true;
   }
   else
   {
      return false;
   }
}

bool CPGSuperDoc::EditClosureJointDescription(const CClosureKey& closureKey,int nPage)
{
   // there aren't any closure joints in a PGSuper model... do nothing
   return true;
}

void CPGSuperDoc::DesignGirder(bool bPrompt,bool bDesignSlabOffset,const CGirderKey& girderKey)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("There are errors that must be corrected before you can design a girder\r\n\r\nSee the Status Center for details."),MB_OK);
      return;
   }

   CGirderKey thisGirderKey = girderKey;
   thisGirderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   thisGirderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   // set up default design options
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE_NOCHECK(IBridge,pBridge);
   bool can_design_Adim    = pSpec->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;

   bDesignSlabOffset = bDesignSlabOffset && can_design_Adim; // only design A if it's possible

   std::vector<CGirderKey> girderKeys;
   if ( bPrompt )
   {
      // only show A design option if it's allowed in the library
      // Do not save this in registry because library selection should be default for new documents

      CDesignGirderDlg dlg(thisGirderKey.groupIndex, 
                           thisGirderKey.girderIndex,
                           can_design_Adim, bDesignSlabOffset, m_pBroker);

      // Set initial dialog values based on last stored in registry. These may be changed
      // internally by dialog based on girder type, and other library values
      dlg.m_DesignForFlexure = (IsDesignFlexureEnabled() ? TRUE : FALSE);
      dlg.m_DesignForShear   = (IsDesignShearEnabled()   ? TRUE : FALSE);
      dlg.m_StartWithCurrentStirrupLayout = (IsDesignStirrupsFromScratchEnabled() ? FALSE : TRUE);

      if ( dlg.DoModal() == IDOK )
      {
         bDesignSlabOffset    = dlg.m_DesignA != FALSE;    // we can override library setting here

         EnableDesignFlexure(dlg.m_DesignForFlexure == TRUE ? true : false);
         EnableDesignShear(  dlg.m_DesignForShear   == TRUE ? true : false);
         EnableDesignStirrupsFromScratch( dlg.m_StartWithCurrentStirrupLayout==TRUE ? false : true);
         m_bDesignSlabOffset = bDesignSlabOffset; // retain value for current document

         girderKeys = dlg.m_GirderKeys;

         if (girderKeys.empty())
         {
            ATLASSERT(false); // dialog should handle this
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
      girderKeys.push_back(thisGirderKey);
   }

   DoDesignGirder(girderKeys, bDesignSlabOffset);
}

void CPGSuperDoc::OnProjectDesignGirder() 
{
   DesignGirder(true,m_bDesignSlabOffset,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirect(CCmdUI* pCmdUI)
{
   GET_IFACE(ISpecification,pSpecification);
   std::_tstring strSpecName(pSpecification->GetSpecification());

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      pCmdUI->Enable(FALSE); // design is not an option with time step losses
   }
   else
   {
      pCmdUI->Enable( TRUE );
   }
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI)
{
   GET_IFACE(ISpecification,pSpecification);
   std::_tstring strSpecName(pSpecification->GetSpecification());

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      pCmdUI->Enable(FALSE); // design is not an option with time step losses
   }
   else
   {
      GET_IFACE_NOCHECK(IBridge,pBridge); // short circuit evaluation may cause this interface to be unused
      bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
      pCmdUI->Enable( bDesignSlabOffset );
   }
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

void CPGSuperDoc::OnEditHaunch() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CEditHaunchDlg dlg(pOldBridgeDesc);
   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IEnvironment, pEnvironment );
      enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
      Float64 oldRelHumidity = pEnvironment->GetRelHumidity();
      CBridgeDescription2 newBridgeDesc = *pOldBridgeDesc;

      // dialog modifies descr
      dlg.ModifyBridgeDescr(&newBridgeDesc);

      txnTransaction* pTxn = new txnEditBridge(*pOldBridgeDesc,     newBridgeDesc,
                                              oldExposureCondition, oldExposureCondition, 
                                              oldRelHumidity,       oldRelHumidity);


      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSuperDoc::OnUpdateEditHaunch(CCmdUI* pCmdUI)
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   pCmdUI->Enable( pBridge->GetDeckType()==pgsTypes::sdtNone ? FALSE : TRUE );
}

void CPGSuperDoc::OnProjectDesignGirderDirect()
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   m_bDesignSlabOffset = bDesignSlabOffset; // retain setting in document

   DesignGirder(false,bDesignSlabOffset,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::OnProjectDesignGirderDirectHoldSlabOffset()
{
   m_bDesignSlabOffset = false; // retain setting in document
   DesignGirder(false,false,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::DoDesignGirder(const std::vector<CGirderKey>& girderKeys, bool doDesignADim)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IArtifact,pIArtifact);

   std::vector<const pgsGirderDesignArtifact*> pArtifacts;

   // Need to scope the following block, otherwise the progress window will carry the cancel
   // and progress buttons past the design outcome and into any reports that need to be generated.
   {
      bool multi = girderKeys.size()>1;

      GET_IFACE(IProgress,pProgress);
      DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (multi)
      {
         pProgress->Init(0,(short)girderKeys.size(),1);  // and for multi-girders, a gauge.
      }

      // Design all girders in list
      std::vector<CGirderKey>::const_iterator girderKeyIter(girderKeys.begin());
      std::vector<CGirderKey>::const_iterator girderKeyIterEnd(girderKeys.end());
      for ( ; girderKeyIter != girderKeyIterEnd; girderKeyIter++ )
      {
         const CGirderKey& girderKey = *girderKeyIter;

         // For each girder we can have multiple design strategies,
         // but only one strategy is needed if we are designing for shear only
         std::vector<arDesignOptions> des_options_coll = pSpecification->GetDesignOptions(girderKey);
         if(!IsDesignFlexureEnabled() && 1 < des_options_coll.size() )
         {
            arDesignOptions tmp_opt = des_options_coll.front();
            des_options_coll.clear();
            des_options_coll.push_back(tmp_opt);  // remove all but one option
         }

         // We must doctor options for selections from dialog
         for(std::vector<arDesignOptions>::iterator it = des_options_coll.begin(); it!=des_options_coll.end(); it++)
         {
            arDesignOptions& des_options = *it;

            des_options.doDesignSlabOffset = doDesignADim;
            des_options.doDesignStirrupLayout = IsDesignStirrupsFromScratchEnabled() ?  slLayoutStirrups : slRetainExistingLayout;

            if(!IsDesignFlexureEnabled())
            {
               des_options.doDesignForFlexure = dtNoDesign;
            }

            des_options.doDesignForShear = IsDesignShearEnabled();
         }

         // Design the girder
         const pgsGirderDesignArtifact* pArtifact = pIArtifact->CreateDesignArtifact( girderKey, des_options_coll);

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

   pMGRptSpec->SetGirderKeys(girderKeys);

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

void CPGSuperDoc::LoadDocumentSettings()
{
   CPGSDocBase::LoadDocumentSettings();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CPGSuperAppPluginApp* pApp = (CPGSuperAppPluginApp*)AfxGetApp();

   // Flexure and stirrup design defaults for design dialog.
   // Default is to design flexure and not shear.
   // If the string is not Off, then assume it is on.
   CString strDefaultDesignFlexure = pApp->GetLocalMachineString(_T("Settings"),_T("DesignFlexure"),_T("On"));
   CString strDesignFlexure = pApp->GetProfileString(_T("Settings"),_T("DesignFlexure"),strDefaultDesignFlexure);
   if ( strDesignFlexure.CompareNoCase(_T("Off")) == 0 )
   {
      m_bDesignFlexureEnabled = false;
   }
   else
   {
      m_bDesignFlexureEnabled = true;
   }

   CString strDefaultDesignShear = pApp->GetLocalMachineString(_T("Settings"),_T("DesignShear"),_T("Off"));
   CString strDesignShear = pApp->GetProfileString(_T("Settings"),_T("DesignShear"),strDefaultDesignShear);
   if ( strDesignShear.CompareNoCase(_T("Off")) == 0 )
   {
      m_bDesignShearEnabled = false;
   }
   else
   {
      m_bDesignShearEnabled = true;
   }

   CString strDefaultDesignStirrupsFromScratch = pApp->GetLocalMachineString(_T("Settings"),_T("DesignStirrupsFromScratch"),_T("On"));
   CString strDesignStirrupsFromScratch = pApp->GetProfileString(_T("Settings"),_T("DesignStirrupsFromScratch"),strDefaultDesignStirrupsFromScratch);
   if ( strDesignStirrupsFromScratch.CompareNoCase(_T("Off")) == 0 )
   {
      m_bDesignStirrupsFromScratchEnabled = false;
   }
   else
   {
      m_bDesignStirrupsFromScratchEnabled = true;
   }
}

void CPGSuperDoc::SaveDocumentSettings()
{
   CPGSDocBase::SaveDocumentSettings();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   // Save the design mode settings
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("DesignFlexure"),m_bDesignFlexureEnabled ? _T("On") : _T("Off") ));
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("DesignShear"),  m_bDesignShearEnabled   ? _T("On") : _T("Off") ));
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("DesignStirrupsFromScratch"),  m_bDesignStirrupsFromScratchEnabled   ? _T("On") : _T("Off") ));
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

bool CPGSuperDoc::IsDesignStirrupsFromScratchEnabled() const
{
   return m_bDesignStirrupsFromScratchEnabled;
}

void CPGSuperDoc::EnableDesignStirrupsFromScratch( bool bEnable )
{
   m_bDesignStirrupsFromScratchEnabled = bEnable;
}

UINT CPGSuperDoc::GetStandardToolbarResourceID()
{
   return IDR_PGSUPER_STDTOOLBAR;
}

LPCTSTR CPGSuperDoc::GetTemplateExtension()
{
   return _T(".pgt");
}
