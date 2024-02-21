///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include "stdafx.h"

#include "SegmentAnalysisResultsGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CSegmentAnalysisResultsGraphViewController::CSegmentAnalysisResultsGraphViewController()
{
   m_pGraphController = nullptr;
}

CSegmentAnalysisResultsGraphViewController::~CSegmentAnalysisResultsGraphViewController()
{
}

void CSegmentAnalysisResultsGraphViewController::Init(CSegmentAnalysisResultsGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CSegmentAnalysisResultsGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CSegmentAnalysisResultsGraphViewController::Close()
{
   m_pStdController->Close();
}

void CSegmentAnalysisResultsGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CSegmentAnalysisResultsGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CSegmentAnalysisResultsGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// ISegmentAnalysisResultsGraphViewController
void CSegmentAnalysisResultsGraphViewController::SetGraphMode(ISegmentAnalysisResultsGraphViewController::GraphMode mode)
{
   m_pGraphController->SetGraphMode((CSegmentAnalysisResultsGraphController::GraphModeType)mode);
}

ISegmentAnalysisResultsGraphViewController::GraphMode CSegmentAnalysisResultsGraphViewController::GetGraphMode() const
{
   return (ISegmentAnalysisResultsGraphViewController::GraphMode)(m_pGraphController->GetGraphMode());
}

void CSegmentAnalysisResultsGraphViewController::SelectSegment(const CSegmentKey& segmentKey)
{
   m_pGraphController->SelectSegment(segmentKey);
}

const CSegmentKey CSegmentAnalysisResultsGraphViewController::GetSegment() const
{
   return m_pGraphController->GetSegmentKey();
}

void CSegmentAnalysisResultsGraphViewController::SetResultsType(ResultsType resultsType)
{
   m_pGraphController->SetResultsType(resultsType);
}

ResultsType CSegmentAnalysisResultsGraphViewController::GetResultsType() const
{
   return m_pGraphController->GetResultsType();
}

std::vector<ActionType> CSegmentAnalysisResultsGraphViewController::GetActionTypes() const
{
   return m_pGraphController->GetActionTypes();
}

LPCTSTR CSegmentAnalysisResultsGraphViewController::GetActionName(ActionType action) const
{
   return m_pGraphController->GetActionName(action);
}

void CSegmentAnalysisResultsGraphViewController::SetActionType(ActionType actionType)
{
   m_pGraphController->SetActionType(actionType);
}

ActionType CSegmentAnalysisResultsGraphViewController::GetActionType() const
{
   return m_pGraphController->GetActionType();
}

void CSegmentAnalysisResultsGraphViewController::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   m_pGraphController->SetAnalysisType(analysisType);
}

pgsTypes::AnalysisType CSegmentAnalysisResultsGraphViewController::GetAnalysisType() const
{
   return m_pGraphController->GetAnalysisType();
}

IndexType CSegmentAnalysisResultsGraphViewController::GetGraphTypeCount() const
{
   return m_pGraphController->GetGraphTypeCount();
}

CString CSegmentAnalysisResultsGraphViewController::GetGraphType(IndexType idx) const
{
   return m_pGraphController->GetGraphType(idx);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraphType(IndexType idx)
{
   m_pGraphController->SelectGraphType(idx);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraphType(LPCTSTR lpszType)
{
   m_pGraphController->SelectGraphType(lpszType);
}

IndexType CSegmentAnalysisResultsGraphViewController::GetGraphCount() const
{
   return m_pGraphController->GetGraphCount();
}

IndexType CSegmentAnalysisResultsGraphViewController::GetSelectedGraphCount() const
{
   return m_pGraphController->GetSelectedGraphCount();
}

std::vector<IndexType> CSegmentAnalysisResultsGraphViewController::GetSelectedGraphs() const
{
   return m_pGraphController->GetSelectedGraphs();
}

CString CSegmentAnalysisResultsGraphViewController::GetGraphName(IndexType graphIdx) const
{
   return m_pGraphController->GetGraphName(graphIdx);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraph(IndexType graphIdx)
{
   m_pGraphController->SelectGraph(graphIdx);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraph(LPCTSTR lpszGraphName)
{
   m_pGraphController->SelectGraph(lpszGraphName);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraphs(const std::vector<IndexType>& vGraphs)
{
   m_pGraphController->SelectGraphs(vGraphs);
}

void CSegmentAnalysisResultsGraphViewController::SelectGraphs(const std::vector<CString>& vGraphs)
{
   m_pGraphController->SelectGraphs(vGraphs);
}

void CSegmentAnalysisResultsGraphViewController::SelectStressLocation(pgsTypes::StressLocation location, bool bSelect)
{
   m_pGraphController->PlotStresses(location,bSelect);
}

bool CSegmentAnalysisResultsGraphViewController::IsStressLocationSelected(pgsTypes::StressLocation location) const
{
   return m_pGraphController->PlotStresses(location);
}

void CSegmentAnalysisResultsGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CSegmentAnalysisResultsGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}

void CSegmentAnalysisResultsGraphViewController::ShowGirder(bool bShow)
{
   m_pGraphController->ShowBeam(bShow);
}

bool CSegmentAnalysisResultsGraphViewController::ShowGirder() const
{
   return m_pGraphController->ShowBeam();
}
