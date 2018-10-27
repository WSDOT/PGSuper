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

#ifndef INCLUDED_EDITLIVELOADTXN_H_
#define INCLUDED_EDITLIVELOADTXN_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>

struct txnEditLiveLoadData
{
   std::vector<std::_tstring> m_VehicleNames;
   Float64 m_TruckImpact;
   Float64 m_LaneImpact;
   ILiveLoads::PedestrianLoadApplicationType m_PedestrianLoadApplicationType;
};

class txnEditLiveLoad : public txnTransaction
{
public:
   txnEditLiveLoad(const txnEditLiveLoadData& oldDesign,const txnEditLiveLoadData& newDesign,
                   const txnEditLiveLoadData& oldFatigue,const txnEditLiveLoadData& newFatigue,
                   const txnEditLiveLoadData& oldPermit,const txnEditLiveLoadData& newPermit,
                   EventIndexType oldEventIdx,EventIndexType newEventIdx);

   ~txnEditLiveLoad();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   txnEditLiveLoadData m_Design[2];
   txnEditLiveLoadData m_Fatigue[2];
   txnEditLiveLoadData m_Permit[2];
   EventIndexType m_EventIndex[2];

   void DoExecute(int i);
};

#endif // INCLUDED_EDITLIVELOADTXN_H_