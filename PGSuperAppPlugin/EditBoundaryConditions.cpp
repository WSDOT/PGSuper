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
#include "EditBoundaryConditions.h"
#include "PGSuperApp.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditBoundaryConditions::txnEditBoundaryConditions(PierIndexType pierIdx,pgsTypes::BoundaryConditionType oldBC,pgsTypes::BoundaryConditionType newBC)
{
   m_bIsPier = true;
   m_bIsBoundaryPier = true;
   m_Index = pierIdx;
   m_BoundaryConditionType[0] = oldBC;
   m_BoundaryConditionType[1] = newBC;
}

txnEditBoundaryConditions::txnEditBoundaryConditions(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType oldBC,EventIndexType oldEventIdx,pgsTypes::PierSegmentConnectionType newBC,EventIndexType newEventIdx)
{
   m_bIsPier = true;
   m_bIsBoundaryPier = false;
   m_Index = pierIdx;
   m_PierSegmentConnectionType[0] = oldBC;
   m_PierSegmentConnectionType[1] = newBC;
   m_CastClosureJointEventIdx[0] = oldEventIdx;
   m_CastClosureJointEventIdx[1] = newEventIdx;
}

txnEditBoundaryConditions::txnEditBoundaryConditions(SupportIndexType tsIdx, pgsTypes::TemporarySupportType oldBC, pgsTypes::TemporarySupportType newBC)
{
   m_bIsPier = false;
   m_bIsBoundaryPier = true;
   m_Index = tsIdx;
   m_SupportType[0] = oldBC;
   m_SupportType[1] = newBC;
}

txnEditBoundaryConditions::txnEditBoundaryConditions(SupportIndexType tsIdx, pgsTypes::TemporarySupportType oldSupportType, pgsTypes::TempSupportSegmentConnectionType oldConnectionType, EventIndexType oldEventIdx, pgsTypes::TemporarySupportType newSupportType, pgsTypes::TempSupportSegmentConnectionType newConnectionType, EventIndexType newEventIdx)
{
   m_bIsPier = false;
   m_bIsBoundaryPier = false;
   m_Index = tsIdx;
   m_SupportType[0] = oldSupportType;
   m_SupportType[1] = newSupportType;
   m_TemporarySupportSegmentConnectionType[0] = oldConnectionType;
   m_TemporarySupportSegmentConnectionType[1] = newConnectionType;
   m_CastClosureJointEventIdx[0] = oldEventIdx;
   m_CastClosureJointEventIdx[1] = newEventIdx;
}

std::_tstring txnEditBoundaryConditions::Name() const
{
   return _T("Change Boundary Conditions");
}

std::unique_ptr<CEAFTransaction> txnEditBoundaryConditions::CreateClone() const
{
   if (m_bIsPier)
   {
      if (m_bIsBoundaryPier)
         return std::make_unique<txnEditBoundaryConditions>(m_Index, m_BoundaryConditionType[0], m_BoundaryConditionType[1]);
      else
         return std::make_unique<txnEditBoundaryConditions>(m_Index, m_PierSegmentConnectionType[0], m_CastClosureJointEventIdx[0], m_PierSegmentConnectionType[1], m_CastClosureJointEventIdx[1]);
   }
   else
   {
      if (m_bIsBoundaryPier)
         return std::make_unique<txnEditBoundaryConditions>(m_Index, m_SupportType[0], m_SupportType[1]);
      else
         return std::make_unique<txnEditBoundaryConditions>(m_Index, m_SupportType[0], m_TemporarySupportSegmentConnectionType[0], m_CastClosureJointEventIdx[0], m_SupportType[1], m_TemporarySupportSegmentConnectionType[1], m_CastClosureJointEventIdx[1]);
   }
}

bool txnEditBoundaryConditions::IsUndoable() const
{
   return true;
}

bool txnEditBoundaryConditions::IsRepeatable() const
{
   return false;
}

bool txnEditBoundaryConditions::Execute()
{
   return DoExecute(1);
}

void txnEditBoundaryConditions::Undo()
{
   DoExecute(0);
}

bool txnEditBoundaryConditions::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   if (m_bIsPier)
   {
      if (m_bIsBoundaryPier)
      {
         ATLASSERT(pBridgeDesc->GetPier(m_Index)->IsBoundaryPier());
         pBridgeDesc->SetBoundaryCondition(m_Index, m_BoundaryConditionType[i]);
      }
      else
      {
         ATLASSERT(pBridgeDesc->GetPier(m_Index)->IsInteriorPier());
         pBridgeDesc->SetBoundaryCondition(m_Index, m_PierSegmentConnectionType[i], m_CastClosureJointEventIdx[i]);
      }
   }
   else
   {
      if (m_bIsBoundaryPier)
      {
         pBridgeDesc->SetBoundaryCondition(m_Index, m_SupportType[i]);
      }
      else
      {
         pBridgeDesc->SetBoundaryCondition(m_Index, m_SupportType[i]);
         pBridgeDesc->SetBoundaryCondition(m_Index, m_TemporarySupportSegmentConnectionType[i], m_CastClosureJointEventIdx[i]);
      }
   }

   return true;
}
