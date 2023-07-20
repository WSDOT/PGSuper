///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawStrandControl.h"

#include <GenericBridge\Helpers.h>

#include <PgsExt\SplicedGirderData.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\BeamFactory.h>
#include <PsgLib\GirderLibraryEntry.h>


// CDrawStrandControl

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CDrawStrandControl, CWnd)

CDrawStrandControl::CDrawStrandControl()
{
   m_pSegment = nullptr;
   m_pStrands = nullptr;
}

CDrawStrandControl::~CDrawStrandControl()
{
}


BEGIN_MESSAGE_MAP(CDrawStrandControl, CWnd)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CDrawStrandControl message handlers
void CDrawStrandControl::CustomInit(const CPrecastSegmentData* pSegment,const CStrandData* pStrands, const CSegmentPTData* pTendons)
{
   m_Shape[pgsTypes::metStart].Release();
   m_Shape[pgsTypes::metEnd].Release();
   m_Profile.Release();
   m_BottomFlangeProfile.Release();

   m_pSegment = pSegment;
   m_pStrands = pStrands;
   m_pTendons = pTendons;

   m_Xoffset[pgsTypes::metStart] = 0;
   m_Xoffset[pgsTypes::metEnd] = 0;

   // Gets the segment height based on the data in the girder library entry
   const GirderLibraryEntry* pGdrEntry = m_pSegment->GetGirder()->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

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

   if (m_pSegment->TopFlangeThickeningType != pgsTypes::tftNone)
   {
      // Add the top flange thickening to the overall depth of the girder
      m_Hg += m_pSegment->TopFlangeThickening;
   }

   // add in precamber to the overall height
   m_Hg += m_pSegment->Precamber;

   // Get the end view cross section shapes of the segment
   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 2);
   const pgsPointOfInterest& startPoi(vPoi.front());
   const pgsPointOfInterest& endPoi(vPoi.back());

   GET_IFACE2(pBroker, IShapes, pShapes);
   pShapes->GetSegmentShape(pSegment,startPoi.GetDistFromStart(), pgsTypes::sbRight, &m_Shape[pgsTypes::metStart]);
   pShapes->GetSegmentShape(pSegment,endPoi.GetDistFromStart(), pgsTypes::sbLeft, &m_Shape[pgsTypes::metEnd]);

   // If the girder is asymmetric, compute the horizontal offset for the strand positions
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)(i);
      CComQIPtr<IAsymmetricSection> asymmetric(m_Shape[endType]);
      if (asymmetric)
      {
         Float64 wLeft, wRight;
         asymmetric->GetTopWidth(&wLeft, &wRight);
         m_Xoffset[endType] = 0.5*(wRight - wLeft);
      }
   }

   // Create the profiles
   CreateSegmentProfiles(&m_Profile,&m_BottomFlangeProfile);

   // capture segment length for use later
   GET_IFACE2(pBroker, IBridge, pBridge);
   m_SegmentLength = pBridge->GetSegmentLength(segmentKey);
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

   // setup a clipping region so we don't draw outside of the control boundaries
   CRect rClient;
   GetClientRect(&rClient);
   CRgn rgn;
   rgn.CreateRectRgnIndirect(&rClient);
   dc.SelectClipRgn(&rgn);


   //
   // Set up coordinate mapping
   //
   std::array<WBFL::Graphing::Rect,2> box;
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
   }

   CComPtr<IRect2d> bounding_box;
   m_Profile->get_BoundingBox(&bounding_box);
   Float64 left, right, top, bottom;
   bounding_box->get_Left(&left);
   bounding_box->get_Right(&right);
   bounding_box->get_Top(&top);
   bounding_box->get_Bottom(&bottom);

   WBFL::Graphing::Rect profileBox;
   profileBox.Set(left, bottom, right, top);

   Float64 aspect_ratio = box[pgsTypes::metStart].Width()/m_Hg;

   CRect rLeft(rClient.left,rClient.top,rClient.left + (LONG)((Float64)(rClient.Height())*aspect_ratio),rClient.bottom);
   CRect rRight(rClient.right - (LONG)((Float64)(rClient.Height())*aspect_ratio), rClient.top, rClient.right, rClient.bottom);
   CRect rCenter(rLeft.right,rClient.top,rRight.left,rClient.bottom);


   // determine of the profile shape has an upward or downward curve
   // due to precamber or top flange thickening
   Float64 precamber = m_pSegment->Precamber;
   Float64 thickening = 0;
   switch (m_pSegment->TopFlangeThickeningType)
   {
   case pgsTypes::tftNone:
      break;
   case pgsTypes::tftEnds:
      thickening = -m_pSegment->TopFlangeThickening;
      break;
   case pgsTypes::tftMiddle:
      thickening = m_pSegment->TopFlangeThickening;
      break;
   default:
      ATLASSERT(false);
   }

   bool bDoesProfileCurveUpwards = (0 < (precamber + thickening) ? true : false);

   Float64 wy = Max(box[etStart].Height(), box[etEnd].Height(), profileBox.Height());
   Float64 woy;
   if (bDoesProfileCurveUpwards)
   {
      woy = Max(box[etStart].Bottom(), box[etEnd].Bottom(), profileBox.Bottom());
   }
   else
   {
      woy = Min(box[etStart].Top(), box[etEnd].Top(), profileBox.Top());
   }

   WBFL::Graphing::PointMapper leftMapper;
   leftMapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   leftMapper.SetWorldExt(box[pgsTypes::metStart].Width(),wy);
   leftMapper.SetDeviceExt(rLeft.Size().cx, rLeft.Size().cy);
   leftMapper.SetWorldOrg(box[pgsTypes::metStart].Left(), woy);
   if (bDoesProfileCurveUpwards)
   {
      leftMapper.SetDeviceOrg(rLeft.left, rLeft.bottom);
   }
   else
   {
      leftMapper.SetDeviceOrg(rLeft.left, rLeft.top);
   }

   // we want an end face view at the right end
   // since all the section cuts are looking ahead on the girder
   // we need to mirror the shape... we can do this through
   // the point mapper
   WBFL::Graphing::PointMapper rightMapper;
   rightMapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   rightMapper.SetWorldExt(box[pgsTypes::metEnd].Width(),wy);
   rightMapper.SetWorldOrg(box[pgsTypes::metEnd].TopRight());
   rightMapper.SetDeviceExt(-rRight.Size().cx, rRight.Size().cy); // use -cx to mirror the shape (mirrors about the right edge)
   rightMapper.SetWorldOrg(box[pgsTypes::metEnd].Right(), woy);
   if (bDoesProfileCurveUpwards)
   {
      rightMapper.SetDeviceOrg(rRight.left, rRight.bottom); // maps the right world point to the left device point (translates the shape back into position)
   }
   else
   {
      rightMapper.SetDeviceOrg(rRight.left, rRight.top); // maps the right world point to the left device point (translates the shape back into position)
   }

   m_SegmentXLeft = left;

   UINT buffer = 10;
   WBFL::Graphing::PointMapper centerMapper;
   centerMapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Anisotropic);
   centerMapper.SetWorldExt(profileBox.Width(),wy);
   centerMapper.SetDeviceExt(rCenter.Size().cx - buffer, rCenter.Size().cy);
   centerMapper.SetWorldOrg(profileBox.Left(), woy);
   if (bDoesProfileCurveUpwards)
   {
      centerMapper.SetDeviceOrg(rCenter.left + buffer / 2, rCenter.bottom);
   }
   else
   {
      centerMapper.SetDeviceOrg(rCenter.left + buffer / 2, rCenter.top);
   }

   CPen pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush brush(SEGMENT_FILL_COLOR);

   CPen* pOldPen = dc.SelectObject(&pen);
   CBrush* pOldBrush = dc.SelectObject(&brush);

   DrawShape(&dc,leftMapper, m_Shape[pgsTypes::metStart]);
   DrawShape(&dc,centerMapper,m_Profile);
   Draw(&dc,centerMapper,m_BottomFlangeProfile,FALSE);
   DrawShape(&dc,rightMapper,m_Shape[pgsTypes::metEnd]);

   DrawStrands(&dc, leftMapper, centerMapper, rightMapper);
   DrawTendons(&dc, leftMapper, centerMapper, rightMapper);

   // Clean up
   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
}

void CDrawStrandControl::CreateSegmentProfiles(IShape** ppShape,IPoint2dCollection** ppPoints)
{
   const CSplicedGirderData* pSplicedGirder = m_pSegment->GetGirder();
   CSegmentKey segmentKey(m_pSegment->GetSegmentKey());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pGirder);
   pGirder->GetSegmentProfile(segmentKey,pSplicedGirder,false/*don't include closure joint*/,ppShape);
   pGirder->GetSegmentBottomFlangeProfile(segmentKey,pSplicedGirder,false/*don't include closure joint*/,ppPoints);
}

void CDrawStrandControl::DrawShape(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   Draw(pDC,mapper,polypoints,TRUE);
}

void CDrawStrandControl::Draw(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon)
{
   IndexType nPoints;
   pPolyPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   pPolyPoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( IndexType i = 0; i < nPoints; i++ )
   {
      LONG dx,dy;
      WBFL::Graphing::Point point;
      points[i]->Location(&point.X(), &point.Y());
      mapper.WPtoDP(point,&dx,&dy);
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

void CDrawStrandControl::DrawStrands(CDC* pDC, WBFL::Graphing::PointMapper& leftMapper, WBFL::Graphing::PointMapper& centerMapper, WBFL::Graphing::PointMapper& rightMapper)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pGirder);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2_NOCHECK(pBroker, IStrandGeometry, pStrandGeom);

   CPen strandPen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CBrush strandBrush(STRAND_FILL_COLOR);

   CPen tempStrandPen(PS_SOLID,1,TEMPORARY_FILL_COLOR);
   CBrush tempStrandBrush(TEMPORARY_FILL_COLOR);

   CPen extendedPen(PS_SOLID,1,EXTENDED_FILL_COLOR);
   CBrush extendedBrush(EXTENDED_FILL_COLOR);

   CPen debondedPen(PS_SOLID,1,DEBOND_FILL_COLOR);
   CBrush debondedBrush(DEBOND_FILL_COLOR);

   CPen errorPen(PS_SOLID, 1, RED);
   CBrush errorBrush(RED);

   // Get the harp point locations and convert them to girder path coordinates
   std::array<Float64, 4> Xs; // harp point locations in segment coordinates
   std::array<Float64, 4> Xgp; // harp point locations in girder path coordinates
   m_pStrands->GetHarpPoints(&Xs[ZoneBreakType::Start], &Xs[ZoneBreakType::LeftBreak], &Xs[ZoneBreakType::RightBreak], &Xs[ZoneBreakType::End]);
   for (auto i = 0; i < 4; i++)
   {
      if (Xs[i] < 0)
      {
         // fractional measure
         if (Xs[i] < -1)
         {
            // fractional length can't be more than 100%
            // this is probably a case where the user changed the unit of measure to % and hasn't updated the input value yet
            // just turn this into a direct value input and proceed. The grid validation will balk at the bad input
            Xs[i] *= -1;
         }
         else
         {
            Xs[i] *= -1.0*m_SegmentLength;
         }
      }

      Xgp[i] = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(m_pSegment->GetSegmentKey(), Xs[i]);
      Xgp[i] = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(m_pSegment->GetSegmentKey(), Xgp[i]);
   }

   pgsTypes::StrandDefinitionType strandDefinitionType = m_pStrands->GetStrandDefinitionType();

   std::array<StrandIndexType, 3> strandIdx{ 0,0,0 };
   const auto& strandRows = m_pStrands->GetStrandRows();
   std::array<StrandIndexType, 3> nStrandsDefined{ m_pStrands->GetStrandCount(pgsTypes::Straight), m_pStrands->GetStrandCount(pgsTypes::Harped), m_pStrands->GetStrandCount(pgsTypes::Temporary) };
   for( const auto& strandRow : strandRows)
   {
      GridIndexType nGridPoints = 0;
      if (strandDefinitionType == pgsTypes::sdtDirectStrandInput)
      {
         // direct strand input is for individual strands so there is only one point to draw
         nGridPoints = 1;
      }
      else
      {
         nGridPoints = strandRow.m_nStrands / 2; // strand grid is only half the full grid (just compute points for the grid on the positive X side)
         if (::IsOdd(strandRow.m_nStrands))
         {
            nGridPoints++;
         }
      }

      Float64 Xi = strandRow.m_Z; // distance from CL Girder to strand 
      if (m_pStrands->GetStrandDefinitionType() == pgsTypes::sdtDirectRowInput)
      {
         // strands are in rows so Z (aka Xi) is the inner row spacing
         // divide it by 2 to get distance from CL to strand
         ATLASSERT(0 <= strandRow.m_Z);
         Xi /= 2;
      }

      // grid points are in Girder Section Coordinates (Y = 0 is at the top of the girder)
      // Y is measured positive up and negative down
      std::array<Float64, 4> Xhp, Y;
      pStrandGeom->ResolveStrandRowElevations(m_pSegment, m_pStrands, strandRow, Xhp, Y);

      LONG diameter = 2;
      LONG radius = diameter / 2;
      CSize minStrandSize(2,2);
      for (GridIndexType gridPointIdx = 0; gridPointIdx < nGridPoints; gridPointIdx++)
      {
         Float64 X = Xi + gridPointIdx*strandRow.m_Spacing;

         StrandIndexType nStrandsPerGridPoint = 1;
         if (strandDefinitionType == pgsTypes::sdtDirectRowInput && !IsZero(strandRow.m_Z))
         {
            nStrandsPerGridPoint = 2;
         }


         LONG dx, dy;

         //
         // Draw strands in end cross sections
         //
         for (int i = 0; i < 2; i++)
         {
            pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
            ZoneBreakType zoneBreak = (end == pgsTypes::metStart ? ZoneBreakType::Start : ZoneBreakType::End);
            WBFL::Graphing::PointMapper* pPointMapper = (end == pgsTypes::metStart ? &leftMapper : &rightMapper);

            CPoint point;
            pPointMapper->WPtoDP(X - m_Xoffset[end], Y[zoneBreak], &point.x, &point.y);
            point.Offset(-radius, -radius);
            CRect rect(point, CSize(diameter, diameter));

            rect.NormalizeRect();
            if (rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy)
            {
               rect.InflateRect(minStrandSize.cx - rect.Width(), minStrandSize.cy - rect.Height());
            }

            if (strandDefinitionType == pgsTypes::sdtDirectRowInput && ( (IsOdd(strandRow.m_nStrands) && !IsZero(strandRow.m_Z)) || (IsEven(strandRow.m_nStrands) && IsZero(strandRow.m_Z)) || (3 <= strandRow.m_nStrands && IsZero(strandRow.m_Spacing)) ) )
            {
               // Z must be zero if nStrands is odd... it's not, so use the error color
               // spacing cannot be zero if there is 3 or more strands
               // spacing can be zero if there are 2 strands and Z is not zero
               // if there are 3 strands, Z must be zero so spacing cannot be zero
               pDC->SelectObject(&errorPen);
               pDC->SelectObject(&errorBrush);
            }
            else
            {
               if (strandRow.m_bIsExtendedStrand[end])
               {
                  pDC->SelectObject(&extendedPen);
                  pDC->SelectObject(&extendedBrush);
               }
               else if (strandRow.m_bIsDebonded[end])
               {
                  pDC->SelectObject(&debondedPen);
                  pDC->SelectObject(&debondedBrush);
               }
               else
               {
                  pDC->SelectObject(strandRow.m_StrandType == pgsTypes::Temporary ? &tempStrandPen : &strandPen);
                  pDC->SelectObject(strandRow.m_StrandType == pgsTypes::Temporary ? &tempStrandBrush : &strandBrush);
               }
            }

            pDC->Ellipse(&rect);

            if (strandDefinitionType == pgsTypes::sdtDirectRowInput)
            {
               // strands are in rows, so draw the mirrored strand
               pPointMapper->WPtoDP(-X - m_Xoffset[end], Y[zoneBreak], &point.x, &point.y);
               point.Offset(-radius, -radius);
               rect = CRect(point, CSize(diameter, diameter));

               rect.NormalizeRect();
               if (rect.Width() < minStrandSize.cx || rect.Height() < minStrandSize.cy)
               {
                  rect.InflateRect(minStrandSize.cx - rect.Width(), minStrandSize.cy - rect.Height());
               }

               pDC->Ellipse(&rect);
            }
         }

         //
         // Draw in strands segment profile
         //
         if (strandDefinitionType == pgsTypes::sdtDirectRowInput && ((IsOdd(strandRow.m_nStrands) && !IsZero(strandRow.m_Z)) || (IsEven(strandRow.m_nStrands) && IsZero(strandRow.m_Z))))
         {
            // m_Z must be zero if nStrands is odd
            pDC->SelectObject(&errorPen);
            pDC->SelectObject(&errorBrush);
         }
         else
         {
            pDC->SelectObject(&strandPen);
            pDC->SelectObject(&strandBrush);
         }

         CComPtr<IPoint2dCollection> profile; // profile points in Girder Path Coordinates
         pStrandGeom->GetStrandProfile(m_pSegment, m_pStrands, strandRow.m_StrandType, strandIdx[strandRow.m_StrandType], &profile);
         strandIdx[strandRow.m_StrandType] += nStrandsPerGridPoint;
         // The number of strands and the strand spacing dimensions can be inconsistent
         // so we have to protected against having and index that is beyond the array bounds
         strandIdx[strandRow.m_StrandType] = Min(nStrandsDefined[strandRow.m_StrandType]-1, strandIdx[strandRow.m_StrandType]);

         CComPtr<IPoint2d> pnt;
         IndexType nPoints;
         profile->get_Count(&nPoints);
         for (IndexType idx = 0; idx < nPoints; idx++)
         {
            pnt.Release();
            profile->get_Item(idx, &pnt);
            Float64 z, y;
            pnt->Location(&z, &y);

            centerMapper.WPtoDP(z, y, &dx, &dy);
            if (idx == 0)
            {
               pDC->MoveTo(dx, dy);
            }
            else
            {
               pDC->LineTo(dx, dy);
            }
         }
      }
   }
}

void CDrawStrandControl::DrawTendons(CDC* pDC, WBFL::Graphing::PointMapper& leftMapper, WBFL::Graphing::PointMapper& centerMapper, WBFL::Graphing::PointMapper& rightMapper)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IGirder, pGirder);
   GET_IFACE2_NOCHECK(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);

   const CSegmentKey& segmentKey = m_pSegment->GetSegmentKey();
   
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 2);

   GET_IFACE2(pBroker, ILibrary, pLibrary);
   DuctLibrary* pDuctLibrary = pLibrary->GetDuctLibrary();


   CPen tendonPen(PS_SOLID, 1, SEGMENT_TENDON_LINE_COLOR);
   CBrush tendonBrush(SEGMENT_TENDON_FILL_COLOR);

   CPen errorPen(PS_SOLID, 1, RED);
   CBrush errorBrush(RED);

   WebIndexType nWebs = pGirder->GetWebCount(segmentKey);

   DuctIndexType nDucts = m_pTendons->GetDuctCount();
   for(DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      const auto* pDuct = m_pTendons->GetDuct(ductIdx);
      const DuctLibraryEntry* pDuctLibEntry = (const DuctLibraryEntry*)pDuctLibrary->GetEntry(pDuct->Name.c_str());
      Float64 duct_diameter = pDuctLibEntry->GetOD();

      LONG dx, dy;

      //
      // Draw strands in end cross sections
      //
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
         WBFL::Graphing::PointMapper* pPointMapper = (end == pgsTypes::metStart ? &leftMapper : &rightMapper);

         pPointMapper->GetAdjustedDeviceExt(&dx, &dy);
         auto world_ext = pPointMapper->GetWorldExt();
         LONG diameter = (LONG)(dy*duct_diameter / world_ext.Dy());
         LONG radius = diameter / 2;

         CSegmentDuctData::DuctPointType ductPoint = (i == 0 ? CSegmentDuctData::Left : CSegmentDuctData::Right);

         for (WebIndexType webIdx = 0; webIdx < nWebs; webIdx++)
         {
            CComPtr<IPoint3d> pnt;
            pSegmentTendonGeometry->GetSegmentDuctPoint(vPoi[i], m_pSegment, pDuct, &pnt);

            Float64 X, Y;
            pnt->get_X(&X);
            pnt->get_Y(&Y);

            if (webIdx != 0)
            {
               X *= -1;
            }

            CPoint point;
            pPointMapper->WPtoDP(X, Y, &point.x, &point.y);
            point.Offset(-radius, -radius);
            CRect rect(point, CSize(diameter, diameter));

            rect.NormalizeRect();

            pDC->SelectObject(&tendonPen);
            pDC->SelectObject(&tendonBrush);
            pDC->Ellipse(&rect);
         } // next web
      } // next end face

      //
      // Draw in the segment profile
      //

      CComPtr<IPoint2dCollection> profile; // profile points in Girder Path Coordinates
      pSegmentTendonGeometry->GetDuctCenterline(m_pSegment->GetSegmentKey(), m_pSegment, pDuct, &profile);

      CComPtr<IPoint2d> pnt;
      IndexType nPoints;
      profile->get_Count(&nPoints);
      for (IndexType idx = 0; idx < nPoints; idx++)
      {
         pnt.Release();
         profile->get_Item(idx, &pnt);
         Float64 z, y;
         pnt->Location(&z, &y);

         centerMapper.WPtoDP(z, y, &dx, &dy);
         if (idx == 0)
         {
            pDC->MoveTo(dx, dy);
         }
         else
         {
            pDC->LineTo(dx, dy);
         }
      }
   }
}

