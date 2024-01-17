///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// DebondDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "DebondDlg.h"
#include "GirderDescDlg.h"
#include "PGSuperColors.h"
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\Helpers.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>

#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage dialog


CGirderDescDebondPage::CGirderDescDebondPage()
	: CPropertyPage(CGirderDescDebondPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescDebondPage)
	//}}AFX_DATA_INIT

   m_Radius = WBFL::Units::ConvertToSysUnits(0.3,WBFL::Units::Measure::Inch) * 1.5;

}

void CGirderDescDebondPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   //{{AFX_DATA_MAP(CGirderDescDebondPage)
	//}}AFX_DATA_MAP

   bool bSymmetricDebond = pParent->m_pSegment->Strands.IsSymmetricDebond();
	DDX_Check_Bool(pDX, IDC_SYMMETRIC_DEBOND, bSymmetricDebond );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_pSegment->Strands.IsSymmetricDebond(bSymmetricDebond);

      GET_IFACE2(pBroker, IBridge,pBridge);
      Float64 gdr_length2 = pBridge->GetSegmentLength(pParent->m_SegmentKey)/2.0;

      m_Grid.GetData(*(pParent->m_pSegment));

      std::vector<CDebondData>& vDebond = pParent->m_pSegment->Strands.GetDebonding(pgsTypes::Straight);
      std::vector<CDebondData>::iterator iter(vDebond.begin());
      std::vector<CDebondData>::iterator end(vDebond.end());
      for ( ; iter != end; iter++ )
      {
         CDebondData& debond_info = *iter;
         if (gdr_length2 <= debond_info.Length[pgsTypes::metStart] || gdr_length2 <= debond_info.Length[pgsTypes::metEnd])
         {
            HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	         AfxMessageBox( _T("Debond length cannot exceed one half of girder length."), MB_ICONEXCLAMATION);
	         pDX->Fail();
         }

         if( pParent->m_pSegment->Strands.IsSymmetricDebond() )
         {
            if (debond_info.Length[pgsTypes::metStart] <= 0.0 )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	            AfxMessageBox( _T("Debond length must be greater than zero"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }
         }
         else
         {
            if ( debond_info.Length[pgsTypes::metStart] < 0.0 || debond_info.Length[pgsTypes::metEnd] < 0.0 )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	            AfxMessageBox( _T("Debond length may not be less than zero"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }

            if ( debond_info.Length[pgsTypes::metStart] <= 0.0 && debond_info.Length[pgsTypes::metEnd] <= 0.0 )
            {
               HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	            AfxMessageBox( _T("Debond length at one end must be greater than zero"), MB_ICONEXCLAMATION);
	            pDX->Fail();
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CGirderDescDebondPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescDebondPage)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_SYMMETRIC_DEBOND, OnSymmetricDebond)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage message handlers
BOOL CGirderDescDebondPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();

	m_Grid.SubclassDlgItem(IDC_DEBOND_GRID, this);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   m_Grid.CustomInit(pParent->m_pSegment->Strands.IsSymmetricDebond() ? TRUE : FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CGirderDescDebondPage::OnSetActive() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   bool bSymmetricDebond = pParent->m_pSegment->Strands.IsSymmetricDebond();
   bool bCanDebond = pStrandGeometry->CanDebondStrands(pParent->m_strGirderName.c_str(),pgsTypes::Straight);
   m_Grid.CanDebond(bCanDebond,bSymmetricDebond);
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_NUM_DEBONDED)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);

   CString note(_T("- Only filled straight strands are shown in this view."));

   if (pParent->m_pSegment->Strands.GetAdjustableStrandType() != pgsTypes::asStraight)
   {
      note += _T("\n  Harped strands are not shown.");
   }

   if (bCanDebond)
   {
      CString n;
      n.Format(_T("\n- Strands shown in %s or %s cannot be debonded"),NOT_DEBONDABLE_FILL_COLOR_NAME, HARPED_FILL_COLOR_NAME);
      note += n;
   }
   GetDlgItem(IDC_NOTE2)->SetWindowText(note);
   GetDlgItem(IDC_NOTE)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);

   m_nHelpID = bCanDebond ? IDH_GIRDERDETAILS_DEBOND : IDH_GIRDERDETAILS_STRAND_EXTENSIONS;

   StrandIndexType nStrands = pParent->GetStraightStrandCount();
   ConfigStrandFillVector strtvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   ReconcileDebonding(strtvec, pParent->m_pSegment->Strands.GetDebonding(pgsTypes::Straight)); 

   for ( int i = 0; i < 2; i++ )
   {
      std::vector<GridIndexType> extStrands = pParent->m_pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i);
      bool bChanged = ReconcileExtendedStrands(strtvec, extStrands);

      if ( bChanged )
      {
         pParent->m_pSegment->Strands.SetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i,extStrands);
      }
   }

   // fill up the grid
   m_Grid.FillGrid(*(pParent->m_pSegment));

   BOOL enab = nStrands>0 ? TRUE:FALSE;
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->EnableWindow(enab);

   m_Grid.SelectRange(CGXRange().SetTable(), FALSE);

   OnChange();

   return CPropertyPage::OnSetActive();
}

BOOL CGirderDescDebondPage::OnKillActive()
{
   this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

const std::vector<CDebondData>& CGirderDescDebondPage::GetDebondInfo() const
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   return pParent->m_pSegment->Strands.GetDebonding(pgsTypes::Straight);
}


void CGirderDescDebondPage::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CProperyPage::OnPaint() for painting messages

   // Draw the girder cross section and label the strand locations
   // The basic logic from this code is take from
   // Programming Microsoft Visual C++, Fifth Edition
   // Kruglinski, Shepherd, and Wingo
   // Page 129
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect redit;
   pWnd->GetClientRect(&redit);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(redit.left,redit.top,redit.right,redit.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IShapes, pShapes);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   pgsPointOfInterest poi = pPoi->GetPointOfInterest(pParent->m_SegmentKey, 0.0);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(pParent->m_SegmentKey);

   CComPtr<IShape> shape;
   IndexType gdrShapeIdx;
   pShapes->GetSegmentShape(intervalIdx, poi, false, pgsTypes::scGirder, &shape, &gdrShapeIdx);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);


   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IGirder, pGirder);
   Float64 top_width;
   if (IsTopWidthSpacing(pIBridgeDesc->GetGirderSpacingType()) )
   {
      Float64 wLeft, wRight;
      top_width = Max(pParent->m_Girder.GetTopWidth(pgsTypes::metStart, &wLeft, &wRight), pParent->m_Girder.GetTopWidth(pgsTypes::metEnd, &wLeft, &wRight));
   }
   else
   {
      top_width = pGirder->GetTopWidth(poi);
   }
   Float64 bottom_width = pGirder->GetBottomWidth(poi);

   WBFL::Graphing::Size size;
   size.Dx() = Max(top_width,bottom_width);

   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);

   Float64 height;
   box->get_Height(&height);

   size.Dy() = height;
   if ( IsZero(size.Dy()) )
   {
      size.Dy() = size.Dx()/2;
   }

   CSize csize = redit.Size();

   CComPtr<IPoint2d> objOrg;
   box->get_BottomCenter(&objOrg);

   WBFL::Graphing::Point org;
   objOrg->Location(&org.X(), &org.Y());

   WBFL::Graphing::PointMapper mapper;
   mapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2+5,csize.cy+5);

   // find the world coordinates of the screen center and move to center the shape to it
   Float64 cwx, cwy;
   csize = redit.Size();
   mapper.DPtoWP(csize.cx/2,csize.cy/2, &cwx, &cwy);
   Float64 dist = cwy - size.Dy()/2.0;

   mapper.SetWorldOrg(org.X(), org.Y()-dist);


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

   // strand points are defined in the girder library relative to a centerline. 
   // this centerline can be shifted horizontally for asymmetric girders
   // this adjustment compensates (to see the effect, make Xadjustment equal to zero for a deck bulb tee with unequal overhangs)
   CComQIPtr<IAsymmetricSection> asymmetric(shape);
   if (!asymmetric && compshape)
   {
      CComPtr<ICompositeShapeItem> shapeItem;
      compshape->get_Item(gdrShapeIdx, &shapeItem);
      CComPtr<IShape> gdrShape;
      shapeItem->get_Shape(&gdrShape);
      gdrShape.QueryInterface(&asymmetric);
   }

   Float64 Xadjustment = 0;
   if (asymmetric)
   {
      Float64 wLeft, wRight;
      asymmetric->GetTopWidth(&wLeft, &wRight);
      Xadjustment = -0.5*(wRight - wLeft);
   }

   DrawStrands(pDC,mapper, Xadjustment);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CGirderDescDebondPage::DrawShape(CDC* pDC,IShape* shape, WBFL::Graphing::PointMapper& mapper)
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

void CGirderDescDebondPage::DrawStrands(CDC* pDC, WBFL::Graphing::PointMapper& mapper,Float64 Xadjustment)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CPen strand_pen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CPen no_debond_pen(PS_SOLID,1,NOT_DEBONDABLE_FILL_COLOR);
   CPen debond_pen(PS_SOLID,1,DEBOND_FILL_COLOR);
   CPen extended_pen(PS_SOLID,1,EXTENDED_FILL_COLOR);
   CPen harped_pen(PS_SOLID,1,HARPED_FILL_COLOR);
   CPen* old_pen = (CPen*)pDC->SelectObject(&strand_pen);

   CBrush strand_brush(STRAND_FILL_COLOR);
   CBrush no_debond_brush(NOT_DEBONDABLE_FILL_COLOR);
   CBrush debond_brush(DEBOND_FILL_COLOR);
   CBrush extended_brush(EXTENDED_FILL_COLOR);
   CBrush harped_brush(HARPED_FILL_COLOR);
   CBrush* old_brush = (CBrush*)pDC->SelectObject(&strand_brush);

   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* old_font = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   // Draw all the strands bonded
   StrandIndexType nStrands = GetStrandCount();

   ConfigStrandFillVector  straightStrandFill = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   ConfigStrandFillVector  harpedStrandFill = pParent->ComputeStrandFillVector(pgsTypes::Harped);

   // Want unadjusted strand locations
   PRESTRESSCONFIG config;
   config.SetStrandFill(pgsTypes::Straight, straightStrandFill);
   config.SetStrandFill(pgsTypes::Harped,   harpedStrandFill);

   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(pParent->m_strGirderName.c_str(),0,0,0,0,config,pgsTypes::Straight,pgsTypes::metStart,&points);

   CComPtr<IIndexArray> strandids;
   pStrandGeometry->ComputePermanentStrandIndices(pParent->m_strGirderName.c_str(),config,pgsTypes::Straight,&strandids);

   CComPtr<IIndexArray> debondables;
   pStrandGeometry->ListDebondableStrands(pParent->m_strGirderName.c_str(), straightStrandFill,pgsTypes::Straight, &debondables); 

   const int strand_size = 2;
   for ( StrandIndexType strIdx = 0; strIdx <nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);

      StrandIndexType is_debondable = 0;
      debondables->get_Item(strIdx, &is_debondable);

      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      x += Xadjustment;

      CRect rect;
      mapper.WPtoDP(x-m_Radius,y-m_Radius,&rect.left,&rect.top); 
      mapper.WPtoDP(x+m_Radius,y-m_Radius,&rect.right,&rect.top); 
      mapper.WPtoDP(x-m_Radius,y+m_Radius,&rect.left,&rect.bottom); 
      mapper.WPtoDP(x+m_Radius,y+m_Radius,&rect.right,&rect.bottom); 

      if (is_debondable)
      {
         pDC->SelectObject(&strand_pen);
         pDC->SelectObject(&strand_brush);
      }
      else
      {
         pDC->SelectObject(&no_debond_pen);
         pDC->SelectObject(&no_debond_brush);
      }

      pDC->Ellipse(&rect);

      StrandIndexType permIdx;
      strandids->get_Item(strIdx, &permIdx);

      long lx, ly;
      mapper.WPtoDP(x, y, &lx, &ly);

      // move down slightly
      ly += 4;

      CString strLabel;
      strLabel.Format(_T("%d"),LABEL_INDEX(permIdx));
      pDC->TextOut(lx,ly,strLabel);
   }

   // Redraw the debonded strands
   pDC->SelectObject(&debond_pen);
   pDC->SelectObject(&debond_brush);

   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   CPrecastSegmentData segment = *(pParent->m_pSegment);
   m_Grid.GetData(segment);

   std::vector<CDebondData>& vDebond(segment.Strands.GetDebonding(pgsTypes::Straight));
   std::vector<CDebondData>::iterator debond_iter(vDebond.begin());
   std::vector<CDebondData>::iterator debond_iter_end(vDebond.end());
   for ( ; debond_iter != debond_iter_end; debond_iter++ )
   {
      CDebondData& debond_info = *debond_iter;

      if ( debond_info.strandTypeGridIdx == INVALID_INDEX )
      {
         ATLASSERT(false); // we should be protecting against this
         continue;
      }

      // Library entry uses grid indexing (same as debonding)
      Float64 xs, xe, ys, ye;
      bool candb;
      pGdrEntry->GetStraightStrandCoordinates( debond_info.strandTypeGridIdx, &xs, &ys, &xe, &ye, &candb);

      xs += Xadjustment;
      xe += Xadjustment;

      CRect rect;
      mapper.WPtoDP(xs-m_Radius,ys-m_Radius,&rect.left,&rect.top); 
      mapper.WPtoDP(xs+m_Radius,ys-m_Radius,&rect.right,&rect.top); 
      mapper.WPtoDP(xs-m_Radius,ys+m_Radius,&rect.left,&rect.bottom); 
      mapper.WPtoDP(xs+m_Radius,ys+m_Radius,&rect.right,&rect.bottom); 

      pDC->Ellipse(&rect);

      if ( 0.0 < xs )
      {
         mapper.WPtoDP(-xs-m_Radius,ys-m_Radius,&rect.left,&rect.top); 
         mapper.WPtoDP(-xs+m_Radius,ys-m_Radius,&rect.right,&rect.top); 
         mapper.WPtoDP(-xs-m_Radius,ys+m_Radius,&rect.left,&rect.bottom); 
         mapper.WPtoDP(-xs+m_Radius,ys+m_Radius,&rect.right,&rect.bottom);

         pDC->Ellipse(&rect);
      }
   }


   // Redraw the extended strands
   pDC->SelectObject(&extended_pen);
   pDC->SelectObject(&extended_brush);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      const std::vector<GridIndexType>& extStrandsStart(segment.Strands.GetExtendedStrands(pgsTypes::Straight,endType));
      std::vector<GridIndexType>::const_iterator ext_iter(extStrandsStart.begin());
      std::vector<GridIndexType>::const_iterator ext_iter_end(extStrandsStart.end());
      for ( ; ext_iter != ext_iter_end; ext_iter++ )
      {
         GridIndexType gridIdx = *ext_iter;

         // Library entry uses grid indexing (same as debonding)
         Float64 xs, xe, ys, ye;
         bool candb;
         pGdrEntry->GetStraightStrandCoordinates( gridIdx, &xs, &ys, &xe, &ye, &candb);

         xs += Xadjustment;
         xe += Xadjustment;

         CRect rect;
         mapper.WPtoDP(xs-m_Radius,ys-m_Radius,&rect.left,&rect.top); 
         mapper.WPtoDP(xs+m_Radius,ys-m_Radius,&rect.right,&rect.top); 
         mapper.WPtoDP(xs-m_Radius,ys+m_Radius,&rect.left,&rect.bottom); 
         mapper.WPtoDP(xs+m_Radius,ys+m_Radius,&rect.right,&rect.bottom); 

         pDC->Ellipse(&rect);

         if ( 0.0 < xs )
         {
            mapper.WPtoDP(-xs-m_Radius,ys-m_Radius,&rect.left,&rect.top); 
            mapper.WPtoDP(-xs+m_Radius,ys-m_Radius,&rect.right,&rect.top); 
            mapper.WPtoDP(-xs-m_Radius,ys+m_Radius,&rect.left,&rect.bottom); 
            mapper.WPtoDP(-xs+m_Radius,ys+m_Radius,&rect.right,&rect.bottom);

            pDC->Ellipse(&rect);
         }
      }
   }

   // Draw adjustable strands only if they are straight
   if (pParent->m_pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight)
   {
      StrandIndexType nhStrands = pParent->GetHarpedStrandCount();

      CComPtr<IPoint2dCollection> harped_points;
      pStrandGeometry->GetStrandPositionsEx(pParent->m_strGirderName.c_str(), 0, 0, 0, 0, config, pgsTypes::Harped, pgsTypes::metStart, &harped_points);

      CComPtr<IIndexArray> harped_strandids;
      pStrandGeometry->ComputePermanentStrandIndices(pParent->m_strGirderName.c_str(), config, pgsTypes::Harped, &harped_strandids);

      for (StrandIndexType strIdx = 0; strIdx < nhStrands; strIdx++)
      {
         CComPtr<IPoint2d> point;
         harped_points->get_Item(strIdx, &point);

         Float64 x, y;
         point->get_X(&x);
         point->get_Y(&y);

         x += Xadjustment;

         CRect rect;
         mapper.WPtoDP(x - m_Radius, y - m_Radius, &rect.left, &rect.top);
         mapper.WPtoDP(x + m_Radius, y - m_Radius, &rect.right, &rect.top);
         mapper.WPtoDP(x - m_Radius, y + m_Radius, &rect.left, &rect.bottom);
         mapper.WPtoDP(x + m_Radius, y + m_Radius, &rect.right, &rect.bottom);

         pDC->SelectObject(&harped_pen);
         pDC->SelectObject(&harped_brush);

         pDC->Ellipse(&rect);

         StrandIndexType permIdx;
         harped_strandids->get_Item(strIdx, &permIdx);

         long lx, ly;
         mapper.WPtoDP(x, y, &lx, &ly);

         // move down slightly
         ly += 4;

         CString strLabel;
         strLabel.Format(_T("%d"), LABEL_INDEX(permIdx));
         pDC->TextOut(lx, ly, strLabel);
      }
   }

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
   pDC->SelectObject(old_font);

}

StrandIndexType CGirderDescDebondPage::GetStrandCount()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands =  pParent->GetStraightStrandCount();

   return nStrands;
}

StrandIndexType CGirderDescDebondPage::GetNumPermanentStrands()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands =  pParent->GetStraightStrandCount();
   nStrands += pParent->GetHarpedStrandCount();

   return nStrands;
}

void CGirderDescDebondPage::OnSymmetricDebond() 
{
   UINT checked = IsDlgButtonChecked(IDC_SYMMETRIC_DEBOND);
   m_Grid.CanDebond( true, checked != 0 );
}

void CGirderDescDebondPage::OnHelp() 
{
	EAFHelp( EAFGetDocument()->GetDocumentationSetName(), m_nHelpID );
}

LPCTSTR CGirderDescDebondPage::GetGirderName()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   return pParent->m_strGirderName.c_str();
}
   
void CGirderDescDebondPage::OnChange() 
{
   StrandIndexType ns = GetNumPermanentStrands(); 
   StrandIndexType ndbs =  m_Grid.GetNumDebondedStrands();
   Float64 percent = 0.0;
   if (0 < ns && ns != INVALID_INDEX)
   {
      percent = 100.0 * (Float64)ndbs/(Float64)ns;
   }

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CString str;
   if (pParent->m_pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight)
   {
      str.Format(_T("Straight=%d,  Adj. Straight=%d"), GetStrandCount(),ns-GetStrandCount());
   }
   else
   {
      str.Format(_T("Straight=%d,  Harped=%d"), GetStrandCount(),ns-GetStrandCount());
   }

   CWnd* pNs = GetDlgItem(IDC_NUMSTRAIGHT);
   pNs->SetWindowText(str);

   str.Format(_T("Debonded=%d (%.1f%%)"), ndbs, percent);
   CWnd* pNdb = GetDlgItem(IDC_NUM_DEBONDED);
   pNdb->SetWindowText(str);

   StrandIndexType nExtStrands = m_Grid.GetNumExtendedStrands(pgsTypes::metStart);
   str.Format(_T("Extended Left=%d"),nExtStrands);
   CWnd* pNExt = GetDlgItem(IDC_NUM_EXTENDED_LEFT);
   pNExt->SetWindowText(str);

   nExtStrands = m_Grid.GetNumExtendedStrands(pgsTypes::metEnd);
   str.Format(_T("Extended Right=%d"),nExtStrands);
   pNExt = GetDlgItem(IDC_NUM_EXTENDED_RIGHT);
   pNExt->SetWindowText(str);

   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
}

const CSegmentKey& CGirderDescDebondPage::GetSegmentKey()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   return pParent->m_SegmentKey;
}

ConfigStrandFillVector CGirderDescDebondPage::ComputeStrandFillVector(pgsTypes::StrandType type)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   return pParent->ComputeStrandFillVector(type);
}

HBRUSH CGirderDescDebondPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
   int ID = pWnd->GetDlgCtrlID();
   switch( ID )
   {
   case IDC_NUMSTRAIGHT:
      pDC->SetTextColor(STRAIGHT_FILL_COLOR);
      break;

   case IDC_NUM_DEBONDED:
      pDC->SetTextColor(DEBOND_FILL_COLOR);
      break;

   case IDC_NUM_EXTENDED_LEFT:
   case IDC_NUM_EXTENDED_RIGHT:
      pDC->SetTextColor(EXTENDED_FILL_COLOR);
      break;
   }

   return hbr;
}
