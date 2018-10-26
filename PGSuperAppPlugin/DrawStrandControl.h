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

   void CustomInit(const CPrecastSegmentData* pSegment);

   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   const CPrecastSegmentData* m_pSegment; // this is the segment for which we are drawing strands

   Float64 m_Hg; // maximum height of section

   Float64 m_HgStart; // height of the segment at the start (left) end
   Float64 m_HgEnd; // height of the segment at the end (right) end
   Float64 m_SegmentLength; // length of the segment
   Float64 m_SegmentXLeft; // X-value to offset strands so they end up in the correct location on the drawing

   Float64 m_Radius; // radius of the strand

   CComPtr<IShape> m_Shape[2]; // segment shape at start and end of segment (use pgsTupes::MemberEndType to access array)
   CComPtr<IShape> m_Profile; // profile of segment
   CComPtr<IPoint2dCollection> m_BottomFlange; // top of bottom flange line

   void CreateSegmentShape(IShape** ppShape,IPoint2dCollection** ppPoints);
   void DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape);
   void Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon);
   void DrawStrands(CDC* pDC,grlibPointMapper& leftMapper,grlibPointMapper& centerMapper,grlibPointMapper& rightMapper);
};


