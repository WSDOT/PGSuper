///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "FinishedElevationGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CFinishedElevationGraphViewController::CFinishedElevationGraphViewController()
{
   m_pGraphController = nullptr;
}

CFinishedElevationGraphViewController::~CFinishedElevationGraphViewController()
{
}

void CFinishedElevationGraphViewController::Init(CFinishedElevationGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CFinishedElevationGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CFinishedElevationGraphViewController::Close()
{
   m_pStdController->Close();
}

void CFinishedElevationGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CFinishedElevationGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CFinishedElevationGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IFinishedElevationGraphViewController
void CFinishedElevationGraphViewController::GetIntervalRange(IntervalIndexType* pMin, IntervalIndexType* pMax) const
{
   *pMin = m_pGraphController->GetFirstInterval();
   *pMax = m_pGraphController->GetLastInterval();
}

void CFinishedElevationGraphViewController::SelectInterval(IntervalIndexType intervalIdx)
{
   m_pGraphController->SelectInterval(intervalIdx);
}

void CFinishedElevationGraphViewController::SelectIntervals(const std::vector<IntervalIndexType>& vIntervals)
{
   m_pGraphController->SelectIntervals(vIntervals);
}

std::vector<IntervalIndexType> CFinishedElevationGraphViewController::GetSelectedIntervals() const
{
   return m_pGraphController->GetSelectedIntervals();
}

void CFinishedElevationGraphViewController::SelectGirder(const CGirderKey& girderKey)
{
   m_pGraphController->SelectGirder(girderKey);
}

const CGirderKey& CFinishedElevationGraphViewController::GetGirder() const
{
   return m_pGraphController->GetGirderKey();
}

void CFinishedElevationGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CFinishedElevationGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}

void CFinishedElevationGraphViewController::ShowGirder(bool bShow)
{
   m_pGraphController->ShowBeam(bShow);
}

bool CFinishedElevationGraphViewController::ShowGirder() const
{
   return m_pGraphController->ShowBeam();
}
