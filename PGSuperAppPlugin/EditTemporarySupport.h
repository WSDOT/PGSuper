///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#pragma once

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <IFace\Project.h>

struct txnEditTemporarySupportData
{
   CTemporarySupportData m_TS;
   EventIndexType m_ErectionEvent;
   EventIndexType m_RemovalEvent;
   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   pgsTypes::MeasurementLocation m_GirderMeasurementLocation;
   EventIndexType m_ClosureEvent;
};

class txnEditTemporarySupport : public txnTransaction
{
public:
   txnEditTemporarySupport(SupportIDType tsID,const txnEditTemporarySupportData& oldData,const txnEditTemporarySupportData& newData);

   ~txnEditTemporarySupport();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);

   SupportIDType m_tsID;
   txnEditTemporarySupportData m_TSData[2];
};
