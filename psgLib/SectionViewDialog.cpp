///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SectionViewDialog.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "SectionViewDialog.h"
#include <GraphicsLib\PointMapper.h>
#include <PgsExt\GirderLabel.h>
#include "PGSuperColors.h"

#include <GeomModel\IShape.h>
#include <GeomModel\Circle.h>

#include <GeomModel\Polygon.h>

#include <WBFLGeometry.h>
#include <WBFLSections.h>
#include <WBFLGenericBridge.h>

#include <EAF\EAFApp.h>

#include <IFace\BeamFactory.h>


#ifdef _DEBUG
#include <Plugins\ConfigureStrandMover.h> // including here is a bit of a hack, but drawing the strand mover is debug only
#include <WBFLGenericBridgeTools.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BORDER 7

#define SHAPE_COLOR       RGB(160,160,160)
#define VOID_COLOR        RGB(255,255,255)
#define BACKGROUND_COLOR  RGB(255,255,255)


#define DUMMY_AGENT_ID INVALID_ID

/////////////////////////////////////////////////////////////////////////////
// CSectionViewDialog dialog


CSectionViewDialog::CSectionViewDialog(const GirderLibraryEntry* pEntry,bool isEnd,CWnd* pParent /*=nullptr*/)
	: CDialog(CSectionViewDialog::IDD, pParent)
{
   m_pGirderEntry = pEntry;
   m_bIsEnd = isEnd;
   m_bDrawNumbers = true;

   // assume 0.6" diameter
   m_Radius = ::ConvertToSysUnits(0.3,unitMeasure::Inch);

   // first get outer shape
   CComPtr<IBeamFactory> pFactory;
   m_pGirderEntry->GetBeamFactory(&pFactory);
   GirderLibraryEntry::Dimensions dimensions = m_pGirderEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   pFactory->CreateGirderSection(nullptr,DUMMY_AGENT_ID,dimensions,-1.0,-1.0,&gdrSection);

   gdrSection.QueryInterface(&m_pShape);
   ATLASSERT(m_pShape != nullptr);

#ifdef _DEBUG
   // only use strandmover view for debugging
   CComPtr<IStrandMover> strand_mover;
   pFactory->CreateStrandMover(dimensions, -1,
                               IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0,
                               IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0, 
                               0.0, 0.0, &strand_mover);

   CComQIPtr<IConfigureStrandMover> config(strand_mover);

   IndexType num_shapes;
   config->GetRegionCount(&num_shapes);
   for (IndexType is=0; is<num_shapes; is++)
   {
      CComPtr<IShape> rshape;
      Float64 slope;
      config->GetRegion(is, &rshape, &slope);
      m_RegionShapes.push_back(rshape);
   }
#endif

   m_pShape->get_ShapeProperties(&m_ShapeProps);

   Float64 Yt, Yb;
   m_ShapeProps->get_Ytop(&Yt);
   m_ShapeProps->get_Ybottom(&Yb);
   m_Hg = Yt + Yb;

   Float64 Xl, Xr;
   m_ShapeProps->get_Xleft(&Xl);
   m_ShapeProps->get_Xright(&Xr);
   m_Wg = Xl + Xr;
   m_bDrawWebSections = true;

   CComQIPtr<IPrestressedGirderSection> psgSection(gdrSection);
   psgSection->GetWebSections(&m_Y, &m_W, &m_Desc);


   HRESULT hr = psgSection->GetWebWidthProjectionsForDebonding(&m_WebWidthProjections);
   if (SUCCEEDED(hr) && m_WebWidthProjections)
   {
      m_bDrawWebWidthProjections = true;
   }
   else
   {
      // there aren't any web width projections to show
      m_bDrawWebWidthProjections = false;
   }


	//{{AFX_DATA_INIT(CSectionViewDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CSectionViewDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSectionViewDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSectionViewDialog, CDialog)
	//{{AFX_MSG_MAP(CSectionViewDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
//   ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP

   ON_BN_CLICKED(IDC_SHOWS,OnClickNumbers)
   ON_BN_CLICKED(IDC_WEB_SECTIONS, OnClickWebSections)
   ON_BN_CLICKED(IDC_SHOW_WEB_WIDTH_PROJECTIONS, &CSectionViewDialog::OnBnClickedShowWebWidthProjections)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSectionViewDialog message handlers

HWND hwndOK;
inline BOOL CALLBACK ChildWindowRect(HWND hwnd, LPARAM lParam)
{
   if (hwnd == hwndOK)
      return TRUE;

   RECT r;
   ::GetClientRect(hwnd, &r);

   CRect* pRect = (CRect*)lParam;
   pRect->UnionRect(pRect, &r);
   return TRUE;
}

void CSectionViewDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// Do not call CDialog::OnPaint() for painting messages
   CWnd* pBtn = GetDlgItem(IDOK);
   hwndOK = pBtn->GetSafeHwnd();
   CRect rBtn;
   pBtn->GetWindowRect(&rBtn);

   CRect dRect;
   GetWindowRect(&dRect);
   int bottom_dlg_to_top_of_button = dRect.bottom - rBtn.top;

   CRect childRect(0,0,0,0);
   EnumChildWindows(GetSafeHwnd(), ChildWindowRect, (LPARAM)&childRect);

   CRect cr;
   GetClientRect(&cr);
   CSize csize = cr.Size();
   csize.cy -= bottom_dlg_to_top_of_button;
   csize.cy -= 3*BORDER;
   csize.cx -= 3*BORDER;
   csize.cx -= childRect.Width();


   CComPtr<IRect2d> bbox;
   m_pShape->get_BoundingBox(&bbox);
   Float64 left,right,top,bottom;
   bbox->get_Left(&left);
   bbox->get_Right(&right);
   bbox->get_Top(&top);
   bbox->get_Bottom(&bottom);

   GraphRect box(left,bottom,right,top);
   GraphSize size = box.Size();
   GraphPoint org = box.BottomCenter();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(csize.cx / 2 + 2 * BORDER + childRect.Width(), csize.cy + 2 * BORDER);

   DrawShape(&dc,mapper);
   DrawStrands(&dc,mapper,m_bIsEnd);

   if (m_bDrawWebSections)
   {
      DrawWebSections(&dc, mapper);
   }

   if (m_bDrawWebWidthProjections)
   {
      DrawWebWidthProjections(&dc, mapper);
   }

#ifdef _DEBUG
   // strand mover bounds
   CBrush shape_brush(HS_FDIAGONAL, GREEN);
   CBrush* pOldBrush = dc.SelectObject(&shape_brush);

   for ( const auto& shape : m_RegionShapes)
   {
      DrawShape(&dc,mapper,shape);
   }

   dc.SelectObject(pOldBrush);
#endif
}

void PrintNumber(CDC* pDC, grlibPointMapper& Mapper, const GraphPoint& loc, StrandIndexType strandIdx)
{
   long x, y;
   Mapper.WPtoDP(loc.X(), loc.Y(), &x, &y);

   CString str;
   str.Format(_T("%d"),strandIdx);

   pDC->TextOut(x, y, str);
}

void CSectionViewDialog::DrawShape(CDC* pDC, grlibPointMapper& Mapper)
{
   CComQIPtr<ICompositeShape> compshape(m_pShape);

   CBrush shape_brush(SHAPE_COLOR);
   CBrush void_brush(VOID_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&shape_brush);

   if ( compshape )
   {
      DrawShape(pDC,Mapper,compshape,shape_brush,void_brush);
   }
   else
   {
      DrawShape(pDC,Mapper,m_pShape);
   }

   pDC->SelectObject(pOldBrush);
}

void CSectionViewDialog::DrawShape(CDC* pDC,grlibPointMapper& mapper,ICompositeShape* pCompositeShape,CBrush& solidBrush,CBrush& voidBrush)
{
   CollectionIndexType nShapes;
   pCompositeShape->get_Count(&nShapes);
   for ( CollectionIndexType idx = 0; idx < nShapes; idx++ )
   {
      CComPtr<ICompositeShapeItem> item;
      pCompositeShape->get_Item(idx,&item);

      CComPtr<IShape> shape;
      item->get_Shape(&shape);

      VARIANT_BOOL bVoid;
      item->get_Void(&bVoid);

      if ( bVoid == VARIANT_TRUE )
      {
         pDC->SelectObject(&voidBrush);
      }
      else
      {
         pDC->SelectObject(&solidBrush);
      }

      CComQIPtr<ICompositeShape> composite(shape);
      if ( composite )
      {
         DrawShape(pDC,mapper,composite,solidBrush,voidBrush);
      }
      else
      {
         DrawShape(pDC,mapper,shape);
      }
   }
}

void CSectionViewDialog::DrawShape(CDC* pDC,grlibPointMapper& Mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   CollectionIndexType nPoints;
   polypoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   polypoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      long dx,dy;
      GraphPoint point;
      points[i]->Location(&point.X(), &point.Y());
      Mapper.WPtoDP(point,&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   pDC->Polygon(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}

void CSectionViewDialog::DrawStrands(CDC* pDC, grlibPointMapper& Mapper, bool isEnd)
{
   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* pOldFont = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   CBrush straight_strand_brush(STRAIGHT_FILL_COLOR);
   CPen   straight_strand_pen(1,PS_SOLID,STRAIGHT_FILL_COLOR);

   CBrush straight_db_strand_brush(DEBOND_FILL_COLOR); // debondable
   CPen   straight_db_strand_pen(1,PS_SOLID,DEBOND_FILL_COLOR);

   CBrush harped_strand_brush(HARPED_FILL_COLOR);
   CPen   harped_strand_pen(1,PS_SOLID,HARPED_FILL_COLOR);

   CBrush temp_strand_brush(TEMPORARY_FILL_COLOR);
   CPen   temp_strand_pen(1,PS_SOLID,TEMPORARY_FILL_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&straight_strand_brush);
   CPen*   pOldPen   = pDC->SelectObject(&straight_strand_pen);

   // loop over global strand fill order for permanent strands
   bool use_diff_hg = m_pGirderEntry->IsDifferentHarpedGridAtEndsUsed();

   StrandIndexType total_strand_cnt = 0;
   StrandIndexType num_global = m_pGirderEntry->GetPermanentStrandGridSize();
   for (StrandIndexType idx = 0; idx < num_global; idx++)
   {
      StrandIndexType strand_idx;
      GirderLibraryEntry::psStrandType strand_type;
      m_pGirderEntry->GetGridPositionFromPermStrandGrid(idx, &strand_type, &strand_idx);

      if (strand_type == GirderLibraryEntry::stStraight)
      {
         Float64 xStart,yStart,xEnd,yEnd;
         bool can_debond;
         m_pGirderEntry->GetStraightStrandCoordinates(strand_idx, &xStart, &yStart, &xEnd, &yEnd, &can_debond);

         pDC->SelectObject(can_debond ? &straight_db_strand_brush : &straight_strand_brush);
         pDC->SelectObject(can_debond ? &straight_db_strand_pen   : &straight_strand_pen);

         total_strand_cnt = DrawStrand(pDC, Mapper, xStart, yStart, total_strand_cnt);
      }
      else if (strand_type == GirderLibraryEntry::stAdjustable)
      {
         Float64 start_x, start_y, hp_x, hp_y, end_x, end_y;
         m_pGirderEntry->GetHarpedStrandCoordinates(strand_idx, &start_x, &start_y, &hp_x, &hp_y, &end_x, &end_y);

         Float64 x = hp_x;
         Float64 y = hp_y;
         if (isEnd && use_diff_hg)
         {
            x = start_x;
            y = start_y;
         }

         pDC->SelectObject(&harped_strand_brush);
         pDC->SelectObject(&harped_strand_pen);

         StrandIndexType strandInc = 1;
         if (IsZero(x) && (!IsZero(start_x) || !IsZero(hp_x) || !IsZero(end_x)))
         {
            strandInc = 2;
         }

         total_strand_cnt = DrawStrand(pDC, Mapper, x, y, total_strand_cnt,strandInc);
      }
      else
      {
         ATLASSERT(false);
      }
   }

   // temporary strands
   pDC->SelectObject(&temp_strand_brush);
   pDC->SelectObject(&temp_strand_pen);
   StrandIndexType nTemp = m_pGirderEntry->GetNumTemporaryStrandCoordinates();
   StrandIndexType tid=0;
   for ( StrandIndexType idx = 0; idx < nTemp; idx++  )
   {
      Float64 x_start,y_start,x_end,y_end;
      m_pGirderEntry->GetTemporaryStrandCoordinates(idx,&x_start,&y_start,&x_end,&y_end);

      tid = DrawStrand(pDC, Mapper, x_start, y_start, tid);
   }

   pDC->SelectObject(pOldFont);
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
}

StrandIndexType CSectionViewDialog::DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index,StrandIndexType strandInc)
{
   // Strands are defined assuming bottom of girder is at (0,0), however
   // the girder shape is actually defined with (0,0) at the top center
   // For each strand elevation, let y = yStrand - Hg... this will put
   // the strand elevation on the same basis as the girder section
   y -= m_Hg;

   CRect rect;
   Mapper.WPtoDP(x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
   Mapper.WPtoDP(x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
   Mapper.WPtoDP(x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
   Mapper.WPtoDP(x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

   pDC->Ellipse(&rect);

   index += strandInc;

   if (m_bDrawNumbers)
   {
      PrintNumber(pDC, Mapper, GraphPoint(x, y), index);
   }

   if (0.0 < x)
   {
      Mapper.WPtoDP(-x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
      Mapper.WPtoDP(-x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
      Mapper.WPtoDP(-x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
      Mapper.WPtoDP(-x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

      pDC->Ellipse(&rect);

      ATLASSERT(strandInc == 1);
      index += strandInc;

      GraphPoint np(-x,y);
      if (m_bDrawNumbers)
      {
         PrintNumber(pDC, Mapper, np, index);
      }
   }

   return index;
}

void CSectionViewDialog::DrawWebSections(CDC* pDC, grlibPointMapper& Mapper)
{
   USES_CONVERSION;
   CPen pen(1, PS_DASH, BROWN);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CFont font;
   font.CreatePointFont(80, _T("Arial"), pDC);
   CFont* pOldFont = pDC->SelectObject(&font);

   auto old_align = pDC->SetTextAlign(TA_CENTER | TA_BOTTOM);

   IndexType nItems;
   m_Y->get_Count(&nItems);
   for (IndexType i = 0; i < nItems; i++)
   {
      Float64 Yg;
      m_Y->get_Item(i, &Yg);
      LONG x, y;
      Mapper.WPtoDP(-m_Wg / 2, Yg, &x, &y);
      pDC->MoveTo(x, y);
      Mapper.WPtoDP(m_Wg / 2, Yg, &x, &y);
      pDC->LineTo(x, y);

      Mapper.WPtoDP(0, Yg, &x, &y);
      CComBSTR bstrDesc;
      m_Desc->get_Item(i, &bstrDesc);
      pDC->TextOut(x, y, OLE2T(bstrDesc));
   }
   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldFont);
   pDC->SetTextAlign(old_align);
}

void CSectionViewDialog::DrawWebWidthProjections(CDC* pDC, grlibPointMapper& Mapper)
{
   if (m_WebWidthProjections)
   {
      CBrush shape_brush(HS_BDIAGONAL, RED);
      CBrush* pOldBrush = pDC->SelectObject(&shape_brush);

      CComPtr<IEnumUnkArray> enumElements;
      m_WebWidthProjections->get__EnumElements(&enumElements);
      CComPtr<IUnknown> unk;
      while (enumElements->Next(1, &unk, nullptr) != S_FALSE)
      {
         CComQIPtr<IRect2d> rect(unk);

         CComPtr<IPoint2d> pnt;
         rect->get_TopLeft(&pnt);

         Float64 x, y;
         pnt->Location(&x, &y);

         LONG Xtl, Ytl;
         Mapper.WPtoDP(x, y, &Xtl, &Ytl);

         pnt.Release();
         rect->get_BottomRight(&pnt);
         pnt->Location(&x, &y);
         LONG Xbr, Ybr;
         Mapper.WPtoDP(x, y, &Xbr, &Ybr);

         pDC->Rectangle(Xtl, Ytl, Xbr, Ybr);

         unk.Release();
      }

      pDC->SelectObject(pOldBrush);
   }
}

void CSectionViewDialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
   CRect rClient;
   GetClientRect(&rClient);

   CWnd* pBtn = GetDlgItem(IDC_SECTION_PROPERTIES);

   if (pBtn)
   {
	   // ok button
      pBtn = GetDlgItem(IDOK);

      CRect rBtn;
      pBtn->GetWindowRect(&rBtn);

      int top_close = rClient.Size().cy - rBtn.Size().cy - BORDER;

      pBtn->MoveWindow(rClient.Size().cx - rBtn.Size().cx - BORDER,
                       top_close,
                       rBtn.Size().cx, rBtn.Size().cy);

      Invalidate();
   }
}

HBRUSH CSectionViewDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

   if (pWnd->GetDlgCtrlID() == IDC_SS)
   {
       pDC->SetTextColor(STRAIGHT_FILL_COLOR);
   }
   else if (pWnd->GetDlgCtrlID() == IDC_DB)
   {
       pDC->SetTextColor(DEBOND_FILL_COLOR);
   }
   else if (pWnd->GetDlgCtrlID() == IDC_HS)
   {
       pDC->SetTextColor(HARPED_FILL_COLOR);
   }
   else if (pWnd->GetDlgCtrlID() == IDC_TS)
   {
       pDC->SetTextColor(TEMPORARY_FILL_COLOR);
   }

	return hbr;
}

BOOL CSectionViewDialog::OnInitDialog() 
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CDialog::OnInitDialog();
	
   CButton* pBtn = (CButton*)GetDlgItem(IDC_SHOWS);
   pBtn->SetCheck(m_bDrawNumbers ? BST_CHECKED : BST_UNCHECKED);

   pBtn = (CButton*)GetDlgItem(IDC_WEB_SECTIONS);
   pBtn->SetCheck(m_bDrawWebSections ? BST_CHECKED : BST_UNCHECKED);

   pBtn = (CButton*)GetDlgItem(IDC_SHOW_WEB_WIDTH_PROJECTIONS);
   pBtn->SetCheck(m_bDrawWebWidthProjections ? BST_CHECKED : BST_UNCHECKED);

   if (m_WebWidthProjections == nullptr)
   {
      GetDlgItem(IDC_SHOW_WEB_WIDTH_PROJECTIONS)->EnableWindow(FALSE);
   }

   // label for harped or adj straight
   CString hlbl;
   hlbl.Format(_T("%s Strands"), LABEL_HARP_TYPE(m_pGirderEntry->GetAdjustableStrandType()!=pgsTypes::asHarped));
   CWnd* pWnd = GetDlgItem(IDC_HS);
   pWnd->SetWindowText(hlbl);

   CStatic* pShapeProps = (CStatic*)GetDlgItem(IDC_SECTION_PROPERTIES);

   Float64 Area, Ix, Iy, Ytop, Ybot, Xleft, Xright, Stop, Sbot;
   m_ShapeProps->get_Area(&Area);
   m_ShapeProps->get_Ixx(&Ix);
   m_ShapeProps->get_Iyy(&Iy);
   m_ShapeProps->get_Ytop(&Ytop);
   m_ShapeProps->get_Ybottom(&Ybot);
   m_ShapeProps->get_Xleft(&Xleft);
   m_ShapeProps->get_Xright(&Xright);
   Stop = Ix/Ytop;
   Sbot = Ix/Ybot;

   Float64 Kt = Sbot/Area;
   Float64 Kb = Stop/Area;

   Float64 H = Ytop + Ybot;
   Float64 W = Xleft + Xright;

   CString strProps;
   strProps.Format(_T("Nominal Properties\r\nArea = %s\r\nYt = %s\r\nYb = %s\r\nIx = %s\r\nIy = %s\r\nSt = %s\r\nSb = %s\r\nH = %s\r\nW = %s\r\nKt = %s\r\nKb = %s"),
      ::FormatDimension(Area,pDisplayUnits->Area),
      ::FormatDimension(Ytop,pDisplayUnits->ComponentDim),
      ::FormatDimension(Ybot,pDisplayUnits->ComponentDim),
      ::FormatDimension(Ix,pDisplayUnits->MomentOfInertia),
      ::FormatDimension(Iy,pDisplayUnits->MomentOfInertia),
      ::FormatDimension(Stop,pDisplayUnits->SectModulus),
      ::FormatDimension(Sbot,pDisplayUnits->SectModulus),
      ::FormatDimension(H,pDisplayUnits->ComponentDim),
      ::FormatDimension(W,pDisplayUnits->ComponentDim),
      ::FormatDimension(Kt,pDisplayUnits->ComponentDim),
      ::FormatDimension(Kb,pDisplayUnits->ComponentDim));

   pShapeProps->SetWindowText(strProps);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSectionViewDialog::OnClickNumbers()
{
   m_bDrawNumbers = !m_bDrawNumbers;
   Invalidate();
}

void CSectionViewDialog::OnClickWebSections()
{
   m_bDrawWebSections = !m_bDrawWebSections;
   Invalidate();
}

void CSectionViewDialog::OnBnClickedShowWebWidthProjections()
{
   m_bDrawWebWidthProjections = !m_bDrawWebWidthProjections;
   Invalidate();
}

BOOL CSectionViewDialog::OnEraseBkgnd(CDC* pDC)
{
    CRect rect;
    GetClientRect(&rect);
    CBrush myBrush(BACKGROUND_COLOR);    // dialog background color
    CBrush *pOld = pDC->SelectObject(&myBrush);
    BOOL bRes  = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
    pDC->SelectObject(pOld);    // restore old brush
    return bRes;                       // CDialog::OnEraseBkgnd(pDC);
}
