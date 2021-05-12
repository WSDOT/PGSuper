///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// DrawTendonsControl.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawDeckRegionControl.h"
#include "CastDeckDlg.h"
#include <IFace\Bridge.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDrawDeckRegionControl

IMPLEMENT_DYNAMIC(CDrawDeckRegionControl, CWnd)

CDrawDeckRegionControl::CDrawDeckRegionControl()
{
}

CDrawDeckRegionControl::~CDrawDeckRegionControl()
{
}


BEGIN_MESSAGE_MAP(CDrawDeckRegionControl, CWnd)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CDrawDeckRegionControl message handlers

void CDrawDeckRegionControl::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   // setup a clipping region so we don't draw outside of the control boundaries
   CRect rClient;
   GetClientRect(&rClient);
   CRgn rgn;
   rgn.CreateRectRgnIndirect(&rClient);
   dc.SelectClipRgn(&rgn);


   CCastDeckDlg* pParent = (CCastDeckDlg*)GetParent();
   CCastDeckActivity castDeckActivity = pParent->GetCastDeckActivity();
   IndexType nRegions = castDeckActivity.GetCastingRegionCount();

   const auto* pBridgeDesc = pParent->m_TimelineMgr.GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);

   CComPtr<IPoint2d> pntLeft, pntBridge, pntRight;
   CComPtr<IPoint2d> rotation_center, end_point;
   pBridge->GetPierPoints(0, pgsTypes::pcLocal, &pntLeft, &rotation_center, &pntBridge, &pntRight);
   pntLeft.Release();
   pntBridge.Release();
   pntRight.Release();
   pBridge->GetPierPoints(nPiers-1, pgsTypes::pcLocal, &pntLeft, &end_point, &pntBridge, &pntRight);

   Float64 x1, y1, x2, y2;
   rotation_center->Location(&x1,&y1);
   end_point->Location(&x2,&y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 angle = atan2(dy, dx);

   // Setup the coordinate mapping using the perimeter of the entire deck
   CComPtr<IShape> shape;
   CComPtr<ICompositeShape> compShape;
   compShape.CoCreateInstance(CLSID_CompositeShape);

   CCastingRegion::RegionType regionType;
   IndexType sequenceIdx;
   CComPtr<IPoint2dCollection> points;
   pBridge->GetDeckCastingRegionPerimeter(0, 4, pgsTypes::pcLocal, &regionType, &sequenceIdx, &castDeckActivity, &points);
   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);
   polyShape->AddPoints(points);
   polyShape->get_Shape(&shape);
   compShape->AddShape(shape, VARIANT_FALSE);
   shape.Release();
   polyShape.Release();
   points.Release();
   pBridge->GetDeckCastingRegionPerimeter(nRegions - 1, 4, pgsTypes::pcLocal, &regionType, &sequenceIdx, &castDeckActivity, &points);
   polyShape.CoCreateInstance(CLSID_PolyShape);
   polyShape->AddPoints(points);
   polyShape->get_Shape(&shape);
   compShape->AddShape(shape, VARIANT_FALSE);

   CComQIPtr<IXYPosition> position(compShape);
   position->RotateEx(rotation_center, -angle);

   shape.Release();
   compShape->get_Shape(&shape);

   CComPtr<IRect2d> rect;
   shape->get_BoundingBox(&rect);

   Float64 left, top, bottom, right;
   rect->get_Left(&left);
   rect->get_Top(&top);
   rect->get_Bottom(&bottom);
   rect->get_Right(&right);
   gpRect2d box(left, bottom, right, top);
   gpSize2d size = box.Size();
   gpPoint2d org = box.Center();

   rClient.DeflateRect(1, 1, 1, 1);
   CSize sClient = rClient.Size();

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(sClient.cx, sClient.cy);
   mapper.SetDeviceOrg(sClient.cx / 2, sClient.cy / 2);

   CPen pen(PS_SOLID, 1, DECK_BORDER_COLOR);
   CBrush pm_brush(DECK_FILL_POS_MOMENT_REGION_COLOR);
   CBrush nm_brush(DECK_FILL_NEG_MOMENT_REGION_COLOR);
   CFont* pFont = GetParent()->GetFont();

   dc.SetBkMode(TRANSPARENT);
   dc.SetTextAlign(TA_BOTTOM | TA_CENTER);

   CPen* pOldPen = dc.SelectObject(&pen);
   CBrush* pOldBrush = dc.SelectObject(&pm_brush);
   CFont* pOldFont = dc.SelectObject(pFont);

   for (IndexType regionIdx = 0; regionIdx < nRegions; regionIdx++)
   {
      CCastingRegion::RegionType regionType;
      IndexType sequenceIdx;
      CComPtr<IPoint2dCollection> regionPoints;
      pBridge->GetDeckCastingRegionPerimeter(regionIdx, 10, pgsTypes::pcLocal, &regionType, &sequenceIdx, &castDeckActivity, &regionPoints);

      dc.SelectObject(regionType == CCastingRegion::Span ? &pm_brush : &nm_brush);

      polyShape->Clear();
      polyShape->AddPoints(regionPoints);
      CComQIPtr<IXYPosition> position(polyShape);
      position->RotateEx(rotation_center, -angle);

      CComQIPtr<IShape> shape(polyShape);
      Draw(&dc, mapper, shape, TRUE);

      // need to label sequence number
      CComPtr<IShapeProperties> shapeProps;
      shape->get_ShapeProperties(&shapeProps);
      CComPtr<IPoint2d> pntCG;
      shapeProps->get_Centroid(&pntCG);
      LONG dx, dy;
      mapper.WPtoDP(pntCG, &dx, &dy);
      CString strType(regionType == CCastingRegion::Pier ? _T("-M") : _T("+M"));
      CString strSequence;
      if (regionIdx == 0)
      {
         strSequence.Format(_T("Region:   %d\nSequence: %d\n%s"), LABEL_INDEX(regionIdx), LABEL_INDEX(sequenceIdx),strType);
      }
      else
      {
         strSequence.Format(_T("%d\n%d\n%s"), LABEL_INDEX(regionIdx), LABEL_INDEX(sequenceIdx), strType);
      }
      MultiLineTextOut(&dc, dx, dy, strSequence);
   }

   // Clean up
   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
   dc.SelectObject(pOldFont);
}

void CDrawDeckRegionControl::Draw(CDC* pDC,grlibPointMapper& mapper,IShape* pShape,BOOL bPolygon)
{
   CComPtr<IPoint2dCollection> polyPoints;
   pShape->get_PolyPoints(&polyPoints);

   CollectionIndexType nPoints;
   polyPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   polyPoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      LONG dx,dy;
      mapper.WPtoDP(points[i],&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   if (bPolygon)
   {
      pDC->Polygon(dev_points, (int)nPoints);
   }
   else
   {
      pDC->Polyline(dev_points, (int)nPoints);
   }

   delete[] points;
   delete[] dev_points;
}


BOOL CDrawDeckRegionControl::OnEraseBkgnd(CDC* pDC)
{
   // TODO: Add your message handler code here and/or call default
   CRect rClient;
   GetClientRect(&rClient);

   auto* bkBrush = CBrush::FromHandle(::GetSysColorBrush(COLOR_3DFACE));
   auto* pOldBrush = pDC->SelectObject(bkBrush);
   auto* pOldPen = pDC->SelectStockObject(NULL_PEN);
   pDC->Rectangle(&rClient);
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);
   return CWnd::OnEraseBkgnd(pDC);
}
