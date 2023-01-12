///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "StabilityGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CStabilityGraphViewController::CStabilityGraphViewController()
{
   m_pGraphController = nullptr;
}

CStabilityGraphViewController::~CStabilityGraphViewController()
{
}

void CStabilityGraphViewController::Init(CStabilityGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CStabilityGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CStabilityGraphViewController::Close()
{
   m_pStdController->Close();
}

void CStabilityGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CStabilityGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CStabilityGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IStabilityGraphViewController
void CStabilityGraphViewController::SelectSegment(const CSegmentKey& segmentKey)
{
   m_pGraphController->SelectSegment(segmentKey);
}

const CSegmentKey& CStabilityGraphViewController::GetSegment() const
{
   return m_pGraphController->GetSegment();
}

void CStabilityGraphViewController::SetViewMode(IStabilityGraphViewController::ViewMode mode)
{
   m_pGraphController->SetGraphType(mode == IStabilityGraphViewController::Lifting ? GT_LIFTING : GT_HAULING);
}

IStabilityGraphViewController::ViewMode CStabilityGraphViewController::GetViewMode() const
{
   return m_pGraphController->GetGraphType() == GT_LIFTING ? IStabilityGraphViewController::Lifting : IStabilityGraphViewController::Hauling;
}

void CStabilityGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CStabilityGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}