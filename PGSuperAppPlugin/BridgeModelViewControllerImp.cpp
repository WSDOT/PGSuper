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

#include "BridgeModelViewControllerImp.h"
#include "BridgePlanView.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CBridgeModelViewController::CBridgeModelViewController()
{
   m_pFrame = nullptr;
}

CBridgeModelViewController::~CBridgeModelViewController()
{
}

void CBridgeModelViewController::Init(CBridgeModelViewChildFrame* pFrame, IEAFViewController* pStandardController)
{
   m_pFrame = pFrame;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CBridgeModelViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CBridgeModelViewController::Close()
{
   m_pStdController->Close();
}

void CBridgeModelViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CBridgeModelViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CBridgeModelViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// IBridgeModelViewController
void CBridgeModelViewController::GetGroupRange(GroupIndexType* pStartGroupIdx,GroupIndexType* pEndGroupIdx) const
{
   if (IsOpen())
   {
      CBridgePlanView* pPlanView = m_pFrame->GetBridgePlanView();
      pPlanView->GetGroupRange(pStartGroupIdx, pEndGroupIdx);
   }
}

void CBridgeModelViewController::SetGroupRange(GroupIndexType startGroupIdx, GroupIndexType endGroupIdx)
{
   if (IsOpen())
   {
      CBridgePlanView* pPlanView = m_pFrame->GetBridgePlanView();
      pPlanView->SetGroupRange(startGroupIdx, endGroupIdx, false);
   }
}

Float64 CBridgeModelViewController::GetCutStation() const
{
   if (IsOpen())
   {
      return m_pFrame->GetCurrentCutLocation();
   }
   else
   {
      return -1;
   }
}

void CBridgeModelViewController::SetCutStation(Float64 station)
{
   if (IsOpen())
   {
      m_pFrame->CutAt(station);
   }
}

void CBridgeModelViewController::SetViewMode(IBridgeModelViewController::ViewMode mode)
{
   if (IsOpen())
   {
      m_pFrame->SetViewMode((CBridgeModelViewChildFrame::ViewMode)mode);
   }
}

IBridgeModelViewController::ViewMode CBridgeModelViewController::GetViewMode() const
{
   if (IsOpen())
   {
      return (IBridgeModelViewController::ViewMode)m_pFrame->GetViewMode();
   }
   else
   {
      return IBridgeModelViewController::Bridge;
   }
}

void CBridgeModelViewController::NorthUp(bool bNorthUp)
{
   if (IsOpen())
   {
      m_pFrame->NorthUp(bNorthUp);
   }
}

bool CBridgeModelViewController::NorthUp() const
{
   if (IsOpen())
   {
      return m_pFrame->NorthUp();
   }
   else
   {
      return true;
   }
}

void CBridgeModelViewController::ShowLabels(bool bShowLabels)
{
   if (IsOpen())
   {
      m_pFrame->ShowLabels(bShowLabels);
   }
}

bool CBridgeModelViewController::ShowLabels() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowLabels();
   }
   else
   {
      return true;
   }
}

void CBridgeModelViewController::ShowDimensions(bool bShowDimensions)
{
   if (IsOpen())
   {
      m_pFrame->ShowDimensions(bShowDimensions);
   }
}

bool CBridgeModelViewController::ShowDimensions() const
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

void CBridgeModelViewController::ShowBridge(bool bShowBridge)
{
   if (IsOpen())
   {
      m_pFrame->ShowBridge(bShowBridge);
   }
}

bool CBridgeModelViewController::ShowBridge() const
{
   if (IsOpen())
   {
      return m_pFrame->ShowBridge();
   }
   else
   {
      return true;
   }
}

void CBridgeModelViewController::Schematic(bool bSchematic)
{
   if (IsOpen())
   {
      m_pFrame->Schematic(bSchematic);
   }
}

bool CBridgeModelViewController::Schematic() const
{
   if (IsOpen())
   {
      return m_pFrame->Schematic();
   }
   else
   {
      return false;
   }
}
