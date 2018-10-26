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

#include <Graphing\GraphingExp.h>
#include <EAF\EAFAutoCalcGraphBuilder.h>
#include <GraphicsLib\GraphXY.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PointOfInterest.h>

class CStressHistoryGraphController;
class arvPhysicalConverter;

class GRAPHINGCLASS CStressHistoryGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CStressHistoryGraphBuilder();
   CStressHistoryGraphBuilder(const CStressHistoryGraphBuilder& other);
   virtual ~CStressHistoryGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID);
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID);
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC);
   virtual CGraphBuilder* Clone();

   virtual CEAFGraphControlWindow* GetGraphControlWindow();

protected:
   CStressHistoryGraphController* m_pGraphController;

   afx_msg void OnShowGrid();
   afx_msg void OnTopDeck();
   afx_msg void OnBottomDeck();
   afx_msg void OnTopGirder();
   afx_msg void OnBottomGirder();

   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   unitmgtScalar m_Scalar;
   arvPhysicalConverter* m_pTimeFormat;
   arvPhysicalConverter* m_pIntervalFormat;
   arvPhysicalConverter* m_pYFormat;
   grGraphXY m_Graph;

   bool m_bTopDeck;
   bool m_bBottomDeck;
   bool m_bTopGirder;
   bool m_bBottomGirder;

   int m_XAxisType;

   virtual bool UpdateNow();

   void UpdateXAxis();
   void UpdateYAxis();
   void UpdateGraphTitle(const pgsPointOfInterest& poi);
   void UpdateGraphData(const pgsPointOfInterest& poi);
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
};
