///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

   pgsTypes::FilletType m_NewFilletType;
   bool    m_DidFilletDesign; // true only if fillet was designed
   Float64 m_DesignFillet;

   // Old data
   pgsTypes::SlabOffsetType m_OldSlabOffsetType;
   Float64 m_OldBridgeSlabOffset; // Only if old type was sotBridge
   typedef struct OldSlabOffsetData // Data for each girder or span/pier depending on SlabOffsetType
   {
      GroupIndexType  GroupIdx;
      PierIndexType   PierIdx;
      GirderIndexType GirderIdx; // only used for sotGirder
      Float64 SlabOffset;

      // constructor
      OldSlabOffsetData(GroupIndexType groupIdx, PierIndexType pierIdx, GirderIndexType girderIdx, Float64 slabOffset):
         GroupIdx(groupIdx),PierIdx(pierIdx),GirderIdx(girderIdx),SlabOffset(slabOffset)
         {;}

   } OldSlabOffsetData;
   std::vector<OldSlabOffsetData> m_OldSlabOffsetData;

   pgsTypes::FilletType m_OldFilletType;
   Float64 m_OldBridgeFillet; // Only if old type was fttBridge
   typedef struct OldFilletData // Data for each girder or span/pier depending on FilletType
   {
      GroupIndexType  GroupIdx;
      GirderIndexType GirderIdx; // only used for fttGirder
      Float64 Fillet;

      // constructor
      OldFilletData(GroupIndexType groupIdx, GirderIndexType girderIdx, Float64 Fillet):
         GroupIdx(groupIdx),GirderIdx(girderIdx),Fillet(Fillet)
         {;}

   } OldFilletData;
   std::vector<OldFilletData> m_OldFilletData;

};

#endif // INCLUDED_DESIGNGIRDER_H_