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

#include "stdafx.h"

#include "AnalysisResultsGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CAnalysisResultsGraphViewController::CAnalysisResultsGraphViewController()
{
   m_pGraphController = nullptr;
}

CAnalysisResultsGraphViewController::~CAnalysisResultsGraphViewController()
{
}

void CAnalysisResultsGraphViewController::Init(CAnalysisResultsGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CAnalysisResultsGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CAnalysisResultsGraphViewController::Close()
{
   m_pStdController->Close();
}

void CAnalysisResultsGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CAnalysisResultsGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CAnalysisResultsGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IAnalysisResultsGraphViewController
void CAnalysisResultsGraphViewController::SetGraphMode(IAnalysisResultsGraphViewController::GraphMode mode)
{
   m_pGraphController->SetGraphMode((CAnalysisResultsGraphController::GraphModeType)mode);
}

IAnalysisResultsGraphViewController::GraphMode CAnalysisResultsGraphViewController::GetGraphMode() const
{
   return (IAnalysisResultsGraphViewController::GraphMode)(m_pGraphController->GetGraphMode());
}

void CAnalysisResultsGraphViewController::SelectGirder(const CGirderKey& girderKey)
{
   m_pGraphController->SelectGirder(girderKey);
}

const CGirderKey& CAnalysisResultsGraphViewController::GetGirder() const
{
   return m_pGraphController->GetGirderKey();
}

void CAnalysisResultsGraphViewController::SetResultsType(ResultsType resultsType)
{
   m_pGraphController->SetResultsType(resultsType);
}

ResultsType CAnalysisResultsGraphViewController::GetResultsType() const
{
   return m_pGraphController->GetResultsType();
}

std::vector<ActionType> CAnalysisResultsGraphViewController::GetActionTypes() const
{
   return m_pGraphController->GetActionTypes();
}

LPCTSTR CAnalysisResultsGraphViewController::GetActionName(ActionType action) const
{
   return m_pGraphController->GetActionName(action);
}

void CAnalysisResultsGraphViewController::SetActionType(ActionType actionType)
{
   m_pGraphController->SetActionType(actionType);
}

ActionType CAnalysisResultsGraphViewController::GetActionType() const
{
   return m_pGraphController->GetActionType();
}

void CAnalysisResultsGraphViewController::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   m_pGraphController->SetAnalysisType(analysisType);
}

pgsTypes::AnalysisType CAnalysisResultsGraphViewController::GetAnalysisType() const
{
   return m_pGraphController->GetAnalysisType();
}

IndexType CAnalysisResultsGraphViewController::GetGraphTypeCount() const
{
   return m_pGraphController->GetGraphTypeCount();
}

CString CAnalysisResultsGraphViewController::GetGraphType(IndexType idx) const
{
   return m_pGraphController->GetGraphType(idx);
}

void CAnalysisResultsGraphViewController::SelectGraphType(IndexType idx)
{
   m_pGraphController->SelectGraphType(idx);
}

void CAnalysisResultsGraphViewController::SelectGraphType(LPCTSTR lpszType)
{
   m_pGraphController->SelectGraphType(lpszType);
}

IndexType CAnalysisResultsGraphViewController::GetGraphCount() const
{
   return m_pGraphController->GetGraphCount();
}

IndexType CAnalysisResultsGraphViewController::GetSelectedGraphCount() const
{
   return m_pGraphController->GetSelectedGraphCount();
}

std::vector<IndexType> CAnalysisResultsGraphViewController::GetSelectedGraphs() const
{
   return m_pGraphController->GetSelectedGraphs();
}

CString CAnalysisResultsGraphViewController::GetGraphName(IndexType graphIdx) const
{
   return m_pGraphController->GetGraphName(graphIdx);
}

void CAnalysisResultsGraphViewController::SelectGraph(IndexType graphIdx)
{
   m_pGraphController->SelectGraph(graphIdx);
}

void CAnalysisResultsGraphViewController::SelectGraph(LPCTSTR lpszGraphName)
{
   m_pGraphController->SelectGraph(lpszGraphName);
}

void CAnalysisResultsGraphViewController::SelectGraphs(const std::vector<IndexType>& vGraphs)
{
   m_pGraphController->SelectGraphs(vGraphs);
}

void CAnalysisResultsGraphViewController::SelectGraphs(const std::vector<CString>& vGraphs)
{
   m_pGraphController->SelectGraphs(vGraphs);
}

void CAnalysisResultsGraphViewController::SelectStressLocation(pgsTypes::StressLocation location, bool bSelect)
{
   m_pGraphController->PlotStresses(location,bSelect);
}

bool CAnalysisResultsGraphViewController::IsStressLocationSelected(pgsTypes::StressLocation location) const
{
   return m_pGraphController->PlotStresses(location);
}

void CAnalysisResultsGraphViewController::IncludeElevationAdjustment(bool bInclude)
{
   m_pGraphController->IncludeElevationAdjustment(bInclude);
}

bool CAnalysisResultsGraphViewController::IncludeElevationAdjustment() const
{
   return m_pGraphController->IncludeElevationAdjustment();
}

void CAnalysisResultsGraphViewController::IncludePrecamber(bool bInclude)
{
   m_pGraphController->IncludePrecamber(bInclude);
}

bool CAnalysisResultsGraphViewController::IncludePrecamber() const
{
   return m_pGraphController->IncludePrecamber();
}

void CAnalysisResultsGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CAnalysisResultsGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}

void CAnalysisResultsGraphViewController::ShowGirder(bool bShow)
{
   m_pGraphController->ShowBeam(bShow);
}

bool CAnalysisResultsGraphViewController::ShowGirder() const
{
   return m_pGraphController->ShowBeam();
}
