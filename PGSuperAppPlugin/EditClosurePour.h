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
#include <PgsExt\ClosurePourData.h>

struct txnEditClosurePourData
{
   bool operator<(const txnEditClosurePourData& rOther) const { return m_ClosureKey < rOther.m_ClosureKey; }
   CSegmentKey m_ClosureKey;
   CClosurePourData m_ClosurePour;
   EventIndexType m_ClosureEventIdx;
};

class txnEditClosurePour : public txnTransaction
{
public:
   txnEditClosurePour(const CSegmentKey& closureKey,const txnEditClosurePourData& newData);

   ~txnEditClosurePour();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void SetClosurePourData(const CSegmentKey& closureKey,const txnEditClosurePourData& data);

   CSegmentKey m_ClosureKey; // closure pour(s) to be edited
   txnEditClosurePourData m_NewData;
   std::set<txnEditClosurePourData> m_OldData;
};
