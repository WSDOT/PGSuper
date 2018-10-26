///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <BridgeModeling\GirderProfile.h>

#include <GeomModel\Polygon.h>

#include <WBFLGeometry.h>
#include <WBFLSections.h>
#include <WBFLGenericBridge.h>

#include <EAF\EAFApp.h>

#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#include <Plugins\Beams.h> // including here is a bit of a hack, but drawing the strand mover is debug only
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


CSectionViewDialog::CSectionViewDialog(const GirderLibraryEntry* pEntry,bool isEnd,CWnd* pParent /*=NULL*/)
	: CDialog(CSectionViewDialog::IDD, pParent)
{
   m_pGirderEntry = pEntry;
   m_IsEnd = isEnd;
   m_DrawNumbers = true;

   // assume 0.6" diameter
   m_Radius = ::ConvertToSysUnits(0.3,unitMeasure::Inch);

   // first get outer shape
   CComPtr<IBeamFactory> pFactory;
   m_pGirderEntry->GetBeamFactory(&pFactory);
   GirderLibraryEntry::Dimensions dimensions = m_pGirderEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   pFactory->CreateGirderSection(NULL,DUMMY_AGENT_ID,dimensions,-1.0,-1.0,&gdrSection);

   gdrSection.QueryInterface(&m_pShape);
   ATLASSERT(m_pShape != NULL);

// only use strandmover view for debugging
#ifdef _DEBUG
   CComPtr<IStrandMover> strand_mover;
   pFactory->CreateStrandMover(dimensions, 
                               IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0,
                               IBeamFactory::BeamTop, 0.0, IBeamFactory::BeamBottom, 0.0, 
                               0.0, 0.0, &strand_mover);

   CComQIPtr<IConfigureStrandMover> config(strand_mover);

   IndexType num_shapes;
   config->get_NumRegions(&num_shapes);
   for (IndexType is=0; is<num_shapes; is++)
   {
      CComPtr<IShape> rshape;
      Float64 slope;
      config->GetRegion(is, &rshape, &slope);
      m_RegionShapes.push_back(rshape);
   }
#endif

   m_pShape->get_ShapeProperties(&m_ShapeProps);

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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSectionViewDialog message handlers

void CSectionViewDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// Do not call CDialog::OnPaint() for painting messages
   CWnd* pBtn = GetDlgItem(IDOK);
   CRect rBtn;
   pBtn->GetWindowRect(&rBtn);

   CRect dRect;
   GetWindowRect(&dRect);
   int bottom_dlg_to_top_of_button = dRect.bottom - rBtn.top;

   CWnd* pProps = GetDlgItem(IDC_SECTION_PROPERTIES);
   CRect rProps;
   pProps->GetClientRect(&rProps);

   CRect cr;
   GetClientRect(&cr);
   CSize csize = cr.Size();
   csize.cy -= bottom_dlg_to_top_of_button;
   csize.cy -= rProps.Height();
   csize.cy -= 3*BORDER;
   csize.cx -= 2*BORDER;


   CComPtr<IRect2d> bbox;
   m_pShape->get_BoundingBox(&bbox);
   Float64 left,right,top,bottom;
   bbox->get_Left(&left);
   bbox->get_Right(&right);
   bbox->get_Top(&top);
   bbox->get_Bottom(&bottom);

   gpRect2d box(left,bottom,right,top);
   gpSize2d size = box.Size();
   gpPoint2d org = box.BottomCenter();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2 + BORDER,rProps.Height() + csize.cy + 2*BORDER);

   DrawShape(&dc,mapper);
   DrawStrands(&dc,mapper,m_IsEnd);

// strand mover - bounds drawn in read
#ifdef _DEBUG
   CBrush shape_brush(HS_FDIAGONAL, RGB(255,0,0));
   CBrush* pOldBrush = dc.SelectObject(&shape_brush);

   CollectionIndexType cnt = m_RegionShapes.size();
   for (CollectionIndexType ir=0; ir<cnt; ir++)
   {
      DrawShape(&dc,mapper,m_RegionShapes[ir]);
   }

   dc.SelectObject(pOldBrush);
#endif

}

void PrintNumber(CDC* pDC, grlibPointMapper& Mapper, const gpPoint2d& loc, StrandIndexType strandIdx)
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
      CollectionIndexType count;
      compshape->get_Count(&count);

      for ( CollectionIndexType idx = 0; idx < count; idx++ )
      {
         CComPtr<ICompositeShapeItem> item;
         compshape->get_Item(idx,&item);

         VARIANT_BOOL bVoid;
         item->get_Void(&bVoid);

         CComPtr<IShape> shape;
         item->get_Shape(&shape);


         if ( bVoid == VARIANT_TRUE )
            pDC->SelectObject(&void_brush);
         else
            pDC->SelectObject(&shape_brush);

         DrawShape(pDC,Mapper,shape);
      }
   }
   else
   {
      DrawShape(pDC,Mapper,m_pShape);
   }

   pDC->SelectObject(pOldBrush);
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
      Mapper.WPtoDP(points[i],&dx,&dy);
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

   StrandIndexType total_strand_cnt=0;
   StrandIndexType num_global = m_pGirderEntry->GetPermanentStrandGridSize();
   for (StrandIndexType idx=0; idx<num_global; idx++)
   {
      StrandIndexType strand_idx;
      GirderLibraryEntry::psStrandType strand_type;
      m_pGirderEntry->GetGridPositionFromPermStrandGrid(idx, &strand_type, &strand_idx);

      if (strand_type==GirderLibraryEntry::stStraight)
      {
         Float64 xStart,yStart,xEnd,yEnd;
         bool can_debond;
         m_pGirderEntry->GetStraightStrandCoordinates(strand_idx, &xStart, &yStart, &xEnd, &yEnd, &can_debond);

         pDC->SelectObject(can_debond ? &straight_db_strand_brush : &straight_strand_brush);
         pDC->SelectObject(can_debond ? &straight_db_strand_pen   : &straight_strand_pen);

         total_strand_cnt = DrawStrand(pDC, Mapper, xStart, yStart, total_strand_cnt);
      }
      else if (strand_type==GirderLibraryEntry::stHarped)
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

         total_strand_cnt = DrawStrand(pDC, Mapper, x, y, total_strand_cnt);
      }
      else
         ATLASSERT(0);
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

StrandIndexType CSectionViewDialog::DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index)
{
   CRect rect;
   Mapper.WPtoDP(x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
   Mapper.WPtoDP(x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
   Mapper.WPtoDP(x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
   Mapper.WPtoDP(x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

   pDC->Ellipse(&rect);

   index++;

   if ( m_DrawNumbers )
      PrintNumber(pDC, Mapper, gpPoint2d(x,y), index);

   if (0.0 < x)
   {
      Mapper.WPtoDP(-x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
      Mapper.WPtoDP(-x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
      Mapper.WPtoDP(-x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
      Mapper.WPtoDP(-x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

      pDC->Ellipse(&rect);

      index++;

      gpPoint2d np(-x,y);
      if ( m_DrawNumbers )
         PrintNumber(pDC, Mapper, np, index);
   }

   return index;
}

void CSectionViewDialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
   CRect rClient;
   GetClientRect(&rClient);

   CWnd* pBtn = GetDlgItem(IDC_SECTION_PROPERTIES);

   if (pBtn)
   {
      // section properties area
      CRect rProps;
      pBtn->GetWindowRect(&rProps);
      pBtn->MoveWindow(BORDER,BORDER,rClient.Width()-2*BORDER,rProps.Height());

	   // ok button
      pBtn = GetDlgItem(IDOK);

      CRect rBtn;
      pBtn->GetWindowRect(&rBtn);

      int top_close = rClient.Size().cy - rBtn.Size().cy - BORDER;

      pBtn->MoveWindow(rClient.Size().cx - rBtn.Size().cx - BORDER,
                       top_close,
                       rBtn.Size().cx, rBtn.Size().cy);

      // check box
      pBtn = GetDlgItem(IDC_SHOWS);
      pBtn->GetWindowRect(&rBtn);

      int chk_wid = rBtn.Size().cx;

      pBtn->MoveWindow(rClient.Size().cx - rBtn.Size().cx - BORDER,
                       top_close - rBtn.Size().cy - BORDER,
                       rBtn.Size().cx, rBtn.Size().cy);

	   // legend text
      pBtn = GetDlgItem(IDC_DB);
      pBtn->GetWindowRect(&rBtn);

      int leg_wid = (rClient.Size().cx - chk_wid);
      int wid = rBtn.Size().cx;
      int spacing = (leg_wid - 2*BORDER - wid)/3;

      int xloc = BORDER;
      int yloc = rClient.Size().cy - rBtn.Size().cy - BORDER;

      pBtn->MoveWindow(xloc, yloc,
                       rBtn.Size().cx, rBtn.Size().cy);

      pBtn = GetDlgItem(IDC_SS);
      pBtn->GetWindowRect(&rBtn);

      xloc += spacing;
      pBtn->MoveWindow(xloc, yloc,
                       rBtn.Size().cx, rBtn.Size().cy);

      pBtn = GetDlgItem(IDC_HS);
      pBtn->GetWindowRect(&rBtn);

      xloc += spacing;
      pBtn->MoveWindow(xloc, yloc,
                       rBtn.Size().cx, rBtn.Size().cy);

      pBtn = GetDlgItem(IDC_TS);
      pBtn->GetWindowRect(&rBtn);

      xloc += spacing;
      pBtn->MoveWindow(xloc, yloc,
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
   pBtn->SetCheck(TRUE);

   // label for harped or straight-web
   CString hlbl;
   hlbl.Format(_T("%s Strands"), LABEL_HARP_TYPE(m_pGirderEntry->IsForceHarpedStrandsStraight()));
   CWnd* pWnd = GetDlgItem(IDC_HS);
   pWnd->SetWindowTextW(hlbl);

   CStatic* pShapeProps = (CStatic*)GetDlgItem(IDC_SECTION_PROPERTIES);
   CString strProps;

   Float64 Area, Ix, Ytop, Ybot, Stop, Sbot;
   m_ShapeProps->get_Area(&Area);
   m_ShapeProps->get_Ixx(&Ix);
   m_ShapeProps->get_Ytop(&Ytop);
   m_ShapeProps->get_Ybottom(&Ybot);
   Stop = Ix/Ytop;
   Sbot = Ix/Ybot;

   Float64 Kt = Sbot/Area;
   Float64 Kb = Stop/Area;

   CString strAreaUnit, strIxUnit, strYUnit, strSUnit;
   Area = ::ConvertFromSysUnits(Area,pDisplayUnits->Area.UnitOfMeasure);
   strAreaUnit = pDisplayUnits->Area.UnitOfMeasure.UnitTag().c_str();

   Ix = ::ConvertFromSysUnits(Ix,pDisplayUnits->MomentOfInertia.UnitOfMeasure);
   strIxUnit = pDisplayUnits->MomentOfInertia.UnitOfMeasure.UnitTag().c_str();

   Ytop = ::ConvertFromSysUnits(Ytop,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Ybot = ::ConvertFromSysUnits(Ybot,pDisplayUnits->ComponentDim.UnitOfMeasure);
   strYUnit = pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str();

   Stop = ::ConvertFromSysUnits(Stop,pDisplayUnits->SectModulus.UnitOfMeasure);
   Sbot = ::ConvertFromSysUnits(Sbot,pDisplayUnits->SectModulus.UnitOfMeasure);
   strSUnit = pDisplayUnits->SectModulus.UnitOfMeasure.UnitTag().c_str();

   Kt = ::ConvertFromSysUnits(Kt,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Kb = ::ConvertFromSysUnits(Kb,pDisplayUnits->ComponentDim.UnitOfMeasure);
   strYUnit = pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str();

   strProps.Format(_T("Area = %.3f %s\t\t\tYt = %.3f %s\t\t\tYb = %.3f %s\nIx = %.1f %s\t\t\tSt = %.3f %s\t\t\tSb = %.3f %s\nH = %.3f %s\t\t\tKt = %.3f %s\t\t\tKb = %.3f %s"),Area,strAreaUnit,Ytop,strYUnit,Ybot,strYUnit,Ix,strIxUnit,Stop,strSUnit,Sbot,strSUnit,(Ytop+Ybot),strYUnit,Kt,strYUnit,Kb,strYUnit);
   pShapeProps->SetWindowText(strProps);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSectionViewDialog::OnClickNumbers()
{
   m_DrawNumbers = !m_DrawNumbers;
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