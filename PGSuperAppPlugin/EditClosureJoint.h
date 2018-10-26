///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\TimelineManager.h>

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

class txnEditClosureJoint : public txnTransaction
{
public:
   txnEditClosureJoint(const CSegmentKey& closureKey,const txnEditClosureJointData& newData);

   ~txnEditClosureJoint();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void SetClosureJointData(const CSegmentKey& closureKey,const txnEditClosureJointData& data);

   CSegmentKey m_ClosureKey; // closure joint(s) to be edited
   txnEditClosureJointData m_NewData;
   std::set<txnEditClosureJointData> m_OldData;
};
