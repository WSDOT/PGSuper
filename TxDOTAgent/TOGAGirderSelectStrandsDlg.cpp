///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
// TOGAGirderSelectStrandsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TOGAGirderSelectStrandsDlg.h"
#include <WBFLGenericBridgeTools.h>

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\BeamFactory.h>

#include <PgsExt\DesignConfigUtil.h>
#include "PGSuperColors.h"
#include "PGSuperUIUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define BORDER 7

// CTOGAGirderSelectStrandsDlg dialog

IMPLEMENT_DYNAMIC(CTOGAGirderSelectStrandsDlg, CDialog)

CTOGAGirderSelectStrandsDlg::CTOGAGirderSelectStrandsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CTOGAGirderSelectStrandsDlg::IDD, pParent),
   m_IsMidSpan(0),
   m_bFirstSize(true),
   m_bSymmetricDebond(true)
{
   m_DrawNumbers = TRUE;

   m_Radius = WBFL::Units::ConvertToSysUnits(0.3,WBFL::Units::Measure::Inch) * 1.5;

   m_pToolTip = nullptr;
}

CTOGAGirderSelectStrandsDlg::~CTOGAGirderSelectStrandsDlg()
{
   delete  m_pToolTip;
}

void CTOGAGirderSelectStrandsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   DDX_Check(pDX, IDC_SHOW_NUMBERS, m_DrawNumbers);
   DDX_CBIndex(pDX,IDC_COMBO_VIEWLOC, m_IsMidSpan);

   if (pDX->m_bSaveAndValidate)
   {
      // must call grid to exchange its data
      if(!m_Grid.UpdateData(true))
      {
         HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_STRAND_GRID);
         pDX->Fail();
      }
   }
}

BEGIN_MESSAGE_MAP(CTOGAGirderSelectStrandsDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
   ON_WM_LBUTTONUP()
   ON_BN_CLICKED(IDC_SHOW_NUMBERS, &CTOGAGirderSelectStrandsDlg::OnBnClickedShowNumbers)
   ON_CBN_SELCHANGE(IDC_COMBO_VIEWLOC, &CTOGAGirderSelectStrandsDlg::OnCbnSelchangeComboViewloc)
   ON_BN_CLICKED(ID_HELP, &CTOGAGirderSelectStrandsDlg::OnHelp)
   ON_WM_GETMINMAXINFO()
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// CTOGAGirderSelectStrandsDlg message handlers

BOOL CTOGAGirderSelectStrandsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

    //Set up the tooltip
    m_pToolTip = new CToolTipCtrl;
    VERIFY(m_pToolTip->Create(this));
    VERIFY(m_pToolTip->AddTool(this,_T("Click on strands to select.")));

	m_Grid.SubclassDlgItem(IDC_STRAND_GRID, this);
   m_Grid.CustomInit(this,m_pGdrEntry);

   OnNumStrandsChanged();

   // Collect data for dialog resizing
   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rPicture;
   pPicture->GetWindowRect(&rPicture);

   // need distance to left edge of picture
   CRect rwndRect;
   GetClientRect(&rwndRect);
   ClientToScreen(&rwndRect);

   // Get some layout data
   m_BottomOffset = rwndRect.bottom - rPicture.bottom; // distance from bottom of DLG to bottom of picture
   m_RightOffset  = rwndRect.right - rPicture.right;

   CRect rect;
   GetDlgItem(IDC_STRAIGHT)->GetClientRect(&rect);
   GetDlgItem(IDC_STRAIGHT)->ClientToScreen(&rect);
   m_Row1Offset = rect.top - rPicture.bottom; // distace from bottom of picture to top of first row of state data controls
   GetDlgItem(IDC_DEBONDED)->GetClientRect(&rect);
   GetDlgItem(IDC_DEBONDED)->ClientToScreen(&rect);
   m_Row2Offset = rect.top - rPicture.bottom; // distace from bottom of picture to top of second row of state data controls

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTOGAGirderSelectStrandsDlg::InitializeData(SpanIndexType span, GirderIndexType girder,
                     const CDirectStrandFillCollection& directFilledStraightStrands,
                     const std::vector<CDebondData>& straightDebond, 
                     const SpecLibraryEntry* pSpecEntry,const GirderLibraryEntry* pGdrEntry, Float64 maxDebondLength)
{
   m_pGdrEntry = pGdrEntry;
   m_Span = span;
   m_Girder = girder;
   m_DirectFilledStraightStrands  = directFilledStraightStrands;

   m_CanDebondStrands = pGdrEntry->CanDebondStraightStrands();
   m_StraightDebond   = straightDebond;

   m_MaxDebondLength = maxDebondLength;
}

bool CTOGAGirderSelectStrandsDlg::GetData(CDirectStrandFillCollection& directFilledStraightStrands,
                                          std::vector<CDebondData>& straightDebond)
{
   directFilledStraightStrands = m_DirectFilledStraightStrands;

   if (m_CanDebondStrands)
   {
      straightDebond = m_StraightDebond;
   }
   else
   {
      straightDebond.clear();
   }

   return true;
}

void CTOGAGirderSelectStrandsDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

   CWnd* pPicture = GetDlgItem(IDC_PICTURE);

   if ( pPicture == nullptr )
      return; // child controls have not yet been created

   // get the picture and dialog rects in screen coordinates
   CRect rPicture;
   pPicture->GetWindowRect(&rPicture);

   CRect rwndRect;
   GetWindowRect(&rwndRect);
   
   // adjust the picture rect (top and left edges don't move)
   rPicture.bottom = rwndRect.bottom - m_BottomOffset;
   rPicture.right  = rwndRect.right  - m_RightOffset;
   ScreenToClient(rPicture); // make client coordinates
   pPicture->MoveWindow(rPicture); // move the window

   CWnd* pWnd = GetDlgItem(IDC_STRAIGHT);
   CRect rect;
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   int h = rect.Height();
   rect.top = rPicture.bottom + m_Row1Offset;
   rect.bottom = rect.top + h;
   pWnd->MoveWindow(&rect);

   pWnd = GetDlgItem(IDC_DEBONDED);
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   h = rect.Height();
   rect.top = rPicture.bottom + m_Row2Offset;
   rect.bottom = rect.top + h;
   pWnd->MoveWindow(&rect);

   Invalidate();
   UpdateWindow();
}

void CTOGAGirderSelectStrandsDlg::OnPaint() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Make sure we have up to date grid data
   m_Grid.UpdateData(false);

	CPaintDC dc(this); // device context for painting
	
   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect rClient;
   pWnd->GetClientRect(&rClient);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(rClient.left,rClient.top,rClient.right,rClient.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   // Get girder shape
   CComPtr<IBeamFactory> factory;
   m_pGdrEntry->GetBeamFactory(&factory);

   GirderLibraryEntry::Dimensions dimensions = m_pGdrEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   factory->CreateGirderSection(pBroker,INVALID_ID,dimensions,-1,-1,&gdrSection);

   CComQIPtr<IShape> shape(gdrSection);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);

   // Anchor bottom of drawing at bottom center of section shape
   Float64 wleft, wright;
   gdrSection->get_BottomWidth(&wleft, &wright);
   Float64 bottom_width = wleft + wright;

   CComPtr<IRect2d> shape_box;
   shape->get_BoundingBox(&shape_box);

   CComPtr<IPoint2d> objOrg;
   shape_box->get_BottomCenter(&objOrg);

   WBFL::Graphing::Point orgin;
   Float64 x,y;
   objOrg->get_X(&x);
   objOrg->get_Y(&y);
   orgin.X() = x;
   orgin.Y() = y;

   // Get height and width of the area occupied by all possible strand locations
   // straight strands
   Float64 xmax(0.0), ymax(0.0);
   StrandIndexType ngrid = m_pGdrEntry->GetNumStraightStrandCoordinates();
   for(StrandIndexType igrid=0; igrid<ngrid; igrid++)
   {
      Float64 xs, ys, xe, ye;
      bool candb;
      m_pGdrEntry->GetStraightStrandCoordinates(igrid, &xs, &ys, &xe, &ye, &candb);

      xmax = Max(xmax, xs);
      ymax = Max(ymax, ys);
   }

   WBFL::Graphing::Rect strand_bounds(-xmax-m_Radius, 0.0, xmax+m_Radius, ymax+m_Radius);

   WBFL::Graphing::Size world_size;
   world_size.Dx() = Max(bottom_width,strand_bounds.Width());

   world_size.Dy() = strand_bounds.Height();
   if ( IsZero(world_size.Dy()) )
      world_size.Dy() = world_size.Dx()/2;

   CSize client_size = rClient.Size();
   client_size -= CSize(10,10); // make client size slightly smaller so there 
   // is some space between the drawing and the edge of the picture control

   // This mapping pushes image to bottom
   WBFL::Graphing::PointMapper mapper;
   mapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   mapper.SetWorldExt(world_size);
   mapper.SetWorldOrg(orgin);
   mapper.SetDeviceExt(client_size.cx,client_size.cy);
   mapper.SetDeviceOrg(client_size.cx/2+5,client_size.cy+5);

   // find the world coordinates of the screen center and move to center the shape to it
   Float64 cwx, cwy;
   client_size = rClient.Size();
   mapper.DPtoWP(client_size.cx/2,client_size.cy/2, &cwx, &cwy);
   Float64 dist = cwy - world_size.Dy()/2.0;

   mapper.SetWorldOrg(orgin.X(), orgin.Y()-dist);

   CPen solid_pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush solid_brush(SEGMENT_FILL_COLOR);

   CPen void_pen(PS_SOLID,1,VOID_BORDER_COLOR);
   CBrush void_brush(GetSysColor(COLOR_WINDOW));

   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   CComQIPtr<ICompositeShape> compshape(shape);
   if ( compshape )
   {
      IndexType nShapes;
      compshape->get_Count(&nShapes);
      for ( IndexType idx = 0; idx < nShapes; idx++ )
      {
         CComPtr<ICompositeShapeItem> item;
         compshape->get_Item(idx,&item);

         CComPtr<IShape> s;
         item->get_Shape(&s);

         VARIANT_BOOL bVoid;
         item->get_Void(&bVoid);

         if ( bVoid )
         {
            pDC->SelectObject(&void_pen);
            pDC->SelectObject(&void_brush);
         }
         else
         {
            pDC->SelectObject(&solid_pen);
            pDC->SelectObject(&solid_brush);
         }

         DrawShape(pDC,s,mapper);
      }
   }
   else
   {
      DrawShape(pDC,shape,mapper);
   }

   DrawStrands(pDC,mapper);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CTOGAGirderSelectStrandsDlg::DrawShape(CDC* pDC,IShape* shape, WBFL::Graphing::PointMapper& mapper)
{
   CComPtr<IPoint2dCollection> objPoints;
   shape->get_PolyPoints(&objPoints);

   IndexType nPoints;
   objPoints->get_Count(&nPoints);

   CPoint* points = new CPoint[nPoints];

   CComPtr<IPoint2d> point;
   long dx,dy;

   long i = 0;
   CComPtr<IEnumPoint2d> enumPoints;
   objPoints->get__Enum(&enumPoints);
   while ( enumPoints->Next(1,&point,nullptr) != S_FALSE )
   {
      WBFL::Graphing::Point pnt;
      point->Location(&pnt.X(), &pnt.Y());
      mapper.WPtoDP(pnt,&dx,&dy);

      points[i] = CPoint(dx,dy);

      point.Release();
      i++;
   }

   pDC->Polygon(points,(int)nPoints);

   delete[] points;
}

void CTOGAGirderSelectStrandsDlg::DrawStrands(CDC* pDC, WBFL::Graphing::PointMapper& Mapper)
{
   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* pOldFont = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   CBrush straight_strand_brush(STRAIGHT_FILL_COLOR);
   CPen   straight_strand_pen(1,PS_SOLID,STRAIGHT_FILL_COLOR);

   CBrush straight_db_strand_brush(DEBOND_FILL_COLOR);
   CPen   straight_db_strand_pen(1,PS_SOLID,DEBOND_FILL_COLOR);

   CBrush harped_strand_brush(HARPED_FILL_COLOR);
   CPen   harped_strand_pen(1,PS_SOLID,HARPED_FILL_COLOR);

   CBrush temp_strand_brush(TEMPORARY_FILL_COLOR);
   CPen   temp_strand_pen(1,PS_SOLID,TEMPORARY_FILL_COLOR);

   CBrush extended_strand_brush(EXTENDED_FILL_COLOR);
   CPen   extended_strand_pen(1,PS_SOLID,EXTENDED_FILL_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&straight_strand_brush);
   CPen*   pOldPen   = pDC->SelectObject(&straight_strand_pen);

   // loop over global strand fill order for permanent strands
   bool use_diff_hg = m_pGdrEntry->IsDifferentHarpedGridAtEndsUsed();

   // Track row index of each strand in grid - this is to catch and use mouse clicks
   ROWCOL grid_row = m_Grid.GetRowCount();
   m_StrandLocations.clear();

   StrandIndexType total_strand_cnt=0;
   StrandIndexType num_global = m_pGdrEntry->GetPermanentStrandGridSize();
   for (StrandIndexType idx=0; idx<num_global; idx++)
   {
      StrandIndexType strand_idx;
      GirderLibraryEntry::psStrandType strand_type;
      m_pGdrEntry->GetGridPositionFromPermStrandGrid(idx, &strand_type, &strand_idx);

      if (strand_type == GirderLibraryEntry::stStraight)
      {
         Float64 xStart,yStart,xEnd,yEnd;
         bool can_debond;
         m_pGdrEntry->GetStraightStrandCoordinates(strand_idx, &xStart, &yStart, &xEnd, &yEnd, &can_debond);

         bool is_filled = m_DirectFilledStraightStrands.IsStrandFilled(strand_idx);

         Float64 leftDebond,rightDebond;
         bool bIsDebonded = m_Grid.GetDebondInfo(strand_idx,&leftDebond,&rightDebond);

         if ( is_filled )
         {
            pDC->SelectObject(bIsDebonded ? &straight_db_strand_brush : &straight_strand_brush);
            pDC->SelectObject(bIsDebonded ? &straight_db_strand_pen   : &straight_strand_pen);
         }
         else
         {
            pDC->SelectObject(can_debond ? &straight_db_strand_brush : &straight_strand_brush);
            pDC->SelectObject(can_debond ? &straight_db_strand_pen   : &straight_strand_pen);
         }

         total_strand_cnt = DrawStrand(pDC, Mapper, xStart, yStart, total_strand_cnt, is_filled, grid_row);
      }
     else
      {
         ATLASSERT(false);
      }

      grid_row--;
   }

   pDC->SelectObject(pOldFont);
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

void PrintNumber(CDC* pDC, WBFL::Graphing::PointMapper& Mapper, const WBFL::Graphing::Point& loc, StrandIndexType strandIdx)
{
   long x, y;
   Mapper.WPtoDP(loc.X(), loc.Y(), &x, &y);

   // move down slightly
   y += 2;

   CString str;
   str.Format(_T("%d"),strandIdx);

   pDC->TextOut(x, y, str);
}

StrandIndexType CTOGAGirderSelectStrandsDlg::DrawStrand(CDC* pDC, WBFL::Graphing::PointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index, bool isFilled, ROWCOL gridRow)
{
   CRect rect;
   Mapper.WPtoDP(x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
   Mapper.WPtoDP(x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
   Mapper.WPtoDP(x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
   Mapper.WPtoDP(x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

   if (isFilled)
   {
      pDC->Ellipse(&rect);
   }
   else
   {
      pDC->MoveTo(rect.left, rect.top);
      pDC->LineTo(rect.right, rect.bottom);
      pDC->MoveTo(rect.right, rect.top);
      pDC->LineTo( rect.left, rect.bottom);
   }

   // save location for click tracking
   AddClickRect(rect, gridRow);

   index++;

   if ( m_DrawNumbers )
      PrintNumber(pDC, Mapper, WBFL::Graphing::Point(x,y), index);

   if (0.0 < x)
   {
      Mapper.WPtoDP(-x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
      Mapper.WPtoDP(-x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
      Mapper.WPtoDP(-x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
      Mapper.WPtoDP(-x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

      if (isFilled)
      {
         pDC->Ellipse(&rect);
      }
      else
      {
         pDC->MoveTo(rect.left, rect.top);
         pDC->LineTo(rect.right, rect.bottom);
         pDC->MoveTo(rect.right, rect.top);
         pDC->LineTo( rect.left, rect.bottom);
      }

      AddClickRect(rect, gridRow);

      index++;

      WBFL::Graphing::Point np(-x,y);
      if ( m_DrawNumbers )
         PrintNumber(pDC, Mapper, np, index);
   }

   return index;
}

void CTOGAGirderSelectStrandsDlg::OnNumStrandsChanged()
{
   UpdateStrandInfo();
   UpdatePicture();
}

void CTOGAGirderSelectStrandsDlg::UpdateStrandInfo()
{
   m_Grid.UpdateData(false);
   StrandIndexType nStraight  = m_DirectFilledStraightStrands.GetFilledStrandCount();

   StrandIndexType nDebonded = 0;
   std::vector<CDebondData>::iterator iter(m_StraightDebond.begin());
   std::vector<CDebondData>::iterator end(m_StraightDebond.end());
   for ( ; iter != end; iter++ )
   {
      CDebondData& debond_info = *iter;
      nDebonded += m_DirectFilledStraightStrands.GetFillCountAtIndex(debond_info.strandTypeGridIdx);

#ifdef _DEBUG
      Float64 xStart,yStart,xEnd,yEnd;
      bool can_debond;
      m_pGdrEntry->GetStraightStrandCoordinates(debond_info.strandTypeGridIdx, &xStart, &yStart, &xEnd, &yEnd, &can_debond);
      ATLASSERT(can_debond && m_DirectFilledStraightStrands.IsStrandFilled(debond_info.strandTypeGridIdx));
#endif 
   }

   Float64 percent = 0.0;
   if ( 0 < nStraight )
      percent = 100.0*((Float64)nDebonded/(Float64)nStraight);

   CString msg;
   msg.Format(_T("Debonded (S-DB)=%d (%.1f%%)"), nDebonded, percent);
   GetDlgItem(IDC_DEBONDED)->SetWindowText(msg);

   msg.Format(_T("Straight (S)=%d"), nStraight);
   GetDlgItem(IDC_STRAIGHT)->SetWindowText(msg);
}

void CTOGAGirderSelectStrandsDlg::UpdatePicture()
{
   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
}

void CTOGAGirderSelectStrandsDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   this->MapWindowPoints(pWnd,&point,1);

   // try and find click point in our strand locations
   std::vector< std::pair<CRect, ROWCOL> >::iterator it = m_StrandLocations.begin();
   std::vector< std::pair<CRect, ROWCOL> >::iterator itend = m_StrandLocations.end();

   while(it != itend)
   {
      CRect& rcr = it->first;
      if(point.x>=rcr.left && point.x<=rcr.right && point.y<=rcr.top && point.y>=rcr.bottom)
      {
         // found it - toggle strand in grid
         ROWCOL i = it->second;
         m_Grid.ToggleFill(i);
         break;
      }

      it++;
   }

   CDialog::OnLButtonUp(nFlags, point);
}

void CTOGAGirderSelectStrandsDlg::AddClickRect(CRect rect, ROWCOL gridRow)
{
   // inflate rect a bit so it's easier to click 
   CSize size = rect.Size();
   int downinf = int(size.cy/1.5);
   rect.InflateRect(3,downinf,3,3); 
   m_StrandLocations.push_back( std::make_pair(rect, gridRow) );
}

BOOL CTOGAGirderSelectStrandsDlg::PreTranslateMessage(MSG* pMsg)
{

  if (nullptr != m_pToolTip)
      m_pToolTip->RelayEvent(pMsg);

   return CDialog::PreTranslateMessage(pMsg);
}

void CTOGAGirderSelectStrandsDlg::OnBnClickedShowNumbers()
{
   CButton* pBut = (CButton*)GetDlgItem(IDC_SHOW_NUMBERS);
   m_DrawNumbers = pBut->GetCheck()!=0 ? TRUE:FALSE;
   Invalidate(TRUE);
}

void CTOGAGirderSelectStrandsDlg::OnCbnSelchangeComboViewloc()
{
   CComboBox* ppcl_ctrl = (CComboBox*)GetDlgItem(IDC_COMBO_VIEWLOC);
   m_IsMidSpan = ppcl_ctrl->GetCurSel();
   Invalidate(TRUE);
}

void CTOGAGirderSelectStrandsDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_DIRECT_STRAND_FILL );
}

void CTOGAGirderSelectStrandsDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
   // Control min size of dialog to initial size
   if(m_bFirstSize && ::IsWindow(this->m_hWnd))
   {
      // Grab initial size of dialog
      CRect rect;
      GetWindowRect(&rect);
      if (!rect.IsRectEmpty())
      {
         m_FirstSize = rect.Size();
         m_bFirstSize = false;
      }
   }

   if (!m_bFirstSize)
   {
      lpMMI->ptMinTrackSize.x =  m_FirstSize.cx;//  - allow X to be shrunk, but dont allow buttons to be hidden
      lpMMI->ptMinTrackSize.y =  m_FirstSize.cy;
   }

   CDialog::OnGetMinMaxInfo(lpMMI);
}

HBRUSH CTOGAGirderSelectStrandsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   int ID = pWnd->GetDlgCtrlID();
   switch( ID )
   {
   case IDC_STRAIGHT:
      pDC->SetTextColor(STRAIGHT_FILL_COLOR);
      break;

   case IDC_DEBONDED:
      pDC->SetTextColor(DEBOND_FILL_COLOR);
      break;
   }

   return hbr;
}
