///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <Graphs/GraphsExp.h>
#include <EAF\EAFAutoCalcGraphBuilder.h>
#include <Graphing/GraphXY.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PointOfInterest.h>

class CGirderGraphControllerBase;
class CEAFGraphControlWindow;

class WBFL::Units::PhysicalConverter;

// This is an abstract base class for graphs that display girder along the X-axis
class GRAPHCLASS CGirderGraphBuilderBase : public CEAFAutoCalcGraphBuilder
{
public:
   CGirderGraphBuilderBase();
   CGirderGraphBuilderBase(const CGirderGraphBuilderBase& other);
   virtual ~CGirderGraphBuilderBase();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual CEAFGraphControlWindow* GetGraphControlWindow() override;
   
   virtual bool HandleDoubleClick(UINT nFlags,CPoint point) override;

   virtual bool UpdateNow() = 0;

   virtual void UpdateXAxis();
   virtual void UpdateYAxis();

   void ShowGrid(bool bShow);
   void ShowBeam(bool bShow);

   void Shift(bool bShift);
   bool Shift() const;

   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


protected:
   CGirderGraphControllerBase* m_pGraphController;

   // implement this method to create the graph controller object (the C++ object, not the Windows object)
   virtual CGirderGraphControllerBase* CreateGraphController() = 0;

   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;
   WBFL::Graphing::GraphXY m_Graph;
   WBFL::Units::PhysicalConverter* m_pXFormat;
   WBFL::Units::PhysicalConverter* m_pYFormat;

   // zero tolerance values
   Float64 m_ZeroToleranceX;
   Float64 m_ZeroToleranceY;

   Float64 ComputeShift(const CGirderKey& girderKey);
   void GetXValues(const PoiList& vPoi,std::vector<Float64>* pXVals);
   void AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals);
   void AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<WBFL::System::SectionValue>& yvals);
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
   void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC);

   // returns the range of intervals overwhich the beam must be represented
   virtual void GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx,IntervalIndexType* pLastIntervalIdx) = 0;

   // returns the style parameters for beam drawing
   virtual DWORD GetDrawBeamStyle() const;

private:
   bool m_bShift;
};
