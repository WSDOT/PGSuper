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

#pragma once

#include <PgsExt\PrecastSegmentData.h>
#include <GraphicsLib\PointMapper.h>


// CDrawStrandControl

class CDrawStrandControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawStrandControl)

public:
	CDrawStrandControl();
	virtual ~CDrawStrandControl();

   void CustomInit(const CPrecastSegmentData* pSegment,const CStrandData* pStrands,const CSegmentPTData* pTendons);

   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   const CPrecastSegmentData* m_pSegment; // this is the segment for which we are drawing strands
   const CStrandData* m_pStrands; // these are the actual strands (can be edited from UI and thus different from m_pSegment->Strands)
   const CSegmentPTData* m_pTendons; // these are the actual tendons (can be edited from UI and thuse different form m_pSegment->Tendons)

   Float64 m_Hg; // overall height including top flange thickening and precamber

   Float64 m_HgStart; // height of the segment at the start (left) end
   Float64 m_HgEnd; // height of the segment at the end (right) end
   Float64 m_SegmentLength; // length of the segment
   Float64 m_SegmentXLeft; // X-value to offset strands so they end up in the correct location on the drawing

   std::array<Float64,2> m_Xoffset; // horizontal offset needed for asymmetric girder sections

   std::array<CComPtr<IShape>,2> m_Shape; // segment shape at start and end of segment (use pgsTypes::MemberEndType to access array)
   CComPtr<IShape> m_Profile; // profile of segment
   CComPtr<IPoint2dCollection> m_BottomFlangeProfile; // top of bottom flange line

   void CreateSegmentProfiles(IShape** ppShape,IPoint2dCollection** ppPoints);
   void DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape);
   void Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon);
   void DrawStrands(CDC* pDC, grlibPointMapper& leftMapper, grlibPointMapper& centerMapper, grlibPointMapper& rightMapper);
   void DrawTendons(CDC* pDC, grlibPointMapper& leftMapper, grlibPointMapper& centerMapper, grlibPointMapper& rightMapper);
};


