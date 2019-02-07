///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include "PGSuperDoc.h"
#include "PGSuperApp.h"
#include "PGSuperBaseAppPlugin.h"

#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFAutoProgress.h>

#include <MFCTools\AutoRegistry.h>

// Dialogs
#include "SelectGirderDlg.h"
#include "GirderDescDlg.h"
#include "DesignGirderDlg.h"
#include "DesignOutcomeDlg.h"
#include "StructuralAnalysisMethodDlg.h"

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
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECT, OnProjectDesignGirderDirect)
   ON_COMMAND(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnProjectDesignGirderDirectHoldSlabOffset)
   ON_UPDATE_COMMAND_UI(ID_PROJECT_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnUpdateProjectDesignGirderDirectHoldSlabOffset)
	ON_COMMAND(ID_PROJECT_ANALYSIS, OnProjectAnalysis)
	ON_COMMAND(ID_EDIT_HAUNCH, OnEditHaunch)
   ON_UPDATE_COMMAND_UI(ID_EDIT_HAUNCH,OnUpdateEditHaunch)
	ON_COMMAND(ID_EDIT_HAUNCH, OnEditHaunch)
	ON_COMMAND(ID_EDIT_BEARING, OnEditBearing)
   ON_UPDATE_COMMAND_UI(ID_EDIT_BEARING,OnUpdateEditBearing)
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
      newGirderData.m_TimelineMgr = dlg.m_TimelineMgr;

      // copy original girder and then modify the data that changed
      newGirderData.m_Girder = *pGirder;
      pgsTypes::TopWidthType type;
      Float64 leftStart, rightStart, leftEnd, rightEnd;
      dlg.m_Girder.GetTopWidth(&type, &leftStart, &rightStart, &leftEnd, &rightEnd);
      newGirderData.m_Girder.SetTopWidth(type,leftStart,rightStart,leftEnd,rightEnd);
      *newGirderData.m_Girder.GetSegment(segmentKey.segmentIndex) = *dlg.GetSegment();

      newGirderData.m_SlabOffsetType = dlg.m_General.m_SlabOffsetType;
      newGirderData.m_SlabOffset[pgsTypes::metStart] = dlg.m_General.m_SlabOffset[pgsTypes::metStart];
      newGirderData.m_SlabOffset[pgsTypes::metEnd]   = dlg.m_General.m_SlabOffset[pgsTypes::metEnd];

      newGirderData.m_AssumedExcessCamberType = dlg.m_General.m_AssumedExcessCamberType;
      newGirderData.m_AssumedExcessCamber = dlg.m_General.m_AssumedExcessCamber;

      newGirderData.m_strGirderName = dlg.m_strGirderName;

      newGirderData.m_Girder.SetConditionFactorType(dlg.GetConditionFactorType());
      newGirderData.m_Girder.SetConditionFactor(dlg.GetConditionFactor());

      newGirderData.m_BearingType = dlg.m_SpanGdrDetailsBearingsPage.m_BearingInputData.m_BearingType;
      newGirderData.m_BearingData[pgsTypes::metStart] = dlg.m_SpanGdrDetailsBearingsPage.m_Bearings[0];
      newGirderData.m_BearingData[pgsTypes::metEnd] = dlg.m_SpanGdrDetailsBearingsPage.m_Bearings[1];

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

void CPGSuperDoc::DesignGirder(bool bPrompt,arSlabOffsetDesignType designSlabOffset,const CGirderKey& girderKey)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE_NOCHECK(IBridge, pBridge);
   GET_IFACE(ISectionProperties, pSectProp);
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      AfxMessageBox(_T("Prestress losses are computed by the time-step method. Girder design is not available for this prestress loss method."),MB_OK);
      return;
   }

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   if ( pStatusCenter->GetSeverity() == eafTypes::statusError )
   {
      AfxMessageBox(_T("There are errors that must be corrected before you can design a girder\r\n\r\nSee the Status Center for details."),MB_OK);
      return;
   }

   // We cannot design directly if transformed sections or non-prismatic haunch section properties are specified. Inform user if this is the case.
   CString noDesignMsg;
   if (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmTransformed)
   {
      noDesignMsg = (_T("The Project Criteria specifies an analysis based on Transformed Sections. "));
   }

   if (pSectProp->GetHaunchAnalysisSectionPropertiesType() == pgsTypes::hspVariableParabolic && IsStructuralDeck(pBridge->GetDeckType()))
   {
      if (!noDesignMsg.IsEmpty())
      {
         noDesignMsg += _T("Also, ");
      }

      noDesignMsg += _T("The Project Criteria specifies an analysis based on non-prismatic composite section properties accounting for Parabolic Haunch depth. ");
   }

   if (!noDesignMsg.IsEmpty())
   {
      CString msg;
      msg.Format(_T("Warning: %s \n\nThese options are not accurately accounted for by the automated design algorithm. For this reason, girder designs are performed using gross section properties and a constant haunch depth.\n\nThe resulting design may not be optimal. Be sure to verify design results using a Spec Check.\n\nWould you like to proceed?"),noDesignMsg);
      if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) != IDYES)
      {
         return;
      }
   }

   CGirderKey thisGirderKey = girderKey;
   thisGirderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   thisGirderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   // set up default design options
   GET_IFACE(ISpecification,pSpec);
   bool can_design_Adim    = pSpec->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;

   if (!can_design_Adim)
   {
      m_DesignSlabOffset = sodNoSlabOffsetDesign; // only show design A if it's possible
   }
   else
   {
      m_DesignSlabOffset = designSlabOffset;
   }

   std::vector<CGirderKey> girderKeys;
   if ( bPrompt )
   {
      // only show A design option if it's allowed in the library
      // Do not save this in registry because library selection should be default for new documents

      CDesignGirderDlg dlg(thisGirderKey.groupIndex, 
                           thisGirderKey.girderIndex,
                           can_design_Adim, m_DesignSlabOffset, m_pBroker);

      // Set initial dialog values based on last stored in registry. These may be changed
      // internally by dialog based on girder type, and other library values
      dlg.m_DesignForFlexure = (IsDesignFlexureEnabled() ? TRUE : FALSE);
      dlg.m_DesignForShear   = (IsDesignShearEnabled()   ? TRUE : FALSE);
      dlg.m_StartWithCurrentStirrupLayout = (IsDesignStirrupsFromScratchEnabled() ? FALSE : TRUE);

      if ( dlg.DoModal() == IDOK )
      {
         EnableDesignFlexure(dlg.m_DesignForFlexure == TRUE ? true : false);
         EnableDesignShear(  dlg.m_DesignForShear   == TRUE ? true : false);
         EnableDesignStirrupsFromScratch( dlg.m_StartWithCurrentStirrupLayout==TRUE ? false : true);
         m_DesignSlabOffset = dlg.m_DesignSlabOffset; // retain value for current document

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

   DoDesignGirder(girderKeys, m_DesignSlabOffset);
}

void CPGSuperDoc::OnProjectDesignGirder() 
{
   DesignGirder(true,m_DesignSlabOffset,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::OnUpdateProjectDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI)
{
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE_NOCHECK(IBridge,pBridge); // short circuit evaluation may cause this interface to be unused
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   pCmdUI->Enable( bDesignSlabOffset );
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

void CPGSuperDoc::OnProjectDesignGirderDirect()
{
   DesignGirder(false,sodSlabOffsetandAssumedExcessCamberDesign,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::OnProjectDesignGirderDirectHoldSlabOffset()
{
   DesignGirder(false,sodNoSlabOffsetDesign,CGirderKey(m_Selection.GroupIdx,m_Selection.GirderIdx));
}

void CPGSuperDoc::DoDesignGirder(const std::vector<CGirderKey>& girderKeys, arSlabOffsetDesignType designADim)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(ISpecification,pSpecification);
   GET_IFACE(IArtifact,pIArtifact);

   // Make sure we don't fire up any report or view redraws while we are designing
   GET_IFACE(IUIEvents,pIUIEvents);
   CUIEventsHolder eventholder(pIUIEvents);

   std::vector<const pgsGirderDesignArtifact*> pArtifacts;

   auto myGirderKeys(girderKeys); // need to make a copy and work on it in case we have to truncate the list because of design failure

   // Need to scope the following block, otherwise the progress window will carry the cancel
   // and progress buttons past the design outcome and into any reports that need to be generated.
   {
      bool multi = myGirderKeys.size()>1;

      GET_IFACE(IProgress,pProgress);
      DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (multi)
      {
         pProgress->Init(0,(short)myGirderKeys.size(),1);  // and for multi-girders, a gauge.
      }

      // Design all girders in list
      std::vector<CGirderKey>::const_iterator girderKeyIter(myGirderKeys.begin());
      std::vector<CGirderKey>::const_iterator girderKeyIterEnd(myGirderKeys.end());
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

            des_options.doDesignStirrupLayout = IsDesignStirrupsFromScratchEnabled() ?  slLayoutStirrups : slRetainExistingLayout;

            if(IsDesignFlexureEnabled())
            {
               des_options.doDesignSlabOffset = designADim;
            }
            else
            {
               des_options.doDesignForFlexure = dtNoDesign;
            }

            des_options.doDesignForShear = IsDesignShearEnabled();
         }

         // Design the girder
         const pgsGirderDesignArtifact* pArtifact = pIArtifact->CreateDesignArtifact( girderKey, des_options_coll);

         pProgress->Increment();

         if ( pArtifact == nullptr )
         {
            AfxMessageBox(_T("Design Cancelled"),MB_OK);
            return;
         }
         
         if (pArtifact->GetSegmentDesignArtifact(0)->GetOutcome() == pgsSegmentDesignArtifact::DesignNotSupported_Strands)
         {
            CString strMsg;
            strMsg.Format(_T("The design of Span %d Girder %s failed because the strand definition type is not supported for design."), LABEL_SPAN(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex));
            if (1 < myGirderKeys.size())
            {
               strMsg += _T("\nThe design of the remaining girders will skipped.");
               myGirderKeys.erase(girderKeyIter, girderKeyIterEnd);
            }
            AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
            break;
         }
         else
         {
            pArtifacts.push_back(pArtifact);
         }
      }
   }

   if (0 < myGirderKeys.size())
   {
      GET_IFACE(IReportManager, pReportMgr);
      CReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Design Outcome Report"));
      std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
      std::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

      std::shared_ptr<CMultiGirderReportSpecification> pMGRptSpec = std::dynamic_pointer_cast<CMultiGirderReportSpecification, CReportSpecification>(pRptSpec);

      pMGRptSpec->SetGirderKeys(myGirderKeys);

      CDesignOutcomeDlg dlg(pMGRptSpec, myGirderKeys, designADim);
      if (dlg.DoModal() == IDOK)
      {
         SlabOffsetDesignSelectionType slabOffsetDType;
         SpanIndexType fromSpan;
         GirderIndexType fromGirder;
         bool doDesignSO = dlg.GetSlabOffsetDesign(&slabOffsetDType, &fromSpan, &fromGirder);

         // Create our transaction and execute
         GET_IFACE(IEAFTransactions, pTransactions);

         txnDesignGirder* pTxn = new txnDesignGirder(pArtifacts, slabOffsetDType, fromSpan, fromGirder);

         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSuperDoc::LoadDocumentSettings()
{
   CPGSDocBase::LoadDocumentSettings();

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(),pApp);

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

void CPGSuperDoc::ModifyTemplate(LPCTSTR strTemplate)
{
   CPGSDocBase::ModifyTemplate(strTemplate);
}
