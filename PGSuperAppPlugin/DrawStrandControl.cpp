///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// DrawStrandControl.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawStrandControl.h"

#include <PgsExt\SplicedGirderData.h>
#include <IFace\Bridge.h>

#include <IFace\BeamFactory.h>
#include <PsgLib\GirderLibraryEntry.h>


// CDrawStrandControl

IMPLEMENT_DYNAMIC(CDrawStrandControl, CWnd)

CDrawStrandControl::CDrawStrandControl()
{
   m_pSegment = NULL;
}

CDrawStrandControl::~CDrawStrandControl()
{
}


BEGIN_MESSAGE_MAP(CDrawStrandControl, CWnd)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CDrawStrandControl message handlers
void CDrawStrandControl::CustomInit(const CPrecastSegmentData* pSegment)
{
   m_Shape[pgsTypes::metStart].Release();
   m_Shape[pgsTypes::metEnd].Release();
   m_Profile.Release();
   m_BottomFlange.Release();

   m_pSegment = pSegment;

   m_Radius = ::ConvertToSysUnits(0.3,unitMeasure::Inch) * 1.5;

   // Gets the segment height based on the data in the girder library entry
   const GirderLibraryEntry* pGdrEntry = m_pSegment->GetGirder()->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   Float64 HbfStart(-1), HbfEnd(-1);
   if ( m_pSegment->IsVariableBottomFlangeDepthEnabled() )
   {
      HbfStart = m_pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic);
      HbfEnd   = m_pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic);
   }

   switch (m_pSegment->GetVariationType() )
   {
   case pgsTypes::svtNone:
      m_Hg = m_pSegment->GetBasicSegmentHeight();
      m_HgStart = m_Hg;
      m_HgEnd = m_Hg;
      break;

   case pgsTypes::svtLinear:
   case pgsTypes::svtParabolic:
      m_HgStart = m_pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic);
      m_HgEnd   = m_pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic);
      m_Hg = Max(m_HgStart,m_HgEnd);
      break;

   case pgsTypes::svtDoubleLinear:
   case pgsTypes::svtDoubleParabolic:
      m_HgStart = m_pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic);
      m_HgEnd   = m_pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic);
      m_Hg = Max(m_HgStart,
         m_pSegment->GetVariationHeight(pgsTypes::sztLeftTapered),
         m_pSegment->GetVariationHeight(pgsTypes::sztRightTapered),
         m_HgEnd);
      break;

   default:
      ATLASSERT(false); // is there a new variation type?
   }


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   CComPtr<IGirderSection> startGdrSection;
   factory->CreateGirderSection(pBroker,INVALID_ID,pGdrEntry->GetDimensions(),m_HgStart,HbfStart,&startGdrSection);

   CComPtr<IGirderSection> endGdrSection;
   factory->CreateGirderSection(pBroker,INVALID_ID,pGdrEntry->GetDimensions(),m_HgEnd,HbfEnd,&endGdrSection);

   startGdrSection->QueryInterface(&m_Shape[pgsTypes::metStart]);
   endGdrSection->QueryInterface(&m_Shape[pgsTypes::metEnd]);

   // Put the shapes in Girder Section Coordinates
   CComQIPtr<IXYPosition> position(m_Shape[pgsTypes::metStart]);
   CComPtr<IPoint2d> pntTC;
   position->get_LocatorPoint(lpTopCenter,&pntTC);
   pntTC->Move(0,0);
   position->put_LocatorPoint(lpTopCenter,pntTC);

   position.Release();
   pntTC.Release();

   m_Shape[pgsTypes::metEnd].QueryInterface(&position);
   position->get_LocatorPoint(lpTopCenter,&pntTC);
   pntTC->Move(0,0);
   position->put_LocatorPoint(lpTopCenter,pntTC);

   CreateSegmentShape(&m_Profile,&m_BottomFlange);
}

BOOL CDrawStrandControl::OnEraseBkgnd(CDC* pDC)
{
   CBrush brush(::GetSysColor(COLOR_WINDOW));
   brush.UnrealizeObject();

   CPen pen(PS_SOLID,1,::GetSysColor(COLOR_WINDOW));
   pen.UnrealizeObject();

   CBrush* pOldBrush = pDC->SelectObject(&brush);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CRect rect;
   GetClientRect(&rect);
   pDC->Rectangle(rect);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   return TRUE;

   // default isn't working so we have to do our own erasing
   //return CWnd::OnEraseBkgnd(pDC);
}

void CDrawStrandControl::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages


   //
   // Set up coordinate mapping
   //
   gpRect2d box[2];
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType end = pgsTypes::MemberEndType(i);
      CComPtr<IRect2d> bounding_box;
      m_Shape[end]->get_BoundingBox(&bounding_box);
      Float64 left,right,top,bottom;
      bounding_box->get_Left(&left);
      bounding_box->get_Right(&right);
      bounding_box->get_Top(&top);
      bounding_box->get_Bottom(&bottom);

      box[end].Set(left,bottom,right,top);
      box[end].Bottom() = box[end].Top() - m_Hg;
   }

   CRect rClient;
   GetClientRect(&rClient);

   Float64 aspect_ratio = box[pgsTypes::metStart].Width()/m_Hg;

   CRect rLeft(rClient.left,rClient.top,rClient.left + (LONG)((Float64)(rClient.Height())*aspect_ratio),rClient.bottom);
   CRect rRight(rClient.right - (LONG)((Float64)(rClient.Height())*aspect_ratio), rClient.top, rClient.right, rClient.bottom);
   CRect rCenter(rLeft.right,rClient.top,rRight.left,rClient.bottom);

   grlibPointMapper leftMapper;
   leftMapper.SetMappingMode(grlibPointMapper::Isotropic);
   leftMapper.SetWorldExt(box[pgsTypes::metStart].Size());
   leftMapper.SetWorldOrg(box[pgsTypes::metStart].TopLeft());
   leftMapper.SetDeviceExt(rLeft.Size().cx,rLeft.Size().cy);
   leftMapper.SetDeviceOrg(rLeft.left,rLeft.top);

   grlibPointMapper rightMapper;
   rightMapper.SetMappingMode(grlibPointMapper::Isotropic);
   rightMapper.SetWorldExt(box[pgsTypes::metEnd].Size());
   rightMapper.SetWorldOrg(box[pgsTypes::metEnd].TopRight());
   rightMapper.SetDeviceExt(rRight.Size().cx,rRight.Size().cy);
   rightMapper.SetDeviceOrg(rRight.right,rRight.top);

   CComPtr<IRect2d> bounding_box;
   m_Profile->get_BoundingBox(&bounding_box);
   Float64 left,right,top,bottom;
   bounding_box->get_Left(&left);
   bounding_box->get_Right(&right);
   bounding_box->get_Top(&top);
   bounding_box->get_Bottom(&bottom);

   m_SegmentXLeft = left;

   gpRect2d profileBox;
   profileBox.Set(left,bottom,right,top);

   UINT buffer = 10;
   grlibPointMapper centerMapper;
   centerMapper.SetMappingMode(grlibPointMapper::Anisotropic);
   centerMapper.SetWorldExt(profileBox.Size());
   centerMapper.SetWorldOrg(profileBox.TopLeft());
   centerMapper.SetDeviceExt(rCenter.Size().cx-buffer,rCenter.Size().cy);
   centerMapper.SetDeviceOrg(rCenter.left+buffer/2,rCenter.top);

   CPen pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush brush(SEGMENT_FILL_COLOR);
   CPen strandPen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CBrush strandBrush(STRAND_FILL_COLOR);

   CPen* pOldPen = dc.SelectObject(&pen);
   CBrush* pOldBrush = dc.SelectObject(&brush);

   DrawShape(&dc,leftMapper, m_Shape[pgsTypes::metStart]);
   DrawShape(&dc,centerMapper,m_Profile);
   Draw(&dc,centerMapper,m_BottomFlange,FALSE);
   DrawShape(&dc,rightMapper,m_Shape[pgsTypes::metEnd]);

   dc.SelectObject(&strandPen);
   dc.SelectObject(&strandBrush);
   DrawStrands(&dc,leftMapper,centerMapper,rightMapper);

   // Clean up
   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
}

void CDrawStrandControl::CreateSegmentShape(IShape** ppShape,IPoint2dCollection** ppPoints)
{
   const CSplicedGirderData* pSplicedGirder = m_pSegment->GetGirder();
   CSegmentKey segmentKey(m_pSegment->GetSegmentKey());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);
   pGirder->GetSegmentProfile(segmentKey,pSplicedGirder,false/*don't include closure joint*/,ppShape);
   pGirder->GetSegmentBottomFlangeProfile(segmentKey,pSplicedGirder,false/*don't include closure joint*/,ppPoints);

   GET_IFACE2(pBroker,IBridge,pBridge);
   m_SegmentLength = pBridge->GetSegmentLength(segmentKey);
}

void CDrawStrandControl::DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   Draw(pDC,mapper,polypoints,TRUE);
}

void CDrawStrandControl::Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon)
{
   CollectionIndexType nPoints;
   pPolyPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   pPolyPoints->get__Enum(&enum_points);

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

   if ( bPolygon )
   {
      pDC->Polygon(dev_points,(int)nPoints);
   }
   else
   {
      pDC->Polyline(dev_points,(int)nPoints);
   }

   delete[] points;
   delete[] dev_points;
}

void CDrawStrandControl::DrawStrands(CDC* pDC,grlibPointMapper& leftMapper,grlibPointMapper& centerMapper,grlibPointMapper& rightMapper)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const CStrandRowCollection& strandRows = m_pSegment->Strands.GetStrandRows();
   CStrandRowCollection::const_iterator iter(strandRows.begin());
   CStrandRowCollection::const_iterator iterEnd(strandRows.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CStrandRow& strandRow(*iter);
      GridIndexType nGridPoints = strandRow.m_nStrands/2; // strand grid is only half the full grid (just the grid on the positive X side)
      Float64 Xi = strandRow.m_InnerSpacing/2; // distance from CL Girder to first strand

      // grid points are in Girder Section Coordinates (0,0 is at the top center of the girder)
      // Y is measured positive up and negative down
      Float64 Z[4], Y[4];
      for ( int i = 0; i < 4; i++ )
      {
         Z[i] = strandRow.m_X[i];
         if ( Z[i] < 0 )
         {
            // fractional measure
            Z[i] *= -1.0*m_SegmentLength;
         }

         Y[i] = strandRow.m_Y[i];
         if ( strandRow.m_Face[i] == pgsTypes::TopFace )
         {
            // measured down from top of girder... this is negatve in Girder Section Coordinates
            Y[i] *= -1;
         }
         else
         {
            // adjust to be measured from to of girder
            Float64 Xsp = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(m_pSegment->GetSegmentKey(),Z[i]);
            Float64 Hg = pGirder->GetSegmentHeight(m_pSegment->GetSegmentKey(),m_pSegment->GetGirder(),Xsp);
            Y[i] -= Hg;
         }

         Z[i] = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(m_pSegment->GetSegmentKey(),Z[i]);
         Z[i] = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(m_pSegment->GetSegmentKey(),Z[i]);
      }

      CSize minStrandSize(2,2);
      for ( GridIndexType gridPointIdx = 0; gridPointIdx < nGridPoints; gridPointIdx++ )
      {
         Float64 X = Xi + gridPointIdx*strandRow.m_Spacing;

         LONG dx,dy;

         // Draw in left end
         CRect rect;
         leftMapper.WPtoDP(X-m_Radius,Y[LOCATION_START]-m_Radius,&rect.left,&rect.top); 
         leftMapper.WPtoDP(X+m_Radius,Y[LOCATION_START]-m_Radius,&rect.right,&rect.top); 
         leftMapper.WPtoDP(X-m_Radius,Y[LOCATION_START]+m_Radius,&rect.left,&rect.bottom); 
         leftMapper.WPtoDP(X+m_Radius,Y[LOCATION_START]+m_Radius,&rect.right,&rect.bottom); 
         
         rect.NormalizeRect();
         if ( rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy )
         {
            rect.InflateRect(minStrandSize.cx-rect.Width(),minStrandSize.cy-rect.Height());
         }

         pDC->Ellipse(&rect);

         leftMapper.WPtoDP(-X-m_Radius,Y[LOCATION_START]-m_Radius,&rect.left,&rect.top); 
         leftMapper.WPtoDP(-X+m_Radius,Y[LOCATION_START]-m_Radius,&rect.right,&rect.top); 
         leftMapper.WPtoDP(-X-m_Radius,Y[LOCATION_START]+m_Radius,&rect.left,&rect.bottom); 
         leftMapper.WPtoDP(-X+m_Radius,Y[LOCATION_START]+m_Radius,&rect.right,&rect.bottom); 
         
         rect.NormalizeRect();
         if ( rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy )
         {
            rect.InflateRect(minStrandSize.cx-rect.Width(),minStrandSize.cy-rect.Height());
         }

         pDC->Ellipse(&rect);


         // Draw in right end
         rightMapper.WPtoDP(X-m_Radius,Y[LOCATION_END]-m_Radius,&rect.left,&rect.top); 
         rightMapper.WPtoDP(X+m_Radius,Y[LOCATION_END]-m_Radius,&rect.right,&rect.top); 
         rightMapper.WPtoDP(X-m_Radius,Y[LOCATION_END]+m_Radius,&rect.left,&rect.bottom); 
         rightMapper.WPtoDP(X+m_Radius,Y[LOCATION_END]+m_Radius,&rect.right,&rect.bottom); 
         
         rect.NormalizeRect();
         if ( rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy )
         {
            rect.InflateRect(minStrandSize.cx-rect.Width(),minStrandSize.cy-rect.Height());
         }

         pDC->Ellipse(&rect);

         rightMapper.WPtoDP(-X-m_Radius,Y[LOCATION_END]-m_Radius,&rect.left,&rect.top); 
         rightMapper.WPtoDP(-X+m_Radius,Y[LOCATION_END]-m_Radius,&rect.right,&rect.top); 
         rightMapper.WPtoDP(-X-m_Radius,Y[LOCATION_END]+m_Radius,&rect.left,&rect.bottom); 
         rightMapper.WPtoDP(-X+m_Radius,Y[LOCATION_END]+m_Radius,&rect.right,&rect.bottom); 
         
         rect.NormalizeRect();
         if ( rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy )
         {
            rect.InflateRect(minStrandSize.cx-rect.Width(),minStrandSize.cy-rect.Height());
         }

         pDC->Ellipse(&rect);

         // Draw in segment profile
         centerMapper.WPtoDP(Z[LOCATION_START],Y[LOCATION_START],&dx,&dy);
         pDC->MoveTo(dx,dy);

         if ( strandRow.m_StrandType == pgsTypes::Harped )
         {
            centerMapper.WPtoDP(Z[LOCATION_LEFT_HP],Y[LOCATION_LEFT_HP],&dx,&dy);
            pDC->LineTo(dx,dy);

            centerMapper.WPtoDP(Z[LOCATION_RIGHT_HP],Y[LOCATION_RIGHT_HP],&dx,&dy);
            pDC->LineTo(dx,dy);
         }

         centerMapper.WPtoDP(Z[LOCATION_END],Y[LOCATION_END],&dx,&dy);
         pDC->LineTo(dx,dy);
      }
   }

}
