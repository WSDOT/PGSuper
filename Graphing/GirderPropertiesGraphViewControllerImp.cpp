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

#include "GirderPropertiesGraphViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CGirderPropertiesGraphViewController::CGirderPropertiesGraphViewController()
{
   m_pGraphController = nullptr;
}

CGirderPropertiesGraphViewController::~CGirderPropertiesGraphViewController()
{
}

void CGirderPropertiesGraphViewController::Init(CGirderPropertiesGraphController* pGraphController, IEAFViewController* pStandardController)
{
   m_pGraphController = pGraphController;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CGirderPropertiesGraphViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CGirderPropertiesGraphViewController::Close()
{
   m_pStdController->Close();
}

void CGirderPropertiesGraphViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CGirderPropertiesGraphViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CGirderPropertiesGraphViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IGirderPropertiesGraphViewController
bool CGirderPropertiesGraphViewController::SetPropertyType(CGirderPropertiesGraphBuilder::PropertyType propertyType)
{
   return m_pGraphController->SetPropertyType(propertyType);
}

CGirderPropertiesGraphBuilder::PropertyType CGirderPropertiesGraphViewController::GetPropertyType() const
{
   return m_pGraphController->GetPropertyType();
}

bool CGirderPropertiesGraphViewController::IsInvariantProperty(CGirderPropertiesGraphBuilder::PropertyType propertyType) const
{
   return m_pGraphController->IsInvariantProperty(propertyType);
}

bool CGirderPropertiesGraphViewController::SetSectionPropertyType(pgsTypes::SectionPropertyType type)
{
   return m_pGraphController->SetSectionPropertyType(type);
}

pgsTypes::SectionPropertyType CGirderPropertiesGraphViewController::GetSectionPropertyType() const
{
   return m_pGraphController->GetSectionPropertyType();
}

void CGirderPropertiesGraphViewController::SelectGirder(const CGirderKey& girderKey)
{
   m_pGraphController->SelectGirder(girderKey);
}

const CGirderKey& CGirderPropertiesGraphViewController::GetGirder() const
{
   return m_pGraphController->GetGirderKey();
}

void CGirderPropertiesGraphViewController::SetInterval(IntervalIndexType intervalIdx)
{
   m_pGraphController->SetInterval(intervalIdx);
}

IntervalIndexType CGirderPropertiesGraphViewController::GetInterval() const
{
   return m_pGraphController->GetInterval();
}

IntervalIndexType CGirderPropertiesGraphViewController::GetFirstInterval() const
{
   return m_pGraphController->GetFirstInterval();
}

IntervalIndexType CGirderPropertiesGraphViewController::GetLastInterval() const
{
   return m_pGraphController->GetLastInterval();
}

void CGirderPropertiesGraphViewController::ShowGrid(bool bShow)
{
   m_pGraphController->ShowGrid(bShow);
}

bool CGirderPropertiesGraphViewController::ShowGrid() const
{
   return m_pGraphController->ShowGrid();
}

void CGirderPropertiesGraphViewController::ShowGirder(bool bShow)
{
   m_pGraphController->ShowBeam(bShow);
}

bool CGirderPropertiesGraphViewController::ShowGirder() const
{
   return m_pGraphController->ShowBeam();
}
