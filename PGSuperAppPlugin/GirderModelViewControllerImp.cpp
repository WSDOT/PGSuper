///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include "GirderModelViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CGirderModelViewController::CGirderModelViewController()
{
   m_pFrame = nullptr;
}

CGirderModelViewController::~CGirderModelViewController()
{
}

void CGirderModelViewController::Init(CGirderModelChildFrame* pFrame, IEAFViewController* pStandardController)
{
   m_pFrame = pFrame;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CGirderModelViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CGirderModelViewController::Close()
{
   m_pStdController->Close();
}

void CGirderModelViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CGirderModelViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CGirderModelViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IGirderModelViewController
void CGirderModelViewController::SelectGirder(const CGirderKey& girderKey)
{
   if (IsOpen())
   {
      m_pFrame->SelectGirder(girderKey, true/*update*/);
   }
}

const CGirderKey& CGirderModelViewController::GetGirder() const
{
   return m_pFrame->GetSelection();
}

bool CGirderModelViewController::SyncWithBridgeModelView() const
{
   if (IsOpen())
   {
      return m_pFrame->SyncWithBridgeModelView();
   }
   else
   {
      return false;
   }
}

void CGirderModelViewController::SyncWithBridgeModelView(bool bSync)
{
   if (IsOpen())
   {
      m_pFrame->SyncWithBridgeModelView(bSync);
   }
}

EventIndexType CGirderModelViewController::GetEvent() const
{
   if (IsOpen())
   {
      return m_pFrame->GetEvent();
   }
   else
   {
      return INVALID_INDEX;
   }
}

bool CGirderModelViewController::SetEvent(EventIndexType eventIdx)
{
   if (IsOpen())
   {
      return m_pFrame->SetEvent(eventIdx);
   }
   else
   {
      return false;
   }
}

Float64 CGirderModelViewController::GetCutLocation() const
{
   if (IsOpen())
   {
      return m_pFrame->GetCurrentCutLocation();
   }
   else
   {
      return 0;
   }
}

void CGirderModelViewController::CutAt(Float64 Xg)
{
   if (IsOpen())
   {
      m_pFrame->CutAt(Xg);
   }
}

void CGirderModelViewController::CutAtNext()
{
   if (IsOpen())
   {
      m_pFrame->CutAtNext();
   }
}

void CGirderModelViewController::CutAtPrev()
{
   if (IsOpen())
   {
      m_pFrame->CutAtPrev();
   }
}

void CGirderModelViewController::GetCutRange(Float64* pMin, Float64* pMax) const
{
   if (IsOpen())
   {
      m_pFrame->GetCutRange(pMin, pMax);
   }
}

void CGirderModelViewController::ShowStrands(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowStrands(bShow);
   }
}

bool CGirderModelViewController::ShowStrands() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowStrands();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowStrandCG(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowStrandCG(bShow);
   }
}

bool CGirderModelViewController::ShowStrandCG() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowStrandCG();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowCG(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowCG(bShow);
   }
}

bool CGirderModelViewController::ShowCG() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowCG();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowSectionProperties(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowSectionProperties(bShow);
   }
}

bool CGirderModelViewController::ShowSectionProperties() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowSectionProperties();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowDimensions(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowDimensions(bShow);
   }
}

bool CGirderModelViewController::ShowDimensions() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowDimensions();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowLongitudinalReinforcement(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowLongitudinalReinforcement(bShow);
   }
}

bool CGirderModelViewController::ShowLongitudinalReinforcement() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowLongitudinalReinforcement();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowTransverseReinforcement(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowTransverseReinforcement(bShow);
   }
}

bool CGirderModelViewController::ShowTransverseReinforcement() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowTransverseReinforcement();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::ShowLoads(bool bShow)
{
   if (IsOpen())
   {
      m_pFrame->ShowLoads(bShow);
   }
}

bool CGirderModelViewController::ShowLoads() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowLoads();
   }
   else
   {
      return true;
   }
}

void CGirderModelViewController::Schematic(bool bSchematic)
{
   if (IsOpen())
   {
      m_pFrame->Schematic(bSchematic);
   }
}

bool CGirderModelViewController::Schematic() const
{
   if (IsOpen())
   {
      return m_pFrame->Schematic();
   }
   else
   {
      return true;
   }
}
