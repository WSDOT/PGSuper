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

#pragma once

#include <Graphing/PointMapper.h>
#include <PsgLib\SplicedGirderData.h>
#include <PsgLib\Keys.h>

// CDrawTendonsControl
class CDrawTendonsControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawTendonsControl)

public:
	CDrawTendonsControl();
	virtual ~CDrawTendonsControl();

   void CustomInit(const CGirderKey& girderKey,const CSplicedGirderData* pGirder,const CPTData* pPTData);

   void SetMapMode(WBFL::Graphing::PointMapper::MapMode mm);
   WBFL::Graphing::PointMapper::MapMode GetMapMode() const;

   void SetDuct(DuctIndexType ductIdx);
   DuctIndexType GetDuct() const;

   // If true, all ducts are drawn. The duct identifed with SetDuct will be drawn in the duct color and all others will be in a backgroudn color
   void DrawAllDucts(bool bDrawAll);
   bool DrawAllDucts() const;

   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   const CSplicedGirderData* m_pGirder;
   const CPTData* m_pPTData;
   CGirderKey m_GirderKey;
   DuctIndexType m_DuctIdx; // index of the duct to be drawn... Use ALL_DUCTS to draw all ducts

   bool m_bDrawAllDucts;
   WBFL::Graphing::PointMapper::MapMode m_MapMode;

   void DrawShape(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IShape* pShape);
   void Draw(CDC* pDC, WBFL::Graphing::PointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon);
};


