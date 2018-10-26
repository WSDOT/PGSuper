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

#pragma once

#include <WBFLCore.h>
#include "GirderTypes.h"
#include "GirderSpacing.h"
#include <memory>

class CPierData;
class CBridgeDescription;

///////////////////////////////////////////////////////
// NOTE: 
// This class only exists to load old PGSuper files.
///////////////////////////////////////////////////////

/*****************************************************************************
CLASS 
   CSpanData

   Utility class for span description data.

DESCRIPTION
   Utility class for span description data. This class encapsulates all
   the input data for a span and provides coordinate between girder layout, 
   number of girder, and girder types within the span

LOG
   rab : 04.25.208 : Created file
*****************************************************************************/

class CSpanData
{
public:
   CSpanData(SpanIndexType spanIdx = -1,CBridgeDescription* pBridge=nullptr,CPierData* pPrevPier=nullptr,CPierData* pNextPier=nullptr);
   CSpanData(const CSpanData& rOther);
   ~CSpanData();

   void Init(SpanIndexType spanIdx,CBridgeDescription* pBridge,CPierData* pPrevPier,CPierData* pNextPier);

   void SetSpanIndex(SpanIndexType spanIdx);
   SpanIndexType GetSpanIndex() const;
   
   void SetBridgeDescription(CBridgeDescription* pBridge);
   CBridgeDescription* GetBridgeDescription();
   const CBridgeDescription* GetBridgeDescription() const;

   void SetPiers(CPierData* pPrevPier,CPierData* pNextPier);
   CPierData* GetPrevPier();
   CPierData* GetNextPier();
   const CPierData* GetPrevPier() const;
   const CPierData* GetNextPier() const;

   GirderIndexType GetGirderCount() const;
   void SetGirderCount(GirderIndexType nGirders);
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   const CGirderTypes* GetGirderTypes() const;
   CGirderTypes* GetGirderTypes();
   void SetGirderTypes(const CGirderTypes& girderTypes);

   void UseSameSpacingAtBothEndsOfSpan(bool bUseSame);
   bool UseSameSpacingAtBothEndsOfSpan() const;
   const CGirderSpacing* GetGirderSpacing(pgsTypes::MemberEndType end) const;
   const CGirderSpacing* GetGirderSpacing(pgsTypes::PierFaceType pierFace) const;

   void SetGirderSpacing(pgsTypes::MemberEndType end,const CGirderSpacing& spacing);
   void SetGirderSpacing(pgsTypes::PierFaceType pierFace,const CGirderSpacing& spacing);

   void SetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM);
   void SetLLDFPosMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM);
   Float64 GetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

   void SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM);
   void SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM);
   Float64 GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

   void SetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gV);
   void SetLLDFShear(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gV);
   Float64 GetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

   // set/get the slab offset at each end of the span
   // if the slab offset is defined for the entire bridge, the value is pushed up to the bridge level
   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::MemberEndType end) const;
   void SetSlabOffset(pgsTypes::PierFaceType face,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::PierFaceType face) const;

   Float64 GetSpanLength() const;
   bool IsInteriorGirder(GirderIndexType gdrIdx) const;
   bool IsExteriorGirder(GirderIndexType gdrIdx) const;

   CSpanData& operator = (const CSpanData& rOther);
   bool operator==(const CSpanData& rOther) const;
   bool operator!=(const CSpanData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CSpanData& rOther);
   void MakeAssignment(const CSpanData& rOther);

private:
   SpanIndexType m_SpanIdx;

   CBridgeDescription* m_pBridgeDesc;
   CPierData* m_pPrevPier;
   CPierData* m_pNextPier;

   // The following data is only applicable if it is not defined at the bridge level
   CGirderTypes  m_GirderTypes;  // girder types for this span
   GirderIndexType m_nGirders; // # girders in this span

   // girder spacing for this span
   CGirderSpacing m_GirderSpacing[2]; // index 0 is at start of span, index 1 at end

   Float64 m_SlabOffset[2]; // slab offset if defined by span (0 = start, 1 = end);

   CGirderSpacing* GirderSpacing(pgsTypes::MemberEndType end);
   CGirderSpacing* GirderSpacing(pgsTypes::PierFaceType pierFace);

   // LLDF
   // 0 for strength/service limit state, 1 for fatigue limit state
   struct LLDF
   {
      Float64 gNM[2];
      Float64 gPM[2];
      Float64 gV[2];

      LLDF()
      {
         gNM[0]=1.0; gNM[1]=1.0; gPM[0]=1.0; gPM[1]=1.0; gV[0]=1.0; gV[1]=1.0;
      }

      bool operator==(const LLDF& rOther) const
      {
         return IsEqual(gNM[0], rOther.gNM[0]) && IsEqual(gNM[1], rOther.gNM[1]) &&
                IsEqual(gPM[0], rOther.gPM[0]) && IsEqual(gPM[1], rOther.gPM[1]) &&
                IsEqual( gV[0], rOther.gV[0])  && IsEqual( gV[1], rOther.gV[1]);
      }
   };

   mutable std::vector<LLDF> m_LLDFs; // this is mutable because we may have to change the container size on Get functions

   // safe internal function for getting lldfs in lieue of girder count changes
   LLDF& GetLLDF(GirderIndexType igs) const;

   friend CBridgeDescription;
};
