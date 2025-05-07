///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <EAF\Transaction.h>
#include <PsgLib\ClosureJointData.h>
#include <PsgLib\TimelineManager.h>

struct txnEditClosureJointData
{
   txnEditClosureJointData()
   { m_PierIdx = INVALID_INDEX; m_TSIdx = INVALID_INDEX; }

   bool operator<(const txnEditClosureJointData& rOther) const { return m_ClosureKey < rOther.m_ClosureKey; }

   PierIndexType m_PierIdx;
   SupportIndexType m_TSIdx;

   CSegmentKey m_ClosureKey;
   CClosureJointData m_ClosureJoint;
   CTimelineManager m_TimelineMgr;
};

class txnEditClosureJoint : public WBFL::EAF::Transaction
{
public:
   txnEditClosureJoint(const CSegmentKey& closureKey,const txnEditClosureJointData& newData);

   ~txnEditClosureJoint();

   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const;
   virtual bool IsRepeatable() const;

private:
   void SetClosureJointData(const CSegmentKey& closureKey,const txnEditClosureJointData& data);

   CSegmentKey m_ClosureKey; // closure joint(s) to be edited
   txnEditClosureJointData m_NewData;
   std::set<txnEditClosureJointData> m_OldData;
};
