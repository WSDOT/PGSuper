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

// PGSpliceDoc.cpp : implementation of the CPGSpliceDoc class
//
#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSpliceDoc.h"
#include "PGSuperDocProxyAgent.h"

// Dialogs
#include "SelectGirderDlg.h"
#include "SplicedGirderDescDlg.h"
#include "PGSuperAppPlugin\SelectGirderSegmentDlg.h"
#include "PGSuperAppPlugin\SelectClosureJointDlg.h"
#include "PGSuperAppPlugin\GirderSegmentDlg.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"
#include "PGSuperAppPlugin\EditTimelineDlg.h"
#include "PGSuperAppPlugin\CastClosureJointDlg.h" // for Temporary Support labeling
#include "PGSuperAppPlugin\ClosureJointDlg.h"

// Transactions
#include "PGSuperAppPlugin\EditClosureJoint.h"
#include "PGSuperAppPlugin\EditTemporarySupport.h"
#include "PGSuperAppPlugin\InsertDeleteTemporarySupport.h"
#include "PGSuperAppPlugin\EditGirderline.h"
#include "PGSuperAppPlugin\EditPrecastSegment.h"
#include "PGSuperAppPlugin\EditTimeline.h"

// Interfaces
#include <EAF\EAFTransactions.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h> // for EDG_GENERAL


#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPGSpliceDoc

IMPLEMENT_DYNCREATE(CPGSpliceDoc, CPGSuperDocBase)

BEGIN_MESSAGE_MAP(CPGSpliceDoc, CPGSuperDocBase)
	//{{AFX_MSG_MAP(CPGSpliceDoc)
	ON_COMMAND(ID_PROJECT_BRIDGEDESC, OnEditBridgeDescription)
   ON_COMMAND_RANGE(ID_EDIT_SEGMENT,ID_EDIT_SEGMENT_MAX,OnEditSegment)
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(ID_EDIT_CLOSURE, OnEditClosureJoint)
   ON_UPDATE_COMMAND_UI(ID_EDIT_CLOSURE,OnUpdateEditClosureJoint)
	ON_COMMAND(ID_EDIT_GIRDERLINE, OnEditGirderline)
   ON_COMMAND(ID_INSERT_TEMPORARY_SUPPORT,OnInsertTemporarySupport)
   ON_UPDATE_COMMAND_UI(ID_EDIT_TEMPORARY_SUPPORT,OnUpdateEditTemporarySupport)
   ON_COMMAND(ID_EDIT_TEMPORARY_SUPPORT,OnEditTemporarySupport)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
   ON_COMMAND(ID_DELETE_TEMPORARY_SUPPORT,OnDeleteTemporarySupport)
   ON_COMMAND(ID_EDIT_TIMELINE,OnEditTimeline)
   //}}AFX_MSG_MAP

   // this doesn't work for documents... see OnCmdMsg for handling of WM_NOTIFY
   //ON_NOTIFY(TBN_DROPDOWN,ID_STDTOOLBAR,OnEditGirderDropDown)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSpliceDoc construction/destruction

CPGSpliceDoc::CPGSpliceDoc()
{
}

CPGSpliceDoc::~CPGSpliceDoc()
{
}

BOOL CPGSpliceDoc::Init()
{
   GetComponentInfoManager()->SetParent(this);
   GetComponentInfoManager()->SetCATID(GetComponentInfoCategoryID());
   GetComponentInfoManager()->LoadPlugins();

   return CPGSuperDocBase::Init();
}

#ifdef _DEBUG
void CPGSpliceDoc::AssertValid() const
{
   __super::AssertValid();
}

void CPGSpliceDoc::Dump(CDumpContext& dc) const
{
   __super::Dump(dc);
}
#endif // _DEBUG

BOOL CPGSpliceDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
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
           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_EDIT_GIRDER )
              return OnEditGirderDropDown(notify->pNMHDR,notify->pResult); 
        }
    }
	
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPGSpliceDoc::DoIntegrateWithUI(BOOL bIntegrate)
{
   __super::DoIntegrateWithUI(bIntegrate);

   // Add a drop down button for girder edit
   if ( bIntegrate )
   {
      UINT nID = m_pPGSuperDocProxyAgent->GetStdToolBarID();

      GET_IFACE(IEAFToolbars,pToolBars);
      CEAFToolBar* pToolBar = pToolBars->GetToolBar(nID);
      pToolBar->CreateDropDownButton(ID_EDIT_GIRDER,NULL,BTNS_WHOLEDROPDOWN);
   }
}

void CPGSpliceDoc::OnDeleteSelection() 
{
   if (  m_Selection.Type == CSelection::TemporarySupport )
   {
      DeleteTemporarySupport(m_Selection.tsID);
   }
   else
   {
      __super::OnDeleteSelection();
   }
}

void CPGSpliceDoc::OnUpdateDeleteSelection(CCmdUI* pCmdUI) 
{
   if ( m_Selection.Type == CSelection::TemporarySupport )
   {
      CString strLabel;
      strLabel.Format(_T("Delete Temporary Support %d"),LABEL_TEMPORARY_SUPPORT(m_Selection.tsID));
      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      __super::OnUpdateDeleteSelection(pCmdUI);
   }
}

BOOL CPGSpliceDoc::OnEditGirderDropDown(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the edit choices on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_EDIT_GIRDER )
      return FALSE; // not our button

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_EDIT_GIRDER) );
   CMenu* pMenu = menu.GetSubMenu(0);
   //pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_EDIT_GIRDER,NULL);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
}

void CPGSpliceDoc::OnEditSegment(UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GroupIndexType  grpIdx  = m_Selection.GroupIdx;
   GirderIndexType gdrIdx  = m_Selection.GirderIdx;
   SegmentIndexType segIdx = SegmentIndexType(nID - ID_EDIT_SEGMENT);

   CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

   EditGirderSegmentDescription(segmentKey,EGS_GENERAL);
}

void CPGSpliceDoc::OnEditGirder()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSelection selection = GetSelection();
   
   BOOL bEdit = TRUE;
   if ( selection.Type != CSelection::Segment )
   {
      CSelectGirderSegmentDlg dlg;
      dlg.m_Group   = selection.GroupIdx   == INVALID_INDEX ? 0 : selection.GroupIdx;
      dlg.m_Girder  = selection.GirderIdx  == INVALID_INDEX ? 0 : selection.GirderIdx;
      dlg.m_Segment = selection.SegmentIdx == INVALID_INDEX ? 0 : selection.SegmentIdx;

      if ( dlg.DoModal() != IDOK )
         bEdit = FALSE;

      selection.GroupIdx   = dlg.m_Group;
      selection.GirderIdx  = dlg.m_Girder;
      selection.SegmentIdx = dlg.m_Segment;
   }

   CSegmentKey segmentKey(selection.GroupIdx,selection.GirderIdx,selection.SegmentIdx);

   if ( bEdit )
      EditGirderSegmentDescription(segmentKey,EGS_GENERAL);
}

void CPGSpliceDoc::OnEditClosureJoint()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSelection selection = GetSelection();

   CSegmentKey closureKey;
   
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   if ( selection.Type == CSelection::ClosureJoint )
   {
      closureKey.groupIndex   = selection.GroupIdx;
      closureKey.girderIndex  = selection.GirderIdx;
      closureKey.segmentIndex = selection.SegmentIdx;
   }
   else
   {
      // a closure joint is not selected
      // ask to use which one to edit
      CSelectClosureJointDlg dlg(pBridgeDesc);
      dlg.m_GirderIdx     = selection.GirderIdx == INVALID_INDEX ? 0 : selection.GirderIdx;
      dlg.m_PierIdx       = selection.PierIdx;
      dlg.m_TempSupportID = selection.tsID;

      if ( dlg.m_PierIdx == INVALID_INDEX && dlg.m_TempSupportID == INVALID_ID )
      {
         // The default parameters aren't valid. Find the first object with a closure joint
         // and use it as the default
         SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
         for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
         {
            const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
            if ( pTS->GetClosureJoint(dlg.m_GirderIdx) )
            {
               dlg.m_TempSupportID = pTS->GetID();
               break;
            }
         }

         if ( dlg.m_TempSupportID == INVALID_ID )
         {
            // didn't find a Temporary support... search for a pier
            PierIndexType nPiers = pBridgeDesc->GetPierCount();
            for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
            {
               const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
               if ( pPier->GetClosureJoint(dlg.m_GirderIdx) )
               {
                  dlg.m_PierIdx = pPier->GetIndex();
                  break;
               }
            }
         }

         // if there isn't anything valid, the something is messed up. The UI shouldn't have
         // permitted the user to get here
         ATLASSERT( dlg.m_PierIdx != INVALID_INDEX || dlg.m_TempSupportID != INVALID_ID );
      }


      // ask the user
      if ( dlg.DoModal() != IDOK )
         return;

      selection.GirderIdx = dlg.m_GirderIdx;
      if (dlg.m_TempSupportID != INVALID_ID )
      {
         const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(dlg.m_TempSupportID);
         const CClosureJointData* pClosure = pTS->GetClosureJoint(dlg.m_GirderIdx);
         closureKey = pClosure->GetClosureKey();
      }
      else
      {
         const CPierData2* pPier = pBridgeDesc->GetPier(dlg.m_PierIdx);
         const CClosureJointData* pClosure = pPier->GetClosureJoint(dlg.m_GirderIdx);
         closureKey = pClosure->GetClosureKey();
      }
   }

   const CClosureJointData* pClosure = pBridgeDesc->GetClosureJoint(closureKey);
   ATLASSERT(pClosure != NULL);
   EditClosureJointDescription(pClosure,EGS_GENERAL);
}

void CPGSpliceDoc::OnEditGirderline()
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
      EditGirderDescription(CGirderKey(selection.GroupIdx,selection.GirderIdx),EGS_GENERAL);
   }
}

void CPGSpliceDoc::OnEditTemporarySupport()
{
   SupportIDType tsID = INVALID_ID;
   CSelection selection = GetSelection();
   if ( selection.Type == CSelection::TemporarySupport )
   {
      tsID = selection.tsID;
   }
   else
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      CString strChoices;
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

         CString strChoice;
         if ( tsIdx == 0 )
            strChoice = GetLabel(pTS,pDisplayUnits);
         else
            strChoice.Format(_T("\n%s"),GetLabel(pTS,pDisplayUnits));

         strChoices += strChoice;
      }

      int result = AfxChoose(_T("Select Temporary Support"),_T("Select temporary support to edit"),strChoices,0,TRUE);
      if ( 0 <= result )
         tsID = pBridgeDesc->GetTemporarySupport(result)->GetID();
   }

   if ( tsID != INVALID_ID )
      EditTemporarySupportDescription(tsID,ETSD_GENERAL);
}

void CPGSpliceDoc::OnUpdateEditTemporarySupport(CCmdUI* pCmdUI)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();

   pCmdUI->Enable(0 < nTS);
}

void CPGSpliceDoc::OnUpdateEditClosureJoint(CCmdUI* pCmdUI)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   IndexType nClosures = pBridgeDesc->GetClosureJointCount();

   pCmdUI->Enable(0 < nClosures);
}

void CPGSpliceDoc::OnInsertTemporarySupport()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CTemporarySupportDlg dlg(pBridgeDesc,INVALID_INDEX,EAFGetMainFrame());
   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnInsertTemporarySupport(dlg.GetTemporarySupport(),*pBridgeDesc,*dlg.GetBridgeDescription());

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn != NULL )
      {
         txnMacroTxn* pMacro = new txnMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSpliceDoc::OnDeleteTemporarySupport()
{
   ATLASSERT(m_Selection.Type == CSelection::TemporarySupport);
   if ( m_Selection.Type == CSelection::TemporarySupport )
   {
      DeleteTemporarySupport(m_Selection.tsID);
   }
}

void CPGSpliceDoc::OnEditTimeline()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CEditTimelineDlg dlg;
   dlg.m_TimelineManager = *pBridgeDesc->GetTimelineManager();

   if ( dlg.DoModal() == IDOK )
   {
      txnEditTimeline* pTxn = new txnEditTimeline(*pBridgeDesc->GetTimelineManager(),dlg.m_TimelineManager);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSpliceDoc::OnEditBridgeDescription() 
{
   EditBridgeDescription(0); // open to first page
}

bool CPGSpliceDoc::EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   CGirderSegmentDlg dlg(pBridgeDesc,segmentKey,EAFGetMainFrame(),nPage);


#pragma Reminder("UPDATE: Clean up handling of shear data")
   // Shear data is kind of messy. It is the only data on the segment that we have to
   // set on the dialog and then get it for the transaction. This has to do with
   // the way RDP wrote the class for the new shear designer. Update the dialog
   // so it works seamlessly for all cases
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   dlg.m_StirrupsPage.m_ShearData = pSegment->ShearData;

   SegmentIDType segID = pSegment->GetID();

   dlg.m_ConstructionEventIdx = pIBridgeDesc->GetTimelineManager()->GetSegmentConstructionEventIndex(segID);
   dlg.m_ErectionEventIdx     = pIBridgeDesc->GetTimelineManager()->GetSegmentErectionEventIndex(segID);

   if ( dlg.DoModal() == IDOK )
   {
      CPrecastSegmentData* pNewSegment = dlg.m_Girder.GetSegment(segmentKey.segmentIndex);
#pragma Reminder("UPDATE: why do we have to get the stirrup data after we get the segment")
      pNewSegment->ShearData = dlg.m_StirrupsPage.m_ShearData;

      txnEditPrecastSegmentData newData;
      newData.m_SegmentKey           = segmentKey;
      newData.m_SegmentData          = *pNewSegment;
      newData.m_ConstructionEventIdx = dlg.m_ConstructionEventIdx;
      newData.m_ErectionEventIdx     = dlg.m_ErectionEventIdx;

      CSegmentKey thisSegmentKey(segmentKey);
      if ( dlg.m_bCopyToAll )
      {
         thisSegmentKey.girderIndex = ALL_GIRDERS;
      }

      txnEditPrecastSegment* pTxn = new txnEditPrecastSegment(thisSegmentKey,newData);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
   
   return true;
}

bool CPGSpliceDoc::EditClosureJointDescription(const CClosureKey& closureKey,int nPage)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);
   return EditClosureJointDescription(pClosure,nPage);
}

bool CPGSpliceDoc::EditClosureJointDescription(const CClosureJointData* pClosure,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   CSegmentKey closureKey(pClosure->GetClosureKey());

   EventIndexType closureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(closureKey.groupIndex,closureKey.segmentIndex);

   CClosureJointData ClosureJoint(*pClosure);
   CClosureJointDlg dlg(closureKey,pClosure,closureEventIdx);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditClosureJointData newData;
      newData.m_PierIdx         = pClosure->GetPierIndex();
      newData.m_TSIdx           = pClosure->GetTemporarySupportIndex();
      newData.m_ClosureJoint    = dlg.m_ClosureJoint;
      newData.m_ClosureEventIdx = dlg.m_EventIndex;

      if ( dlg.m_bCopyToAllClosureJoints )
      {
         closureKey.girderIndex = ALL_GIRDERS;
      }

      txnEditClosureJoint* pTxn = new txnEditClosureJoint(closureKey,newData);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSpliceDoc::EditGirderDescription(const CGirderKey& girderKey,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSplicedGirderDescDlg dlg(girderKey);

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      txnTransaction* pTxn = new txnEditGirderline(girderKey,dlg.m_bApplyToAll,*pBridgeDesc,dlg.m_BridgeDescription);

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         txnMacroTxn* pMacro = new txnMacroTxn;
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

void CPGSpliceDoc::DeleteTemporarySupport(SupportIDType tsID)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   CBridgeDescription2 oldBridgeDesc = *pBridgeDesc;
   CBridgeDescription2 newBridgeDesc = *pBridgeDesc;

   const CTemporarySupportData* pTS = newBridgeDesc.FindTemporarySupport(tsID);
   SupportIndexType tsIdx = pTS->GetIndex();
   newBridgeDesc.RemoveTemporarySupportByIndex(tsIdx);

   txnDeleteTemporarySupport* pTxn = new txnDeleteTemporarySupport(tsIdx,oldBridgeDesc,newBridgeDesc);
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

bool CPGSpliceDoc::EditTemporarySupportDescription(SupportIDType tsID,int nPage)
{
#pragma Reminder("UPDATE: move temporary support editing to the base document class")
   // We want to be able to eventually handle temporary shoring towers in PGSuper
   // so the editing needs to be at that level

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
   SupportIndexType tsIdx = pTS->GetIndex();

   CTemporarySupportDlg dlg(pBridgeDesc,tsIdx);
   dlg.SetActivePage(nPage);
   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnEditTemporarySupport(tsIdx,*pBridgeDesc,*dlg.GetBridgeDescription());

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn != NULL )
      {
         txnMacroTxn* pMacro = new txnMacroTxn;
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

UINT CPGSpliceDoc::GetStandardToolbarResourceID()
{
   return IDR_PGSPLICE_STDTOOLBAR;
}

LPCTSTR CPGSpliceDoc::GetTemplateExtension()
{
   return _T(".spt");
}

BOOL CPGSpliceDoc::InitMainMenu()
{
   if ( ! __super::InitMainMenu() )
      return FALSE;

   // PGSplice Documents don't use the concentrated moment load
   // Remove it from the Loads menu
   CEAFMenu* pMainMenu = GetMainMenu();
   UINT position = pMainMenu->FindMenuItem(_T("L&oads"));
   CEAFMenu* pLoadMenu = pMainMenu->GetSubMenu(position);
   pLoadMenu->RemoveMenu(ID_ADD_MOMENT_LOAD,MF_BYCOMMAND,NULL);

   return TRUE;
}
