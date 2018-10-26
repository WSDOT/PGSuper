///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <EAF\EAFAutoCalcGraphBuilder.h>
#include <GraphicsLib\GraphXY.h>

class arvPhysicalConverter;
class CStabilityGraphController;

class GRAPHINGCLASS CStabilityGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CStabilityGraphBuilder();
   CStabilityGraphBuilder(const CStabilityGraphBuilder& other);
   virtual ~CStabilityGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID);
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID);
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC);
   virtual CGraphBuilder* Clone();

   virtual CEAFGraphControlWindow* GetGraphControlWindow();

protected:

   CStabilityGraphController* m_pGraphController;

   afx_msg void OnShowGrid();
   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   arvPhysicalConverter* m_pXFormat;
   arvPhysicalConverter* m_pYFormat;
   grGraphXY m_Graph;

   std::_tstring m_PrintSubtitle;


   void UpdateXAxis();

   virtual bool UpdateNow();
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);

   void DrawTheGraph(CWnd* pGraphWnd,CDC* pDC);
   void DrawLegend(CDC* pDC);
};
