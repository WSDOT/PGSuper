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

#pragma once

#include <Graphing\GraphingExp.h>
#include <GraphicsLib\PointMapper.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PhysicalConverter.h>

// This is a utility class draws the beam on graphs
class GRAPHINGCLASS CDrawBeamTool
{
public:
   CDrawBeamTool();
   virtual ~CDrawBeamTool();

   void DrawBeam(IBroker* pBroker,CDC* pDC,Float64 beamShift,grlibPointMapper mapper,arvPhysicalConverter* pUnitConverter,IntervalIndexType intervalIdx,const CGirderKey& girderKey);

protected:
   IBroker* m_pBroker;
   arvPhysicalConverter* m_pUnitConverter;
   CGirderKey m_GirderKey;

   CSize m_SupportSize;

   void DrawSegmentEndSupport(Float64 beamShift,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC);
   void DrawIntermediatePier(Float64 beamShift,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC);
   void DrawIntermediateTemporarySupport(Float64 beamShift,const CPrecastSegmentData* pSegment,IntervalIndexType intervalIdx,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC);
   void DrawPier(IntervalIndexType intervalIdx,const CPierData2* pPier,const CTimelineManager* pTimelineMgr,const CPoint& p,CDC* pDC);
   void DrawTemporarySupport(IntervalIndexType intervalIdx,const CTemporarySupportData* pTS,const CTimelineManager* pTimelineMgr,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC);
   void DrawStrongBack(const CTemporarySupportData* pTS,const CPoint& p,const grlibPointMapper& mapper,CDC* pDC);
   void DrawRoller(CPoint p,CDC* pDC);
   void DrawHinge(CPoint p,CDC* pDC);
   void DrawContinuous(CPoint p,CDC* pDC);
   void DrawIntegral(CPoint p,CDC* pDC);
   void DrawIntegralHingeBack(CPoint p,CDC* pDC);
   void DrawIntegralHingeAhead(CPoint p,CDC* pDC);
   void DrawTendons(Float64 beamShift,const CGirderKey& girderKey,IntervalIndexType intervalIndexType,const CTimelineManager* pTimelineMgr,const grlibPointMapper& mapper,CDC* pDC);
};
