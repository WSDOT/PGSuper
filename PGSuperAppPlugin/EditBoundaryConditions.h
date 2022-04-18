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

#ifndef INCLUDED_EDITBOUNDARYCONDITIONS_H_
#define INCLUDED_EDITBOUNDARYCONDITIONS_H_

#include <System\Transaction.h>
#include <PGSuperTypes.h>
#include <PgsExt\BridgeDescription2.h>

class txnEditBoundaryConditions : public txnTransaction
{
public:
   // pier boundary conditions
   txnEditBoundaryConditions(PierIndexType pierIdx,pgsTypes::BoundaryConditionType oldBC,pgsTypes::BoundaryConditionType newBC);
   txnEditBoundaryConditions(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType oldBC,EventIndexType oldEventIdx,pgsTypes::PierSegmentConnectionType newBC,EventIndexType newEventIdx);

   // temporary support boundary conditions
   txnEditBoundaryConditions(SupportIndexType tsIdx, pgsTypes::TemporarySupportType oldBC, pgsTypes::TemporarySupportType newBC);
   txnEditBoundaryConditions(SupportIndexType tsIdx, pgsTypes::TemporarySupportType oldSupportType, pgsTypes::TempSupportSegmentConnectionType oldConnectionType, EventIndexType oldEventIdx, pgsTypes::TemporarySupportType newSupportType, pgsTypes::TempSupportSegmentConnectionType newConnectionType, EventIndexType newEventIdx);

   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   bool DoExecute(int i);
   bool m_bIsPier; // true if pier, false if temporary support
   bool m_bIsBoundaryPier;
   IndexType m_Index;
   std::array<pgsTypes::BoundaryConditionType, 2> m_BoundaryConditionType;
   std::array<pgsTypes::PierSegmentConnectionType, 2> m_PierSegmentConnectionType;
   std::array<pgsTypes::TemporarySupportType, 2> m_SupportType;
   std::array<pgsTypes::TempSupportSegmentConnectionType, 2> m_TemporarySupportSegmentConnectionType;
   std::array<EventIndexType, 2> m_CastClosureJointEventIdx;
};

#endif // INCLUDED_EDITBOUNDARYCONDITIONS_H_