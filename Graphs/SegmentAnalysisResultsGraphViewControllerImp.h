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

#include <Graphs\SegmentAnalysisResultsGraphViewController.h>
#include "SegmentAnalysisResultsGraphController.h"

// {93D52F90-4E15-4BC5-8C43-DF102B6ED836}
DEFINE_GUID(CLSID_SegmentAnalysisResultsGraphViewController,
   0x93d52f90,0x4e15,0x4bc5,0x8c,0x43,0xdf,0x10,0x2b,0x6e,0xd8,0x36);


class ATL_NO_VTABLE CSegmentAnalysisResultsGraphViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CSegmentAnalysisResultsGraphViewController, &CLSID_SegmentAnalysisResultsGraphViewController>,
   public ISegmentAnalysisResultsGraphViewController
{
public:
   CSegmentAnalysisResultsGraphViewController();
   virtual ~CSegmentAnalysisResultsGraphViewController();

   void Init(CSegmentAnalysisResultsGraphController* pGraphController, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CSegmentAnalysisResultsGraphViewController)
      COM_INTERFACE_ENTRY(ISegmentAnalysisResultsGraphViewController)
      COM_INTERFACE_ENTRY(IEAFViewController)
   END_COM_MAP()

   // IEAFViewController
public:
   virtual bool IsOpen() const override;

   // Closes the view
   virtual void Close() override;

   // implement later....
   //virtual void Move(INT x, INT y) = 0;
   //virtual void Size(INT x, INT y) = 0;
   //virtual void GetSize() = 0;
   //virtual void GetPosition() = 0;
   //virtual void GetState() = 0;
   virtual void Minimize() override;
   virtual void Maximize() override;
   virtual void Restore() override;

   // ISegmentAnalysisResultsGraphViewController
public:
   virtual void SetGraphMode(GraphMode mode) override;
   virtual GraphMode GetGraphMode() const override;

   virtual void SelectSegment(const CSegmentKey& segmentKey) override;
   virtual const CSegmentKey GetSegment() const override;


   virtual void SetResultsType(ResultsType resultsType) override;
   virtual ResultsType GetResultsType() const override;

   virtual std::vector<ActionType> GetActionTypes() const override;
   virtual LPCTSTR GetActionName(ActionType action) const override;

   virtual void SetActionType(ActionType actionType) override;
   virtual ActionType GetActionType() const override;

   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) override;
   virtual pgsTypes::AnalysisType GetAnalysisType() const override;

   virtual IndexType GetGraphTypeCount() const override;
   virtual CString GetGraphType(IndexType idx) const override;
   virtual void SelectGraphType(IndexType idx) override;
   virtual void SelectGraphType(LPCTSTR lpszType) override;

   virtual IndexType GetGraphCount() const override;
   virtual IndexType GetSelectedGraphCount() const override;
   virtual std::vector<IndexType> GetSelectedGraphs() const override;
   virtual CString GetGraphName(IndexType graphIdx) const override;
   virtual void SelectGraph(IndexType graphIdx) override;
   virtual void SelectGraph(LPCTSTR lpszGraphName) override;
   virtual void SelectGraphs(const std::vector<IndexType>& vGraphs) override;
   virtual void SelectGraphs(const std::vector<CString>& vGraphs) override;

   virtual void SelectStressLocation(pgsTypes::StressLocation location, bool bSelect) override;
   virtual bool IsStressLocationSelected(pgsTypes::StressLocation location) const override;

   virtual void ShowGrid(bool bShow) override;
   virtual bool ShowGrid() const override;

   virtual void ShowGirder(bool bShow) override;
   virtual bool ShowGirder() const override;

private:
   CSegmentAnalysisResultsGraphController* m_pGraphController;
   CComPtr<IEAFViewController> m_pStdController;
};
