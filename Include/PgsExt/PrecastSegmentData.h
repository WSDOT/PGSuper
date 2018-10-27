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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>
#include <StrData.h>

#include <PgsExt\SpanData2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\HandlingData.h>
#include <PgsExt\Keys.h>

#include <memory>

class CClosureJointData;
class CSplicedGirderData;
class CGirderSpacing2;

/*****************************************************************************
CLASS 
   CPrecastSegmentData

   Utility class for defining the input parameters for a precast segment

DESCRIPTION
   Utility class for defining the input parameters for a precast segment.
*****************************************************************************/

class PGSEXTCLASS CPrecastSegmentData
{
public:
   CPrecastSegmentData();
   CPrecastSegmentData(CSplicedGirderData* pGirder);
   CPrecastSegmentData(const CPrecastSegmentData& rOther); // copies only data, not ID or Index
   ~CPrecastSegmentData();

   // assigns all segment data including ID and Index
   CPrecastSegmentData& operator=(const CPrecastSegmentData& rOther);


   CStrandData Strands;      // number of strands, debonding and strand data
   CGirderMaterial Material; // concrete
   CShearData2 ShearData;    // stirrups
   CLongitudinalRebarData LongitudinalRebarData;  // mild reinforcing
   CHandlingData HandlingData; // lifting, hauling, etc

   // section transition
   void SetVariationType(pgsTypes::SegmentVariationType variationType);
   pgsTypes::SegmentVariationType GetVariationType() const;
   void SetVariationParameters(pgsTypes::SegmentZoneType zone,Float64 length,Float64 height,Float64 bottomFlangeDepth);
   void GetVariationParameters(pgsTypes::SegmentZoneType zone,bool bRawValues,Float64 *length,Float64 *height,Float64 *bottomFlangeDepth) const;
   Float64 GetVariationLength(pgsTypes::SegmentZoneType zone) const;
   Float64 GetVariationHeight(pgsTypes::SegmentZoneType zone) const;
   void EnableVariableBottomFlangeDepth(bool bEnable);
   bool IsVariableBottomFlangeDepthEnabled() const;
   Float64 GetVariationBottomFlangeDepth(pgsTypes::SegmentZoneType zone) const;
   bool AreSegmentVariationsValid(Float64 segmentFramingLength) const;

   // returns the height of the segment based on the dimensions in the associated
   // library entry.
   Float64 GetBasicSegmentHeight() const;

   // End blocks (index is pgsTypes::MemberEndType enum)
   Float64 EndBlockLength[2];
   Float64 EndBlockTransitionLength[2];
   Float64 EndBlockWidth[2];

   // Copies only segment definition data. Does not copy ID or Index
   void CopySegmentData(const CPrecastSegmentData* pSegment,bool bCopyLocation);

   void SetSpans(const CSpanData2* pStartSpan,const CSpanData2* pEndSpan);
   void SetSpan(pgsTypes::MemberEndType endType,const CSpanData2* pSpan);
   const CSpanData2* GetSpan(pgsTypes::MemberEndType endType) const;
   
   SpanIndexType GetSpanIndex(pgsTypes::MemberEndType endType) const;

   void SetGirder(CSplicedGirderData* pGirder);
   CSplicedGirderData* GetGirder();
   const CSplicedGirderData* GetGirder() const;

   void SetStartClosure(CClosureJointData* pClosure);
   const CClosureJointData* GetStartClosure() const;
   CClosureJointData* GetStartClosure();

   void SetEndClosure(CClosureJointData* pClosure);
   const CClosureJointData* GetEndClosure() const;
   CClosureJointData* GetEndClosure();

   const CPrecastSegmentData* GetPrevSegment() const;
   const CPrecastSegmentData* GetNextSegment() const;

   // Returns the support at the start of the segment. The support could be a
   // pier or a temporary support. The support pointer is not nullptr, the other pointer is nullptr
   void GetSupport(pgsTypes::MemberEndType endType,const CPierData2** ppPier,const CTemporarySupportData** ppTS) const;
   void GetSupport(pgsTypes::MemberEndType endType,CPierData2** ppPier,CTemporarySupportData** ppTS);
   void GetStations(Float64* pStartStation,Float64* pEndStation) const;

   void GetSpacing(const CGirderSpacing2** ppStartSpacing,const CGirderSpacing2** ppEndSpacing) const;

   SegmentIndexType GetIndex() const;
   SegmentIDType GetID() const;
   CSegmentKey GetSegmentKey() const;

   // Returns a vector of all the piers that support this segment
   std::vector<const CPierData2*> GetPiers() const;

   // Returns a vector of all the temporary supports that support this segment
   std::vector<const CTemporarySupportData*> GetTemporarySupports() const;

   // Returns true if the segment is a drop-in segment
   bool IsDropIn() const;


   //------------------------------------------------------------------------
   // specialized function to copy only material data or only prestressing data
   // from another
   void CopyMaterialFrom(const CPrecastSegmentData& rOther);
   void CopyPrestressingFrom(const CPrecastSegmentData& rOther);
   void CopyVariationFrom(const CPrecastSegmentData& rOther);
   void CopyLongitudinalReinforcementFrom(const CPrecastSegmentData& rOther);
   void CopyTransverseReinforcementFrom(const CPrecastSegmentData& rOther);
   void CopyHandlingFrom(const CPrecastSegmentData& rOther);

   bool operator==(const CPrecastSegmentData& rOther) const;
   bool operator!=(const CPrecastSegmentData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   static LPCTSTR GetSegmentVariation(pgsTypes::SegmentVariationType type);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void Init();
   void MakeCopy(const CPrecastSegmentData& rOther,bool bCopyIdentity,bool bCopyLocation,bool bCopyProperties);
   void MakeAssignment(const CPrecastSegmentData& rOther);
   void ResolveReferences();

   // A segment can start/end of a temporary support or a pier (hinge/roller connections only)
   // Temporary supports are acceesed through the closure joint object

   // pointers to the closure joint data objects that are on the left and right of this segment.
   CClosureJointData* m_pStartClosure;  // weak reference, owned by CSplicedGirderData
   CClosureJointData* m_pEndClosure; // weak reference, owned by CSplicedGirderData

   pgsTypes::SegmentVariationType m_VariationType;
   Float64 m_VariationLength[4]; // index is the SegmentZoneType enum
   Float64 m_VariationHeight[4];
   Float64 m_VariationBottomFlangeDepth[4];
   bool m_bVariableBottomFlangeDepthEnabled;

   void AdjustAdjacentSegment();
   Float64 GetSegmentHeight(bool bSegmentHeight) const;

   CSplicedGirderData* m_pGirder;
   const CSpanData2* m_pSpanData[2]; // [0] is a pointer to the span where this segment starts
                                     // [1] is a pointer to the span where this segment ends
                                     // the pgsTypes::MemberEndType enum can be used to access this array
   
   SegmentIndexType m_SegmentIndex;
   void SetIndex(SegmentIndexType segIdx);

   SegmentIDType m_SegmentID;
   void SetID(SegmentIDType segID);
   
   friend CSplicedGirderData;
   SpanIndexType m_SpanIdx[2]; // contains the span index of the span that will be referenced by
                               // m_pSpanData. This value will be INVALID_INDEX with m_pSpanData is defined

   // cache the segment height and bottom flange thickness.
   // they are computed many times and don't ever change.
   mutable bool m_bHeightComputed;
   mutable Float64 m_Height;
   mutable bool m_bBottomFlangeThicknessComputed;
   mutable Float64 m_BottomFlangeThickness;
};
