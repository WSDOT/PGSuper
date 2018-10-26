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

// cfSplitChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "pgsuperDoc.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgeSectionView.h"
#include "BridgeViewPrintJob.h"
#include "StationCutDlg.h"
#include "SelectItemDlg.h"
#include "MainFrm.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\EditByUI.h>
#include <IFace\DisplayUnits.h>
#include "htmlhelp\HelpTopics.hh"
#include <PgsExt\BridgeDescription.h>
#include "EditBoundaryConditions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame

IMPLEMENT_DYNCREATE(CBridgeModelViewChildFrame, CSplitChildFrame)

CBridgeModelViewChildFrame::CBridgeModelViewChildFrame()
{
   m_bCutLocationInitialized = false;
   m_CurrentCutLocation = 0;
}

CBridgeModelViewChildFrame::~CBridgeModelViewChildFrame()
{
}


BEGIN_MESSAGE_MAP(CBridgeModelViewChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CBridgeModelViewChildFrame)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_SPAN,   OnEditSpan)
	ON_COMMAND(ID_EDIT_PIER,   OnEditPier)
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(ID_VIEW_GIRDER, OnViewGirder)
	ON_COMMAND(ID_DELETE_PIER, OnDeletePier)
	ON_UPDATE_COMMAND_UI(ID_DELETE_PIER, OnUpdateDeletePier)
	ON_COMMAND(ID_DELETE_SPAN, OnDeleteSpan)
	ON_UPDATE_COMMAND_UI(ID_DELETE_SPAN, OnUpdateDeleteSpan)
	ON_COMMAND(ID_INSERT_SPAN, OnInsertSpan)
	ON_COMMAND(ID_INSERT_PIER, OnInsertPier)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(IDM_HINGED,IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD,OnBoundaryCondition)
   ON_UPDATE_COMMAND_UI_RANGE(IDM_HINGED,IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD,OnUpdateBoundaryCondition)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame message handlers
CRuntimeClass* CBridgeModelViewChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CBridgeSectionView);
}

double CBridgeModelViewChildFrame::GetTopFrameFraction() const
{
   return 0.5;
}

void CBridgeModelViewChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CBridgeModelViewChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CBridgeModelViewChildFrame::DoFilePrint(bool direct)
{
   CBridgePlanView*    ppv = GetBridgePlanView();
   CBridgeSectionView* psv = GetBridgeSectionView();

   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
   
   // create a print job and do it
   CBridgeViewPrintJob pj(this,ppv, psv, pBroker);
   pj.OnFilePrint(direct);
}



CBridgePlanView* CBridgeModelViewChildFrame::GetBridgePlanView() 
{
   CWnd* pwnd = m_SplitterWnd.GetPane(0, 0);
   CBridgePlanView* pvw = dynamic_cast<CBridgePlanView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

CBridgeSectionView* CBridgeModelViewChildFrame::GetBridgeSectionView() 
{
   CWnd* pwnd = m_SplitterWnd.GetPane(1, 0);
   CBridgeSectionView* pvw = dynamic_cast<CBridgeSectionView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

int CBridgeModelViewChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	this->SetWindowText("Bridge Model View");

	return 0;
}

void CBridgeModelViewChildFrame::CutAt(Float64 cut)
{
   UpdateCutLocation(cut);
}

const char* CBridgeModelViewChildFrame::GetDeckTypeName(pgsTypes::SupportedDeckType deckType) const
{
   switch ( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
      return "Composite cast-in-place deck";

   case pgsTypes::sdtCompositeSIP: 
      return "Composite stay-in-place deck panels";

   case pgsTypes::sdtCompositeOverlay:
      return "Composite structural overlay";

   case pgsTypes::sdtNone:
      return "None";
   }

   return "";
}

void CBridgeModelViewChildFrame::ShowCutDlg()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   double start = pBridge->GetPierStation(0);
   double end   = pBridge->GetPierStation(nPiers-1);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CStationCutDlg dlg(m_CurrentCutLocation,start,end,bUnitsSI);
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetValue());
   }
}

void CBridgeModelViewChildFrame::UpdateCutLocation(Float64 cut)
{
   m_CurrentCutLocation = cut;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   double start = pBridge->GetPierStation(0);
   double end   = pBridge->GetPierStation(pBridge->GetPierCount()-1);

   m_CurrentCutLocation = ForceIntoRange(start,m_CurrentCutLocation,end);

//   UpdateBar();
   GetBridgePlanView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
   GetBridgeSectionView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
}
   
Float64 CBridgeModelViewChildFrame::GetCurrentCutLocation()
{
   if ( !m_bCutLocationInitialized )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridge,pBridge);
      double start = pBridge->GetPierStation(0);
      double end   = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      m_CurrentCutLocation = 0.5*(end-start) + start;
      m_bCutLocationInitialized = true;
   }

   return m_CurrentCutLocation;
}

void CBridgeModelViewChildFrame::OnEditGirder() 
{
   SpanIndexType   spanIdx;
   GirderIndexType gdrIdx;
   if ( GetBridgePlanView()->GetSelectedGirder(&spanIdx,&gdrIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->EditGirderDescription(spanIdx,gdrIdx,EGD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::OnViewGirder() 
{
   SpanIndexType   spanIdx;
   GirderIndexType gdrIdx;
   if ( GetBridgePlanView()->GetSelectedGirder(&spanIdx,&gdrIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->OnViewGirderEditor();
   }
}

void CBridgeModelViewChildFrame::OnEditSpan() 
{
   SpanIndexType spanIdx;
   if ( GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->EditSpanDescription(spanIdx,ESD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::OnEditPier() 
{
   PierIndexType pierIdx;
   if ( GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->EditPierDescription(pierIdx,EPD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::SelectSpan(SpanIndexType spanIdx)
{
   static bool bSelecting = false;

   if ( !bSelecting )
   {
      bSelecting = true;
      
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->SelectSpan(spanIdx);

      GetBridgePlanView()->SelectSpan(spanIdx,TRUE);
      GetBridgeSectionView()->SelectSpan(spanIdx,TRUE);

      bSelecting = false;
   }
}

void CBridgeModelViewChildFrame::SelectPier(PierIndexType pierIdx)
{
   static bool bSelecting = false;

   if ( !bSelecting )
   {
      bSelecting = true;
      
      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->SelectPier(pierIdx);

      GetBridgePlanView()->SelectPier(pierIdx,TRUE);
      GetBridgeSectionView()->SelectPier(pierIdx,TRUE);

      bSelecting = false;
   }
}

void CBridgeModelViewChildFrame::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   static bool bSelecting = false;

   if ( !bSelecting )
   {
      bSelecting = true;

      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->SelectGirder(spanIdx,gdrIdx);

      bSelecting = false;
   }
}

void CBridgeModelViewChildFrame::SelectDeck()
{
   static bool bSelecting = false;

   if ( !bSelecting )
   {
      bSelecting = true;

      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
//      pDoc->SelectPier(-1);
//      pDoc->SelectSpan(-1);
      pDoc->SelectGirder(-1,-1);

      GetBridgePlanView()->SelectDeck(true,true);
      GetBridgeSectionView()->SelectDeck(true,false);


      bSelecting = false;
   }
}

void CBridgeModelViewChildFrame::SelectAlignment()
{
   static bool bSelecting = false;

   if ( !bSelecting )
   {
      bSelecting = true;

      CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
      pDoc->SelectGirder(-1,-1);

      GetBridgePlanView()->SelectAlignment(true,true);
      GetBridgeSectionView()->SelectAlignment(true,false);


      bSelecting = false;
   }
}

void CBridgeModelViewChildFrame::ClearSelection()
{
   GetBridgePlanView()->ClearSelection();
   GetBridgeSectionView()->ClearSelection();
   CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectGirder(-1,-1);
}

LRESULT CBridgeModelViewChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGE_VIEW );
   return TRUE;
}

void CBridgeModelViewChildFrame::OnDeletePier() 
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();
      pDoc->DeletePier(pierIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeletePier(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType pierIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnDeleteSpan() 
{
   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();
      pDoc->DeleteSpan(spanIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeleteSpan(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnInsertSpan() 
{
   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CString strItems;
      strItems.Format("Before Span %d\nAfter Span %d\n",LABEL_SPAN(spanIdx),LABEL_SPAN(spanIdx));

      CSelectItemDlg dlg;
      dlg.m_strItems = strItems;
      dlg.m_strTitle = "Insert Span";
      dlg.m_strLabel = "Select location to insert span";
      dlg.m_ItemIdx = 0;

      if ( dlg.DoModal() == IDOK )
      {
         CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

         PierIndexType refPierIdx;
         pgsTypes::PierFaceType pierFace;
         if ( dlg.m_ItemIdx == 0 )
         {
            refPierIdx = spanIdx;
            pierFace = pgsTypes::Ahead;
         }
         else
         {
            refPierIdx = spanIdx+1;
            pierFace = pgsTypes::Back;
         }

         pDoc->InsertSpan(refPierIdx,pierFace);
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnInsertPier() 
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      PierIndexType nPiers = pBridgeDesc->GetPierCount();

      CString strPierType = (pierIdx == 0 || pierIdx == nPiers-1 ? "Abutment" : "Pier");

      CString strItems;
      strItems.Format("After %s %d\nBefore %s %d\n",strPierType,(pierIdx+1),strPierType,(pierIdx+1));

      CSelectItemDlg dlg;
      dlg.m_strItems = strItems;
      dlg.m_strTitle = "Insert Span";
      dlg.m_strLabel = "Select location to insert span";
      dlg.m_ItemIdx = 0;

      if ( dlg.DoModal() == IDOK )
      {
         CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

         pgsTypes::PierFaceType pierFace;
         if ( dlg.m_ItemIdx == 0 )
         {
            pierFace = pgsTypes::Ahead;
         }
         else
         {
            pierFace = pgsTypes::Back;
         }

         pDoc->InsertSpan(pierIdx,pierFace);

         if ( pierFace == pgsTypes::Back )
         {
            // move the pier selection ahead one pier so that it seems to
            // stay with the currently selected pier
            pView->SelectPier(pierIdx+1,true);
         }
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnBoundaryCondition(UINT nIDC)
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      pgsTypes::PierConnectionType newConnectionType;
      switch( nIDC )
      {
         case IDM_HINGED:                         newConnectionType = pgsTypes::Hinged;                       break;
         case IDM_ROLLER:                         newConnectionType = pgsTypes::Roller;                       break;
         case IDM_CONTINUOUS_AFTERDECK:           newConnectionType = pgsTypes::ContinuousAfterDeck;          break;
         case IDM_CONTINUOUS_BEFOREDECK:          newConnectionType = pgsTypes::ContinuousBeforeDeck;         break;
         case IDM_INTEGRAL_AFTERDECK:             newConnectionType = pgsTypes::IntegralAfterDeck;            break;
         case IDM_INTEGRAL_BEFOREDECK:            newConnectionType = pgsTypes::IntegralBeforeDeck;           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   newConnectionType = pgsTypes::IntegralAfterDeckHingeBack;   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  newConnectionType = pgsTypes::IntegralBeforeDeckHingeBack;  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  newConnectionType = pgsTypes::IntegralAfterDeckHingeAhead;  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: newConnectionType = pgsTypes::IntegralBeforeDeckHingeAhead; break;
         default: ATLASSERT(0); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      pgsTypes::PierConnectionType oldConnectionType = pIBridgeDesc->GetPier(pierIdx)->GetConnectionType();

      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      txnEditBoundaryConditions* pTxn = new txnEditBoundaryConditions(pierIdx,pDoc,oldConnectionType,newConnectionType);
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CBridgeModelViewChildFrame::OnUpdateBoundaryCondition(CCmdUI* pCmdUI)
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      pgsTypes::PierConnectionType connectionType = pIBridgeDesc->GetPier(pierIdx)->GetConnectionType();

      switch( pCmdUI->m_nID )
      {
         case IDM_HINGED:                         pCmdUI->SetCheck(connectionType == pgsTypes::Hinged);                       break;
         case IDM_ROLLER:                         pCmdUI->SetCheck(connectionType == pgsTypes::Roller);                       break;
         case IDM_CONTINUOUS_AFTERDECK:           pCmdUI->SetCheck(connectionType == pgsTypes::ContinuousAfterDeck);          break;
         case IDM_CONTINUOUS_BEFOREDECK:          pCmdUI->SetCheck(connectionType == pgsTypes::ContinuousBeforeDeck);         break;
         case IDM_INTEGRAL_AFTERDECK:             pCmdUI->SetCheck(connectionType == pgsTypes::IntegralAfterDeck);            break;
         case IDM_INTEGRAL_BEFOREDECK:            pCmdUI->SetCheck(connectionType == pgsTypes::IntegralBeforeDeck);           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   pCmdUI->SetCheck(connectionType == pgsTypes::IntegralAfterDeckHingeBack);   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  pCmdUI->SetCheck(connectionType == pgsTypes::IntegralBeforeDeckHingeBack);  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  pCmdUI->SetCheck(connectionType == pgsTypes::IntegralAfterDeckHingeAhead);  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: pCmdUI->SetCheck(connectionType == pgsTypes::IntegralBeforeDeckHingeAhead); break;
         default: ATLASSERT(0); // is there a new connection type?
      }
   }
}