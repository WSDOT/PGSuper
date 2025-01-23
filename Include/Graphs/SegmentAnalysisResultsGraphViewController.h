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

#include <EAF\EAFViewController.h>

// these includes are for data types
#include <IFace\AnalysisResults.h>
#include <Graphs\GraphTypes.h>
#include <PGSuperTypes.h>

// {441AE189-18F9-436F-978F-6F047C626ADB}
DEFINE_GUID(IID_ISegmentAnalysisResultsGraphViewController,
   0x441ae189,0x18f9,0x436f,0x97,0x8f,0x6f,0x4,0x7c,0x62,0x6a,0xdb);
struct __declspec(uuid("{441AE189-18F9-436F-978F-6F047C626ADB}")) ISegmentAnalysisResultsGraphViewController;

interface ISegmentAnalysisResultsGraphViewController : IEAFViewController
{
   enum GraphMode { Interval, Loading };
   virtual void SetGraphMode(GraphMode mode) = 0;
   virtual GraphMode GetGraphMode() const = 0;

   virtual void SelectSegment(const CSegmentKey& segmentKey) = 0;
   virtual const CSegmentKey GetSegment() const = 0;

   virtual std::vector<ActionType> GetActionTypes() const = 0;
   virtual LPCTSTR GetActionName(ActionType action) const = 0;

   virtual void SetActionType(ActionType actionType) = 0;
   virtual ActionType GetActionType() const = 0;

   virtual void SetResultsType(ResultsType resultsType) = 0;
   virtual ResultsType GetResultsType() const = 0;

   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) = 0;
   virtual pgsTypes::AnalysisType GetAnalysisType() const = 0;

   // These methods are for the drop down list box used to
   // select either the interval that loads are being graphed for
   // of a load to graph over multiple intervals
   virtual IndexType GetGraphTypeCount() const = 0;
   virtual CString GetGraphType(IndexType idx) const = 0;
   virtual void SelectGraphType(IndexType idx) = 0;
   virtual void SelectGraphType(LPCTSTR lpszType) = 0;

   // These methods are for the list box for each individual graph
   // Graphs depend on the current graphing mode
   virtual IndexType GetGraphCount() const = 0;
   virtual IndexType GetSelectedGraphCount() const = 0;
   virtual std::vector<IndexType> GetSelectedGraphs() const = 0;
   virtual CString GetGraphName(IndexType graphIdx) const = 0;
   virtual void SelectGraph(IndexType graphIdx) = 0;
   virtual void SelectGraph(LPCTSTR lpszGraphName) = 0;
   virtual void SelectGraphs(const std::vector<IndexType>& vGraphs) = 0;
   virtual void SelectGraphs(const std::vector<CString>& vGraphs) = 0;

   virtual void SelectStressLocation(pgsTypes::StressLocation location, bool bSelect) = 0;
   virtual bool IsStressLocationSelected(pgsTypes::StressLocation location) const = 0;

   virtual void ShowGrid(bool bShow) = 0;
   virtual bool ShowGrid() const = 0;

   virtual void ShowGirder(bool bShow) = 0;
   virtual bool ShowGirder() const = 0;
};
