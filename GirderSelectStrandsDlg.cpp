///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// GirderSelectStrandsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "GirderSelectStrandsDlg.h"
#include "htmlhelp\HelpTopics.hh"
#include <WBFLGenericBridgeTools.h>

#include <EAF\EAFDisplayUnits.h>

#include <DesignConfigUtil.h>
#include "PGSuperColors.h"
#include "PGSuperUIUtil.h"

#define BORDER 7

// Utility functions
inline void UpdateHarpedOffsetLabel(CWnd* pwnd, HarpedStrandOffsetType type, bool areHarpedStraight)
{
   CString msg;
   CString mss(areHarpedStraight ? _T("S-W") : _T("HS"));
   switch(type)
   {
      case hsoCGFROMTOP:
         msg.Format(_T("%s CG to Girder Top"), mss);
         break;
      case hsoCGFROMBOTTOM:
         msg.Format(_T("%s CG to Girder Bottom"), mss);
         break;
      case hsoTOP2TOP:
         msg.Format(_T("Top-Most %s to Girder Top"), mss);
         break;
      case hsoTOP2BOTTOM:
         msg.Format(_T("Top-Most %s to Girder Bottom"), mss);
         break;
      case hsoBOTTOM2BOTTOM:
         msg.Format(_T("Bottom-Most %s to Girder Bottom"), mss);
         break;
      case hsoECCENTRICITY:
         msg.Format(_T("Eccentricity of %s Group"), mss);
         break;
      default:
         msg = _T("Unknown Adjustment type");
         ATLASSERT(0);
   }

   pwnd->SetWindowText(msg);
}


// CGirderSelectStrandsDlg dialog

IMPLEMENT_DYNAMIC(CGirderSelectStrandsDlg, CDialog)

CGirderSelectStrandsDlg::CGirderSelectStrandsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGirderSelectStrandsDlg::IDD, pParent),
   m_IsMidSpan(0),
   m_bFirstSize(true)
{
   m_DrawNumbers = TRUE;

   m_Radius = ::ConvertToSysUnits(0.3,unitMeasure::Inch) * 1.5;

   m_pToolTip = NULL;
}

CGirderSelectStrandsDlg::~CGirderSelectStrandsDlg()
{
   delete  m_pToolTip;
}

void CGirderSelectStrandsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   DDX_Check(pDX, IDC_CHECK_SYMM, m_bSymmetricDebond);
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

BEGIN_MESSAGE_MAP(CGirderSelectStrandsDlg, CDialog)
   ON_BN_CLICKED(IDC_CHECK_SYMM, &CGirderSelectStrandsDlg::OnBnClickedCheckSymm)
	ON_WM_PAINT()
	ON_WM_SIZE()
   ON_WM_LBUTTONUP()
   ON_BN_CLICKED(IDC_SHOW_NUMBERS, &CGirderSelectStrandsDlg::OnBnClickedShowNumbers)
   ON_CBN_SELCHANGE(IDC_COMBO_VIEWLOC, &CGirderSelectStrandsDlg::OnCbnSelchangeComboViewloc)
   ON_CBN_SELCHANGE(IDC_HARP_END_CB, &CGirderSelectStrandsDlg::OnCbnSelchangeHarpEndCb)
   ON_CBN_SELCHANGE(IDC_HARP_HP_CB, &CGirderSelectStrandsDlg::OnCbnSelchangeHarpHpCb)
   ON_BN_CLICKED(ID_HELP, &CGirderSelectStrandsDlg::OnHelp)
   ON_WM_GETMINMAXINFO()
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// CGirderSelectStrandsDlg message handlers

BOOL CGirderSelectStrandsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

    //Set up the tooltip
    m_pToolTip = new CToolTipCtrl;
    VERIFY(m_pToolTip->Create(this));
    VERIFY(m_pToolTip->AddTool(this,_T("Click on strands to select.")));

	m_Grid.SubclassDlgItem(IDC_STRAND_GRID, this);
   m_Grid.CustomInit(this,m_pGdrEntry);

   if (m_CanDebondStrands)
   {
      OnBnClickedCheckSymm();
   }
   else
   {
      CButton* pBut = (CButton*)this->GetDlgItem(IDC_CHECK_SYMM);
      pBut->ShowWindow(SW_HIDE);
   }

   if ( !m_bCanExtendStrands )
   {
      GetDlgItem(IDC_EXTENDED_LEFT)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EXTENDED_RIGHT)->ShowWindow(SW_HIDE);
   }

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

void CGirderSelectStrandsDlg::InitializeData(SpanIndexType span, GirderIndexType girder, const CPrestressData& rPrestressData, 
                    const SpecLibraryEntry* pSpecEntry,const GirderLibraryEntry* pGdrEntry, 
                    bool allowEndAdjustment, bool allowHpAdjustment, HarpedStrandOffsetType endMeasureType, HarpedStrandOffsetType hpMeasureType,
                    Float64 hpOffsetAtEnd, Float64 hpOffsetAtHp, Float64 maxDebondLength)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   m_pGdrEntry = pGdrEntry;
   m_Span = span;
   m_Girder = girder;
   m_DirectFilledStraightStrands  = *(rPrestressData.GetDirectStrandFillStraight());
   m_DirectFilledHarpedStrands    = *(rPrestressData.GetDirectStrandFillHarped());
   m_DirectFilledTemporaryStrands = *(rPrestressData.GetDirectStrandFillTemporary());

   m_AdjustableStrandType = rPrestressData.GetAdjustableStrandType();

   m_AllowEndAdjustment = allowEndAdjustment;
   m_AllowHpAdjustment  = allowHpAdjustment;

   // Get current offset input values - we will force in bounds if needed
   // Make sure legacy values can't sneak in
   m_HsoEndMeasurement = endMeasureType==hsoLEGACY ? hsoCGFROMTOP    : endMeasureType;
   m_HsoHpMeasurement =  hpMeasureType==hsoLEGACY  ? hsoCGFROMBOTTOM : hpMeasureType;

   m_HpOffsetAtEnd = hpOffsetAtEnd;
   m_HpOffsetAtHp  = hpOffsetAtHp;

   m_bCanExtendStrands = pSpecEntry->AllowStraightStrandExtensions();
   m_ExtendedStrands[pgsTypes::metStart] = rPrestressData.GetExtendedStrands(pgsTypes::Straight,pgsTypes::metStart);
   m_ExtendedStrands[pgsTypes::metEnd]   = rPrestressData.GetExtendedStrands(pgsTypes::Straight,pgsTypes::metEnd);

   m_CanDebondStrands = pGdrEntry->CanDebondStraightStrands();
   m_StraightDebond   = rPrestressData.Debond[pgsTypes::Straight];
   m_bSymmetricDebond = rPrestressData.bSymmetricDebond;

   m_MaxDebondLength = maxDebondLength;

}

bool CGirderSelectStrandsDlg::GetData(CPrestressData& rPrestressData)
{
   rPrestressData.SetDirectStrandFillStraight( m_DirectFilledStraightStrands );
   rPrestressData.SetDirectStrandFillHarped( m_DirectFilledHarpedStrands );
   rPrestressData.SetDirectStrandFillTemporary( m_DirectFilledTemporaryStrands );

   if(m_AllowEndAdjustment)
   {
      rPrestressData.HpOffsetAtEnd = m_HpOffsetAtEnd;
   }

   if(m_AllowHpAdjustment)
   {
      rPrestressData.HpOffsetAtHp = m_HpOffsetAtHp;
   }

   if (m_CanDebondStrands)
   {
      rPrestressData.Debond[pgsTypes::Straight] = m_StraightDebond;
      rPrestressData.bSymmetricDebond           = m_bSymmetricDebond!=FALSE;
   }

   if (m_bCanExtendStrands )
   {
      rPrestressData.SetExtendedStrands(pgsTypes::Straight,pgsTypes::metStart,m_ExtendedStrands[pgsTypes::metStart]);
      rPrestressData.SetExtendedStrands(pgsTypes::Straight,pgsTypes::metEnd,  m_ExtendedStrands[pgsTypes::metEnd]);
   }

   return true;
}


void CGirderSelectStrandsDlg::OnBnClickedCheckSymm()
{
   CButton* pBut = (CButton*)this->GetDlgItem(IDC_CHECK_SYMM);
   BOOL is_sym = pBut->GetCheck()!=0 ? TRUE:FALSE;

   m_bSymmetricDebond = is_sym;
   m_Grid.SymmetricDebond(is_sym);
}

void CGirderSelectStrandsDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

   CWnd* pPicture = GetDlgItem(IDC_PICTURE);

   if ( pPicture == NULL )
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

   pWnd = GetDlgItem(IDC_HARPED);
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   h = rect.Height();
   rect.top = rPicture.bottom + m_Row1Offset;
   rect.bottom = rect.top + h;
   pWnd->MoveWindow(&rect);

   pWnd = GetDlgItem(IDC_TEMPORARY);
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   h = rect.Height();
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

   pWnd = GetDlgItem(IDC_EXTENDED_LEFT);
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   h = rect.Height();
   rect.top = rPicture.bottom + m_Row2Offset;
   rect.bottom = rect.top + h;
   pWnd->MoveWindow(&rect);

   pWnd = GetDlgItem(IDC_EXTENDED_RIGHT);
   pWnd->GetWindowRect(rect);
   ScreenToClient(&rect);
   h = rect.Height();
   rect.top = rPicture.bottom + m_Row2Offset;
   rect.bottom = rect.top + h;
   pWnd->MoveWindow(&rect);

   Invalidate();
   UpdateWindow();
}

void CGirderSelectStrandsDlg::OnPaint() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

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
   factory->CreateGirderSection(pBroker,-1,m_Span,m_Girder,dimensions,&gdrSection);

   CComQIPtr<IShape> shape(gdrSection);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);

   // Anchor bottom of drawing at bottom center of section shape
   Float64 bottom_width;
   gdrSection->get_BottomWidth(&bottom_width);

   CComPtr<IRect2d> shape_box;
   shape->get_BoundingBox(&shape_box);

   CComPtr<IPoint2d> objOrg;
   shape_box->get_BottomCenter(&objOrg);

   gpPoint2d orgin;
   Float64 x,y;
   objOrg->get_X(&x);
   objOrg->get_Y(&y);
   orgin.X() = x;
   orgin.Y() = y;

   // Get height and width of the area occupied by all possible strand locations

   // Convert adjustment of harped strands to absolute
   ConfigStrandFillVector  harped_fillvec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Harped, 
                                                        m_pGdrEntry->GetName().c_str(), m_DirectFilledHarpedStrands);


   Float64 absol_end_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType,
                                                        harped_fillvec, m_HsoEndMeasurement, m_HpOffsetAtEnd);

   Float64 absol_hp_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType,
                                                        harped_fillvec, m_HsoHpMeasurement, m_HpOffsetAtHp);

   // We need a strand mover to adjust harped strands
   Float64 end_incr = m_pGdrEntry->IsVerticalAdjustmentAllowedEnd() ? 0.0 : -1.0;
   Float64 hp_incr  = m_pGdrEntry->IsVerticalAdjustmentAllowedHP() ? 0.0 : -1.0;

   CComPtr<IStrandMover> strand_mover;
   factory->CreateStrandMover(dimensions, 
                              IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0,
                              IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0, 
                              end_incr, hp_incr, &strand_mover);

   gpRect2d strand_bounds = ComputeStrandBounds(strand_mover, absol_end_offset, absol_hp_offset);

   gpSize2d world_size;
   world_size.Dx() = max(bottom_width,strand_bounds.Width());

   world_size.Dy() = strand_bounds.Height();
   if ( IsZero(world_size.Dy()) )
      world_size.Dy() = world_size.Dx()/2;

   CSize client_size = rClient.Size();
   client_size -= CSize(10,10); // make client size slightly smaller so there 
   // is some space between the drawing and the edge of the picture control

   // This mapping pushes image to bottom
   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
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

   CPen solid_pen(PS_SOLID,1,GIRDER_BORDER_COLOR);
   CBrush solid_brush(GIRDER_FILL_COLOR);

   CPen void_pen(PS_SOLID,1,VOID_BORDER_COLOR);
   CBrush void_brush(GetSysColor(COLOR_WINDOW));

   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   CComQIPtr<ICompositeShape> compshape(shape);
   if ( compshape )
   {
      CollectionIndexType nShapes;
      compshape->get_Count(&nShapes);
      for ( CollectionIndexType idx = 0; idx < nShapes; idx++ )
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

   DrawStrands(pDC,mapper, strand_mover, absol_end_offset, absol_hp_offset);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CGirderSelectStrandsDlg::DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper)
{
   CComPtr<IPoint2dCollection> objPoints;
   shape->get_PolyPoints(&objPoints);

   CollectionIndexType nPoints;
   objPoints->get_Count(&nPoints);

   CPoint* points = new CPoint[nPoints];

   CComPtr<IPoint2d> point;
   long dx,dy;

   long i = 0;
   CComPtr<IEnumPoint2d> enumPoints;
   objPoints->get__Enum(&enumPoints);
   while ( enumPoints->Next(1,&point,NULL) != S_FALSE )
   {
      mapper.WPtoDP(point,&dx,&dy);

      points[i] = CPoint(dx,dy);

      point.Release();
      i++;
   }

   pDC->Polygon(points,(int)nPoints);

   delete[] points;
}

void CGirderSelectStrandsDlg::DrawStrands(CDC* pDC, grlibPointMapper& Mapper, IStrandMover* strand_mover, Float64 absol_end_offset, Float64 absol_hp_offset)
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

         bool bIsExtended = m_Grid.IsStrandExtended(strand_idx,pgsTypes::metStart) || m_Grid.IsStrandExtended(strand_idx,pgsTypes::metEnd) ? true : false;

         if ( is_filled )
         {
            if ( bIsExtended )
            {
               pDC->SelectObject(extended_strand_brush);
               pDC->SelectObject(extended_strand_pen);
            }
            else
            {
               pDC->SelectObject(bIsDebonded ? &straight_db_strand_brush : &straight_strand_brush);
               pDC->SelectObject(bIsDebonded ? &straight_db_strand_pen   : &straight_strand_pen);
            }
         }
         else
         {
            pDC->SelectObject(can_debond ? &straight_db_strand_brush : &straight_strand_brush);
            pDC->SelectObject(can_debond ? &straight_db_strand_pen   : &straight_strand_pen);
         }

         total_strand_cnt = DrawStrand(pDC, Mapper, xStart, yStart, total_strand_cnt, is_filled, grid_row);
      }
      else if (strand_type==GirderLibraryEntry::stAdjustable)
      {
         Float64 xs, ys, xe, ye, xhp, yhp;
         m_pGdrEntry->GetHarpedStrandCoordinates(strand_idx, &xs, &ys, &xhp, &yhp, &xe, &ye);

         Float64 x, y, offset;
         if (!m_IsMidSpan)
         {
            x = xs;
            y = ys;
            offset = absol_end_offset;
         }
         else
         {
            x = xhp;
            y = yhp;
            offset = absol_hp_offset;
         }

         Float64 xnew, ynew;
         strand_mover->TranslateStrand(x, y, offset, &xnew, &ynew);

         bool is_filled = m_DirectFilledHarpedStrands.IsStrandFilled(strand_idx);

         pDC->SelectObject(&harped_strand_brush);
         pDC->SelectObject(&harped_strand_pen);

         total_strand_cnt = DrawStrand(pDC, Mapper, xnew, ynew, total_strand_cnt, is_filled, grid_row);
      }
      else
      {
         ATLASSERT(0);
      }

      grid_row--;
   }

   // temporary strands
   pDC->SelectObject(&temp_strand_brush);
   pDC->SelectObject(&temp_strand_pen);
   StrandIndexType nTemp = m_pGdrEntry->GetNumTemporaryStrandCoordinates();
   StrandIndexType tid=0;
   for ( StrandIndexType idx = 0; idx < nTemp; idx++  )
   {
      Float64 x_start,y_start,x_end,y_end;
      m_pGdrEntry->GetTemporaryStrandCoordinates(idx,&x_start,&y_start,&x_end,&y_end);

      bool is_filled = m_DirectFilledTemporaryStrands.IsStrandFilled(idx);

      tid = DrawStrand(pDC, Mapper, x_start, y_start, tid, is_filled, grid_row);

      grid_row--;
   }

   pDC->SelectObject(pOldFont);
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

void PrintNumber(CDC* pDC, grlibPointMapper& Mapper, const gpPoint2d& loc, StrandIndexType strandIdx)
{
   long x, y;
   Mapper.WPtoDP(loc.X(), loc.Y(), &x, &y);

   // move down slightly
   y += 2;

   CString str;
   str.Format(_T("%d"),strandIdx);

   pDC->TextOut(x, y, str);
}

StrandIndexType CGirderSelectStrandsDlg::DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index, bool isFilled, ROWCOL gridRow)
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
      PrintNumber(pDC, Mapper, gpPoint2d(x,y), index);

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

      gpPoint2d np(-x,y);
      if ( m_DrawNumbers )
         PrintNumber(pDC, Mapper, np, index);
   }

   return index;
}

gpRect2d CGirderSelectStrandsDlg::ComputeStrandBounds(IStrandMover* strand_mover, Float64 absol_end_offset, Float64 absol_hp_offset)
{
   Float64 xmax(0.0), ymax(0.0);

   // Start with straight strands
   StrandIndexType ngrid = m_pGdrEntry->GetNumStraightStrandCoordinates();
   for(StrandIndexType igrid=0; igrid<ngrid; igrid++)
   {
      Float64 xs, ys, xe, ye;
      bool candb;
      m_pGdrEntry->GetStraightStrandCoordinates(igrid, &xs, &ys, &xe, &ye, &candb);

      xmax = max(xmax, xs);
      ymax = max(ymax, ys);
   }

   // Harped strands
   ngrid = m_pGdrEntry->GetNumHarpedStrandCoordinates();
   for(StrandIndexType igrid=0; igrid<ngrid; igrid++)
   {
      Float64 xs, ys, xe, ye, xhp, yhp;
      m_pGdrEntry->GetHarpedStrandCoordinates(igrid, &xs, &ys, &xhp, &yhp, &xe, &ye);

      Float64 x, y, offset;
      if (!m_IsMidSpan)
      {
         x = xs;
         y = ys;
         offset = absol_end_offset;
      }
      else
      {
         x = xhp;
         y = yhp;
         offset = absol_hp_offset;
      }

      Float64 xnew, ynew;
      strand_mover->TranslateStrand(x, y, offset, &xnew, &ynew);

      xmax = max(xmax, xnew);
      ymax = max(ymax, ynew);
   }

   // Temporary strands
   ngrid = m_pGdrEntry->GetNumTemporaryStrandCoordinates();
   for(StrandIndexType igrid=0; igrid<ngrid; igrid++)
   {
      Float64 xs, ys, xe, ye;
      m_pGdrEntry->GetTemporaryStrandCoordinates(igrid, &xs, &ys, &xe, &ye);

      xmax = max(xmax, xs);
      ymax = max(ymax, ys);
   }

   return gpRect2d(-xmax-m_Radius, 0.0, xmax+m_Radius, ymax+m_Radius);
}

void CGirderSelectStrandsDlg::OnNumStrandsChanged()
{
   UpdateStrandInfo();
   UpdateStrandAdjustments();
   UpdatePicture();
}

void CGirderSelectStrandsDlg::UpdateStrandInfo()
{
   m_Grid.UpdateData(false);
   StrandIndexType nStraight  = m_DirectFilledStraightStrands.GetFilledStrandCount();
   StrandIndexType nHarped    = m_DirectFilledHarpedStrands.GetFilledStrandCount();
   StrandIndexType nTemporary = m_DirectFilledTemporaryStrands.GetFilledStrandCount();

   StrandIndexType nExtendedLeft  = 0;
   std::vector<StrandIndexType>::const_iterator its( m_ExtendedStrands[pgsTypes::metStart].begin() );
   std::vector<StrandIndexType>::const_iterator its_end( m_ExtendedStrands[pgsTypes::metStart].end() );
   for( ; its!=its_end; its++)
   {
      nExtendedLeft += m_DirectFilledStraightStrands.GetFillCountAtIndex( *its );
   }

   StrandIndexType nExtendedRight = 0;
   std::vector<StrandIndexType>::const_iterator ite( m_ExtendedStrands[pgsTypes::metEnd].begin() );
   std::vector<StrandIndexType>::const_iterator ite_end( m_ExtendedStrands[pgsTypes::metEnd].end() );
   for( ; ite!=ite_end; ite++)
   {
      nExtendedRight += m_DirectFilledStraightStrands.GetFillCountAtIndex( *ite );
   }

   StrandIndexType nDebonded = 0;
   std::vector<CDebondInfo>::iterator iter(m_StraightDebond.begin());
   std::vector<CDebondInfo>::iterator end(m_StraightDebond.end());
   for ( ; iter != end; iter++ )
   {
      CDebondInfo& debond_info = *iter;
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

   if (m_AdjustableStrandType==pgsTypes::asStraight)
   {
      msg.Format(_T("Adjustable-Straight (A-S)=%d"), nHarped);
   }
   else
   {
      msg.Format(_T("Harped (H)=%d"), nHarped);
   }
   GetDlgItem(IDC_HARPED)->SetWindowText(msg);

   msg.Format(_T("Temporary (T)=%d"), nTemporary);
   GetDlgItem(IDC_TEMPORARY)->SetWindowText(msg);

   if ( this->m_bCanExtendStrands )
   {
      msg.Format(_T("Extended Left=%d"),nExtendedLeft);
      GetDlgItem(IDC_EXTENDED_LEFT)->SetWindowText(msg);

      msg.Format(_T("Extended Right=%d"),nExtendedRight);
      GetDlgItem(IDC_EXTENDED_RIGHT)->SetWindowText(msg);
   }
}

void CGirderSelectStrandsDlg::UpdatePicture()
{
   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
}

void CGirderSelectStrandsDlg::OnLButtonUp(UINT nFlags, CPoint point)
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

void CGirderSelectStrandsDlg::AddClickRect(CRect rect, ROWCOL gridRow)
{
   // inflate rect a bit so it's easier to click 
   CSize size = rect.Size();
   int downinf = int(size.cy/1.5);
   rect.InflateRect(3,downinf,3,3); 
   m_StrandLocations.push_back( std::make_pair(rect, gridRow) );
}

BOOL CGirderSelectStrandsDlg::PreTranslateMessage(MSG* pMsg)
{

  if (NULL != m_pToolTip)
      m_pToolTip->RelayEvent(pMsg);

   return CDialog::PreTranslateMessage(pMsg);
}

void CGirderSelectStrandsDlg::OnBnClickedShowNumbers()
{
   CButton* pBut = (CButton*)GetDlgItem(IDC_SHOW_NUMBERS);
   m_DrawNumbers = pBut->GetCheck()!=0 ? TRUE:FALSE;
   Invalidate(TRUE);
}

void CGirderSelectStrandsDlg::OnCbnSelchangeComboViewloc()
{
   CComboBox* ppcl_ctrl = (CComboBox*)GetDlgItem(IDC_COMBO_VIEWLOC);
   m_IsMidSpan = ppcl_ctrl->GetCurSel();
   Invalidate(TRUE);
}

void CGirderSelectStrandsDlg::UpdateStrandAdjustments()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   // adjustment of harped strands at ends
   Float64 end_incr = pStrandGeometry->GetHarpedEndOffsetIncrement(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType);
   Float64 hpt_incr = pStrandGeometry->GetHarpedHpOffsetIncrement(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType);

   bool areHarpedStraight = m_AdjustableStrandType==pgsTypes::asStraight;

   if (IsLE(end_incr,0.0) && IsLE(hpt_incr,0.0))
   {
      // No chance for harped strand ajustment
      ShowHarpedAdjustmentControls(FALSE, areHarpedStraight);
   }
   else
   {
      // We can vertically adjust harped strands
      ShowHarpedAdjustmentControls(TRUE, areHarpedStraight);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
      const unitmgtLengthData& measUnit = pDisplayUnits->GetComponentDimUnit();

      // Unit tag
      GetDlgItem(IDC_HARP_END_UNIT)->SetWindowText(measUnit.UnitOfMeasure.UnitTag().c_str());
      GetDlgItem(IDC_HARP_HP_UNIT)->SetWindowText(measUnit.UnitOfMeasure.UnitTag().c_str());

      // End Control
      if (IsLE(end_incr,0.0))
      {
         ShowHarpedEndAdjustmentControls(FALSE,areHarpedStraight);
      }
      else
      {
         CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_HARP_END_CB);
         pctrl->ResetContent();

         StrandIndexType nh = m_DirectFilledHarpedStrands.GetFilledStrandCount();
         if ( nh > 0)
         {
            EnableHarpedEndAdjustmentControls(TRUE);

            ConfigStrandFillVector  harped_fillvec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Harped, 
                                                                 m_pGdrEntry->GetName().c_str(), m_DirectFilledHarpedStrands);


            Float64 lowRange, highRange;
            pStrandGeometry->ComputeValidHarpedOffsetForMeasurementTypeEnd(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType, harped_fillvec, m_HsoEndMeasurement, &lowRange, &highRange);

            Float64 low  = min(lowRange, highRange);
            Float64 high = max(lowRange, highRange);

            // Force current offset into allowable range - we could warn user?
            if(m_HpOffsetAtEnd<low)
               m_HpOffsetAtEnd = low;
            else if (m_HpOffsetAtEnd>high)
               m_HpOffsetAtEnd = high;

            FillComboWithUnitFloatRange(m_HpOffsetAtEnd, low, high, end_incr,
                                        pctrl, 2, measUnit.UnitOfMeasure);
         }
         else
         {
            EnableHarpedEndAdjustmentControls(FALSE);
         }

         UpdateHarpedOffsetLabel(GetDlgItem(IDC_HARP_END_STATIC2), m_HsoEndMeasurement, areHarpedStraight);
      }

      // Hpt Control
      if (IsLE(hpt_incr,0.0) || m_AdjustableStrandType==pgsTypes::asStraight)
      {
         ShowHarpedHpAdjustmentControls(FALSE);
      }
      else
      {
         CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_HARP_HP_CB);
         pctrl->ResetContent();

         StrandIndexType nh = m_DirectFilledHarpedStrands.GetFilledStrandCount();
         if ( nh > 0)
         {
            EnableHarpedHpAdjustmentControls(TRUE);


            ConfigStrandFillVector  harped_fillvec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Harped, 
                                                                 m_pGdrEntry->GetName().c_str(), m_DirectFilledHarpedStrands);

            Float64 lowRange, highRange;
            pStrandGeometry->ComputeValidHarpedOffsetForMeasurementTypeHp(m_pGdrEntry->GetName().c_str(), m_AdjustableStrandType, harped_fillvec, m_HsoHpMeasurement, &lowRange, &highRange);

            Float64 low  = min(lowRange, highRange);
            Float64 high = max(lowRange, highRange);

            if(m_HpOffsetAtHp<low)
               m_HpOffsetAtHp = low;
            else if (m_HpOffsetAtHp>high)
               m_HpOffsetAtHp = high;

            FillComboWithUnitFloatRange(m_HpOffsetAtHp, low, high, hpt_incr,
                                        pctrl, 2, measUnit.UnitOfMeasure);
         }
         else
         {
            EnableHarpedHpAdjustmentControls(FALSE);
         }

         UpdateHarpedOffsetLabel(GetDlgItem(IDC_HARP_HP_STATIC2), m_HsoHpMeasurement, areHarpedStraight);
      }
   }
}

void CGirderSelectStrandsDlg::ShowHarpedAdjustmentControls(BOOL show, bool areHarpStraight)
{
   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_HARP_GROUP);
   CString lbl;
   lbl.Format(_T("Vertical Adjustment of %s Strands"),LABEL_HARP_TYPE(areHarpStraight));
   pwnd->SetWindowText(lbl);

   ShowHarpedEndAdjustmentControls(show,areHarpStraight);

   if (areHarpStraight)
      show = FALSE;

   ShowHarpedHpAdjustmentControls(show);
}

void CGirderSelectStrandsDlg::ShowHarpedHpAdjustmentControls(BOOL show)
{
   GetDlgItem(IDC_HARP_HP_STATIC1)->ShowWindow(show? SW_SHOW:SW_HIDE);
   GetDlgItem(IDC_HARP_HP_STATIC2)->ShowWindow(show? SW_SHOW:SW_HIDE);
   GetDlgItem(IDC_HARP_HP_CB)->ShowWindow(show? SW_SHOW:SW_HIDE);
   GetDlgItem(IDC_HARP_HP_UNIT)->ShowWindow(show? SW_SHOW:SW_HIDE);
}

void CGirderSelectStrandsDlg::ShowHarpedEndAdjustmentControls(BOOL show, bool AreHarpStraight)
{
   CWnd* pWnd = GetDlgItem(IDC_HARP_END_STATIC1);
   if (show)
   {
      if(AreHarpStraight)
      {
         pWnd->SetWindowText(_T("Along Girder:"));
      }
      else
      {
         pWnd->SetWindowText(_T("At Girder Ends:"));
      }
   }
   else
   {
      pWnd->SetWindowText(_T("Not Available"));
   }

   GetDlgItem(IDC_HARP_END_STATIC2)->ShowWindow(show? SW_SHOW:SW_HIDE);
   GetDlgItem(IDC_HARP_END_CB)->ShowWindow(show? SW_SHOW:SW_HIDE);
   GetDlgItem(IDC_HARP_END_UNIT)->ShowWindow(show? SW_SHOW:SW_HIDE);
}

void CGirderSelectStrandsDlg::EnableHarpedHpAdjustmentControls(BOOL enable)
{
   GetDlgItem(IDC_HARP_HP_STATIC1)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_HP_STATIC2)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_HP_CB)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_HP_UNIT)->EnableWindow(enable);
}

void CGirderSelectStrandsDlg::EnableHarpedEndAdjustmentControls(BOOL enable)
{
   GetDlgItem(IDC_HARP_END_STATIC1)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_END_STATIC2)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_END_CB)->EnableWindow(enable);
   GetDlgItem(IDC_HARP_END_UNIT)->EnableWindow(enable);
}

void CGirderSelectStrandsDlg::OnCbnSelchangeHarpEndCb()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& measUnit = pDisplayUnits->GetComponentDimUnit();

   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_HARP_END_CB);
   int idx = pcb->GetCurSel();
   Float64 val = GetFloatFromCb(pcb, idx);
   val = ConvertToSysUnits(val, measUnit.UnitOfMeasure);

   // See if value has changed
   if (!IsEqual(m_HpOffsetAtEnd,val))
   {
      m_HpOffsetAtEnd = val;

      Invalidate(TRUE);
   }
}

void CGirderSelectStrandsDlg::OnCbnSelchangeHarpHpCb()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& measUnit = pDisplayUnits->GetComponentDimUnit();

   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_HARP_HP_CB);
   int idx = pcb->GetCurSel();
   Float64 val = GetFloatFromCb(pcb, idx);
   val = ConvertToSysUnits(val, measUnit.UnitOfMeasure);

   // See if value has changed

   if (!IsEqual(m_HpOffsetAtHp,val))
   {

      m_HpOffsetAtHp = val;

      Invalidate(TRUE);
   }
}

void CGirderSelectStrandsDlg::OnHelp()
{
   UINT helpID = IDH_GIRDER_DIRECT_STRAND_FILL;

   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, helpID );
}

void CGirderSelectStrandsDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
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

HBRUSH CGirderSelectStrandsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   int ID = pWnd->GetDlgCtrlID();
   switch( ID )
   {
   case IDC_STRAIGHT:
      pDC->SetTextColor(STRAIGHT_FILL_COLOR);
      break;

   case IDC_HARPED:
      pDC->SetTextColor(HARPED_FILL_COLOR);
      break;

   case IDC_TEMPORARY:
      pDC->SetTextColor(TEMPORARY_FILL_COLOR);
      break;

   case IDC_DEBONDED:
      pDC->SetTextColor(DEBOND_FILL_COLOR);
      break;

   case IDC_EXTENDED_LEFT:
   case IDC_EXTENDED_RIGHT:
      pDC->SetTextColor(EXTENDED_FILL_COLOR);
      break;
   }

   return hbr;
}
