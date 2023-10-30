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
#include <PgsExt\SpanData2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\BridgeDescription2.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSpanData2
****************************************************************************/


CSpanData2::CSpanData2(SpanIndexType spanIdx,CBridgeDescription2* pBridgeDesc,CPierData2* pPrevPier,CPierData2* pNextPier)
{
   Init(spanIdx,pBridgeDesc,pPrevPier,pNextPier);
}

CSpanData2::CSpanData2(const CSpanData2& rOther)
{
   Init(rOther.GetIndex(),nullptr,nullptr,nullptr);
   MakeCopy(rOther,true /* copy only data*/ );
}

CSpanData2::~CSpanData2()
{
}

void CSpanData2::Init(SpanIndexType spanIdx,CBridgeDescription2* pBridge,CPierData2* pPrevPier,CPierData2* pNextPier)
{
   SetIndex(spanIdx);
   SetBridgeDescription(pBridge);
   SetPiers(pPrevPier,pNextPier);
}

void CSpanData2::SetIndex(SpanIndexType spanIdx)
{
   m_SpanIdx = spanIdx;
}

SpanIndexType CSpanData2::GetIndex() const
{
   return m_SpanIdx;
}

void CSpanData2::SetBridgeDescription(CBridgeDescription2* pBridge)
{
   m_pBridgeDesc = pBridge;
}

CBridgeDescription2* CSpanData2::GetBridgeDescription() 
{
   return m_pBridgeDesc;
}

const CBridgeDescription2* CSpanData2::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

void CSpanData2::SetPiers(CPierData2* pPrevPier,CPierData2* pNextPier)
{
   m_pPrevPier = pPrevPier;
   m_pNextPier = pNextPier;
}

CPierData2* CSpanData2::GetPrevPier()
{
   return m_pPrevPier;
}

CPierData2* CSpanData2::GetNextPier()
{
   return m_pNextPier;
}

CPierData2* CSpanData2::GetPier(pgsTypes::MemberEndType end)
{
   return ( end == pgsTypes::metStart ? m_pPrevPier : m_pNextPier );
}

const CPierData2* CSpanData2::GetPrevPier() const
{
   return m_pPrevPier;
}

const CPierData2* CSpanData2::GetNextPier() const
{
   return m_pNextPier;
}

const CPierData2* CSpanData2::GetPier(pgsTypes::MemberEndType end) const
{
   return ( end == pgsTypes::metStart ? m_pPrevPier : m_pNextPier );
}

CSpanData2& CSpanData2::operator= (const CSpanData2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CSpanData2::CopySpanData(const CSpanData2* pSpan)
{
   MakeCopy(*pSpan,true/*copy only data*/);
}

bool CSpanData2::operator==(const CSpanData2& rOther) const
{
   if ( m_SpanIdx != rOther.m_SpanIdx )
   {
      return false;
   }

   if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      if (m_LLDFs != rOther.m_LLDFs)
      {
         return false;
      }
   }

   if ( m_pBridgeDesc->GetAssumedExcessCamberType() != pgsTypes::aecBridge )
   {
      if (m_vAssumedExcessCambers != rOther.m_vAssumedExcessCambers)
      {
         return false;
      }
   }

   if (m_pBridgeDesc->GetHaunchLayoutType() == pgsTypes::hltAlongSpans && m_pBridgeDesc->GetHaunchInputLocationType() != pgsTypes::hilSame4Bridge)
   {
      if (m_pBridgeDesc->GetHaunchInputLocationType() == pgsTypes::hilSame4AllGirders)
      {
         if (m_vHaunchDepths.front() != rOther.m_vHaunchDepths.front())
         {
            return false;
         }
      }
      else
      {
         if (m_vHaunchDepths != rOther.m_vHaunchDepths)
         {
            return false;
         }
      }
   }

   return true;
}

bool CSpanData2::operator!=(const CSpanData2& rOther) const
{
   return !operator==(rOther);
}

HRESULT CSpanData2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      CComVariant var;

      hr = pStrLoad->BeginUnit(_T("SpanDataDetails"));

      Float64 version;
      pStrLoad->get_Version(&version);

      if ( m_pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
      {
         pStrLoad->BeginUnit(_T("LLDF"));
         Float64 lldf_version;
         pStrLoad->get_Version(&lldf_version);

         if ( lldf_version < 3 )
         {
            // have to convert old data
            Float64 gPM[2][2];
            Float64 gNM[2][2];
            Float64  gV[2][2];

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
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("nLLDFGirders"),&var);
            IndexType ng = VARIANT2INDEX(var);

            var.vt = VT_R8;

            for (IndexType ig=0; ig<ng; ig++)
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

               pStrLoad->EndUnit(); // LLDF_Girder

               m_LLDFs.push_back(lldf);
            }
         }

         pStrLoad->EndUnit(); // LLDF
      }

      if (2 < version)
      {
         if (3 < version)
         {
            if (m_pBridgeDesc->GetAssumedExcessCamberType() != pgsTypes::aecBridge)
            {
               pStrLoad->BeginUnit(_T("AssExcessCambers"));
               Float64 AssumedExcessCamber_version;
               pStrLoad->get_Version(&AssumedExcessCamber_version);

            var.vt = VT_INDEX;
               hr = pStrLoad->get_Property(_T("nFGirders"), &var);
            IndexType ng = VARIANT2INDEX(var);

            var.vt = VT_R8;
            m_vAssumedExcessCambers.clear();
               for (IndexType ig = 0; ig < ng; ig++)
            {
                  hr = pStrLoad->get_Property(_T("AssExcessCamber"), &var);
                  m_vAssumedExcessCambers.push_back(var.dblVal);
            }

               pStrLoad->EndUnit(); // AssExcessCambers
            }
         }
         else
         {
            // version == 3
            // Ugly stuff here: in version 3.1 of pgsuper, fillets could be per-girder. This was revoked in 3.1.4
            // We have be able to read this, and get the max value to set to the whole bridge here.
            if (m_pBridgeDesc->m_bWasVersion3_1FilletRead)
            {
               pStrLoad->BeginUnit(_T("Fillets"));

               var.vt = VT_INDEX;
               hr = pStrLoad->get_Property(_T("nFGirders"), &var);
               IndexType ng = VARIANT2INDEX(var);

               var.vt = VT_R8;
               Float64 max_fillet = 0.0;
               for (IndexType ig = 0; ig < ng; ig++)
               {
                  hr = pStrLoad->get_Property(_T("Fillet"), &var);
                  max_fillet = max(max_fillet, var.dblVal);
               }
            pStrLoad->EndUnit(); // Fillets

               m_pBridgeDesc->m_Fillet = max(m_pBridgeDesc->m_Fillet, max_fillet);
            }
         }
      }

      if (4 < version)
      {
         if (m_pBridgeDesc->GetHaunchInputLocationType() != pgsTypes::hilSame4Bridge)
         {
            m_vHaunchDepths.clear();
            pStrLoad->BeginUnit(_T("HaunchDepthsPerGirder"));

            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("nGirders"),&var);
            IndexType ng = VARIANT2INDEX(var);

            for (IndexType ig = 0; ig < ng; ig++)
            {
               pStrLoad->BeginUnit(_T("HaunchDepths"));

               var.vt = VT_INDEX;
               hr = pStrLoad->get_Property(_T("nValues"),&var);
               IndexType nvals = VARIANT2INDEX(var);

               std::vector<Float64> vdepths;
               var.vt = VT_R8;
               for (IndexType iv = 0; iv < nvals; iv++)
               {
                  hr = pStrLoad->get_Property(_T("HaunchVal"),&var);
                  vdepths.push_back(var.dblVal);
               }

               m_vHaunchDepths.push_back(vdepths);
               pStrLoad->EndUnit(); // HaunchDepths
            }

            pStrLoad->EndUnit(); // HaunchDepthsPerGirder
         }
      }

      hr = pStrLoad->EndUnit(); // span data details
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return hr;
}

HRESULT CSpanData2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("SpanDataDetails"),5.0);

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

   // Assummed excess camber was added in version 4
   if ( m_pBridgeDesc->GetAssumedExcessCamberType() != pgsTypes::aecBridge )
   {
      pStrSave->BeginUnit(_T("AssExcessCambers"),1.0);

      // don't save camber data for no deck bridges
      GirderIndexType ngs = 0;
      if (m_pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
      {
         ngs = GetGirderCount();
      }

      pStrSave->put_Property(_T("nFGirders"),CComVariant(ngs));

      for (GirderIndexType igs = 0; igs < ngs; igs++)
      {
         Float64 filval = GetAssumedExcessCamber(igs);
         pStrSave->put_Property(_T("AssExcessCamber"),CComVariant(filval));
      }

      pStrSave->EndUnit(); // AssExcessCambers
   }

   if (m_pBridgeDesc->GetHaunchInputLocationType() != pgsTypes::hilSame4Bridge)
   {
      pStrSave->BeginUnit(_T("HaunchDepthsPerGirder"),1.0);
      pStrSave->put_Property(_T("nGirders"),CComVariant(m_vHaunchDepths.size()));
      for (const auto& vhaunch : m_vHaunchDepths)
      {
         pStrSave->BeginUnit(_T("HaunchDepths"),1.0);
         pStrSave->put_Property(_T("nValues"),CComVariant(vhaunch.size()));
         for (const auto& val : vhaunch)
         {
            pStrSave->put_Property(_T("HaunchVal"),CComVariant(val));
         }

         pStrSave->EndUnit(); // HaunchDepths
      }

      pStrSave->EndUnit(); // HaunchDepthsPerGirder
   }

   pStrSave->EndUnit(); // span data details

   return hr;
}

void CSpanData2::MakeCopy(const CSpanData2& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      m_SpanIdx = rOther.m_SpanIdx;
   }

   m_LLDFs   = rOther.m_LLDFs;

   m_vAssumedExcessCambers = rOther.m_vAssumedExcessCambers;

   m_vHaunchDepths = rOther.m_vHaunchDepths;

   PGS_ASSERT_VALID;
}

void CSpanData2::MakeAssignment(const CSpanData2& rOther)
{
   MakeCopy( rOther, false /*copy everything*/ );
}

void CSpanData2::SetAssumedExcessCamber(Float64 assumedExcessCamber)
{
   GirderIndexType nGirders = GetGirderCount();
   m_vAssumedExcessCambers.assign(nGirders, assumedExcessCamber);
}

void CSpanData2::SetAssumedExcessCamber(GirderIndexType gdrIdx,Float64 assumedExcessCamber)
{
   ProtectAssumedExcessCamber();
   ATLASSERT(gdrIdx < m_vAssumedExcessCambers.size());

   m_vAssumedExcessCambers[gdrIdx] = assumedExcessCamber;
}

Float64 CSpanData2::GetAssumedExcessCamber(GirderIndexType gdrIdx,bool bGetRawValue) const
{
   if (bGetRawValue)
   {
      ProtectAssumedExcessCamber();
      ATLASSERT(gdrIdx < m_vAssumedExcessCambers.size());
      return m_vAssumedExcessCambers[gdrIdx];
   }
   else
   {
      pgsTypes::AssumedExcessCamberType aecType = m_pBridgeDesc->GetAssumedExcessCamberType();
      if (aecType == pgsTypes::aecBridge)
      {
         return m_pBridgeDesc->GetAssumedExcessCamber();
      }
      else
      {
         ProtectAssumedExcessCamber();
         ATLASSERT(gdrIdx < m_vAssumedExcessCambers.size());
         if (aecType == pgsTypes::aecSpan)
         {
            // use front value for span values
            return m_vAssumedExcessCambers.front();
         }
         else
         {
            return m_vAssumedExcessCambers[gdrIdx];
         }
      }
   }
}

void CSpanData2::CopyAssumedExcessCamber(GirderIndexType sourceGdrIdx, GirderIndexType targetGdrIdx)
{
   ProtectAssumedExcessCamber();
   ATLASSERT(sourceGdrIdx < m_vAssumedExcessCambers.size() && targetGdrIdx < m_vAssumedExcessCambers.size());

   m_vAssumedExcessCambers[targetGdrIdx] = m_vAssumedExcessCambers[sourceGdrIdx];
}

void CSpanData2::ProtectAssumedExcessCamber() const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType nGirders = GetGirderCount();
   IndexType nFlts = m_vAssumedExcessCambers.size();

   if (nFlts == 0)
   {
      // probably switched from aecBridge. Get AssumedExcessCamber value from bridge and assign as a default
      Float64 defVal = m_pBridgeDesc->GetAssumedExcessCamber();
      defVal = Max(0.0, defVal);

      m_vAssumedExcessCambers.assign(nGirders,defVal);
   }
   else if (nFlts < nGirders)
   {
      // More girders than data - use back value for remaining girders
      Float64 back = m_vAssumedExcessCambers.back();

      m_vAssumedExcessCambers.resize(nGirders); // performance
      for (IndexType i = nFlts; i < nGirders; i++)
      {
         m_vAssumedExcessCambers.push_back(back);
      }
    }
   else if (nGirders < nFlts)
   {
      // more AssumedExcessCambers than girders - truncate
      m_vAssumedExcessCambers.resize(nGirders);
   }
}

void CSpanData2::SetDirectHaunchDepths(const std::vector<Float64>& HaunchDepths)
{
   GirderIndexType nGirders = GetGirderCount();
   m_vHaunchDepths.assign(nGirders,HaunchDepths);
}

void CSpanData2::SetDirectHaunchDepths(GirderIndexType gdrIdx, const std::vector<Float64>& HaunchDepth)
{
   ProtectHaunchDepth();
   ATLASSERT(gdrIdx < m_vHaunchDepths.size());

   m_vHaunchDepths[gdrIdx] = HaunchDepth;
}

std::vector<Float64> CSpanData2::GetDirectHaunchDepths(GirderIndexType gdrIdx,bool bGetRawValue) const
{
   if (bGetRawValue)
   {
      ProtectHaunchDepth();
      ATLASSERT(gdrIdx < m_vHaunchDepths.size());
      return m_vHaunchDepths[gdrIdx];
   }
   else
   {
      pgsTypes::HaunchInputLocationType type = m_pBridgeDesc->GetHaunchInputLocationType();
      if (type == pgsTypes::hilSame4Bridge)
      {
         return m_pBridgeDesc->GetDirectHaunchDepths();
      }
      else
      {
         ProtectHaunchDepth();
         return m_vHaunchDepths[gdrIdx];
      }
   }
}

void CSpanData2::CopyHaunchDepth(GirderIndexType sourceGdrIdx,GirderIndexType targetGdrIdx)
{
   ProtectHaunchDepth();
   ATLASSERT(sourceGdrIdx < m_vHaunchDepths.size() && targetGdrIdx < m_vHaunchDepths.size());

   m_vHaunchDepths[targetGdrIdx] = m_vHaunchDepths[sourceGdrIdx];
}

void CSpanData2::ProtectHaunchDepth() const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType nGirders = GetGirderCount();
   IndexType nFlts = m_vHaunchDepths.size();

   if (nFlts == 0)
   {
      // probably switched from hilSame4Bridge. Get HaunchDepth value from bridge and assign as a default
      std::vector<Float64> defVal = m_pBridgeDesc->GetDirectHaunchDepths();
      m_vHaunchDepths.assign(nGirders,defVal);
   }
   else if (nFlts < nGirders)
   {
      // More girders than data - use back value for remaining girders
      std::vector<Float64> back = m_vHaunchDepths.back();

      m_vHaunchDepths.resize(nGirders); // performance
      for (IndexType i = nFlts; i < nGirders; i++)
      {
         m_vHaunchDepths.push_back(back);
      }
   }
   else if (nGirders < nFlts)
   {
      // more HaunchDepths than girders - truncate
      m_vHaunchDepths.resize(nGirders);
   }
}

void CSpanData2::SetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gPM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CSpanData2::SetLLDFPosMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM)
{
   GirderIndexType nGirders = GetGirderCount();
   if (nGirders>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<nGirders-1; ig++)
      {
         SetLLDFPosMoment(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFPosMoment(0,ls,gM);
      SetLLDFPosMoment(nGirders-1,ls,gM);
   }
}

Float64 CSpanData2::GetLLDFPosMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gPM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CSpanData2::SetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gM)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gNM[ls == pgsTypes::FatigueI ? 1 : 0] = gM;
}

void CSpanData2::SetLLDFNegMoment(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM)
{
   GirderIndexType nGirders = GetGirderCount();
   if (nGirders>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<nGirders-1; ig++)
      {
         SetLLDFNegMoment(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFNegMoment(0,ls,gM);
      SetLLDFNegMoment(nGirders-1,ls,gM);
   }
}

Float64 CSpanData2::GetLLDFNegMoment(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gNM[ls == pgsTypes::FatigueI ? 1 : 0];
}

void CSpanData2::SetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls,Float64 gV)
{
   LLDF& rlldf = GetLLDF(gdrIdx);

   rlldf.gV[ls == pgsTypes::FatigueI ? 1 : 0] = gV;
}

void CSpanData2::SetLLDFShear(pgsTypes::GirderLocation gdrloc, pgsTypes::LimitState ls,Float64 gM)
{
   GirderIndexType nGirders = GetGirderCount();
   if (nGirders>2 && gdrloc==pgsTypes::Interior)
   {
      for (GirderIndexType ig=1; ig<nGirders-1; ig++)
      {
         SetLLDFShear(ig,ls,gM);
      }
   }
   else if (gdrloc==pgsTypes::Exterior)
   {
      SetLLDFShear(0,ls,gM);
      SetLLDFShear(nGirders-1,ls,gM);
   }
}

Float64 CSpanData2::GetLLDFShear(GirderIndexType gdrIdx, pgsTypes::LimitState ls) const
{
   const LLDF& rlldf = GetLLDF(gdrIdx);

   return rlldf.gV[ls == pgsTypes::FatigueI ? 1 : 0];
}

Float64 CSpanData2::GetSpanLength() const
{
   return m_pNextPier->GetStation() - m_pPrevPier->GetStation();
}

std::vector<const CTemporarySupportData*> CSpanData2::GetTemporarySupports() const
{
   std::vector<const CTemporarySupportData*> vTS;
   if ( m_pBridgeDesc != nullptr )
   {
      SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
         if ( pTS->GetSpan() == this )
         {
            vTS.push_back(pTS);
         }
      }
   }
   return vTS;
}

std::vector<CTemporarySupportData*> CSpanData2::GetTemporarySupports()
{
   std::vector<CTemporarySupportData*> vTS;
   if ( m_pBridgeDesc != nullptr )
   {
      SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
         if ( pTS->GetSpan() == this )
         {
            vTS.push_back(pTS);
         }
      }
   }
   return vTS;
}

GirderIndexType CSpanData2::GetGirderCount() const
{
   ATLASSERT(m_pBridgeDesc);
   const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(this);
   return pGroup->GetGirderCount();
}

CSpanData2::LLDF& CSpanData2::GetLLDF(GirderIndexType gdrIdx) const
{
   // First: Compare size of our collection with current number of girders and resize if they don't match
   GirderIndexType nGirders = GetGirderCount();
   IndexType nLLDF = m_LLDFs.size();

   if (nLLDF == 0)
   {
      m_LLDFs.resize(nGirders);
   }
   else if (nLLDF < nGirders)
   {
      // More girders than factors - move exterior to last girder and use last interior for new interiors
      LLDF exterior = m_LLDFs.back();
      IndexType inter_idx = nLLDF-2>0 ? nLLDF-2 : 0; // one-girder bridges could otherwise give us trouble
      LLDF interior = m_LLDFs[inter_idx];

      m_LLDFs[nLLDF-1] = interior;
      for (IndexType i = nLLDF; i < nGirders; i++)
      {
         if (i != nGirders-1)
         {
            m_LLDFs.push_back(interior);
         }
         else
         {
            m_LLDFs.push_back(exterior);
         }
      }
    }
   else if (nGirders < nLLDF)
   {
      // more factors than girders - truncate, then move last exterior to end
      LLDF exterior = m_LLDFs.back();
      m_LLDFs.resize(nGirders);
      m_LLDFs.back() = exterior;
   }

   // Next: let's deal with retrieval
   if (gdrIdx < 0)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs[0];
   }
   else if (nGirders <= gdrIdx)
   {
      ATLASSERT(false); // problemo in calling routine - let's not crash
      return m_LLDFs.back();
   }
   else
   {
      return m_LLDFs[gdrIdx];
   }
}

#if defined _DEBUG
void CSpanData2::AssertValid()
{
   if ( m_pBridgeDesc )
   {
      _ASSERT(m_pPrevPier != nullptr);
      _ASSERT(m_pNextPier != nullptr);
      _ASSERT(m_pPrevPier->GetNextSpan() == this);
      _ASSERT(m_pNextPier->GetPrevSpan() == this);
   }
   else
   {
      _ASSERT(m_pPrevPier == nullptr);
      _ASSERT(m_pNextPier == nullptr);
   }
}
#endif
