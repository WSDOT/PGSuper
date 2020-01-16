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

#pragma once

#include <Graphing\GraphingExp.h>
#include <GraphicsLib\PointMapper.h>
#include <PgsExt\BridgeDescription2.h>
#include <UnitMgt\UnitValueNumericalFormatTools.h>

interface IGirder;
interface IPointOfInterest;
interface IIntervals;

#define DBS_ERECTED_SEGMENTS_ONLY 0x0001 // draw only erected segments
#define DBS_HAULED_SEGMENTS_ONLY  0x0002 // in hauling intervals, draw only those segments that are being hauled

// This is a utility class draws the beam on graphs
class GRAPHINGCLASS CDrawBeamTool
{
public:
   CDrawBeamTool();
   virtual ~CDrawBeamTool();

   void SetStyle(DWORD dwStyle);
   DWORD GetStyle() const;

   // Draws the beam
   // graphMapper is the point mapper used to draw the graph. 
   // pUnitConverter is used to convert X-axis values into display units. Use the same unit converter as the graph
   // intervalIdx is the interval for when the beam is to be drawn. The color of the girder segments is different for
   // segments that have not yet been erected as it is for segments that have been erected
   // girderKey is for the girder that is being drawn
   // beamShift tells how must left or right to shift the beam so that it lines up with the Y axis
   // To draw an individual span or segment, use the following code
   // GET_IFACE(IPointOfInterest,pPoi);
   // Float64 beamShift = -1*pPoi->ConvertPoiToGirderlineCoordinate(pgsPointOfInterest(segmentKey,0.0));

   void DrawBeam(IBroker* pBroker,CDC* pDC,const grlibPointMapper& graphMapper,arvPhysicalConverter* pUnitConverter, IntervalIndexType firstPlottingIntervalIdx, IntervalIndexType lastPlottingIntervalIdx,const CGirderKey& girderKey,Float64 beamShift);

protected:
   IBroker* m_pBroker;
   arvPhysicalConverter* m_pUnitConverter;
   
   CSize m_SupportSize;

   DWORD m_dwStyle;

   void DrawSegment(Float64 beamShift, IntervalIndexType intervalIdx, bool bIsHaulingInterval,const CSegmentKey& segmentKey, IIntervals* pIntervals, IGirder* pIGirder, IPointOfInterest* pPoi, const grlibPointMapper& mapper, CDC* pDC);
   void DrawClosureJoint(Float64 beamShift, IntervalIndexType intervalIdx, const CClosureKey& closureKey, IIntervals* pIntervals, IGirder* pIGirder, IPointOfInterest* pPoi, const grlibPointMapper& mapper, CDC* pDC);
   void DrawSegmentEndSupport(Float64 beamShift,IntervalIndexType intervalIdx, bool bIsHaulingInterval, const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType, IIntervals* pIntervals, IPointOfInterest* pPoi,const grlibPointMapper& mapper,CDC* pDC);

   void DrawPier(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, PierIndexType pierIdx, const grlibPointMapper& mapper, CDC* pDC);
   CPoint GetPierPoint(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, PierIndexType pierIdx, const grlibPointMapper& mapper);

   void DrawTemporarySupport(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, SupportIndexType tsIdx, const grlibPointMapper& mapper, CDC* pDC);
   CPoint GetTemporarySupportPoint(Float64 beamShift, IntervalIndexType intervalIdx, const CGirderKey& girderKey, SupportIndexType tsIdx, const grlibPointMapper& mapper);

   void DrawStrongBack(const CGirderKey& girderKey,const CTemporarySupportData* pTS,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC);
   void DrawRoller(CPoint p,CDC* pDC);
   void DrawHinge(CPoint p,CDC* pDC);
   void DrawContinuous(CPoint p,CDC* pDC);
   void DrawIntegral(CPoint p,CDC* pDC);
   void DrawIntegralHingeBack(CPoint p,CDC* pDC);
   void DrawIntegralHingeAhead(CPoint p,CDC* pDC);
   void DrawTendons(Float64 beamShift,IntervalIndexType intervalIndexType, const CGirderKey& girderKey, const grlibPointMapper& mapper,CDC* pDC);
   Float64 ComputeShift(const CGirderKey& girderKey);
};
