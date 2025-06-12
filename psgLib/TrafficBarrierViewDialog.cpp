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


// TrafficBarrierViewDialog.cpp : implementation file
//

#include "stdafx.h"
#include <PsgLib\PsgLib.h>
#include "TrafficBarrierViewDialog.h"
#include <Graphing/PointMapper.h>

#include <GeomModel/Shape.h>
#include <GeomModel/Circle.h>
#include <GeomModel/Polygon.h>

#include "PGSuperColors.h"

#include <WBFLGeometry.h>

#include <WBFLGenericBridge.h>



#define BORDER 7

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierViewDialog dialog


CTrafficBarrierViewDialog::CTrafficBarrierViewDialog(IShape* pShape, CWnd* pParent)
	: CDialog(CTrafficBarrierViewDialog::IDD, pParent)
{
   m_pShape = pShape;

	//{{AFX_DATA_INIT(CTrafficBarrierViewDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTrafficBarrierViewDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrafficBarrierViewDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrafficBarrierViewDialog, CDialog)
	//{{AFX_MSG_MAP(CTrafficBarrierViewDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierViewDialog message handlers

void CTrafficBarrierViewDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CDialog::OnPaint() for painting messages
   CWnd* pBtn = GetDlgItem(IDOK);
   CRect rBtn;
   pBtn->GetWindowRect(&rBtn);

   CRect dRect;
   GetWindowRect(&dRect);
   int bottom_dlg_to_top_of_button = dRect.bottom - rBtn.top;

   CRect cr;
   GetClientRect(&cr);
   CSize csize = cr.Size();
   csize.cy -= bottom_dlg_to_top_of_button;
   csize.cy -= 2*BORDER;
   csize.cx -= 2*BORDER;

   CComPtr<IRect2d> bbox;
   m_pShape->get_BoundingBox(&bbox);
   Float64 left,right,top,bottom;
   bbox->get_Left(&left);
   bbox->get_Right(&right);
   bbox->get_Top(&top);
   bbox->get_Bottom(&bottom);

   bbox->BoundPoint(0.0, 0.0); // make sure origin is inside

   WBFL::Graphing::Rect box(left,bottom,right,top);
   WBFL::Graphing::Size size = box.Size();
   WBFL::Graphing::Point org = box.BottomCenter();

   WBFL::Graphing::PointMapper mapper;
   mapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx,csize.cy);
   mapper.SetDeviceOrg(csize.cx/2 + BORDER,csize.cy + BORDER);

   DrawShape(&dc,mapper);

   // Draw origin point
   CBrush shape_brush(RED);
   CBrush* pOldBrush = dc.SelectObject(&shape_brush);

   LONG zx, zy;
   mapper.WPtoDP(0.0, 0.0, &zx, &zy);
   const LONG pts=5;
   CRect rct(zx-pts, zy-pts, zx+pts, zy+pts);
   dc.Ellipse(rct);

   dc.SelectObject(pOldBrush);
}

void CTrafficBarrierViewDialog::DrawShape(CDC* pDC, WBFL::Graphing::PointMapper& Mapper)
{
   CComQIPtr<ICompositeShape> compshape(m_pShape);

   CBrush shape_brush(SHAPE_COLOR);
   CBrush void_brush(VOID_COLOR);

   CBrush* pOldBrush = pDC->SelectObject(&shape_brush);

   if ( compshape )
   {
      IndexType count;
      compshape->get_Count(&count);

      for ( IndexType idx = 0; idx < count; idx++ )
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

void CTrafficBarrierViewDialog::DrawShape(CDC* pDC, WBFL::Graphing::PointMapper& Mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   IndexType nPoints;
   polypoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   polypoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( IndexType i = 0; i < nPoints; i++ )
   {
      long dx,dy;
      WBFL::Graphing::Point point;
      points[i]->Location(&point.X(), &point.Y());
      Mapper.WPtoDP(point,&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   pDC->Polygon(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}


void CTrafficBarrierViewDialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
   CRect rClient;
   GetClientRect(&rClient);

	// ok button
   CWnd* pBtn = GetDlgItem(IDOK);
   if (pBtn)
   {
      CRect rBtn;
      pBtn->GetWindowRect(&rBtn);

      pBtn->MoveWindow(rClient.Size().cx - rBtn.Size().cx - BORDER,
                       rClient.Size().cy - rBtn.Size().cy - BORDER,
                       rBtn.Size().cx, rBtn.Size().cy);


      Invalidate();
   }
}

HBRUSH CTrafficBarrierViewDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
    
     if (pWnd->GetDlgCtrlID() == IDC_SS)
     {
          pDC->SetTextColor(STRAIGHT_COLOR);
     }
     else if (pWnd->GetDlgCtrlID() == IDC_HS)
     {
          pDC->SetTextColor(HARPED_COLOR);
     }
     else if (pWnd->GetDlgCtrlID() == IDC_TS)
     {
          pDC->SetTextColor(TEMP_COLOR);
     }
	
	return hbr;
}

BOOL CTrafficBarrierViewDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
