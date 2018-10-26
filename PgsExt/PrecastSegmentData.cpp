///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

   m_pSpanData[pgsTypes::metStart] = NULL;
   m_pSpanData[pgsTypes::metEnd]   = NULL;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd] = INVALID_INDEX;

   m_pLeftClosure = NULL;
   m_pRightClosure = NULL;
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
   m_SegmentIndex = INVALID_INDEX;
   m_SegmentID    = INVALID_ID;

   m_VariationType = pgsTypes::svtLinear;
   for ( int i = 0; i < 4; i++ )
   {
      m_VariationLength[i] = 0;
      m_VariationHeight[i] = 0;
      m_VariationBottomFlangeDepth[i] = 0;
   }

   m_bVariableBottomFlangeDepthEnabled = false;

   m_VariationHeight[0] = ::ConvertToSysUnits(10,unitMeasure::Feet);
   m_VariationHeight[3] = m_VariationHeight[0];

   for ( int j = 0; j < 2; j++ )
   {
      EndBlockLength[j] = 0;
      EndBlockTransitionLength[j] = 0;
      EndBlockWidth[j] = 0;
   }

   m_pGirder = NULL;

   m_pSpanData[pgsTypes::metStart] = NULL;
   m_pSpanData[pgsTypes::metEnd]   = NULL;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd] = INVALID_INDEX;

   m_pLeftClosure = NULL;
   m_pRightClosure = NULL;
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

void CPrecastSegmentData::SetSpans(const CSpanData2* pStartSpan,const CSpanData2* pEndSpan)
{
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pStartSpan->GetBridgeDescription() );
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pEndSpan->GetBridgeDescription() );

   m_pSpanData[pgsTypes::metStart] = pStartSpan;
   m_pSpanData[pgsTypes::metEnd]   = pEndSpan;

   m_SpanIdx[pgsTypes::metStart] = INVALID_INDEX;
   m_SpanIdx[pgsTypes::metEnd]   = INVALID_INDEX;
}

void CPrecastSegmentData::SetSpan(pgsTypes::MemberEndType endType,const CSpanData2* pSpan)
{
   ATLASSERT( m_pGirder->GetGirderGroup()->GetBridgeDescription() == pSpan->GetBridgeDescription() );
   m_pSpanData[endType] = pSpan;
   m_SpanIdx[endType] = INVALID_INDEX;
}

const CSpanData2* CPrecastSegmentData::GetSpan(pgsTypes::MemberEndType endType) const
{
   return m_pSpanData[endType];
}

SpanIndexType CPrecastSegmentData::GetSpanIndex(pgsTypes::MemberEndType endType) const
{
   return (m_pSpanData[endType] != NULL ? m_pSpanData[endType]->GetIndex() : m_SpanIdx[endType]);
}

void CPrecastSegmentData::SetLeftClosure(CClosureJointData* pClosure)
{
   m_pLeftClosure = pClosure;
}

const CClosureJointData* CPrecastSegmentData::GetLeftClosure() const
{
   return m_pLeftClosure;
}

CClosureJointData* CPrecastSegmentData::GetLeftClosure()
{
   return m_pLeftClosure;
}

void CPrecastSegmentData::SetRightClosure(CClosureJointData* pClosure)
{
   m_pRightClosure = pClosure;
}

const CClosureJointData* CPrecastSegmentData::GetRightClosure() const
{
   return m_pRightClosure;
}

CClosureJointData* CPrecastSegmentData::GetRightClosure()
{
   return m_pRightClosure;
}

const CPrecastSegmentData* CPrecastSegmentData::GetPrevSegment() const
{
   return m_pLeftClosure == NULL ? NULL : m_pLeftClosure->GetLeftSegment();
}

const CPrecastSegmentData* CPrecastSegmentData::GetNextSegment() const
{
   return m_pRightClosure == NULL ? NULL : m_pRightClosure->GetRightSegment();
}

void CPrecastSegmentData::GetSupport(pgsTypes::MemberEndType endType,const CPierData2** ppPier,const CTemporarySupportData** ppTS) const
{
   *ppPier = NULL;
   *ppTS = NULL;
   if ( endType == pgsTypes::metStart )
   {
      if ( m_pLeftClosure )
      {
         if ( m_pLeftClosure->GetPier() )
         {
            *ppPier = m_pLeftClosure->GetPier();
         }
         else
         {
            *ppTS = m_pLeftClosure->GetTemporarySupport();
         }
      }
      else
      {
         *ppPier = m_pGirder->GetPier(pgsTypes::metStart);
      }
   }
   else
   {
      if ( m_pRightClosure )
      {
         if ( m_pRightClosure->GetPier() )
         {
            *ppPier = m_pRightClosure->GetPier();
         }
         else
         {
            *ppTS = m_pRightClosure->GetTemporarySupport();
         }
      }
      else
      {
         *ppPier = m_pGirder->GetPier(pgsTypes::metEnd);
      }
   }
}

void CPrecastSegmentData::GetSupport(pgsTypes::MemberEndType endType,CPierData2** ppPier,CTemporarySupportData** ppTS)
{
   *ppPier = NULL;
   *ppTS = NULL;
   if ( endType == pgsTypes::metStart )
   {
      if ( m_pLeftClosure )
      {
         if ( m_pLeftClosure->GetPier() )
         {
            *ppPier = m_pLeftClosure->GetPier();
         }
         else
         {
            *ppTS = m_pLeftClosure->GetTemporarySupport();
         }
      }
      else
      {
         *ppPier = m_pGirder->GetPier(pgsTypes::metStart);
      }
   }
   else
   {
      if ( m_pRightClosure )
      {
         if ( m_pRightClosure->GetPier() )
         {
            *ppPier = m_pRightClosure->GetPier();
         }
         else
         {
            *ppTS = m_pRightClosure->GetTemporarySupport();
         }
      }
      else
      {
         *ppPier = m_pGirder->GetPier(pgsTypes::metEnd);
      }
   }
}

void CPrecastSegmentData::GetStations(Float64* pStartStation,Float64* pEndStation) const
{
   const CPierData2* pPier = NULL;
   const CTemporarySupportData* pTS = NULL;
   GetSupport(pgsTypes::metStart,&pPier,&pTS);
   if ( pPier )
   {
      *pStartStation = pPier->GetStation();
   }
   else
   {
      *pStartStation = pTS->GetStation();
   }

   GetSupport(pgsTypes::metEnd,&pPier,&pTS);
   if ( pPier )
   {
      *pEndStation = pPier->GetStation();
   }
   else
   {
      *pEndStation = pTS->GetStation();
   }
}

void CPrecastSegmentData::GetSpacing(const CGirderSpacing2** ppStartSpacing,const CGirderSpacing2** ppEndSpacing) const
{
   if ( m_pLeftClosure )
   {
      if ( m_pLeftClosure->GetPier() )
      {
         *ppStartSpacing = m_pLeftClosure->GetPier()->GetGirderSpacing(pgsTypes::Ahead);
      }
      else
      {
         *ppStartSpacing = m_pLeftClosure->GetTemporarySupport()->GetSegmentSpacing();
      }
   }
   else
   {
      *ppStartSpacing = m_pGirder->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
   }

   if ( m_pRightClosure )
   {
      if ( m_pRightClosure->GetPier() )
      {
         *ppEndSpacing = m_pRightClosure->GetPier()->GetGirderSpacing(pgsTypes::Back);
      }
      else
      {
         *ppEndSpacing = m_pRightClosure->GetTemporarySupport()->GetSegmentSpacing();
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
   if ( m_pLeftClosure && m_pLeftClosure->GetPier() )
   {
      vPiers.push_back( m_pLeftClosure->GetPier() );
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
   if ( m_pRightClosure && m_pRightClosure->GetPier() )
   {
      vPiers.push_back( m_pRightClosure->GetPier() );
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
   if ( m_pLeftClosure && m_pLeftClosure->GetTemporarySupport() )
   {
      tempSupports.push_back( m_pLeftClosure->GetTemporarySupport() );
   }

   // get intermediate temporary supports
   Float64 startStation,endStation;
   GetStations(&startStation,&endStation);

   const CSpanData2* pSpan = GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan = GetSpan(pgsTypes::metEnd);

   do
   {
      std::vector<const CTemporarySupportData*> spanTempSupports(pSpan->GetTemporarySupports());
      std::vector<const CTemporarySupportData*>::iterator iter(spanTempSupports.begin());
      std::vector<const CTemporarySupportData*>::iterator iterEnd(spanTempSupports.end());
      for ( ; iter != iterEnd; iter++ )
      {
         const CTemporarySupportData* pTS = *iter;
         Float64 tsStation = pTS->GetStation();
         if ( startStation < tsStation && tsStation < endStation )
         {
            tempSupports.push_back(pTS);
         }
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   } while ( pSpan && pSpan->GetIndex() <= pEndSpan->GetIndex() );


   // get temporary support at end of segment (if there is one)
   if ( m_pRightClosure && m_pRightClosure->GetTemporarySupport() )
   {
      tempSupports.push_back( m_pRightClosure->GetTemporarySupport() );
   }

   return tempSupports;
}

bool CPrecastSegmentData::IsDropIn() const
{
   std::vector<const CPierData2*> vPiers(GetPiers());
   std::vector<const CTemporarySupportData*> vTS(GetTemporarySupports());
   if ( vPiers.size() == 0 && vTS.size() == 2 )
   {
      // segment is supported by exactly 2 temporary supports... there is a chance it 
      // could be a drop-in segment

      const CTemporarySupportData* pLeftTS = vTS.front();
      const CTemporarySupportData* pRightTS = vTS.back();

      if ( pLeftTS->GetSupportType() == pgsTypes::StrongBack && pRightTS->GetSupportType() == pgsTypes::StrongBack )
      {
         // the temporary supports are both strong backs... segment is a drop-in
         return true;
      }
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

   if ( Strands != rOther.Strands )
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

      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_SegmentID = VARIANT2ID(var);
      ATLASSERT(m_SegmentID != INVALID_ID);
      m_pGirder->GetGirderGroup()->GetBridgeDescription()->UpdateNextSegmentID(m_SegmentID);

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("StartSpan"),&var);
      SpanIndexType startSpanIdx = VARIANT2INDEX(var);

      hr = pStrLoad->get_Property(_T("EndSpan"),&var);
      SpanIndexType endSpanIdx = VARIANT2INDEX(var);

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
         m_pSpanData[pgsTypes::metStart] = NULL;
         m_pSpanData[pgsTypes::metEnd]   = NULL;

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


      Float64 version;
      hr = Strands.Load(pStrLoad,pProgress,&version);
      hr = Material.Load(pStrLoad,pProgress);

      CStructuredLoad load(pStrLoad);
      hr = ShearData.Load(&load);

      hr = LongitudinalRebarData.Load(pStrLoad,pProgress);
      hr = HandlingData.Load(pStrLoad,pProgress);

      hr = pStrLoad->EndUnit(); // PrecastSegment
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return S_OK;
      
}

HRESULT CPrecastSegmentData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("PrecastSegment"),1.0);

   pStrSave->put_Property(_T("ID"),CComVariant(m_SegmentID));

   pStrSave->put_Property(_T("StartSpan"),CComVariant(m_pSpanData[pgsTypes::metStart]->GetIndex()));
   pStrSave->put_Property(_T("EndSpan"),  CComVariant(m_pSpanData[pgsTypes::metEnd]->GetIndex()));

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
   Material.Save(pStrSave,pProgress);

   CStructuredSave save(pStrSave);
   ShearData.Save(&save);

   LongitudinalRebarData.Save(pStrSave,pProgress);
   HandlingData.Save(pStrSave,pProgress);

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
            m_pSpanData[endType] = NULL;
            m_SpanIdx[endType] = rOther.m_pSpanData[endType]->GetIndex();
         }
         else
         {
            m_pSpanData[endType] = NULL;
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
   }

   ResolveReferences();

   ASSERT_VALID;
}

void CPrecastSegmentData::MakeAssignment(const CPrecastSegmentData& rOther)
{
   MakeCopy( rOther,true,true,true);
}

Float64 CPrecastSegmentData::GetSegmentHeight(bool bSegmentHeight) const
{
   // Gets the segment height based on the data in the girder library entry
   const GirderLibraryEntry* pGdrEntry = GetGirder()->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

#pragma Reminder("UPDATE: this could be more efficient if the girder section was cached somewhere")
   // it is also kind of kludgy that we have to access the broker way down in a plain-old-data type
   // of class
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   CComPtr<IGirderSection> gdrSection;
   factory->CreateGirderSection(pBroker,INVALID_ID,pGdrEntry->GetDimensions(),-1,-1,&gdrSection);
   Float64 height;
   if ( bSegmentHeight )
   {
      gdrSection->get_GirderHeight(&height);
   }
   else
   {
      gdrSection->get_MinBottomFlangeThickness(&height);
   }

   return height;
}

void CPrecastSegmentData::AdjustAdjacentSegment()
{
   // Force the ends of this segment and the adjacent segments to match in height
   CPrecastSegmentData* pPrevSegment = NULL;
   CPrecastSegmentData* pNextSegment = NULL;
   if ( m_pLeftClosure )
   {
      pPrevSegment = m_pLeftClosure->GetLeftSegment();
   }

   if ( m_pRightClosure )
   {
      pNextSegment = m_pRightClosure->GetRightSegment();
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
   if ( m_pGirder == NULL )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup() == NULL )
   {
      return;
   }

   const CBridgeDescription2* pBridge = m_pGirder->GetGirderGroup()->GetBridgeDescription();
   if ( pBridge == NULL )
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

   return NULL;
}

#if defined _DEBUG
#include <PGSuperTypes.h>
#undef _DEBUG
#include <IFace\Bridge.h>
#define _DEBUG
void CPrecastSegmentData::AssertValid()
{
   // if any of these first 3 checks are NULL, then this segment isn't an a structure so it can't be validated
   if ( m_pGirder == NULL )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup() == NULL )
   {
      return;
   }

   if ( m_pGirder->GetGirderGroup()->GetBridgeDescription() == NULL )
   {
      return;
   }

   ATLASSERT(m_SegmentID != INVALID_ID);

   // Make sure the segment is not "reversed"
   Float64 startStation, endStation;
   GetStations(&startStation,&endStation);
   ATLASSERT( startStation < endStation );

#pragma Reminder("UPDATE: validate segment geometry")
   //CComPtr<IBroker> pBroker;
   //EAFGetBroker(&pBroker);
   //GET_IFACE2(pBroker,IGirderSegment,pISegment);

   //const CGirderGroupData* pGroup = m_pGirder->GetGirderGroup();

   //GroupIndexType grpIdx   = pGroup->GetGroupIndex();
   //GirderIndexType gdrIdx  = m_pGirder->GetGirderIndex();
   //SegmentIndexType segIdx = GetIndex();
   //CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

   //Float64 segmentLength = pISegment->GetSegmentLayoutLength(segmentKey);

   //Float64 x = segmentLength - (m_VariationLength[0]+m_VariationLength[1]+m_VariationLength[2]+m_VariationLength[3]);
   //_ASSERT(0 <= x);

}
#endif
