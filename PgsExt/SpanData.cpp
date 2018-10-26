///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <PgsExt\SpanData.h>
#include <PgsExt\PierData.h>
#include <PgsExt\BridgeDescription.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PIER_FACE_TO_GIRDER_END(_face_) _face_ == pgsTypes::Ahead ? pgsTypes::metStart : pgsTypes::metEnd

/****************************************************************************
CLASS
   CSpanData
****************************************************************************/


CSpanData::CSpanData(SpanIndexType spanIdx,CBridgeDescription* pBridgeDesc,CPierData* pPrevPier,CPierData* pNextPier)
{
   Init(spanIdx,pBridgeDesc,pPrevPier,pNextPier);
}

CSpanData::CSpanData(const CSpanData& rOther)
{
   Init(rOther.GetSpanIndex(),NULL,NULL,NULL);
   MakeCopy(rOther);
}

CSpanData::~CSpanData()
{
}

void CSpanData::Init(SpanIndexType spanIdx,CBridgeDescription* pBridge,CPierData* pPrevPier,CPierData* pNextPier)
{
   m_nGirders = 0;
   m_bUseSameSpacing = false;

   SetSpanIndex(spanIdx);
   SetBridgeDescription(pBridge);
   SetPiers(pPrevPier,pNextPier);

   m_GirderTypes.SetSpan(this);
   m_GirderSpacing[pgsTypes::metStart].SetSpan(this);
   m_GirderSpacing[pgsTypes::metEnd].SetSpan(this);

   m_SlabOffset[pgsTypes::metStart] = ::ConvertToSysUnits(10.0,unitMeasure::Inch);
   m_SlabOffset[pgsTypes::metEnd]   = m_SlabOffset[pgsTypes::metStart];

}

void CSpanData::SetSpanIndex(SpanIndexType spanIdx)
{
   m_SpanIdx = spanIdx;
}

SpanIndexType CSpanData::GetSpanIndex() const
{
   return m_SpanIdx;
}

void CSpanData::SetBridgeDescription(CBridgeDescription* pBridge)
{
   m_pBridgeDesc = pBridge;
}

CBridgeDescription* CSpanData::GetBridgeDescription() 
{
   return m_pBridgeDesc;
}

const CBridgeDescription* CSpanData::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

void CSpanData::SetPiers(CPierData* pPrevPier,CPierData* pNextPier)
{
   m_pPrevPier = pPrevPier;
   m_pNextPier = pNextPier;
}

CPierData* CSpanData::GetPrevPier()
{
   return m_pPrevPier;
}

CPierData* CSpanData::GetNextPier()
{
   return m_pNextPier;
}

const CPierData* CSpanData::GetPrevPier() const
{
   return m_pPrevPier;
}

const CPierData* CSpanData::GetNextPier() const
{
   return m_pNextPier;
}

CSpanData& CSpanData::operator= (const CSpanData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CSpanData::operator==(const CSpanData& rOther) const
{
   if ( m_SpanIdx != rOther.m_SpanIdx )
      return false;

   if ( m_GirderTypes != rOther.m_GirderTypes )
      return false;

   if ( m_bUseSameSpacing != rOther.m_bUseSameSpacing )
      return false;

   if ( m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneral || m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacent )
   {
      if ( m_GirderSpacing[pgsTypes::metStart] != rOther.m_GirderSpacing[pgsTypes::metStart] )
         return false;

      if ( !m_bUseSameSpacing && m_GirderSpacing[pgsTypes::metEnd] != rOther.m_GirderSpacing[pgsTypes::metEnd] )
         return false;
   }

   if ( !m_pBridgeDesc->UseSameNumberOfGirdersInAllSpans() && m_nGirders != rOther.m_nGirders )
      return false;

   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      if (m_LLDFs != rOther.m_LLDFs)
         return false;
   }

   return true;
}

bool CSpanData::operator!=(const CSpanData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CSpanData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      CComVariant var;

      hr = pStrLoad->BeginUnit(_T("SpanDataDetails"));

      double version;
      pStrLoad->get_Version(&version);

      if ( !m_pBridgeDesc->UseSameNumberOfGirdersInAllSpans() )
      {
         var.vt = VT_I2;
         hr = pStrLoad->get_Property(_T("GirderCount"),&var);
         m_nGirders = var.iVal;

         m_GirderTypes.SetGirderCount(m_nGirders);
         m_GirderSpacing[0].SetGirderCount(m_nGirders);
         m_GirderSpacing[1].SetGirderCount(m_nGirders);
      }
      else
      {
         SetGirderCount( m_pBridgeDesc->GetGirderCount() );
         m_GirderTypes.SetGirderName(0, m_pBridgeDesc->GetGirderName() );
      }

      hr = pStrLoad->BeginUnit(_T("Girders"));
      hr = m_GirderTypes.Load(pStrLoad,pProgress);
      hr = pStrLoad->EndUnit();

      if ( 2 <= version )
      {
         // added in version 2 of SpanDataDetails data block
         if ( m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotSpan )
         {
            var.vt = VT_R8;
            pStrLoad->get_Property(_T("SlabOffsetAtStart"),&var);
            m_SlabOffset[pgsTypes::metStart] = var.dblVal;

            pStrLoad->get_Property(_T("SlabOffsetAtEnd"),&var);
            m_SlabOffset[pgsTypes::metEnd] = var.dblVal;
         }
      }

      if ( m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneral || m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacent )
      {
         hr = pStrLoad->BeginUnit(_T("GirderSpacing"));
         
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("UseSameSpacingAtBothEnds"),&var);
         m_bUseSameSpacing = (var.boolVal == VARIANT_TRUE ? true : false);

         hr = pStrLoad->BeginUnit(_T("StartSpacing"));
         hr = m_GirderSpacing[pgsTypes::metStart].Load(pStrLoad,pProgress);
         hr = pStrLoad->EndUnit(); // start spacing

         if ( !m_bUseSameSpacing )
         {
            hr = pStrLoad->BeginUnit(_T("EndSpacing"));
            hr = m_GirderSpacing[pgsTypes::metEnd].Load(pStrLoad,pProgress);
            hr = pStrLoad->EndUnit(); // end spacing
         }
         else
         {
            m_GirderSpacing[pgsTypes::metEnd] = m_GirderSpacing[pgsTypes::metStart];
         }

         // NOTE: RAB 7/23/2009
         // There was a bug in the UI grid the caused bad data to be stored in the spacing grid.
         // This bad data was saved and has been loaded. If this data is left alone, PGSuper will crash.
         // The bad data is the last girder index is for one more girder than in the bridge
         // Fix the bad input now.
         for (  int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(i);
            GroupIndexType nGroups = m_GirderSpacing[endType].GetSpacingGroupCount();
            if ( m_nGirders <= m_GirderSpacing[endType].m_SpacingGroups[nGroups-1].second )
            {
               // there is bad data

               // fixed the last girder index and remove extra groups
               m_GirderSpacing[endType].m_SpacingGroups[nGroups-1].second = m_nGirders-1;
               if (m_GirderSpacing[endType].m_SpacingGroups[nGroups-1].first >= m_GirderSpacing[endType].m_SpacingGroups[nGroups-1].second )
                  m_GirderSpacing[endType].m_SpacingGroups.pop_back();

               // remove extra girder spacing data
               std::vector<Float64>::iterator spacing_begin = m_GirderSpacing[endType].m_GirderSpacing.begin() + (m_nGirders-1);
               std::vector<Float64>::iterator spacing_end   = m_GirderSpacing[endType].m_GirderSpacing.end();
               m_GirderSpacing[endType].m_GirderSpacing.erase( spacing_begin, spacing_end );
            }
         }

         hr = pStrLoad->EndUnit(); // girder spacing
      }
      else
      {
         m_GirderSpacing[pgsTypes::metStart].SetGirderSpacing(0,m_pBridgeDesc->GetGirderSpacing());
         m_GirderSpacing[pgsTypes::metStart].SetMeasurementLocation(m_pBridgeDesc->GetMeasurementLocation());
         m_GirderSpacing[pgsTypes::metStart].SetMeasurementType(m_pBridgeDesc->GetMeasurementType());

         m_GirderSpacing[pgsTypes::metEnd].SetGirderSpacing(0,m_pBridgeDesc->GetGirderSpacing());
         m_GirderSpacing[pgsTypes::metEnd].SetMeasurementLocation(m_pBridgeDesc->GetMeasurementLocation());
         m_GirderSpacing[pgsTypes::metEnd].SetMeasurementType(m_pBridgeDesc->GetMeasurementType());
      }

      if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
      {
         pStrLoad->BeginUnit(_T("LLDF"));
         double lldf_version;
         pStrLoad->get_Version(&lldf_version);

         if ( lldf_version < 3 )
         {
            // have to convert old data
            double gPM[2][2];
            double gNM[2][2];
            double  gV[2][2];

            if ( lldf_version < 2 )
            {
               var.vt = VT_R8;

               hr = pStrLoad->get_Property(_T("gPM_Interior"),&var);
               gPM[0][pgsTypes::Interior] = var.dblVal;
               gPM[1][pgsTypes::Interior] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gPM_Exterior"),&var);
               gPM[0][pgsTypes::Exterior] = var.dblVal;
               gPM[1][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Interior"),&var);
               gNM[0][pgsTypes::Interior] = var.dblVal;
               gNM[1][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Exterior"),&var);
               gNM[0][pgsTypes::Exterior] = var.dblVal;
               gNM[1][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Interior"), &var);
               gV[0][pgsTypes::Interior] = var.dblVal;
               gV[1][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Exterior"), &var);
               gV[0][pgsTypes::Exterior] = var.dblVal;
               gV[1][pgsTypes::Exterior] = var.dblVal;
            }
            else if ( lldf_version < 3 )
            {
               var.vt = VT_R8;

               hr = pStrLoad->get_Property(_T("gPM_Interior_Strength"),&var);
               gPM[0][pgsTypes::Interior] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gPM_Exterior_Strength"),&var);
               gPM[0][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Interior_Strength"),&var);
               gNM[0][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Exterior_Strength"),&var);
               gNM[0][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Interior_Strength"), &var);
               gV[0][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Exterior_Strength"), &var);
               gV[0][pgsTypes::Exterior] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gPM_Interior_Fatigue"),&var);
               gPM[1][pgsTypes::Interior] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gPM_Exterior_Fatigue"),&var);
               gPM[1][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Interior_Fatigue"),&var);
               gNM[1][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gNM_Exterior_Fatigue"),&var);
               gNM[1][pgsTypes::Exterior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Interior_Fatigue"), &var);
               gV[1][pgsTypes::Interior] = var.dblVal;

               pStrLoad->get_Property(_T("gV_Exterior_Fatigue"), &var);
               gV[1][pgsTypes::Exterior] = var.dblVal;
            }

            GirderIndexType ngs = GetGirderCount();
            for (GirderIndexType igs=0; igs<ngs; igs++)
            {
               LLDF lldf;
               if (igs==0 || igs==ngs-1)
               {
                  lldf.gNM[0] = gNM[0][pgsTypes::Exterior];
                  lldf.gNM[1] = gNM[1][pgsTypes::Exterior];
                  lldf.gPM[0] = gPM[0][pgsTypes::Exterior];
                  lldf.gPM[1] = gPM[1][pgsTypes::Exterior];
                  lldf.gV[0]  = gV[0][pgsTypes::Exterior];
                  lldf.gV[1]  = gV[1][pgsTypes::Exterior];
               }
               else
               {
                  lldf.gNM[0] = gNM[0][pgsTypes::Interior];
                  lldf.gNM[1] = gNM[1][pgsTypes::Interior];
                  lldf.gPM[0] = gPM[0][pgsTypes::Interior];
                  lldf.gPM[1] = gPM[1][pgsTypes::Interior];
                  lldf.gV[0]  = gV[0][pgsTypes::Interior];
                  lldf.gV[1]  = gV[1][pgsTypes::Interior];
               }

               m_LLDFs.push_back(lldf);
            }
         }
         else
         {
            // version 3
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("nLLDFGirders"),&var);
            int ng = var.lVal;

            var.vt = VT_R8;

            for (int ig=0; ig<ng; ig++)
            {
               LLDF lldf;

               hr = pStrLoad->BeginUnit(_T("LLDF_Girder"));

               hr = pStrLoad->get_Property(_T("gPM_Strength"),&var);
               lldf.gPM[0] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gNM_Strength"),&var);
               lldf.gNM[0] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gV_Strength"),&var);
               lldf.gV[0] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gPM_Fatigue"),&var);
               lldf.gPM[1] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gNM_Fatigue"),&var);
               lldf.gNM[1] = var.dblVal;

               hr = pStrLoad->get_Property(_T("gV_Fatigue"),&var);
               lldf.gV[1] = var.dblVal;

               pStrLoad->EndUnit(); // LLDF

               m_LLDFs.push_back(lldf);
            }
         }

         pStrLoad->EndUnit(); // LLDF
      }

      hr = pStrLoad->EndUnit(); // span data details
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CSpanData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("SpanDataDetails"),2.0);

   if ( !m_pBridgeDesc->UseSameNumberOfGirdersInAllSpans() )
   {
      pStrSave->put_Property(_T("GirderCount"),CComVariant(m_nGirders));
   }

   pStrSave->BeginUnit(_T("Girders"),1.0);
   m_GirderTypes.Save(pStrSave,pProgress);
   pStrSave->EndUnit();


   // added in version 2 of SpanDataDetails data block
   if ( m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotSpan )
   {
      pStrSave->put_Property(_T("SlabOffsetAtStart"),CComVariant(m_SlabOffset[pgsTypes::metStart]));
      pStrSave->put_Property(_T("SlabOffsetAtEnd"),  CComVariant(m_SlabOffset[pgsTypes::metEnd]));
   }

   if ( m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneral || m_pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacent)
   {
      pStrSave->BeginUnit(_T("GirderSpacing"),1.0);
      pStrSave->put_Property(_T("UseSameSpacingAtBothEnds"),CComVariant(m_bUseSameSpacing));

      pStrSave->BeginUnit(_T("StartSpacing"),1.0);
      m_GirderSpacing[pgsTypes::metStart].Save(pStrSave,pProgress);
      pStrSave->EndUnit(); // start spacing

      if (!m_bUseSameSpacing)
      {
         pStrSave->BeginUnit(_T("EndSpacing"),1.0);
         m_GirderSpacing[pgsTypes::metEnd].Save(pStrSave,pProgress);
         pStrSave->EndUnit(); // end spacing
      }

      pStrSave->EndUnit(); // girder spacing
   }

   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pStrSave->BeginUnit(_T("LLDF"),3.0);
      GirderIndexType ngs = GetGirderCount();
      pStrSave->put_Property(_T("nLLDFGirders"),CComVariant(ngs));

      for (GirderIndexType igs=0; igs<ngs; igs++)
      {
         pStrSave->BeginUnit(_T("LLDF_Girder"),1.0);
         LLDF& lldf = GetLLDF(igs);

         pStrSave->put_Property(_T("gPM_Strength"),CComVariant(lldf.gPM[0]));
         pStrSave->put_Property(_T("gNM_Strength"),CComVariant(lldf.gNM[0]));
         pStrSave->put_Property(_T("gV_Strength"), CComVariant(lldf.gV[0]));
         pStrSave->put_Property(_T("gPM_Fatigue"),CComVariant(lldf.gPM[1]));
         pStrSave->put_Property(_T("gNM_Fatigue"),CComVariant(lldf.gNM[1]));
         pStrSave->put_Property(_T("gV_Fatigue"), CComVariant(lldf.gV[1]));
         pStrSave->EndUnit(); // LLDF_Girder
      }

      pStrSave->EndUnit(); // LLDF
   }

   pStrSave->EndUnit(); // span data details

   return hr;
}

void CSpanData::MakeCopy(const CSpanData& rOther)
{
   m_SpanIdx                           = rOther.m_SpanIdx;
   m_bUseSameSpacing                   = rOther.m_bUseSameSpacing;
   m_nGirders                          = rOther.m_nGirders;
   m_GirderTypes                       = rOther.m_GirderTypes;
   m_GirderSpacing[pgsTypes::metStart] = rOther.m_GirderSpacing[pgsTypes::metStart];
   m_GirderSpacing[pgsTypes::metEnd]   = rOther.m_GirderSpacing[pgsTypes::metEnd];
   m_SlabOffset[pgsTypes::metStart]    = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd]      = rOther.m_SlabOffset[pgsTypes::metEnd];

   m_LLDFs = rOther.m_LLDFs;

   m_GirderTypes.SetSpan(this);
   m_GirderSpacing[pgsTypes::metStart].SetSpan(this);
   m_GirderSpacing[pgsTypes::metEnd].SetSpan(this);
}

void CSpanData::MakeAssignment(const CSpanData& rOther)
{
   MakeCopy( rOther );
}

GirderIndexType CSpanData::GetGirderCount() const
{
   if ( m_pBridgeDesc->UseSameNumberOfGirdersInAllSpans() )
      return m_pBridgeDesc->GetGirderCount();
   else
      return m_nGirders;
}

void CSpanData::SetGirderCount(GirderIndexType nGirders)
{
   m_nGirders = nGirders;
   m_GirderTypes.SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metStart].SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metEnd].SetGirderCount(m_nGirders);
}

void CSpanData::AddGirders(GirderIndexType nGirders)
{
   m_nGirders += nGirders;
   m_GirderTypes.SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metStart].SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metEnd].SetGirderCount(m_nGirders);
}

void CSpanData::RemoveGirders(GirderIndexType nGirders)
{
   if ( m_nGirders < nGirders )
      m_nGirders = 0; // removing more than we have... make it 0
   else
      m_nGirders -= nGirders;

   m_GirderTypes.SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metStart].SetGirderCount(m_nGirders);
   m_GirderSpacing[pgsTypes::metEnd].SetGirderCount(m_nGirders);
}

const CGirderTypes* CSpanData::GetGirderTypes() const
{
   ATLASSERT( m_GirderTypes.m_pSpan == this);
   return &m_GirderTypes;
}

CGirderTypes* CSpanData::GetGirderTypes()
{
   ATLASSERT( m_GirderTypes.m_pSpan == this);
   return &m_GirderTypes;
}

void CSpanData::SetGirderTypes(const CGirderTypes& girderTypes)
{
   m_GirderTypes = girderTypes;
   m_GirderTypes.SetSpan(this);
}

void CSpanData::UseSameSpacingAtBothEndsOfSpan(bool bUseSame)
{
   m_bUseSameSpacing = bUseSame;
}

bool CSpanData::UseSameSpacingAtBothEndsOfSpan() const
{
   return m_bUseSameSpacing;
}

const CGirderSpacing* CSpanData::GetGirderSpacing(pgsTypes::MemberEndType end) const
{
   if ( m_bUseSameSpacing )
      return &m_GirderSpacing[pgsTypes::metStart];
   else
      return &m_GirderSpacing[end];
}

CGirderSpacing* CSpanData::GetGirderSpacing(pgsTypes::MemberEndType end)
{
   if ( m_bUseSameSpacing )
      return &m_GirderSpacing[pgsTypes::metStart];
   else
      return &m_GirderSpacing[end];
}

const CGirderSpacing* CSpanData::GetGirderSpacing(pgsTypes::PierFaceType pierFace) const
{
   return GetGirderSpacing(PIER_FACE_TO_GIRDER_END(pierFace));
}

CGirderSpacing* CSpanData::GetGirderSpacing(pgsTypes::PierFaceType pierFace)
{
   return GetGirderSpacing(PIER_FACE_TO_GIRDER_END(pierFace));
}

void CSpanData::SetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,double gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gPM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CSpanData::SetLLDFPosMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,double gM)
{
   GirderIndexType ngdrs = GetGirderCount();
   if (ngdrs>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<ngdrs-1; ig++)
      {
         SetLLDFPosMoment(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFPosMoment(0,ls,gM);
      SetLLDFPosMoment(ngdrs-1,ls,gM);
   }
}


double CSpanData::GetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gPM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CSpanData::SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,double gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gNM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CSpanData::SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,double gM)
{
   GirderIndexType ngdrs = GetGirderCount();
   if (ngdrs>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<ngdrs-1; ig++)
      {
         SetLLDFNegMoment(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFNegMoment(0,ls,gM);
      SetLLDFNegMoment(ngdrs-1,ls,gM);
   }
}

double CSpanData::GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gNM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CSpanData::SetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls,double gV)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gV[ls == pgsTypes::FatigueI ? 1 : 0] = gV;
}

void CSpanData::SetLLDFShear(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,double gM)
{
   GirderIndexType ngdrs = GetGirderCount();
   if (ngdrs>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<ngdrs-1; ig++)
      {
         SetLLDFShear(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFShear(0,ls,gM);
      SetLLDFShear(ngdrs-1,ls,gM);
   }
}

double CSpanData::GetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gV[ls == pgsTypes::FatigueI ? 1 : 0];
}

double CSpanData::GetSpanLength() const
{
   return m_pNextPier->GetStation() - m_pPrevPier->GetStation();
}

bool CSpanData::IsInteriorGirder(GirderIndexType gdrIdx) const
{
   return !IsExteriorGirder(gdrIdx);
}

bool CSpanData::IsExteriorGirder(GirderIndexType gdrIdx) const
{
   GirderIndexType nGirders = GetGirderCount();
   return ( gdrIdx == 0 || gdrIdx == nGirders-1 ) ? true : false;
}

void CSpanData::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
   pgsTypes::SlabOffsetType slabOffsetType = m_pBridgeDesc->GetSlabOffsetType();
   if ( slabOffsetType == pgsTypes::sotBridge )
   {
      m_pBridgeDesc->SetSlabOffset(offset);
   }
   else if ( slabOffsetType == pgsTypes::sotSpan )
   {
      m_SlabOffset[end] = offset;
   }
   else
   {
      m_GirderTypes.SetSlabOffset(ALL_GIRDERS,end,offset);
   }
}

Float64 CSpanData::GetSlabOffset(pgsTypes::MemberEndType end) const
{
   pgsTypes::SlabOffsetType slabOffsetType = m_pBridgeDesc->GetSlabOffsetType();

   if ( slabOffsetType == pgsTypes::sotBridge )
      return m_pBridgeDesc->GetSlabOffset();
   else if ( slabOffsetType == pgsTypes::sotSpan )
      return m_SlabOffset[end];

   ATLASSERT(false); // slab offset type is by girder and you are asking for it at the span level
   return m_GirderTypes.GetSlabOffset(0,end); // a reasonable default
}

void CSpanData::SetSlabOffset(pgsTypes::PierFaceType face,Float64 offset)
{
   SetSlabOffset(PIER_FACE_TO_GIRDER_END(face), offset);
}

Float64 CSpanData::GetSlabOffset(pgsTypes::PierFaceType face) const
{
   return GetSlabOffset(PIER_FACE_TO_GIRDER_END(face));
}

CSpanData::LLDF& CSpanData::GetLLDF(GirderIndexType igs) const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType ngdrs = GetGirderCount();
   IndexType ndfs = m_LLDFs.size();

   if (ndfs==0)
   {
      for (GirderIndexType i=0; i<ngdrs; i++)
      {
         m_LLDFs.push_back(LLDF());
      }
   }
   else if (ndfs<ngdrs)
   {
      // More girders than factors - move exterior to last girder and use last interior for new interiors
      LLDF exterior = m_LLDFs.back();
      GirderIndexType inter_idx = ndfs-2>0 ? ndfs-2 : 0; // one-girder bridges could otherwise give us trouble
      LLDF interior = m_LLDFs[inter_idx];

      m_LLDFs[ndfs-1] = interior;
      for (GirderIndexType i=ndfs; i<ngdrs; i++)
      {
         if (i != ngdrs-1)
         {
            m_LLDFs.push_back(interior);
         }
         else
         {
            m_LLDFs.push_back(exterior);
         }
      }
    }
   else if (ndfs>ngdrs)
   {
      // more factors than girders - truncate, then move last exterior to end
      LLDF exterior = m_LLDFs.back();
      m_LLDFs.resize(ngdrs);
      m_LLDFs.back() = exterior;
   }

   // Next: let's deal with retrieval
   if (igs<0)
   {
      ATLASSERT(0); // problemo in calling routine - let's not crash
      return m_LLDFs[0];
   }
   else if (igs>=ngdrs)
   {
      ATLASSERT(0); // problemo in calling routine - let's not crash
      return m_LLDFs.back();
   }
   else
   {
      return m_LLDFs[igs];
   }
}
