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

#include <PsgLib\DeckDescription.h>
#include <PsgLib\RailingSystem.h>
#include <PsgLib\SplicedGirderData.h>
#include <PsgLib\TimelineManager.h>


#include "PierData.h"
#include "SpanData.h"

class CBridgeDescription2;

///////////////////////////////////////////////////////
// NOTE: 
// This class only exists to load old PGSuper files.
///////////////////////////////////////////////////////

/*****************************************************************************
CLASS 
   CBridgeDescription

   Utility class for managing the bridge description data

DESCRIPTION
   Utility class for managing the bridge description data

LOG
   rab : 04.29.2008 : Created file
*****************************************************************************/

class CBridgeDescription
{
public:
   CBridgeDescription();
   ~CBridgeDescription();

   HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
   HRESULT Load(Float64 version,IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);

   CDeckDescription* GetDeckDescription();
   const CDeckDescription* GetDeckDescription() const;

   CRailingSystem* GetLeftRailingSystem();
   const CRailingSystem* GetLeftRailingSystem() const;

   CRailingSystem* GetRightRailingSystem();
   const CRailingSystem* GetRightRailingSystem() const;

   void CreateFirstSpan(const CPierData* pFirstPier=nullptr,const CSpanData* pFirstSpan=nullptr,const CPierData* pNextPier=nullptr);
   void AppendSpan(const CSpanData* pSpanData=nullptr,const CPierData* pPierData=nullptr);
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData* pSpanData=nullptr,const CPierData* pPierData=nullptr);
   void RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType);

   PierIndexType GetPierCount() const;
   SpanIndexType GetSpanCount() const;

   CPierData* GetPier(PierIndexType pierIdx);
   const CPierData* GetPier(PierIndexType pierIdx) const;
   
   CSpanData* GetSpan(SpanIndexType spanIdx);
   const CSpanData* GetSpan(SpanIndexType spanIdx) const;

   void UseSameNumberOfGirdersInAllSpans(bool bSame);
   bool UseSameNumberOfGirdersInAllSpans() const;
   void SetGirderCount(GirderIndexType nGirders);
   GirderIndexType GetGirderCount() const;

   void SetGirderFamilyName(LPCTSTR strName);
   LPCTSTR GetGirderFamilyName() const;

   void UseSameGirderForEntireBridge(bool bSame);
   bool UseSameGirderForEntireBridge() const;
   LPCTSTR GetGirderName() const;
   void RenameGirder(LPCTSTR strName);
   void SetGirderName(LPCTSTR strName);
   const GirderLibraryEntry* GetGirderLibraryEntry() const;
   void SetGirderLibraryEntry(const GirderLibraryEntry* pEntry);

   void SetGirderOrientation(pgsTypes::GirderOrientationType gdrOrientation);
   pgsTypes::GirderOrientationType GetGirderOrientation() const;

   void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs);
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType() const;
   Float64 GetGirderSpacing() const;
   void SetGirderSpacing(Float64 spacing);

   void SetMeasurementType(pgsTypes::MeasurementType mt);
   pgsTypes::MeasurementType GetMeasurementType() const;

   void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   pgsTypes::MeasurementLocation GetMeasurementLocation() const;

   // set/get the reference girder index. if index is < 0 then
   // the center of the girder group is the reference point
   void SetRefGirder(GirderIndexType refGdrIdx);
   GirderIndexType GetRefGirder() const;

   // offset to the reference girder, measured from either the CL bridge or alignment
   // as indicated by RegGirderOffsetDatum, MeasurementType, and LocationType
   void SetRefGirderOffset(Float64 offset);
   Float64 GetRefGirderOffset() const;

   // indicated what the reference girder offset is measured relative to
   void SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum);
   pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;

   void SetAlignmentOffset(Float64 alignmentOffset);
   Float64 GetAlignmentOffset() const;

   // set/get the slab offset type
   void SetSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType);
   pgsTypes::SlabOffsetType GetSlabOffsetType() const;

   // Set/get the slab offset. Has no net effect if slab offset type is not sotBridge
   // Get method returns invalid data if slab offset type is not sotBridge
   void SetSlabOffset(Float64 slabOffset);
   Float64 GetSlabOffset() const;

   // returns the least slab offset defined for the bridge
   Float64 GetLeastSlabOffset() const;

   bool SetSpanLength(SpanIndexType spanIdx,Float64 newLength);
   bool MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption);
   bool MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation);

   void SetDistributionFactorMethod(pgsTypes::DistributionFactorMethod method);
   pgsTypes::DistributionFactorMethod GetDistributionFactorMethod() const;

   void CopyDown(bool bGirderCount,bool bGirderType,bool bSpacing,bool bSlabOffset); 
                    // takes all the data defined at the bridge level and copies
                    // it down to the spans and girders (only for this parameters set to true)

   // It's pretty much impossible to take care of all data in this tree from the editing dialog
   // so the function below takes half-baked edits, compares what changed from the original
   // version, and finishes them.
   void ReconcileEdits(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CBridgeDescription* pOriginalDesc);

   void Clear();

   Float64 GetLength() const;
   void GetStationRange(Float64& startStation,Float64& endStation) const;
   bool IsOnBridge(Float64 station) const;

   void SetBridgeData(CBridgeDescription2* pBridgeDesc) const;

protected:
   void MakeCopy(const CBridgeDescription& rOther);
   void MakeAssignment(const CBridgeDescription& rOther);

private:
   CDeckDescription m_Deck;
   CRailingSystem m_LeftRailingSystem;
   CRailingSystem m_RightRailingSystem;

   std::vector<CPierData*> m_Piers;
   std::vector<CSpanData*> m_Spans;

   bool m_bSameNumberOfGirders;
   bool m_bSameGirderName;

   Float64 m_AlignmentOffset; // offset from Alignment to CL Bridge (< 0 = right of alignment)

   Float64 m_SlabOffset;
   pgsTypes::SlabOffsetType m_SlabOffsetType;

   GirderIndexType m_nGirders;
   Float64 m_GirderSpacing;

   GirderIndexType m_RefGirderIdx;
   Float64 m_RefGirderOffset;
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;

   pgsTypes::MeasurementType m_MeasurementType;
   pgsTypes::MeasurementLocation m_MeasurementLocation;

   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;

   const GirderLibraryEntry* m_pGirderLibraryEntry;

   pgsTypes::GirderOrientationType m_GirderOrientation;
   std::_tstring m_strGirderName;
   std::_tstring m_strGirderFamilyName;

   pgsTypes::DistributionFactorMethod m_LLDFMethod;

   bool MoveBridge(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation);

   void RenumberSpans();

#if defined _DEBUG
   void AssertValid();
#endif
};
