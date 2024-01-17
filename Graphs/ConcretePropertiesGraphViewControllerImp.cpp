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

#include "ConcretePropertiesGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CConcretePropertiesGraphViewController::CConcretePropertiesGraphViewController()
{
   m_pGraphController = nullptr;
}

CConcretePropertiesGraphViewController::~CConcretePropertiesGraphViewController()
{
}

void CConcretePropertiesGraphViewController::Init(CConcretePropertyGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CConcretePropertiesGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CConcretePropertiesGraphViewController::Close()
{
   m_pStdController->Close();
}

void CConcretePropertiesGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CConcretePropertiesGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CConcretePropertiesGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IConcretePropertiesGraphViewController
void CConcretePropertiesGraphViewController::SetGraphType(IConcretePropertiesGraphViewController::GraphType type)
{
   m_pGraphController->SetGraphType((int)type);
}

IConcretePropertiesGraphViewController::GraphType CConcretePropertiesGraphViewController::GetGraphType() const
{
   return (IConcretePropertiesGraphViewController::GraphType)m_pGraphController->GetGraphType();
}

void CConcretePropertiesGraphViewController::SetElementType(IConcretePropertiesGraphViewController::ElementType type)
{
   m_pGraphController->SetGraphElement((int)type);
}

IConcretePropertiesGraphViewController::ElementType CConcretePropertiesGraphViewController::GetElementType() const
{
   return (IConcretePropertiesGraphViewController::ElementType)m_pGraphController->GetGraphElement();
}

void CConcretePropertiesGraphViewController::SetXAxisType(IConcretePropertiesGraphViewController::XAxisType type)
{
   m_pGraphController->SetXAxisType((int)type);
}

IConcretePropertiesGraphViewController::XAxisType CConcretePropertiesGraphViewController::GetXAxisType() const
{
   return (IConcretePropertiesGraphViewController::XAxisType)m_pGraphController->GetXAxisType();
}

void CConcretePropertiesGraphViewController::SetSegment(const CSegmentKey& segmentKey)
{
   m_pGraphController->SetSegment(segmentKey);
}

const CSegmentKey& CConcretePropertiesGraphViewController::GetSegment() const
{
   return m_pGraphController->GetSegment();
}

void CConcretePropertiesGraphViewController::SetClosureJoint(const CClosureKey& closureKey)
{
   m_pGraphController->SetClosureJoint(closureKey);
}

const CClosureKey& CConcretePropertiesGraphViewController::GetClosureJoint() const
{
   return m_pGraphController->GetClosureJoint();
}

void CConcretePropertiesGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CConcretePropertiesGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}
