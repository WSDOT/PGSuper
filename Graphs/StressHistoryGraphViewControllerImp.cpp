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

#include "stdafx.h"

#include "StressHistoryGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CStressHistoryGraphViewController::CStressHistoryGraphViewController()
{
   m_pGraphController = nullptr;
}

CStressHistoryGraphViewController::~CStressHistoryGraphViewController()
{
}

void CStressHistoryGraphViewController::Init(CStressHistoryGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CStressHistoryGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CStressHistoryGraphViewController::Close()
{
   m_pStdController->Close();
}

void CStressHistoryGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CStressHistoryGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CStressHistoryGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IStressHistoryGraphViewController
void CStressHistoryGraphViewController::SelectLocation(const pgsPointOfInterest& poi)
{
   m_pGraphController->SelectLocation(poi);
}

const pgsPointOfInterest& CStressHistoryGraphViewController::GetLocation() const
{
   return m_pGraphController->GetLocation();
}

void CStressHistoryGraphViewController::SetXAxisType(IStressHistoryGraphViewController::XAxisType type)
{
   m_pGraphController->SetXAxisType((int)type);
}

IStressHistoryGraphViewController::XAxisType CStressHistoryGraphViewController::GetXAxisType() const
{
   return (IStressHistoryGraphViewController::XAxisType)m_pGraphController->GetXAxisType();
}

void CStressHistoryGraphViewController::Stresses(pgsTypes::StressLocation stressLocation, bool bEnable)
{
   m_pGraphController->Stresses(stressLocation, bEnable);
}

bool CStressHistoryGraphViewController::Stresses(pgsTypes::StressLocation stressLocation) const
{
   return m_pGraphController->Stresses(stressLocation);
}

void CStressHistoryGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CStressHistoryGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}