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

#ifndef INCLUDED_EDITGIRDER_H_
#define INCLUDED_EDITGIRDER_H_

#include <EAF\EAFTransaction.h>
#include <PgsExt\SplicedGirderData.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\Keys.h>
#include <IFace\Project.h>

struct txnEditGirderData
{
   txnEditGirderData();
   txnEditGirderData(const txnEditGirderData& rOther);

   // for sorting and lookup by girder key
   bool operator<(const txnEditGirderData& rOther) const { return m_GirderKey < rOther.m_GirderKey; }

   CTimelineManager m_TimelineMgr;
   CGirderKey m_GirderKey;
   bool m_bUseSameGirder;
   std::_tstring m_strGirderName;
   CSplicedGirderData m_Girder;

   pgsTypes::SlabOffsetType m_SlabOffsetType;
   std::array<Float64, 2> m_SlabOffset; // index is pgsTypes::MemberEndType
   // if slab offset is whole bridge then m_SlabOffset[pgsTypes::metStart] contains the value

   pgsTypes::AssumedExcessCamberType m_AssumedExcessCamberType;
   Float64 m_AssumedExcessCamber;

   // Vector below is used for direct input of haunch depths. It can contain 0, 1, or 2 values indexed by pgsTypes::MemberEndType. 
   // If empty, it means that no haunch depths were collected at input for this girder
   std::vector<Float64> m_HaunchDepths;

   pgsTypes::BearingType m_BearingType; // 
   std::array<CBearingData2, 2> m_BearingData;  // index is pgsTypes::MemberEndType
};

class txnEditGirder : public CEAFTransaction
{
public:
   txnEditGirder(const CGirderKey& girderKey,const txnEditGirderData& newGirderData);

   ~txnEditGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction>CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const;
   virtual bool IsRepeatable() const;

private:
   void SetGirderData(const CGirderKey& girderKey,const txnEditGirderData& gdrData,bool bUndo);

   CGirderKey m_GirderKey; // indicates the girder or girders to be edited
   txnEditGirderData m_NewGirderData;
   std::set<txnEditGirderData> m_OldGirderData; // data saved for undo
};

#endif // INCLUDED_EDITGIRDER_H_