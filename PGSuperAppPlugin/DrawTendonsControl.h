///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <GraphicsLib\PointMapper.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\Keys.h>

// CDrawTendonsControl
class CDrawTendonsControl : public CWnd
{
	DECLARE_DYNAMIC(CDrawTendonsControl)

public:
	CDrawTendonsControl();
	virtual ~CDrawTendonsControl();

   void CustomInit(const CGirderKey& girderKey,const CSplicedGirderData* pGirder);

   afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
   const CSplicedGirderData* m_pGirder;
   CGirderKey m_GirderKey;

   void DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape);
   void Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon);
};


