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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>
#include <PgsExt\GirderSpacing2.h>
#include <PgsExt\TemporarySupportData.h>
#include <memory>

class CPierData2;
class CBridgeDescription2;

/*****************************************************************************
CLASS 
   CSpanData2

   Utility class for span description data.

DESCRIPTION
   Utility class for span description data. This class encapsulates all
   the input data for a span and provides coordinate between girder layout, 
   number of girder, and girder types within the span

LOG
   rab : 04.25.208 : Created file
*****************************************************************************/

class PGSEXTCLASS CSpanData2
{
public:
   CSpanData2(SpanIndexType spanIdx = INVALID_INDEX,CBridgeDescription2* pBridge=nullptr,CPierData2* pPrevPier=nullptr,CPierData2* pNextPier=nullptr);
   CSpanData2(const CSpanData2& rOther); // copies only data, not ID or Index
   ~CSpanData2();


   CSpanData2& operator = (const CSpanData2& rOther);
   void CopySpanData(const CSpanData2* pSpan);

   bool operator==(const CSpanData2& rOther) const;
   bool operator!=(const CSpanData2& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // =================================================================================
   // Methods called by the framework. Don't call these methods directly
   // =================================================================================
   void Init(SpanIndexType spanIdx,CBridgeDescription2* pBridge,CPierData2* pPrevPier,CPierData2* pNextPier);
   void SetIndex(SpanIndexType spanIdx);
   void SetBridgeDescription(CBridgeDescription2* pBridge);
   void SetPiers(CPierData2* pPrevPier,CPierData2* pNextPier);


   // =================================================================================
   // Configuration information
   // =================================================================================

   // retuns the index of this span
   SpanIndexType GetIndex() const;
   
   // returns the bridge this span is a part of
   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   // returns the piers at the start/end of this span
   CPierData2* GetPrevPier();
   CPierData2* GetNextPier();
   CPierData2* GetPier(pgsTypes::MemberEndType end);
   const CPierData2* GetPrevPier() const;
   const CPierData2* GetNextPier() const;
   const CPierData2* GetPier(pgsTypes::MemberEndType end) const;

   // Returns the number of girders in this span
   GirderIndexType GetGirderCount() const;

   // Returns the length of this span measured between the stations of the prev/next piers
   Float64 GetSpanLength() const;

   // Returns all of the temporary supports defined in this span
   std::vector<const CTemporarySupportData*> GetTemporarySupports() const;
   std::vector<CTemporarySupportData*> GetTemporarySupports();

   // =================================================================================
   // Assumed Excess Camber Used only when the parent bridge's AssumedExcessCamberType is aecSpan or aecGirder
   // =================================================================================
   // Set the Assumed Excess Camber at a span (same for all girders)
   // Use when AssumedExcessCamber type is pgsTypes::aecSpan
   void SetAssumedExcessCamber(Float64 assumedExcessCamber);

   // Set/Get the Assumed Excess Camber at a span for a specific girder
   // Use when Assumed Excess Camber type is pgsTypes::aecGirder
   void SetAssumedExcessCamber(GirderIndexType gdrIdx,Float64 assumedExcessCamber);
   Float64 GetAssumedExcessCamber(GirderIndexType gdrIdx,bool bGetRawValue = false) const;

   // Copies girder-by-girder Assumed ExcessCamber data from one girder to another
   void CopyAssumedExcessCamber(GirderIndexType sourceGdrIdx,GirderIndexType targetGdrIdx);

   // =================================================================================
   // Direct Haunch Depth Used only when the parent bridge's HaunchLayoutType==hltAlongSpans
   // and HaunchInputLocationType==hilSame4AllGirders or hilPerEach
   // =================================================================================
   // Set the Haunch at a span (same for all girders)
   void SetDirectHaunchDepths(const std::vector<Float64>& haunchDepths);

   // Set/Get the Haunch Depth at a span for a specific girder
   // Use when hilSame4AllGirders
   void SetDirectHaunchDepths(GirderIndexType gdrIdx, const std::vector<Float64>& HaunchDepth);
   std::vector<Float64> GetDirectHaunchDepths(GirderIndexType gdrIdx,bool bGetRawValue = false) const;

   // Copies girder-by-girder Haunch Depth data from one girder to another
   void CopyHaunchDepth(GirderIndexType sourceGdrIdx,GirderIndexType targetGdrIdx);


   // =================================================================================
   // Live Load Distribution Factors (for Directly Input)
   // =================================================================================
   void SetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM);
   void SetLLDFPosMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM);
   Float64 GetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

   void SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM);
   void SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM);
   Float64 GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

   void SetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gV);
   void SetLLDFShear(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gV);
   Float64 GetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const;

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CSpanData2& rOther,bool bCopyDataOnly);
   void MakeAssignment(const CSpanData2& rOther);

private:
   SpanIndexType m_SpanIdx;

   CBridgeDescription2* m_pBridgeDesc;
   CPierData2* m_pPrevPier;
   CPierData2* m_pNextPier;


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

   mutable std::vector<Float64> m_vAssumedExcessCambers; // Assummed Excess Camber for each girder in span. First value is used for aec:Span

   // make sure AssumedExcessCamber data stays intact from girder count changes
   void ProtectAssumedExcessCamber() const;

   mutable std::vector< std::vector<Float64>> m_vHaunchDepths; // vector of Haunch Depths for each girder in span. First value is used for hilSame4AllGirders

   // make sure Haunch Depth data stays intact from girder count changes
   void ProtectHaunchDepth() const;

   friend CBridgeDescription2;
};
