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

#include "DeflectionHistoryGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CDeflectionHistoryGraphViewController::CDeflectionHistoryGraphViewController()
{
   m_pGraphController = nullptr;
}

CDeflectionHistoryGraphViewController::~CDeflectionHistoryGraphViewController()
{
}

void CDeflectionHistoryGraphViewController::Init(CDeflectionHistoryGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CDeflectionHistoryGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CDeflectionHistoryGraphViewController::Close()
{
   m_pStdController->Close();
}

void CDeflectionHistoryGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CDeflectionHistoryGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CDeflectionHistoryGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IDeflectionHistoryGraphViewController
void CDeflectionHistoryGraphViewController::SelectLocation(const pgsPointOfInterest& poi)
{
   m_pGraphController->SelectLocation(poi);
}

const pgsPointOfInterest& CDeflectionHistoryGraphViewController::GetLocation() const
{
   return m_pGraphController->GetLocation();
}

void CDeflectionHistoryGraphViewController::SetXAxisType(IDeflectionHistoryGraphViewController::XAxisType type)
{
   m_pGraphController->SetXAxisType((int)type);
}

IDeflectionHistoryGraphViewController::XAxisType CDeflectionHistoryGraphViewController::GetXAxisType() const
{
   return (IDeflectionHistoryGraphViewController::XAxisType)m_pGraphController->GetXAxisType();
}

void CDeflectionHistoryGraphViewController::IncludeElevationAdjustment(bool bAdjust)
{
   m_pGraphController->IncludeElevationAdjustment(bAdjust);
}

bool CDeflectionHistoryGraphViewController::IncludeElevationAdjustment() const
{
   return m_pGraphController->IncludeElevationAdjustment();
}

void CDeflectionHistoryGraphViewController::IncludeUnrecoverableDefl(bool bInclude)
{
   m_pGraphController->IncludeUnrecoverableDefl(bInclude);
}

bool CDeflectionHistoryGraphViewController::IncludeUnrecoverableDefl() const
{
   return m_pGraphController->IncludeUnrecoverableDefl();
}

void CDeflectionHistoryGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CDeflectionHistoryGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}