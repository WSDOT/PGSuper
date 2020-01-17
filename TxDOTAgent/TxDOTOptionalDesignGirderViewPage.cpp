///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// TxDOTOptionalDesignGirderViewPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignGirderViewPage.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <EAF\EAFDisplayUnits.h>

#include "TogaGirderModelElevationView.h"
#include "TogaGirderModelSectionView.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TogaSectionCutDlgEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CTxDOTOptionalDesignGirderViewPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignGirderViewPage, CPropertyPage)

CTxDOTOptionalDesignGirderViewPage::CTxDOTOptionalDesignGirderViewPage()
	: CPropertyPage(CTxDOTOptionalDesignGirderViewPage::IDD),
   m_pData(nullptr),
   m_pBrokerRetriever(nullptr),
   m_pElevationView(nullptr),
   m_pSectionView(nullptr),
   m_ChangeStatus(0),
   m_CurrentCutLocation(0),
   m_CutLocation(Center),
   m_SelectedGirder(TOGA_FABR_GDR)
{
}

CTxDOTOptionalDesignGirderViewPage::~CTxDOTOptionalDesignGirderViewPage()
{
}

void CTxDOTOptionalDesignGirderViewPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_ERROR_MSG, m_ErrorMsgStatic);
   DDX_Control(pDX, ID_SELECTED_GIRDER, m_GirderCtrl);
   DDX_Control(pDX, IDC_SECTION_CUT, m_SectionBtn);
}

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignGirderViewPage, CPropertyPage)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_CBN_SELCHANGE(ID_SELECTED_GIRDER, &CTxDOTOptionalDesignGirderViewPage::OnCbnSelchangeSelectedGirder)
   ON_BN_CLICKED(IDC_SECTION_CUT, &CTxDOTOptionalDesignGirderViewPage::OnBnClickedSectionCut)
   ON_COMMAND(ID_VIEW_SECTIONCUTLOCATION, &CTxDOTOptionalDesignGirderViewPage::OnViewSectioncutlocation)
   ON_COMMAND(ID_HELP, &CTxDOTOptionalDesignGirderViewPage::OnHelpFinder)
   ON_COMMAND(ID_HELP_FINDER, &CTxDOTOptionalDesignGirderViewPage::OnHelpFinder)
END_MESSAGE_MAP()


// CTxDOTOptionalDesignGirderViewPage message handlers

BOOL CTxDOTOptionalDesignGirderViewPage::OnSetActive()
{
   UpdateBar();

   if (m_ChangeStatus!=0)
   {
      try
      {
         m_pSectionView->ShowWindow(SW_SHOW);
         m_pElevationView->ShowWindow(SW_SHOW);

         // any change forces an update
         m_pElevationView->OnUpdate(nullptr, HINT_GIRDERCHANGED, nullptr);
         m_pSectionView->OnUpdate(nullptr, HINT_GIRDERCHANGED, nullptr);

         // our data is updated
         m_ChangeStatus = 0;

      }
      catch(TxDOTBrokerRetrieverException exc)
      {
         m_pSectionView->ShowWindow(SW_HIDE);
         m_pElevationView->ShowWindow(SW_HIDE);
      }
      catch(...)
      {
         m_pSectionView->ShowWindow(SW_HIDE);
         m_pElevationView->ShowWindow(SW_HIDE);
      }
   }


   return CPropertyPage::OnSetActive();
}

BOOL CTxDOTOptionalDesignGirderViewPage::OnInitDialog()
{
   __super::OnInitDialog();

   // At this point our document is alive. 
   // We aren't a view, so we subvert doc view and listen directly to data source
   ASSERT(m_pData);
   m_pData->Attach(this);

   // This is our first update - we know changes have happened
   m_ChangeStatus = ITxDataObserver::ctPGSuper;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Selected Girder
   m_SelectedGirder  = TOGA_FABR_GDR;
   CComboBox* pgirder_ctrl   = (CComboBox*)GetDlgItem(ID_SELECTED_GIRDER);
   pgirder_ctrl->SetCurSel(1);

   // Embed views
   CCreateContext pContext;
	CWnd* pFrameWnd = this;
	pContext.m_pCurrentDoc = m_pDocument;

   // Elevation
	pContext.m_pNewViewClass = RUNTIME_CLASS(CTogaGirderModelElevationView);
	m_pElevationView = (CTogaGirderModelElevationView *)((CFrameWnd*)pFrameWnd)->CreateView(&pContext);
   m_pElevationView->SendMessage(WM_INITIALUPDATE);
	m_pElevationView->ShowWindow(SW_NORMAL);

   // Section view
	pContext.m_pNewViewClass = RUNTIME_CLASS(CTogaGirderModelSectionView);
	m_pSectionView = (CTogaGirderModelSectionView *)((CFrameWnd*)pFrameWnd)->CreateView(&pContext);
   m_pSectionView->SendMessage(WM_INITIALUPDATE);
	m_pSectionView->ShowWindow(SW_NORMAL);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTxDOTOptionalDesignGirderViewPage::OnTxDotDataChanged(int change)
{
   // save change information
   m_ChangeStatus |= change;
}

BOOL CTxDOTOptionalDesignGirderViewPage::OnEraseBkgnd(CDC* pDC)
{
   // Set brush to dialog background color
   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   // Save old brush
   CBrush* pOldBrush = pDC->SelectObject(&backBrush);

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
       PATCOPY);
   pDC->SelectObject(pOldBrush);

   return true;
}

HBRUSH CTxDOTOptionalDesignGirderViewPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}

void CTxDOTOptionalDesignGirderViewPage::AssertValid() const
{
   // Asserts will fire if not in static module state
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   __super::AssertValid();
}

void CTxDOTOptionalDesignGirderViewPage::GetSpanAndGirderSelection(SpanIndexType* pSpan,GirderIndexType* pGirder)
{
   *pSpan = TOGA_SPAN;
   *pGirder = m_SelectedGirder;
}

void CTxDOTOptionalDesignGirderViewPage::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   Float64 val  = m_CurrentCutLocation;
   Float64 high = m_MaxCutLocation;

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   SpanIndexType span;
   GirderIndexType gdr;
   GetSpanAndGirderSelection(&span,&gdr);

   CSegmentKey segmentKey(span,gdr,0);

   ATLASSERT( span != ALL_SPANS && gdr != ALL_GIRDERS  );
   IndexType nHarpPoints = pStrandGeom->GetNumHarpPoints(segmentKey);

   CTogaSectionCutDlgEx dlg(nHarpPoints,m_CurrentCutLocation,0.0,high,m_CutLocation);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      m_CurrentCutLocation = dlg.GetValue();
      UpdateCutLocation(dlg.GetCutLocation(),m_CurrentCutLocation);
   }
}

void CTxDOTOptionalDesignGirderViewPage::CutAt(Float64 cut)
{
   UpdateCutLocation(UserInput,cut);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLeftEnd() 
{
   UpdateCutLocation(LeftEnd);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLeftHp() 
{
   UpdateCutLocation(LeftHarp);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtCenter() 
{
   UpdateCutLocation(Center);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtRightHp() 
{
   UpdateCutLocation(RightHarp);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtRightEnd() 
{
   UpdateCutLocation(RightEnd);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtNext()
{
   Float64 f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f+0.1,0.1);
   if ( 1 < f )
      f = 1;

   CutAt(f*m_MaxCutLocation);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtPrev()
{
   Float64 f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f-0.1,0.1);
   if ( f < 0 )
      f = 0;

   CutAt(f*m_MaxCutLocation);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLocation()
{
   m_CutLocation = UserInput;

   ShowCutDlg();

   // Because the dialog messes with the screen
   // force an update (this is a hack because of the selection tool).
   m_pElevationView->Invalidate();
   m_pElevationView->UpdateWindow();
}


void CTxDOTOptionalDesignGirderViewPage::UpdateCutLocation(CutLocation cutLoc,Float64 cut)
{
   m_CurrentCutLocation = cut;
   m_CutLocation = cutLoc;

   UpdateBar();
   m_pElevationView->OnUpdate(nullptr, HINT_GIRDERVIEWSECTIONCUTCHANGED, nullptr);
   m_pSectionView->OnUpdate(nullptr, HINT_GIRDERVIEWSECTIONCUTCHANGED, nullptr);
}

void CTxDOTOptionalDesignGirderViewPage::UpdateBar()
{
   SpanIndexType spanIdx, gdrIdx;
   GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   CSegmentKey segmentKey(spanIdx,gdrIdx,0);

   try
   {
      CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();
      GET_IFACE2(pBroker, IBridge, pBridge);

      // cut location
      Float64 gird_len = pBridge->GetSegmentLength(segmentKey);
      m_MaxCutLocation = gird_len;

      if (m_CutLocation==UserInput)
      {
         if (m_CurrentCutLocation > gird_len)
            m_CurrentCutLocation = gird_len;
      }
      else if (m_CutLocation == LeftEnd)
      {
         m_CurrentCutLocation = 0.0;
      }
      else if (m_CutLocation == RightEnd)
      {
         m_CurrentCutLocation = gird_len;
      }
      else if (m_CutLocation == Center)
      {
         m_CurrentCutLocation = gird_len/2.0;
      }
      else
      {
         // cut was taken at a harping point, must enlist poi interface
         GET_IFACE2(pBroker, IPointOfInterest, pPoi);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
         IndexType nPoi = vPoi.size();
         ATLASSERT( 0 <= nPoi && nPoi <= 2 );
         auto iter = vPoi.begin();
         pgsPointOfInterest left_hp_poi = *iter++;
         pgsPointOfInterest right_hp_poi = left_hp_poi;
         if (nPoi == 2)
         {
            right_hp_poi = *iter++;
         }

         if (m_CutLocation == LeftHarp)
         {
            m_CurrentCutLocation = left_hp_poi.GetDistFromStart();
         }
         else if (m_CutLocation == RightHarp)
         {
            m_CurrentCutLocation = right_hp_poi.GetDistFromStart();
         }
         else
         {
            ASSERT(0); // unknown cut location type
         }
      }

      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      CString msg;
      msg.Format(_T("Section Cut Offset: %s"),FormatDimension(m_CurrentCutLocation,pDisplayUnits->GetXSectionDimUnit()));

      m_SectionBtn.SetWindowText(msg);

      // made it - we can show our controls
      m_pSectionView->ShowWindow(SW_SHOW);
      m_pElevationView->ShowWindow(SW_SHOW);
      m_GirderCtrl.EnableWindow(TRUE);
      m_SectionBtn.EnableWindow(TRUE);
   }
   catch(TxDOTBrokerRetrieverException exc)
   {
      // An error occurred in analysis - go to error mode
      DisplayErrorMode(exc);
   }
   catch(...)
   {
      ASSERT(0);
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("An Unknown Error has Occurred");
      DisplayErrorMode(exc);
   }

}

void CTxDOTOptionalDesignGirderViewPage::OnSize(UINT nType, int cx, int cy)
{
//   __super::OnSize(nType, cx, cy);

   if (m_pElevationView && m_pSectionView)
   {
      // Convert a 7du x 7du rect into pixels
      CRect sizeRect(0,0,7,7);
      MapDialogRect(&sizeRect);

      CRect toolBarRect(0,0,22,22); // tool bar is 22du high
      MapDialogRect(&toolBarRect);

      CRect clientRect;
      GetClientRect( &clientRect );

      // Rect around both windows
      clientRect.bottom = clientRect.bottom - 3 * sizeRect.Height();
      clientRect.left = clientRect.left + sizeRect.Width();
      clientRect.top  = clientRect.top + toolBarRect.Height();
      clientRect.right  = clientRect.right  - sizeRect.Width();

      int orig_hgt = clientRect.Height();

      // Figure out the new position
      if (::IsWindow(m_pElevationView->m_hWnd) && ::IsWindow(m_pSectionView->m_hWnd))
      {
         // elevation view on top 1/3
         CRect elevRect(clientRect);
         elevRect.bottom = elevRect.top + orig_hgt/3 - sizeRect.Height()/2;

         m_pElevationView->MoveWindow( elevRect, FALSE );

         CRect sectRect(clientRect);
         sectRect.top = elevRect.bottom + sizeRect.Height();

         m_pSectionView->MoveWindow( sectRect, FALSE );
      }
   }

   Invalidate();
}

void CTxDOTOptionalDesignGirderViewPage::OnCbnSelchangeSelectedGirder()
{
   CComboBox* pgirder_ctrl   = (CComboBox*)GetDlgItem(ID_SELECTED_GIRDER);
   if (pgirder_ctrl->GetCurSel()==0)
      m_SelectedGirder = TOGA_ORIG_GDR;
   else
      m_SelectedGirder = TOGA_FABR_GDR;

   m_pElevationView->OnUpdate(nullptr, HINT_GIRDERCHANGED, nullptr);
   m_pSectionView->OnUpdate(nullptr, HINT_GIRDERCHANGED, nullptr);
}

void CTxDOTOptionalDesignGirderViewPage::OnBnClickedSectionCut()
{
   CutAtLocation();
}

void CTxDOTOptionalDesignGirderViewPage::DisplayErrorMode(TxDOTBrokerRetrieverException& exc)
{
   m_pSectionView->ShowWindow(SW_HIDE);
   m_pElevationView->ShowWindow(SW_HIDE);
   m_GirderCtrl.EnableWindow(FALSE);
   m_SectionBtn.EnableWindow(FALSE);

   CString msg;
   msg.Format(_T("Error - Analysis run Failed because: \n %s"),exc.Message);

   m_ErrorMsgStatic.SetWindowText(msg);
   m_ErrorMsgStatic.ShowWindow(SW_SHOW);
}

void CTxDOTOptionalDesignGirderViewPage::OnViewSectioncutlocation()
{
   this->ShowCutDlg();
}


void CTxDOTOptionalDesignGirderViewPage::OnHelpFinder()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_VIEW );
}
