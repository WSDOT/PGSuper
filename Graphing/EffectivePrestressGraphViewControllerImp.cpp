///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "EffectivePrestressGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CEffectivePrestressGraphViewController::CEffectivePrestressGraphViewController()
{
   m_pGraphController = nullptr;
}

CEffectivePrestressGraphViewController::~CEffectivePrestressGraphViewController()
{
}

void CEffectivePrestressGraphViewController::Init(CEffectivePrestressGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CEffectivePrestressGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CEffectivePrestressGraphViewController::Close()
{
   m_pStdController->Close();
}

void CEffectivePrestressGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CEffectivePrestressGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CEffectivePrestressGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IEffectivePrestressGraphViewController
void CEffectivePrestressGraphViewController::GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const
{
   *pMin = m_pGraphController->GetFirstInterval();
   *pMax = m_pGraphController->GetLastInterval();
}

void CEffectivePrestressGraphViewController::SelectInterval(IntervalIndexType intervalIdx)
{
   m_pGraphController->SelectInterval(intervalIdx);
}

void CEffectivePrestressGraphViewController::SelectIntervals(const std::vector<IntervalIndexType>& vIntervals)
{
   m_pGraphController->SelectIntervals(vIntervals);
}

std::vector<IntervalIndexType> CEffectivePrestressGraphViewController::GetSelectedIntervals() const
{
   return m_pGraphController->GetSelectedIntervals();
}

void CEffectivePrestressGraphViewController::SelectGirder(const CGirderKey& girderKey)
{
   m_pGraphController->SelectGirder(girderKey);
}

const CGirderKey& CEffectivePrestressGraphViewController::GetGirder() const
{
   return m_pGraphController->GetGirderKey();
}

void CEffectivePrestressGraphViewController::SetViewMode(IEffectivePrestressGraphViewController::ViewMode mode)
{
   m_pGraphController->SetViewMode((CEffectivePrestressGraphController::ViewMode)mode);
}

IEffectivePrestressGraphViewController::ViewMode CEffectivePrestressGraphViewController::GetViewMode() const
{
   return (IEffectivePrestressGraphViewController::ViewMode)m_pGraphController->GetViewMode();
}

void CEffectivePrestressGraphViewController::SetStrandType(IEffectivePrestressGraphViewController::StrandType strandType)
{
   m_pGraphController->SetStrandType((CEffectivePrestressGraphController::StrandType)strandType);
}

IEffectivePrestressGraphViewController::StrandType CEffectivePrestressGraphViewController::GetStrandType() const
{
   return (IEffectivePrestressGraphViewController::StrandType)m_pGraphController->GetStrandType();
}

void CEffectivePrestressGraphViewController::SetDuct(DuctIndexType ductIdx)
{
   m_pGraphController->SetDuct(ductIdx);
}

DuctIndexType CEffectivePrestressGraphViewController::GetDuct() const
{
   return m_pGraphController->GetDuct();
}

void CEffectivePrestressGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CEffectivePrestressGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}

void CEffectivePrestressGraphViewController::ShowGirder(bool bShow)
{
   m_pGraphController->ShowBeam(bShow);
}

bool CEffectivePrestressGraphViewController::ShowGirder() const
{
   return m_pGraphController->ShowBeam();
}
