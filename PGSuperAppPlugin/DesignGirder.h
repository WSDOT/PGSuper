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

#ifndef INCLUDED_DESIGNGIRDER_H_
#define INCLUDED_DESIGNGIRDER_H_

#include <System\Transaction.h>
#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\GirderMaterial.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\HandlingData.h>

typedef enum SlabOffsetDesignSelectionType
{
   sodtBridge,
   sodtPier,  
   sodtGirder,
   sodtAllSelectedGirders ,
   sodtDoNotDesign
} SlabOffsetType;


class txnDesignGirder : public txnTransaction
{
public:
   txnDesignGirder(std::vector<const pgsGirderDesignArtifact*>& artifacts, SlabOffsetDesignSelectionType soSelectionType, SpanIndexType fromSpan, GirderIndexType fromGirder);
   ~txnDesignGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   txnDesignGirder();

   void Init();
   bool m_bInit;

   void DoExecute(int i);

   // Store all design data for a girder
   struct DesignData
   {
      DesignData(const pgsGirderDesignArtifact& gdrDesignArtifact) : m_DesignArtifact(gdrDesignArtifact) {}

      pgsGirderDesignArtifact m_DesignArtifact;

      // index 0 = old data (before design), 1 = new data (design outcome)
      CPrecastSegmentData m_SegmentData[2];
   };

   typedef std::vector<DesignData> DesignDataColl;
   typedef DesignDataColl::iterator DesignDataIter;
   typedef DesignDataColl::const_iterator DesignDataConstIter;
   DesignDataColl m_DesignDataColl;

   // Slab Offset data is a bit more challenging:
   // Must store slab offset data unique for old and new data. no other way to make undoable
   // New design data
   SlabOffsetDesignSelectionType m_NewSlabOffsetType;
   SpanIndexType m_FromSpanIdx;
   GirderIndexType m_FromGirderIdx;
   bool    m_DidSlabOffsetDesign; // true only if slab offset was designed
   Float64 m_DesignSlabOffset[2]; // ahead:back

   pgsTypes::AssumedExcessCamberType m_NewAssumedExcessCamberType;
   bool    m_DidAssumedExcessCamberDesign; // true only if Assumed Excess Camber was designed
   Float64 m_DesignAssumedExcessCamber;

   // Old data
   pgsTypes::SlabOffsetType m_OldSlabOffsetType;
   Float64 m_OldBridgeSlabOffset; // Only if old type was sotBridge
   std::vector<std::pair<Float64,Float64>> m_OldPierSlabOffsets; // slab offsets for sotBearingLine
   std::map<CSegmentKey, std::pair<Float64, Float64>> m_OldSegmentSlabOffsets; // slab offsets for sotSegment

   pgsTypes::AssumedExcessCamberType m_OldAssumedExcessCamberType;
   Float64 m_OldBridgeAssumedExcessCamber; // Only if old type was aecBridge
   typedef struct OldAssumedExcessCamberData // Data for each girder or span/pier depending on AssumedExcessCamberType
   {
      GroupIndexType  GroupIdx;
      GirderIndexType GirderIdx; // only used for fttGirder
      Float64 AssumedExcessCamber;

      // constructor
      OldAssumedExcessCamberData(GroupIndexType groupIdx, GirderIndexType girderIdx, Float64 assumedExcessCamber):
         GroupIdx(groupIdx),GirderIdx(girderIdx),AssumedExcessCamber(assumedExcessCamber)
         {;}

   } OldAssumedExcessCamberData;
   std::vector<OldAssumedExcessCamberData> m_OldAssumedExcessCamberData;

};

#endif // INCLUDED_DESIGNGIRDER_H_