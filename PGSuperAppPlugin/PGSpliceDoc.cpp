///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "stdafx.h"
#include "resource.h"
#include "PGSpliceDoc.h"
#include "PGSuperDocProxyAgent.h"

// Dialogs
#include "SelectGirderDlg.h"
#include "SplicedGirderDescDlg.h"
#include "SelectGirderSegmentDlg.h"
#include "SelectClosureJointDlg.h"
#include "GirderSegmentDlg.h"
#include "TemporarySupportDlg.h"
#include "EditTimelineDlg.h"
#include "CastClosureJointDlg.h" // for Temporary Support labeling
#include "ClosureJointDlg.h"
#include "CopyTempSupportDlg.h"

// Transactions
#include "EditClosureJoint.h"
#include "EditTemporarySupport.h"
#include "InsertDeleteTemporarySupport.h"
#include "EditGirderline.h"
#include "EditPrecastSegment.h"
#include <PgsExt\MacroTxn.h>

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

IMPLEMENT_DYNCREATE(CPGSpliceDoc, CPGSDocBase)

BEGIN_MESSAGE_MAP(CPGSpliceDoc, CPGSDocBase)
	//{{AFX_MSG_MAP(CPGSpliceDoc)
	ON_COMMAND(ID_PROJECT_BRIDGEDESC, OnEditBridgeDescription)
   ON_COMMAND_RANGE(ID_EDIT_SEGMENT_MIN,ID_EDIT_SEGMENT_MAX,OnEditSegment)
	ON_COMMAND(ID_EDIT_SEGMENT, OnEditSegment)
	ON_COMMAND(ID_EDIT_CLOSURE, OnEditClosureJoint)
   ON_UPDATE_COMMAND_UI(ID_EDIT_CLOSURE,OnUpdateEditClosureJoint)
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
   ON_COMMAND(ID_INSERT_TEMPORARY_SUPPORT,OnInsertTemporarySupport)
   ON_UPDATE_COMMAND_UI(ID_EDIT_TEMPORARY_SUPPORT,OnUpdateEditTemporarySupport)
   ON_COMMAND(ID_EDIT_TEMPORARY_SUPPORT,OnEditTemporarySupport)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
   ON_COMMAND(ID_DELETE_TEMPORARY_SUPPORT,OnDeleteTemporarySupport)
   ON_COMMAND_RANGE(FIRST_COPY_TEMP_SUP_PLUGIN,LAST_COPY_TEMP_SUP_PLUGIN, OnCopyTempSupportProps)
   ON_UPDATE_COMMAND_UI_RANGE(FIRST_COPY_TEMP_SUP_PLUGIN,LAST_COPY_TEMP_SUP_PLUGIN,OnUpdateCopyTempSupportProps)
	ON_UPDATE_COMMAND_UI(ID_COPY_TEMPSUPPORT_PROPS, OnUpdateCopyTempSupportProps)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYTEMPSUPPORTPROPERTIES, OnUpdateCopyTempSupportProps)
   ON_COMMAND(ID_EDIT_COPYTEMPSUPPORTPROPERTIES,OnCopyTempSupportProps)

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

   return CPGSDocBase::Init();
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
           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_EDIT_SEGMENT )
           {
              return OnEditGirderDropDown(notify->pNMHDR,notify->pResult); 
           }

           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_COPY_TEMPSUPPORT_PROPS )
           {
              return OnCopyTempSupportPropsTb(notify->pNMHDR, notify->pResult);
           }
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
      pToolBar->CreateDropDownButton(ID_EDIT_SEGMENT,nullptr,BTNS_WHOLEDROPDOWN);
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
   if ( pnmtb->iItem != ID_EDIT_SEGMENT )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_EDIT_GIRDER) );
   CMenu* pMenu = menu.GetSubMenu(0);

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_EDIT_SEGMENT,nullptr);
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
   SegmentIndexType segIdx = SegmentIndexType(nID - ID_EDIT_SEGMENT_MIN);

   CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

   EditGirderSegmentDescription(segmentKey,EGS_GENERAL);
}

void CPGSpliceDoc::OnEditSegment()
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
      {
         bEdit = FALSE;
      }

      selection.GroupIdx   = dlg.m_Group;
      selection.GirderIdx  = dlg.m_Girder;
      selection.SegmentIdx = dlg.m_Segment;
   }

   CSegmentKey segmentKey(selection.GroupIdx,selection.GirderIdx,selection.SegmentIdx);

   if ( bEdit )
   {
      EditGirderSegmentDescription(segmentKey,EGS_GENERAL);
   }
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

      if (selection.Type == CSelection::Pier)
      {
         // current selection is a pier... make sure it has a closure joint
         const CPierData2* pPier = pBridgeDesc->GetPier(selection.PierIdx);
         if (pPier && pPier->IsInteriorPier() && pPier->GetClosureJoint(dlg.m_GirderIdx))
         {
            dlg.m_PierIdx = pPier->GetIndex();
         }
      }
      else if (selection.Type == CSelection::TemporarySupport)
      {
         // current selection is a temporary support... make sure it has a closure joint
         const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(selection.tsID);
         if (pTS && pTS->GetClosureJoint(dlg.m_GirderIdx))
         {
            dlg.m_TempSupportID = pTS->GetID();
         }
      }

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
               if ( pPier->IsInteriorPier() && pPier->GetClosureJoint(dlg.m_GirderIdx) )
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
      {
         return;
      }

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

   EditClosureJointDescription(closureKey,EGS_GENERAL);
}

void CPGSpliceDoc::OnEditGirder()
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
         {
            strChoice = GetLabel(pTS,pDisplayUnits);
         }
         else
         {
            strChoice.Format(_T("\n%s"),GetLabel(pTS,pDisplayUnits));
         }

         strChoices += strChoice;
      }

      int result = AfxChoose(_T("Select Temporary Support"),_T("Select temporary support to edit"),strChoices,0,TRUE);
      if ( 0 <= result )
      {
         tsID = pBridgeDesc->GetTemporarySupport(result)->GetID();
      }
   }

   if ( tsID != INVALID_ID )
   {
      EditTemporarySupportDescription(tsID,ETSD_GENERAL);
   }
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
      std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnInsertTemporarySupport>(dlg.GetTemporarySupport(),*pBridgeDesc,*dlg.GetBridgeDescription()));

      auto pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         std::unique_ptr<CEAFMacroTxn> pMacro(std::make_unique<CEAFMacroTxn>());
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(std::move(pTxn));
         pMacro->AddTransaction(std::move(pExtensionTxn));
         pTxn = std::move(pMacro);
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(std::move(pTxn));
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

void CPGSpliceDoc::OnCopyTempSupportProps()
{
   OnCopyTempSupportProps(INVALID_ID); // default
}

void CPGSpliceDoc::OnCopyTempSupportProps(UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   try
   {
      IDType cb_id;
      if (nID == INVALID_ID)
      {
         cb_id = m_CopyTempSupportPropertiesCallbacksCmdMap.begin()->second;
      }
      else
      {
         cb_id = m_CopyTempSupportPropertiesCallbacksCmdMap.at(nID);
      }

      CCopyTempSupportDlg dlg(m_pBroker, m_CopyTempSupportPropertiesCallbacks, cb_id);
      dlg.DoModal();
   }
   catch (...)
   {
      ATLASSERT(0); // map access out of range is the likely problem
   }
}

void CPGSpliceDoc::OnUpdateCopyTempSupportProps(CCmdUI * pCmdUI)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Can't copy from/to unless there is more than one temp support
   PierIndexType nts = pBridgeDesc->GetTemporarySupportCount();
   pCmdUI->Enable(nts > 1);
}

BOOL CPGSpliceDoc::OnCopyTempSupportPropsTb(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_COPY_TEMPSUPPORT_PROPS )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_GRAPHS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());

   int i = 0;
   for (const auto& ICallBack : m_CopyTempSupportPropertiesCallbacks)
   {
      UINT nCmd = i++ + FIRST_COPY_TEMP_SUP_PLUGIN;
      CString copyName = _T("Copy ") + CString(ICallBack.second->GetName());
      contextMenu.AppendMenu(nCmd, copyName, nullptr);
   }

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_COPY_TEMPSUPPORT_PROPS,nullptr);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
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

   bool bRetVal = false;

   if ( dlg.DoModal() == IDOK )
   {
      CPrecastSegmentData* pNewSegment = dlg.m_Girder.GetSegment(segmentKey.segmentIndex);
#pragma Reminder("UPDATE: why do we have to get the stirrup data after we get the segment")
      pNewSegment->ShearData = dlg.m_StirrupsPage.m_ShearData;

      txnEditPrecastSegmentData newData;
      newData.m_SegmentKey     = segmentKey;
      newData.m_SegmentData    = *pNewSegment;
      newData.m_TimelineMgr    = dlg.m_TimelineMgr;

      CSegmentKey thisSegmentKey(segmentKey);
      if ( dlg.m_bCopyToAll )
      {
         thisSegmentKey.girderIndex = ALL_GIRDERS;
      }

      std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnEditPrecastSegment>(thisSegmentKey,newData));
      auto pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         std::unique_ptr<CEAFMacroTxn> pMacro(std::make_unique<pgsMacroTxn>());
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(std::move(pTxn));
         pMacro->AddTransaction(std::move(pExtensionTxn));
         pTxn = std::move(pMacro);
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(std::move(pTxn));
      bRetVal = true;
   }
   
   return bRetVal;
}

bool CPGSpliceDoc::EditClosureJointDescription(const CClosureKey& closureKey,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   CClosureJointDlg dlg(pIBridgeDesc->GetBridgeDescription(),closureKey);

   if ( dlg.DoModal() == IDOK )
   {
      const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);
      txnEditClosureJointData newData;
      newData.m_PierIdx         = pClosure->GetPierIndex();
      newData.m_TSIdx           = pClosure->GetTemporarySupportIndex();
      newData.m_ClosureJoint    = dlg.m_ClosureJoint;
      newData.m_TimelineMgr     = dlg.m_TimelineMgr;

      CClosureKey thisClosureKey(closureKey);
      if ( dlg.m_bCopyToAllClosureJoints )
      {
         thisClosureKey.girderIndex = ALL_GIRDERS;
      }

      std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnEditClosureJoint>(thisClosureKey,newData));
      auto pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         std::unique_ptr<CEAFMacroTxn> pMacro(std::make_unique<pgsMacroTxn>());
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(std::move(pTxn));
         pMacro->AddTransaction(std::move(pExtensionTxn));
         pTxn = std::move(pMacro);
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(std::move(pTxn));
   }

   return true;
}

bool CPGSpliceDoc::EditGirderDescription(const CGirderKey& girderKey,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   bool bRetVal = false;

   CSplicedGirderDescDlg dlg(girderKey);

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnEditGirderline>(girderKey,dlg.m_bApplyToAll,*pBridgeDesc,dlg.m_BridgeDescription));

      auto pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         std::unique_ptr<CEAFMacroTxn> pMacro(std::make_unique<CEAFMacroTxn>());
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(std::move(pTxn));
         pMacro->AddTransaction(std::move(pExtensionTxn));
         pTxn = std::move(pMacro);
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(std::move(pTxn));

      bRetVal = true;
   }

   return bRetVal;
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

   std::unique_ptr<txnDeleteTemporarySupport> pTxn(std::make_unique<txnDeleteTemporarySupport>(tsIdx,oldBridgeDesc,newBridgeDesc));
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(std::move(pTxn));
}

bool CPGSpliceDoc::EditTemporarySupportDescription(SupportIDType tsID,int nPage)
{
   // NOTE: in the future, if we handle temporary shorting towers in PGSuper,
   // we will want to move this to the base document class so that one
   // thoe takes care of all the editing needs

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(tsID);
   SupportIndexType tsIdx = pTS->GetIndex();

   CTemporarySupportDlg dlg(pBridgeDesc,tsIdx);
   dlg.SetActivePage(nPage);
   if ( dlg.DoModal() == IDOK )
   {
      std::unique_ptr<CEAFTransaction> pTxn(std::make_unique<txnEditTemporarySupport>(tsIdx,*pBridgeDesc,*dlg.GetBridgeDescription()));

      auto pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         std::unique_ptr<CEAFMacroTxn> pMacro(std::make_unique<CEAFMacroTxn>());
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(std::move(pTxn));
         pMacro->AddTransaction(std::move(pExtensionTxn));
         pTxn = std::move(pMacro);
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(std::move(pTxn));
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
   {
      return FALSE;
   }

   // PGSplice Documents don't use the concentrated moment load
   // Remove it from the Loads menu
   CEAFMenu* pMainMenu = GetMainMenu();
   UINT position = pMainMenu->FindMenuItem(_T("L&oads"));
   CEAFMenu* pLoadMenu = pMainMenu->GetSubMenu(position);
   pLoadMenu->RemoveMenu(ID_ADD_MOMENT_LOAD,MF_BYCOMMAND,nullptr);

   PopulateCopyTempSupportMenu();

   return TRUE;
}

void CPGSpliceDoc::ModifyTemplate(LPCTSTR strTemplate)
{
   CPGSDocBase::ModifyTemplate(strTemplate);
}

void CPGSpliceDoc::PopulateCopyTempSupportMenu()
{
   m_CopyTempSupportPropertiesCallbacksCmdMap.clear();

   // if this assert fires, there are more graphs than can be put into the menus
   // EAF only reserves enough room for EAF_GRAPH_MENU_COUNT graphs
   const int MENU_COUNT = LAST_COPY_TEMP_SUP_PLUGIN - FIRST_COPY_TEMP_SUP_PLUGIN;
   ATLASSERT(m_CopyTempSupportPropertiesCallbacks.size() < MENU_COUNT);

   UINT i = 0;
   for (const auto& ICallBack : m_CopyTempSupportPropertiesCallbacks )
   {
      UINT nCmd = i + FIRST_COPY_TEMP_SUP_PLUGIN;

      // save command ID so we can map UI
      m_CopyTempSupportPropertiesCallbacksCmdMap.insert(std::make_pair(nCmd, ICallBack.first));

      i++;

      ASSERT(i <= MENU_COUNT);
   }
}
