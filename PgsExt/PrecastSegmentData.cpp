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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\StructuredSave.h>
#include <PsgLib\StructuredLoad.h>

#include <WBFLGenericBridge.h>
#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPrecastSegmentData::CPrecastSegmentData()
{
   Init();
}

CPrecastSegmentData::CPrecastSegmentData(CSplicedGirderData* pGirder)
{
   Init();

   m_pGirder = pGirder;

   m_pSpanData[pgsTypes::metStart] = nullptr;
   m_pSpanData[pgsTypes::metEnd]   = nullptr;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd] = INVALID_INDEX;

   m_pClosure[pgsTypes::metStart] = nullptr;
   m_pClosure[pgsTypes::metEnd] = nullptr;
}

CPrecastSegmentData::CPrecastSegmentData(const CPrecastSegmentData& rOther)
{
   Init();
   MakeCopy(rOther,false,true,true);
}

CPrecastSegmentData::~CPrecastSegmentData()
{
}

void CPrecastSegmentData::Init()
{
   m_bHeightComputed = false;
   m_Height = -1;
   m_bBottomFlangeThicknessComputed = false;
   m_BottomFlangeThickness = -1;

   m_SegmentIndex = INVALID_INDEX;
   m_SegmentID    = INVALID_ID;

   m_VariationType = pgsTypes::svtNone;
   for ( int i = 0; i < 4; i++ )
   {
      m_VariationLength[i] = 0;
      m_VariationHeight[i] = 0;
      m_VariationBottomFlangeDepth[i] = 0;
   }

   m_bVariableBottomFlangeDepthEnabled = false;

   m_VariationHeight[0] = WBFL::Units::ConvertToSysUnits(10,WBFL::Units::Measure::Feet);
   m_VariationHeight[3] = m_VariationHeight[0];

   for ( int j = 0; j < 2; j++ )
   {
      EndBlockLength[j] = 0;
      EndBlockTransitionLength[j] = 0;
      EndBlockWidth[j] = 0;
   }

   m_pGirder = nullptr;

   m_pSpanData[pgsTypes::metStart] = nullptr;
   m_pSpanData[pgsTypes::metEnd]   = nullptr;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd] = INVALID_INDEX;

   m_pClosure[pgsTypes::metStart] = nullptr;
   m_pClosure[pgsTypes::metEnd] = nullptr;

   TopFlangeThickeningType = pgsTypes::tftNone;
   TopFlangeThickening = 0.0;

   Precamber = 0.0;

   Tendons.SetSegment(this);
}

void CPrecastSegmentData::SetGirder(CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;
}

CSplicedGirderData* CPrecastSegmentData::GetGirder()
{
   return m_pGirder;
}

const CSplicedGirderData* CPrecastSegmentData::GetGirder() const
{
   return m_pGirder;
}

void CPrecastSegmentData::SetSpans(CSpanData2* pStartSpan,CSpanData2* pEndSpan)
{
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pStartSpan->GetBridgeDescription() );
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pEndSpan->GetBridgeDescription() );

   m_pSpanData[pgsTypes::metStart] = pStartSpan;
   m_pSpanData[pgsTypes::metEnd]   = pEndSpan;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd]   = INVALID_INDEX;
}

void CPrecastSegmentData::SetSpan(pgsTypes::MemberEndType endType,CSpanData2* pSpan)
{
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pSpan->GetBridgeDescription() );
   m_pSpanData[endType] = pSpan;
   m_SpanIdx[endType] = INVALID_INDEX;
}

const CSpanData2* CPrecastSegmentData::GetSpan(pgsTypes::MemberEndType endType) const
{
   return m_pSpanData[endType];
}

CSpanData2* CPrecastSegmentData::GetSpan(pgsTypes::MemberEndType endType)
{
   return m_pSpanData[endType];
}

SpanIndexType CPrecastSegmentData::GetSpanIndex(pgsTypes::MemberEndType endType) const
{
   return (m_pSpanData[endType] != nullptr ? m_pSpanData[endType]->GetIndex() : m_SpanIdx[endType]);
}

void CPrecastSegmentData::SetClosureJoint(pgsTypes::MemberEndType endType, CClosureJointData* pClosure)
{
   m_pClosure[endType] = pClosure;
}

const CClosureJointData* CPrecastSegmentData::GetClosureJoint(pgsTypes::MemberEndType endType) const
{
   return m_pClosure[endType];
}

CClosureJointData* CPrecastSegmentData::GetClosureJoint(pgsTypes::MemberEndType endType)
{
   return m_pClosure[endType];
}

const CPrecastSegmentData* CPrecastSegmentData::GetPrevSegment() const
{
   return m_pClosure[pgsTypes::metStart] == nullptr ? nullptr : m_pClosure[pgsTypes::metStart]->GetLeftSegment();
}

const CPrecastSegmentData* CPrecastSegmentData::GetNextSegment() const
{
   return m_pClosure[pgsTypes::metEnd] == nullptr ? nullptr : m_pClosure[pgsTypes::metEnd]->GetRightSegment();
}

void CPrecastSegmentData::GetSupport(pgsTypes::MemberEndType endType,const CPierData2** ppPier,const CTemporarySupportData** ppTS) const
{
   *ppPier = nullptr;
   *ppTS = nullptr;

   if (m_pClosure[endType])
   {
      if (m_pClosure[endType]->GetPier())
      {
         *ppPier = m_pClosure[endType]->GetPier();
      }
      else
      {
         *ppTS = m_pClosure[endType]->GetTemporarySupport();
      }
   }
   else
   {
      *ppPier = (m_pSpanData[endType] ? m_pSpanData[endType]->GetPier(endType) : m_pGirder->GetPier(endType));
   }
}

void CPrecastSegmentData::GetSupport(pgsTypes::MemberEndType endType,CPierData2** ppPier,CTemporarySupportData** ppTS)
{
   *ppPier = nullptr;
   *ppTS = nullptr;

   if (m_pClosure[endType])
   {
      if (m_pClosure[endType]->GetPier())
      {
         *ppPier = m_pClosure[endType]->GetPier();
      }
      else
      {
         *ppTS = m_pClosure[endType]->GetTemporarySupport();
      }
   }
   else
   {
      *ppPier = (m_pSpanData[endType] ? m_pSpanData[endType]->GetPier(endType) : m_pGirder->GetPier(endType));
   }
}

void CPrecastSegmentData::GetStations(Float64* pStartStation,Float64* pEndStation) const
{
   const CPierData2* pPier = nullptr;
   const CTemporarySupportData* pTS = nullptr;
   GetSupport(pgsTypes::metStart,&pPier,&pTS);
   *pStartStation = pPier ?  pPier->GetStation() : pTS->GetStation();

   GetSupport(pgsTypes::metEnd,&pPier,&pTS);
   *pEndStation = pPier ? pPier->GetStation() : pTS->GetStation();
}

void CPrecastSegmentData::GetSpacing(const CGirderSpacing2** ppStartSpacing,const CGirderSpacing2** ppEndSpacing) const
{
   if ( m_pClosure[pgsTypes::metStart] )
   {
      if (m_pClosure[pgsTypes::metStart]->GetPier() )
      {
         *ppStartSpacing = m_pClosure[pgsTypes::metStart]->GetPier()->GetGirderSpacing(pgsTypes::Ahead);
      }
      else
      {
         *ppStartSpacing = m_pClosure[pgsTypes::metStart]->GetTemporarySupport()->GetSegmentSpacing();
      }
   }
   else
   {
      *ppStartSpacing = m_pGirder->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
   }

   if (m_pClosure[pgsTypes::metEnd])
   {
      if (m_pClosure[pgsTypes::metEnd]->GetPier() )
      {
         *ppEndSpacing = m_pClosure[pgsTypes::metEnd]->GetPier()->GetGirderSpacing(pgsTypes::Back);
      }
      else
      {
         *ppEndSpacing = m_pClosure[pgsTypes::metEnd]->GetTemporarySupport()->GetSegmentSpacing();
      }
   }
   else
   {
      *ppEndSpacing = m_pGirder->GetPier(pgsTypes::metEnd)->GetGirderSpacing(pgsTypes::Back);
   }
}

void CPrecastSegmentData::SetVariationType(pgsTypes::SegmentVariationType variationType)
{
   m_VariationType = variationType;

   AdjustAdjacentSegment();
}

pgsTypes::SegmentVariationType CPrecastSegmentData::GetVariationType() const
{
   return m_VariationType;
}

void CPrecastSegmentData::SetVariationParameters(pgsTypes::SegmentZoneType zone,Float64 length,Float64 height,Float64 bottomFlangeDepth)
{
   m_VariationLength[zone]            = length;
   m_VariationHeight[zone]            = height;
   m_VariationBottomFlangeDepth[zone] = bottomFlangeDepth;

   AdjustAdjacentSegment();
}

void CPrecastSegmentData::GetVariationParameters(pgsTypes::SegmentZoneType zone,bool bRawValues,Float64 *length,Float64 *height,Float64 *bottomFlangeDepth) const
{
   if ( bRawValues )
   {
      *length            = m_VariationLength[zone];
      *height            = m_VariationHeight[zone];
      *bottomFlangeDepth = m_VariationBottomFlangeDepth[zone];
   }
   else
   {
      *length            = m_VariationLength[zone];
      *height            = GetVariationHeight(zone);
      *bottomFlangeDepth = GetVariationBottomFlangeDepth(zone);
   }
}

Float64 CPrecastSegmentData::GetVariationLength(pgsTypes::SegmentZoneType zone) const
{
   if ( m_VariationType == pgsTypes::svtNone )
   {
      if ( zone == pgsTypes::sztLeftPrismatic || zone == pgsTypes::sztRightPrismatic )
      {
         return -0.5; // 50% of segment length
      }
      else
      {
         return 0.0;
      }
   }
   else
   {
      return m_VariationLength[zone];
   }
}

Float64 CPrecastSegmentData::GetVariationHeight(pgsTypes::SegmentZoneType zone) const
{
   if ( m_VariationType == pgsTypes::svtNone )
   {
      return GetSegmentHeight(true);
   }
   else
   {
      return m_VariationHeight[zone];
   }
}

Float64 CPrecastSegmentData::GetVariationBottomFlangeDepth(pgsTypes::SegmentZoneType zone) const
{
   if ( m_VariationType == pgsTypes::svtNone || !m_bVariableBottomFlangeDepthEnabled )
   {
      return GetSegmentHeight(false);
   }
   else
   {
      return m_VariationBottomFlangeDepth[zone];
   }
}

bool CPrecastSegmentData::AreSegmentVariationsValid(Float64 segmentFramingLength) const
{
   Float64 L = 0;
   Float64 L1 = m_VariationLength[pgsTypes::sztLeftPrismatic];
   Float64 L2 = m_VariationLength[pgsTypes::sztLeftTapered];
   Float64 L3 = m_VariationLength[pgsTypes::sztRightTapered];
   Float64 L4 = m_VariationLength[pgsTypes::sztRightPrismatic];
   if ( L1 < 0 )
   {
      L1 *= -segmentFramingLength;
   }

   if ( L2 < 0 )
   {
      L2 *= -segmentFramingLength;
   }

   if ( L3 < 0 )
   {
      L3 *= -segmentFramingLength;
   }

   if ( L4 < 0 )
   {
      L4 *= -segmentFramingLength;
   }

   switch(m_VariationType)
   {
   case pgsTypes::svtNone:
      break;

   case pgsTypes::svtLinear:
   case pgsTypes::svtParabolic:
      L = L1 + L4;
      break;

   case pgsTypes::svtDoubleLinear:
   case pgsTypes::svtDoubleParabolic:
      L = L1 + L2 + L3 + L4;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return (::IsLE(L,segmentFramingLength) ? true : false);
}

Float64 CPrecastSegmentData::GetBasicSegmentHeight() const
{
   return GetSegmentHeight(true);
}

void CPrecastSegmentData::EnableVariableBottomFlangeDepth(bool bEnable)
{
   m_bVariableBottomFlangeDepthEnabled = bEnable;
}

bool CPrecastSegmentData::IsVariableBottomFlangeDepthEnabled() const
{
   return m_bVariableBottomFlangeDepthEnabled;
}

SegmentIndexType CPrecastSegmentData::GetIndex() const
{
   return m_SegmentIndex;
}

SegmentIDType CPrecastSegmentData::GetID() const
{
   return m_SegmentID;
}

std::vector<const CPierData2*> CPrecastSegmentData::GetPiers() const
{
   std::vector<const CPierData2*> vPiers;

   // get pier at start of segment (if there is one)
   if (m_pClosure[pgsTypes::metStart] && m_pClosure[pgsTypes::metStart]->GetPier() )
   {
      vPiers.push_back(m_pClosure[pgsTypes::metStart]->GetPier() );
   }
   else if ( m_SegmentIndex == 0 )
   {
      vPiers.push_back( m_pGirder->GetPier(pgsTypes::metStart) );
   }

   // get intermediate piers
   Float64 startStation,endStation;
   GetStations(&startStation,&endStation);

   const CSpanData2* pSpan = GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan = GetSpan(pgsTypes::metEnd);

   do
   {
      const CPierData2* pPier = pSpan->GetNextPier();
      if ( startStation < pPier->GetStation() && pPier->GetStation() < endStation )
      {
         vPiers.push_back(pPier);
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   } while ( pSpan && pSpan->GetIndex() <= pEndSpan->GetIndex() );


   // get pier at end of segment (if there is one)
   if (m_pClosure[pgsTypes::metEnd] && m_pClosure[pgsTypes::metEnd]->GetPier() )
   {
      vPiers.push_back(m_pClosure[pgsTypes::metEnd]->GetPier() );
   }
   else if ( m_SegmentIndex == m_pGirder->GetSegmentCount()-1 )
   {
      vPiers.push_back( m_pGirder->GetPier(pgsTypes::metEnd) );
   }

   return vPiers;
}

std::vector<const CTemporarySupportData*> CPrecastSegmentData::GetTemporarySupports() const
{
   std::vector<const CTemporarySupportData*> tempSupports;

   // get temporary support at start of segment (if there is one)
   if (m_pClosure[pgsTypes::metStart] && m_pClosure[pgsTypes::metStart]->GetTemporarySupport() )
   {
      tempSupports.push_back(m_pClosure[pgsTypes::metStart]->GetTemporarySupport() );
   }

   // get intermediate temporary supports
   Float64 startStation,endStation;
   GetStations(&startStation,&endStation);

   const CSpanData2* pSpan = GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan = GetSpan(pgsTypes::metEnd);

   do
   {
      std::vector<const CTemporarySupportData*> spanTempSupports(pSpan->GetTemporarySupports());
      for(const auto* pTS : spanTempSupports)
      {
         Float64 tsStation = pTS->GetStation();
         if ( startStation < tsStation && tsStation < endStation )
         {
            tempSupports.push_back(pTS);
         }
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   } while ( pSpan && pSpan->GetIndex() <= pEndSpan->GetIndex() );


   // get temporary support at end of segment (if there is one)
   if (m_pClosure[pgsTypes::metEnd] && m_pClosure[pgsTypes::metEnd]->GetTemporarySupport() )
   {
      tempSupports.push_back(m_pClosure[pgsTypes::metEnd]->GetTemporarySupport() );
   }

   return tempSupports;
}

bool RemoveStrongbacksSupports(const CTemporarySupportData* pTS)
{
   return (pTS->GetSupportType() == pgsTypes::StrongBack ? true : false);
}

pgsTypes::DropInType CPrecastSegmentData::IsDropIn() const
{
   std::vector<const CPierData2*> vPiers(GetPiers());
   IndexType nPiers = vPiers.size();
   std::vector<const CTemporarySupportData*> vTS(GetTemporarySupports());
   IndexType nTS = vTS.size();

   std::vector<const CTemporarySupportData*> vTowers = vTS;
   vTowers.erase(std::remove_if(vTowers.begin(), vTowers.end(), RemoveStrongbacksSupports), vTowers.end());
   IndexType nTowers = vTowers.size();
   IndexType nStrongBacks = nTS - nTowers; // only two types

   if ( nPiers == 0 && nTowers == 0 && nStrongBacks == 2)
   {
      // segment is supported only by 2 strongbacks. Both ends are supported by adjacent segments
      return pgsTypes::ditYesFreeBothEnds;
   }
   else if ( nPiers == 1 && nTowers==0 && nStrongBacks >= 1 )
   {
      // segment is supported only by one pier and one strongback...
      // it could be a "drop-in" in the sense that one Free end is suspended by an adjacent segment
      const CPierData2* pPier1;
      const CTemporarySupportData* pTS1;
      GetSupport(pgsTypes::metStart,&pPier1,&pTS1);

      const CPierData2* pPier2;
      const CTemporarySupportData* pTS2;
      GetSupport(pgsTypes::metEnd,&pPier2,&pTS2);

      // Determine which end is free
      if (pPier1 != nullptr)
      {
         if (GetPrevSegment() == nullptr || pPier1->GetClosureJoint(0) != nullptr && pTS2 != nullptr)
         {
            return pgsTypes::ditYesFreeEndEnd;
         }
      }
      else if (pPier2 != nullptr)
      {
         if (GetNextSegment() == nullptr || pPier2->GetClosureJoint(0) != nullptr && pTS1 != nullptr)
         {
            return pgsTypes::ditYesFreeStartEnd;
         }
      }
   }
   else if (nPiers == 1 && nTowers == 1 && nStrongBacks == 0)
   {
      // segment is supported only by one pier and one tower...
      // it could be a "drop-in" in the sense that one Free end is suspended by the adjacent segment that has full fixity at the tower end 
      const CPierData2* pPier1;
      const CTemporarySupportData* pTS1;
      GetSupport(pgsTypes::metStart, &pPier1, &pTS1);

      const CPierData2* pPier2;
      const CTemporarySupportData* pTS2;
      GetSupport(pgsTypes::metEnd, &pPier2, &pTS2);

      // Determine which end is free
      if (pPier1 != nullptr && pTS2 != nullptr)
      {
         // pier on left, tower on right. See if adjacent segment at tower is fixed
         ATLASSERT(pTS2->GetSupportType() == pgsTypes::ErectionTower);
         if (pTS2->GetClosureJoint(0) != nullptr )
         {
            // consider segment stabley fixed if adequate rigid supports
            const CPrecastSegmentData* pNextSeg = this->GetNextSegment();
            const std::vector<const CPierData2*> vPiers = pNextSeg->GetPiers();
            IndexType nNextPiers = vPiers.size();
            if (nNextPiers > 1)
            {
               return pgsTypes::ditYesFreeEndEnd; // 2 or more piers will fix adjacent segment
            }
            else if (nNextPiers==1)
            {
               std::vector<const CTemporarySupportData*> vTS = pNextSeg->GetTemporarySupports();
               vTS.erase(std::remove_if(vTS.begin(), vTS.end(), RemoveStrongbacksSupports), vTS.end());
               if (vTS.size() > 1)
               {
                  return pgsTypes::ditYesFreeEndEnd; // 1 pier plus 2 or more towers will fix segment
               }
            }
         }
      }
      else if (pPier2 != nullptr && pTS1 != nullptr)
      {
         // pier on right, tower on left
         ATLASSERT(pTS1->GetSupportType() == pgsTypes::ErectionTower);
         if (pTS1->GetClosureJoint(0) != nullptr)
         {
            // consider segment stabley fixed if adequate rigid supports
            const CPrecastSegmentData* pPrevSeg = this->GetPrevSegment();
            const std::vector<const CPierData2*> vPiers = pPrevSeg->GetPiers();
            IndexType nPrevPiers = vPiers.size();
            if (nPrevPiers > 1)
            {
               return pgsTypes::ditYesFreeStartEnd; // 2 or more piers will fix adjacent segment
            }
            else if (nPrevPiers == 1)
            {
               std::vector<const CTemporarySupportData*> vTS = pPrevSeg->GetTemporarySupports();
               vTS.erase(std::remove_if(vTS.begin(), vTS.end(), RemoveStrongbacksSupports), vTS.end());
               if (vTS.size() > 1)
               {
                  return pgsTypes::ditYesFreeStartEnd; // 1 pier plus 2 or more towers will fix segment
               }
            }
         }
      }
   }
   else if (nPiers == 0 && nTowers == 1 && nStrongBacks >= 1)
   {
      // segment is supported only by one erection tower and one strongback...
      // it could be a "drop-in" in the sense that one Free end is suspended by an adjacent segment
      const CPierData2* pPier1;
      const CTemporarySupportData* pTS1;
      GetSupport(pgsTypes::metStart, &pPier1, &pTS1);

      const CPierData2* pPier2;
      const CTemporarySupportData* pTS2;
      GetSupport(pgsTypes::metEnd, &pPier2, &pTS2);

      // Determine which end is free, if any
      if (pTS1 != nullptr && pTS1->GetClosureJoint(0) != nullptr)
      {
         if ( pTS2 != nullptr && pTS2->GetSupportType() == pgsTypes::StrongBack)
         {
            return pgsTypes::ditYesFreeEndEnd;
         }
      }
      else if (pTS2 != nullptr && pTS2->GetClosureJoint(0) != nullptr)
      {
         if (pTS1 != nullptr && pTS1->GetSupportType() == pgsTypes::StrongBack)
         {
            return pgsTypes::ditYesFreeStartEnd;
         }
      }
   }

   return pgsTypes::ditNotDropIn;
}

bool CPrecastSegmentData::IsPropped() const
{
   std::vector<const CPierData2*> vPiers(GetPiers());
   std::vector<const CTemporarySupportData*> vTS(GetTemporarySupports());
   IndexType nPiers = vPiers.size();
   IndexType nTS = vTS.size();
   if (nPiers == 2 && nTS == 1)
   {
      const CPierData2* pPier1;
      const CTemporarySupportData* pTS1;
      GetSupport(pgsTypes::metStart, &pPier1, &pTS1);

      const CPierData2* pPier2;
      const CTemporarySupportData* pTS2;
      GetSupport(pgsTypes::metEnd, &pPier2, &pTS2);

      const CTemporarySupportData* pTS = (pTS1 == nullptr ? pTS2 : pTS1);
      ATLASSERT(pTS);
      return pTS->GetSupportType() == pgsTypes::StrongBack ? true : false;
   }
   return false;
}

CSegmentKey CPrecastSegmentData::GetSegmentKey() const
{
   CSegmentKey segmentKey(INVALID_INDEX,INVALID_INDEX,GetIndex());
   
   if ( m_pGirder )
   {
      segmentKey.girderIndex = m_pGirder->GetIndex();
      segmentKey.groupIndex  = m_pGirder->GetGirderGroupIndex();
   }

   return segmentKey;
}

void CPrecastSegmentData::SetSlabOffset(pgsTypes::MemberEndType end, Float64 slabOffset)
{
   m_SlabOffset[end] = slabOffset;
}

void CPrecastSegmentData::SetSlabOffset(Float64 start, Float64 end)
{
   m_SlabOffset[pgsTypes::metStart] = start;
   m_SlabOffset[pgsTypes::metEnd] = end;
}

Float64 CPrecastSegmentData::GetSlabOffset(pgsTypes::MemberEndType end,bool bRawData) const
{
   if (bRawData || m_pGirder == nullptr || m_pGirder->GetGirderGroup() == nullptr || m_pGirder->GetGirderGroup()->GetBridgeDescription() == nullptr)
   {
      return m_SlabOffset[end];
   }
   else
   {
      const CBridgeDescription2* pBridge = m_pGirder->GetGirderGroup()->GetBridgeDescription();
      ATLASSERT(pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber); // Should be asking if this is not the case

      switch (pBridge->GetSlabOffsetType())
      {
         case pgsTypes::sotBridge:
            return pBridge->GetSlabOffset();

         case pgsTypes::sotBearingLine:
         {
            const CPierData2* pPier;
            const CTemporarySupportData* pTS;
            this->GetSupport(end, &pPier, &pTS);
            pgsTypes::PierFaceType face = (end == pgsTypes::metStart ? pgsTypes::Ahead : pgsTypes::Back);
            if (pPier)
            {
               return pPier->GetSlabOffset(face);
            }
            else
            {
               // Function call below is for older versions of PGSplice only. Slab offset data was removed in 2023
               Float64 backA,aheadA;
               pTS->GetSlabOffsetPrivate(&backA, &aheadA);
               return face == pgsTypes::Ahead ? aheadA : backA;
            }
         }
         case pgsTypes::sotSegment:
            return m_SlabOffset[end];

         default:
            ATLASSERT(false); // should never get here
            return m_SlabOffset[end];
      }
   }
}

void CPrecastSegmentData::GetSlabOffset(Float64* pStart, Float64* pEnd,bool bRawData) const
{
   *pStart = GetSlabOffset(pgsTypes::metStart, bRawData);
   *pEnd = GetSlabOffset(pgsTypes::metEnd, bRawData);
}

CPrecastSegmentData& CPrecastSegmentData::operator = (const CPrecastSegmentData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
      AdjustAdjacentSegment();
   }

   return *this;
}

void CPrecastSegmentData::CopySegmentData(const CPrecastSegmentData* pSegment,bool bCopyLocation)
{
   MakeCopy(*pSegment,false,bCopyLocation,true);
   AdjustAdjacentSegment();
}

void CPrecastSegmentData::CopyMaterialFrom(const CPrecastSegmentData& rOther)
{
   Material = rOther.Material;
}

void CPrecastSegmentData::CopyPrestressingFrom(const CPrecastSegmentData& rOther)
{
   Strands = rOther.Strands;
   Tendons = rOther.Tendons;
}

void CPrecastSegmentData::CopyLongitudinalReinforcementFrom(const CPrecastSegmentData& rOther)
{
   LongitudinalRebarData = rOther.LongitudinalRebarData;
}

void CPrecastSegmentData::CopyTransverseReinforcementFrom(const CPrecastSegmentData& rOther)
{
   ShearData             = rOther.ShearData;
}

void CPrecastSegmentData::CopyHandlingFrom(const CPrecastSegmentData& rOther)
{
   HandlingData          = rOther.HandlingData;
}

void CPrecastSegmentData::CopyVariationFrom(const CPrecastSegmentData& rOther)
{
   m_VariationType = rOther.m_VariationType;
   for ( int i = 0; i < 4; i++ )
   {
      m_VariationLength[i]            = rOther.m_VariationLength[i];
      m_VariationHeight[i]            = rOther.m_VariationHeight[i];
      m_VariationBottomFlangeDepth[i] = rOther.m_VariationBottomFlangeDepth[i];
   }

   m_bVariableBottomFlangeDepthEnabled = rOther.m_bVariableBottomFlangeDepthEnabled;

   for ( int j = 0; j < 2; j++ )
   {
      EndBlockLength[j]           = rOther.EndBlockLength[j];
      EndBlockTransitionLength[j] = rOther.EndBlockTransitionLength[j];
      EndBlockWidth[j]            = rOther.EndBlockWidth[j];
   }
}

bool CPrecastSegmentData::operator==(const CPrecastSegmentData& rOther) const
{
   if ( m_VariationType != rOther.m_VariationType )
   {
      return false;
   }

   for ( int i = 0; i < 4; i++ )
   {
      if ( m_VariationLength[i] != rOther.m_VariationLength[i] )
      {
         return false;
      }

      if ( m_VariationHeight[i] != rOther.m_VariationHeight[i] )
      {
         return false;
      }

      if ( m_bVariableBottomFlangeDepthEnabled )
      {
         if ( m_VariationBottomFlangeDepth[i] != rOther.m_VariationBottomFlangeDepth[i] )
         {
            return false;
         }
      }
   }

   if ( m_bVariableBottomFlangeDepthEnabled != rOther.m_bVariableBottomFlangeDepthEnabled )
   {
      return false;
   }

   for ( int j = 0; j < 2; j++ )
   {
      if ( !IsEqual(EndBlockLength[j],rOther.EndBlockLength[j]) )
      {
         return false;
      }

      if ( !IsEqual(EndBlockTransitionLength[j],rOther.EndBlockTransitionLength[j]) )
      {
         return false;
      }

      if ( !IsEqual(EndBlockWidth[j],rOther.EndBlockWidth[j]) )
      {
         return false;
      }
   }

   if (m_pGirder)
   {
      if (m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         if (m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment)
         {
            if (!IsEqual(GetSlabOffset(pgsTypes::metStart),rOther.GetSlabOffset(pgsTypes::metStart)) || !IsEqual(GetSlabOffset(pgsTypes::metEnd),rOther.GetSlabOffset(pgsTypes::metEnd)))
      {
         return false;
      }
   }
      }
      else
      {
         // direct input of haunch
         if (m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetHaunchInputLocationType() != pgsTypes::hilSame4Bridge)
         {
            if (m_vHaunchDepths.size() != rOther.m_vHaunchDepths.size())
            {
               return false;
            }

            std::vector<Float64>::const_iterator ithis = m_vHaunchDepths.begin();
            std::vector<Float64>::const_iterator iother = rOther.m_vHaunchDepths.begin();
            while (ithis != m_vHaunchDepths.end())
            {
               if (!IsEqual(*ithis,*iother))
               {
                  return false;
               }

               ithis++;
               iother++;
            }
         }
      }
   }

   if ( Strands != rOther.Strands )
   {
      return false;
   }

   if (Tendons != rOther.Tendons)
   {
      return false;
   }
   
   if ( Material != rOther.Material )
   {
      return false;
   }
   
   if ( ShearData != rOther.ShearData )
   {
      return false;
   }
   
   if ( LongitudinalRebarData != rOther.LongitudinalRebarData )
   {
      return false;
   }
   
   if ( HandlingData != rOther.HandlingData )
   {
      return false;
   }

   if (TopFlangeThickeningType != rOther.TopFlangeThickeningType)
   {
      return false;
   }

   if (TopFlangeThickeningType != pgsTypes::tftNone && !IsEqual(TopFlangeThickening,rOther.TopFlangeThickening))
   {
      return false;
   }

   if (!IsEqual(Precamber, rOther.Precamber))
   {
      return false;
   }

   
   return true;
}

bool CPrecastSegmentData::operator!=(const CPrecastSegmentData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CPrecastSegmentData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;
   CComVariant var;
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("PrecastSegment"));
      Float64 version;
      pStrLoad->get_Version(&version);

      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"), &var);
      m_SegmentID = VARIANT2ID(var);
      ATLASSERT(m_SegmentID != INVALID_ID);
      m_pGirder->GetGirderGroup()->GetBridgeDescription()->UpdateNextSegmentID(m_SegmentID);

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("StartSpan"), &var);
      SpanIndexType startSpanIdx = VARIANT2INDEX(var);

      hr = pStrLoad->get_Property(_T("EndSpan"), &var);
      SpanIndexType endSpanIdx = VARIANT2INDEX(var);

      if (2 < version)
      {
         // added in version 3
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("StartSlabOffset"), &var);
         m_SlabOffset[pgsTypes::metStart] = var.dblVal;

         hr = pStrLoad->get_Property(_T("EndSlabOffset"), &var);
         m_SlabOffset[pgsTypes::metEnd] = var.dblVal;
      }

      if ( m_pGirder && m_pGirder->GetGirderGroup() && m_pGirder->GetGirderGroup()->GetBridgeDescription() )
      {
         // resolve references now
         m_pSpanData[pgsTypes::metStart] = m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSpan(startSpanIdx);
         m_pSpanData[pgsTypes::metEnd]   = m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSpan(endSpanIdx);

         m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
         m_SpanIdx[pgsTypes::metEnd]   = INVALID_INDEX;
      }
      else
      {
         // resolve references later
         m_pSpanData[pgsTypes::metStart] = nullptr;
         m_pSpanData[pgsTypes::metEnd]   = nullptr;

         m_SpanIdx[pgsTypes::metStart] = startSpanIdx;
         m_SpanIdx[pgsTypes::metEnd]   = endSpanIdx;
      }


      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("SegmentVariation"),&var);
      CString strVariationType(OLE2T(var.bstrVal));
      if ( strVariationType == _T("None") )
      {
         m_VariationType = pgsTypes::svtNone;
      }
      else if (strVariationType == _T("Linear") || strVariationType == _T("Parabolic") ) 
      {
         m_VariationType = (strVariationType == _T("Linear") ? pgsTypes::svtLinear : pgsTypes::svtParabolic);
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("LeftPrismaticLength"),&var);
         m_VariationLength[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftPrismaticHeight"),&var);
         m_VariationHeight[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftPrismaticBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticLength"),&var);
         m_VariationLength[pgsTypes::sztRightPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticHeight"),&var);
         m_VariationHeight[pgsTypes::sztRightPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic] = var.dblVal;

         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("VariableBottomFlangeDepthEnabled"),&var);
         m_bVariableBottomFlangeDepthEnabled = (var.boolVal == VARIANT_TRUE ? true : false);
      }
      else if (strVariationType == _T("DoubleLinear") || strVariationType == _T("DoubleParabolic")) 
      {
         m_VariationType = (strVariationType == _T("DoubleLinear") ? pgsTypes::svtDoubleLinear : pgsTypes::svtDoubleParabolic);
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("LeftPrismaticLength"),&var);
         m_VariationLength[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftPrismaticHeight"),&var);
         m_VariationHeight[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftPrismaticBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftTaperedLength"),&var);
         m_VariationLength[pgsTypes::sztLeftTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftTaperedHeight"),&var);
         m_VariationHeight[pgsTypes::sztLeftTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("LeftTaperedBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztLeftTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightTaperedLength"),&var);
         m_VariationLength[pgsTypes::sztRightTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightTaperedHeight"),&var);
         m_VariationHeight[pgsTypes::sztRightTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightTaperedBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztRightTapered] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticLength"),&var);
         m_VariationLength[pgsTypes::sztRightPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticHeight"),&var);
         m_VariationHeight[pgsTypes::sztRightPrismatic] = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightPrismaticBottomFlangeDepth"),&var);
         m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic] = var.dblVal;

         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("VariableBottomFlangeDepthEnabled"),&var);
         m_bVariableBottomFlangeDepthEnabled = (var.boolVal == VARIANT_TRUE ? true : false);
      }
      else if (strVariationType == _T("General")) 
      {
         ATLASSERT(false); // not implemented
      }
      else
      {
         // bad input
         ATLASSERT(false); // bad input
      }

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LeftEndBlockLength"),&var);
      EndBlockLength[pgsTypes::metStart] = var.dblVal;

      hr = pStrLoad->get_Property(_T("LeftEndBlockTransition"),&var);
      EndBlockTransitionLength[pgsTypes::metStart] = var.dblVal;

      hr = pStrLoad->get_Property(_T("LeftEndBlockWidth"),&var);
      EndBlockWidth[pgsTypes::metStart] = var.dblVal;

      hr = pStrLoad->get_Property(_T("RightEndBlockLength"),&var);
      EndBlockLength[pgsTypes::metEnd] = var.dblVal;

      hr = pStrLoad->get_Property(_T("RightEndBlockTransition"),&var);
      EndBlockTransitionLength[pgsTypes::metEnd] = var.dblVal;

      hr = pStrLoad->get_Property(_T("RightEndBlockWidth"),&var);
      EndBlockWidth[pgsTypes::metEnd] = var.dblVal;


      Float64 strand_version;
      hr = Strands.Load(pStrLoad,pProgress,&strand_version);
      if (3 < version)
      {
         // added in version 4
         hr = Tendons.Load(pStrLoad, pProgress);
      }
      hr = Material.Load(pStrLoad,pProgress);

      CStructuredLoad load(pStrLoad);
      hr = ShearData.Load(&load);

      hr = LongitudinalRebarData.Load(pStrLoad,pProgress);
      hr = HandlingData.Load(pStrLoad,pProgress);

      if (1 < version)
      {
         // added in version 2
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Precamber"), &var);
         Precamber = var.dblVal;

         hr = pStrLoad->BeginUnit(_T("TopFlangeThickening"));

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("Type"), &var);
         TopFlangeThickeningType = (pgsTypes::TopFlangeThickeningType)var.lVal;

         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Value"), &var);
         TopFlangeThickening = var.dblVal;

         hr = pStrLoad->EndUnit(); // TopFlangeThickening
      }

      if (4 < version)
      {
         m_vHaunchDepths.clear();
         pStrLoad->BeginUnit(_T("HaunchDepthsPerSegment"));

         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("nValues"),&var);
         IndexType nvals = VARIANT2INDEX(var);
         var.vt = VT_R8;
         for (IndexType iv = 0; iv < nvals; iv++)
         {
            hr = pStrLoad->get_Property(_T("HaunchDepthVal"),&var);
            m_vHaunchDepths.push_back(var.dblVal);
         }

         pStrLoad->EndUnit(); // HaunchDepthsPerSegment
      }

      hr = pStrLoad->EndUnit(); // PrecastSegment
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return S_OK;
      
}

HRESULT CPrecastSegmentData::Save(IStructuredSave* pStrSave, IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("PrecastSegment"), 5.0);

   pStrSave->put_Property(_T("ID"), CComVariant(m_SegmentID));

   pStrSave->put_Property(_T("StartSpan"), CComVariant(m_pSpanData[pgsTypes::metStart]->GetIndex()));
   pStrSave->put_Property(_T("EndSpan"), CComVariant(m_pSpanData[pgsTypes::metEnd]->GetIndex()));

   // added in version 3
   pStrSave->put_Property(_T("StartSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::metStart]));
   pStrSave->put_Property(_T("EndSlabOffset"), CComVariant(m_SlabOffset[pgsTypes::metEnd]));

   switch(m_VariationType)
   {
   case pgsTypes::svtNone:
      pStrSave->put_Property(_T("SegmentVariation"),CComVariant(_T("None")));
      break;

   case pgsTypes::svtLinear:
      pStrSave->put_Property(_T("SegmentVariation"),CComVariant(_T("Linear")));
      pStrSave->put_Property(_T("LeftPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("VariableBottomFlangeDepthEnabled"),CComVariant(m_bVariableBottomFlangeDepthEnabled));
      break;

   case pgsTypes::svtParabolic:
      pStrSave->put_Property(_T("SegmentVariation"),CComVariant(_T("Parabolic")));
      pStrSave->put_Property(_T("LeftPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("VariableBottomFlangeDepthEnabled"),CComVariant(m_bVariableBottomFlangeDepthEnabled));
      break;

   case pgsTypes::svtDoubleLinear:
      pStrSave->put_Property(_T("SegmentVariation"),CComVariant(_T("DoubleLinear")));
      pStrSave->put_Property(_T("LeftPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic]));

      pStrSave->put_Property(_T("LeftTaperedLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftTapered]));
      pStrSave->put_Property(_T("LeftTaperedHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftTapered]));
      pStrSave->put_Property(_T("LeftTaperedBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftTapered]));

      pStrSave->put_Property(_T("RightTaperedLength"),CComVariant(m_VariationLength[pgsTypes::sztRightTapered]));
      pStrSave->put_Property(_T("RightTaperedHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightTapered]));
      pStrSave->put_Property(_T("RightTaperedBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightTapered]));

      pStrSave->put_Property(_T("RightPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic]));

      pStrSave->put_Property(_T("VariableBottomFlangeDepthEnabled"),CComVariant(m_bVariableBottomFlangeDepthEnabled));
      break;

   case pgsTypes::svtDoubleParabolic:
      pStrSave->put_Property(_T("SegmentVariation"),CComVariant(_T("DoubleParabolic")));
      pStrSave->put_Property(_T("LeftPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftPrismatic]));
      pStrSave->put_Property(_T("LeftPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic]));

      pStrSave->put_Property(_T("LeftTaperedLength"),CComVariant(m_VariationLength[pgsTypes::sztLeftTapered]));
      pStrSave->put_Property(_T("LeftTaperedHeight"),CComVariant(m_VariationHeight[pgsTypes::sztLeftTapered]));
      pStrSave->put_Property(_T("LeftTaperedBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztLeftTapered]));

      pStrSave->put_Property(_T("RightTaperedLength"),CComVariant(m_VariationLength[pgsTypes::sztRightTapered]));
      pStrSave->put_Property(_T("RightTaperedHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightTapered]));
      pStrSave->put_Property(_T("RightTaperedBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightTapered]));

      pStrSave->put_Property(_T("RightPrismaticLength"),CComVariant(m_VariationLength[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticHeight"),CComVariant(m_VariationHeight[pgsTypes::sztRightPrismatic]));
      pStrSave->put_Property(_T("RightPrismaticBottomFlangeDepth"),CComVariant(m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic]));

      pStrSave->put_Property(_T("VariableBottomFlangeDepthEnabled"),CComVariant(m_bVariableBottomFlangeDepthEnabled));
      break;

   //case CPrecastSegmentData::General:
   //   ATLASSERT(false); // not implemented yet
   //   break;

   default:
      ATLASSERT(false); // is there a new varation type?
      break;
   }

   pStrSave->put_Property(_T("LeftEndBlockLength"),CComVariant(EndBlockLength[pgsTypes::metStart]));
   pStrSave->put_Property(_T("LeftEndBlockTransition"),CComVariant(EndBlockTransitionLength[pgsTypes::metStart]));
   pStrSave->put_Property(_T("LeftEndBlockWidth"),CComVariant(EndBlockWidth[pgsTypes::metStart]));

   pStrSave->put_Property(_T("RightEndBlockLength"),CComVariant(EndBlockLength[pgsTypes::metEnd]));
   pStrSave->put_Property(_T("RightEndBlockTransition"),CComVariant(EndBlockTransitionLength[pgsTypes::metEnd]));
   pStrSave->put_Property(_T("RightEndBlockWidth"),CComVariant(EndBlockWidth[pgsTypes::metEnd]));

   Strands.Save(pStrSave,pProgress);
   Tendons.Save(pStrSave, pProgress); // added in version 4
   Material.Save(pStrSave,pProgress);

   CStructuredSave save(pStrSave);
   ShearData.Save(&save);

   LongitudinalRebarData.Save(pStrSave,pProgress);
   HandlingData.Save(pStrSave,pProgress);

   // added in version 2
   pStrSave->put_Property(_T("Precamber"), CComVariant(Precamber));

   // added in version 2
   pStrSave->BeginUnit(_T("TopFlangeThickening"),1.0);
      pStrSave->put_Property(_T("Type"), CComVariant(TopFlangeThickeningType));
      pStrSave->put_Property(_T("Value"), CComVariant(TopFlangeThickening));
   pStrSave->EndUnit(); // TopFlangeThickening

   // added in version 5
   pStrSave->BeginUnit(_T("HaunchDepthsPerSegment"),1.0);
   pStrSave->put_Property(_T("nValues"),CComVariant(m_vHaunchDepths.size()));
   for (const auto& vhaunch : m_vHaunchDepths)
   {
         pStrSave->put_Property(_T("HaunchDepthVal"),CComVariant(vhaunch));
   }
   pStrSave->EndUnit(); // HaunchDepths

   pStrSave->EndUnit(); // PrecastSegment

   return S_OK;
}

void CPrecastSegmentData::MakeCopy(const CPrecastSegmentData& rOther,bool bCopyIdentity,bool bCopyLocation,bool bCopyProperties)
{
   if ( bCopyIdentity )
   {
      m_SegmentIndex = rOther.m_SegmentIndex;
      m_SegmentID    = rOther.m_SegmentID;
   }

   if ( bCopyLocation )
   {
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(i);
         if ( rOther.m_pSpanData[endType] )
         {
            m_pSpanData[endType] = nullptr;
            m_SpanIdx[endType] = rOther.m_pSpanData[endType]->GetIndex();
         }
         else
         {
            m_pSpanData[endType] = nullptr;
            m_SpanIdx[endType] = rOther.m_SpanIdx[endType];
         }
      }
   }

   if ( bCopyProperties )
   {
      CopyMaterialFrom(rOther);
      CopyPrestressingFrom(rOther);
      CopyLongitudinalReinforcementFrom(rOther);
      CopyTransverseReinforcementFrom(rOther);
      CopyHandlingFrom(rOther);
      CopyVariationFrom(rOther);

      TopFlangeThickeningType = rOther.TopFlangeThickeningType;
      TopFlangeThickening = rOther.TopFlangeThickening;

      Precamber = rOther.Precamber;

      m_SlabOffset = rOther.m_SlabOffset;

      m_vHaunchDepths = rOther.m_vHaunchDepths;
   }

   ResolveReferences();

   PGS_ASSERT_VALID;
}

void CPrecastSegmentData::MakeAssignment(const CPrecastSegmentData& rOther)
{
   MakeCopy( rOther,true,true,true);
}

Float64 CPrecastSegmentData::GetSegmentHeight(bool bSegmentHeight) const
{
   if ( bSegmentHeight && m_bHeightComputed )
   {
      return m_Height;
   }

   if ( !bSegmentHeight && m_bBottomFlangeThicknessComputed )
   {
      return m_BottomFlangeThickness;
   }

   // Gets the segment height based on the data in the girder library entry
   const GirderLibraryEntry* pGdrEntry = GetGirder()->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   CComPtr<IGirderSection> gdrSection;
   factory->CreateGirderSection(pBroker,INVALID_ID,pGdrEntry->GetDimensions(),-1,-1,&gdrSection);
   Float64 height;
   if ( bSegmentHeight )
   {
      m_bHeightComputed = true;
      gdrSection->get_OverallHeight(&m_Height);
      height = m_Height;
   }
   else
   {
      m_bBottomFlangeThicknessComputed = true;
      gdrSection->get_MinBottomFlangeThickness(&m_BottomFlangeThickness);
      height = m_BottomFlangeThickness;
   }

   return height;
}

void CPrecastSegmentData::AdjustAdjacentSegment()
{
   // Force the ends of this segment and the adjacent segments to match in height
   CPrecastSegmentData* pPrevSegment = nullptr;
   CPrecastSegmentData* pNextSegment = nullptr;
   if (m_pClosure[pgsTypes::metStart])
   {
      pPrevSegment = m_pClosure[pgsTypes::metStart]->GetLeftSegment();
   }

   if (m_pClosure[pgsTypes::metEnd])
   {
      pNextSegment = m_pClosure[pgsTypes::metEnd]->GetRightSegment();
   }

   if ( pPrevSegment )
   {
      // the right end of the previous segment must match the left end of this segment
      pPrevSegment->m_VariationHeight[pgsTypes::sztRightPrismatic]            = GetVariationHeight(pgsTypes::sztLeftPrismatic);
      pPrevSegment->m_VariationBottomFlangeDepth[pgsTypes::sztRightPrismatic] = GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic);
   }


   if ( pNextSegment )
   {
      // the left end of the next segment must match the right end of this segment
      pNextSegment->m_VariationHeight[pgsTypes::sztLeftPrismatic]            = GetVariationHeight(pgsTypes::sztRightPrismatic);
      pNextSegment->m_VariationBottomFlangeDepth[pgsTypes::sztLeftPrismatic] = GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic);
   }

}

void CPrecastSegmentData::ResolveReferences()
{
   if ( m_pGirder == nullptr )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup() == nullptr )
   {
      return;
   }

   CBridgeDescription2* pBridge = m_pGirder->GetGirderGroup()->GetBridgeDescription();
   if ( pBridge == nullptr )
   {
      return;
   }

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(i);
      if ( m_SpanIdx[endType] != INVALID_INDEX )
      {
         m_pSpanData[endType] = pBridge->GetSpan(m_SpanIdx[endType]);
         m_SpanIdx[endType] = INVALID_INDEX;
      }
   }
}

void CPrecastSegmentData::SetIndex(SegmentIndexType segIdx)
{
   m_SegmentIndex = segIdx;
}

void CPrecastSegmentData::SetID(SegmentIDType segID)
{
   m_SegmentID = segID;
}

LPCTSTR CPrecastSegmentData::GetSegmentVariation(pgsTypes::SegmentVariationType type)
{
   switch(type)
   {
   case pgsTypes::svtLinear:
      return _T("Linear");
      
   case pgsTypes::svtParabolic:
      return _T("Parabolic");

   case pgsTypes::svtDoubleLinear:
      return _T("Double Linear");

   case pgsTypes::svtDoubleParabolic:
      return _T("Double Parabolic");

   case pgsTypes::svtNone:
      return _T("None");

   //case General:
   //   return _T("General");

   default:
      ATLASSERT(false); // is there a new variation type?
   }

   return nullptr;
}

void CPrecastSegmentData::SetDirectHaunchDepths(const std::vector<Float64>& HaunchDepths)
{
   m_vHaunchDepths = HaunchDepths;
}

std::vector<Float64> CPrecastSegmentData::GetDirectHaunchDepths(bool bGetRawValue) const
{
   if (bGetRawValue || m_pGirder == nullptr || m_pGirder->GetGirderGroup() == nullptr || m_pGirder->GetGirderGroup()->GetBridgeDescription() == nullptr)
   {
      ATLASSERT(m_vHaunchDepths.size()>0 || bGetRawValue); // someone upstream should have set our data
      return m_vHaunchDepths;
   }
   else
   {
      const CBridgeDescription2* pBridge = m_pGirder->GetGirderGroup()->GetBridgeDescription();
      ATLASSERT(pBridge->GetHaunchInputDepthType() != pgsTypes::hidACamber || pBridge->GetHaunchLayoutType() != pgsTypes::hltAlongSpans); // Should not be asking if this is the case

      if (pBridge->GetHaunchInputLocationType() == pgsTypes::hilPerEach)
      {
         ATLASSERT(!m_vHaunchDepths.empty()); // someone upstream should have set our data
         return m_vHaunchDepths;
      }
      else
      {
         return m_pGirder->GetDirectHaunchDepths(this->GetIndex(),bGetRawValue);
      }
   }
}

#if defined _DEBUG
#include <PGSuperTypes.h>
#undef _DEBUG
#include <IFace\Bridge.h>
#define _DEBUG
void CPrecastSegmentData::AssertValid()
{
   // if any of these first 3 checks are nullptr, then this segment isn't an a structure so it can't be validated
   if ( m_pGirder == nullptr )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup() == nullptr )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup()->GetBridgeDescription() == nullptr )
   {
      return;
   }

   ATLASSERT(m_SegmentID != INVALID_ID);

   // Make sure the segment is not "reversed"
   Float64 startStation, endStation;
   GetStations(&startStation,&endStation);
   ATLASSERT( startStation < endStation );

   Strands.AssertValid();
   Tendons.AssertValid();
   ShearData.AssertValid();
   LongitudinalRebarData.AssertValid();
   HandlingData.AssertValid();
}
#endif
