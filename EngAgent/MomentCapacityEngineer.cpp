
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

#include "StdAfx.h"
#include "MomentCapacityEngineer.h"
#include <PGSuperException.h>

#include <EAF\EAFAutoProgress.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\MomentCapacity.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\statusitem.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DevelopmentLength.h>

#include <Lrfd\ConcreteUtil.h>

#include <algorithm>

// for the diagnostic file dump
#include <initguid.h>
#include <WBFLTools_i.c>
#include <EAF\EAFUIIntegration.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DIAG_DEFINE_GROUP(MomCap,DIAG_GROUP_DISABLE,0);

static const Float64 ANGLE_TOL=1.0e-6;
static const Float64 D_TOL=1.0e-10;

void AddShape2Section(CComBSTR bstrName,IGeneralSection *pSection, IShape *pShape, IStressStrain *pfgMaterial, IStressStrain *pbgMaterial, IPlane3d* initialStrain,Float64 Le,bool bPrimaryShape)
{
#if defined USE_ORIGINAL_SHAPE
   // Just add shape as is
   pSection->AddShape(bstrName,pShape, pfgMaterial, pbgMaterial, initialStrain, Le, bPrimaryShape ? VARIANT_TRUE : VARIANT_FALSE);
#else
   // Convert shape to a fast polygon (the standard polyshape now uses the faster polyshape implementation)
   // get points from shape and create a faster poly
    CComQIPtr<IGenericShape> generic_shape(pShape);
    if (generic_shape)
    {
       pSection->AddShape(bstrName, pShape, pfgMaterial, pbgMaterial, initialStrain, Le, bPrimaryShape ? VARIANT_TRUE : VARIANT_FALSE);
    }
    else
    {
        CComPtr<IPoint2dCollection> points;
        pShape->get_PolyPoints(&points);

        CComPtr<IPolyShape> poly;
        HRESULT hr = poly.CoCreateInstance(CLSID_PolyShape);

        poly->AddPoints(points);

        CComQIPtr<IShape> shape(poly);

       pSection->AddShape(bstrName, shape, pfgMaterial, pbgMaterial, initialStrain, Le, bPrimaryShape ? VARIANT_TRUE : VARIANT_FALSE);
    }
#endif
}

/****************************************************************************
CLASS
   pgsMomentCapacityEngineer
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsMomentCapacityEngineer::pgsMomentCapacityEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   ATLASSERT(pBroker != nullptr);

   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
   CREATE_LOGFILE("MomentCapacity");

   HRESULT hr = S_OK;

   // create solvers
   hr = m_MomentCapacitySolver.CoCreateInstance(CLSID_MomentCapacitySolver);

   if ( FAILED(hr) )
   {
      THROW_SHUTDOWN(_T("Installation Problem - Unable to create Moment Capacity Solver"),XREASON_COMCREATE_ERROR,true);
   }

   hr = m_CrackedSectionSolver.CoCreateInstance(CLSID_CrackedSectionSolver);

   if ( FAILED(hr) )
   {
      THROW_SHUTDOWN(_T("Installation Problem - Unable to create Cracked Section Solver"),XREASON_COMCREATE_ERROR,true);
   }

   GET_IFACE(IEAFStatusCenter, pStatusCenter);
   m_scidMomentCapacity = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning));
}

pgsMomentCapacityEngineer::~pgsMomentCapacityEngineer()
{
   InvalidateMomentCapacity();
   InvalidateCrackedSectionDetails();
   InvalidateMinMomentCapacity();
   InvalidateCrackingMoments();
   CLOSE_LOGFILE;
}

//======================== OPERATIONS =======================================
void pgsMomentCapacityEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsMomentCapacityEngineer::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;

   if (m_scidMomentCapacity == INVALID_ID)
   {
      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      m_scidMomentCapacity = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning));
   }
}

Float64 pgsMomentCapacityEngineer::GetMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   const MOMENTCAPACITYDETAILS* pmcd = GetMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, pConfig);
   return pmcd->Mr;
}

std::vector<Float64> pgsMomentCapacityEngineer::GetMomentCapacity(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   std::vector<Float64> Mr;
   Mr.reserve(vPoi.size());

   const auto vDetails = GetMomentCapacityDetails(intervalIdx, vPoi, bPositiveMoment, pConfig);
   for (const auto& details : vDetails)
   {
      Mr.push_back(details->Mr);
   }

   return Mr;
}

const MOMENTCAPACITYDETAILS* pgsMomentCapacityEngineer::GetMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment,const GDRCONFIG* pConfig) const
{
   const MOMENTCAPACITYDETAILS* pMCD = nullptr;
   if ( pConfig == nullptr )
   {
      if (poi.GetID() == INVALID_ID)
      {
         // compute but don't cache since poiID is the key
         m_InvalidPoiMomentCapacity = ComputeMomentCapacity(intervalIdx, poi, bPositiveMoment);
         pMCD = &m_InvalidPoiMomentCapacity;
      }
      else
      {
         pMCD = GetCachedMomentCapacity(intervalIdx, poi, bPositiveMoment);
         if (pMCD == nullptr)
         {
            pMCD = ValidateMomentCapacity(intervalIdx, poi, bPositiveMoment);
         }
      }
   }
   else
   {
      // Get the current configuration and compare it to the provided one
      // If same, call GetMomentCapacityDetails w/o config.
      GET_IFACE(IBridge, pBridge);
      GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(poi.GetSegmentKey());

      if (poi.GetID() != INVALID_ID && curr_config.IsFlexuralDataEqual(*pConfig))
      {
         pMCD = GetMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);
      }
      else
      {
         // the capacity details for the requested girder configuration is not the same as for the
         // current input... see if it is cached
         pMCD = GetCachedMomentCapacity(intervalIdx, poi, bPositiveMoment, pConfig);
         if (pMCD == nullptr)
         {
            // the capacity has not yet been computed for this config, moment type, stage, and poi
            pMCD = ValidateMomentCapacity(intervalIdx, poi, bPositiveMoment, pConfig); // compute it
         }
      }
   }

   ATLASSERT(pMCD != nullptr);

   return pMCD;
}

std::vector<const MOMENTCAPACITYDETAILS*> pgsMomentCapacityEngineer::GetMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   std::vector<const MOMENTCAPACITYDETAILS*> details;
   details.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const MOMENTCAPACITYDETAILS* pmcd = GetMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, pConfig);
      details.push_back(pmcd);
   }

   return details;
}

std::vector<Float64> pgsMomentCapacityEngineer::GetCrackingMoment(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const
{
   std::vector<Float64> Mcr;
   Mcr.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Mcr.push_back(GetCrackingMoment(intervalIdx, poi, bPositiveMoment));
   }

   return Mcr;
}

Float64 pgsMomentCapacityEngineer::GetCrackingMoment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   const CRACKINGMOMENTDETAILS* pcmd = GetCrackingMomentDetails(intervalIdx, poi, bPositiveMoment);

   Float64 Mcr = pcmd->Mcr;
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // in LRFD 2nd Edition 2003 Mcr = Sc(fr + fcpe) - Mdnc(Sc/Snc - 1) <= Scfr
   // however there is a typographical error... Mcr should be
   // Mcr = Sc(fr + fcpe) - Mdnc(Sc/Snc - 1) >= Scfr
   // This correction was made in LRFD 3rd Edition 2005.
   // 
   // We are going to use the correct equation from 2nd Edition 2003 forward.
   // The limiting value was removed in LRFD 6th Edition, 2012
   bool bAfter2002 = (lrfdVersionMgr::SecondEditionWith2003Interims <= pSpecEntry->GetSpecificationType() ? true : false);
   bool bBefore2012 = (pSpecEntry->GetSpecificationType() <  lrfdVersionMgr::SixthEdition2012 ? true : false);
   if (bAfter2002 && bBefore2012)
   {
      Mcr = (bPositiveMoment ? Max(pcmd->Mcr, pcmd->McrLimit) : Min(pcmd->Mcr, pcmd->McrLimit));
   }
   else
   {
      Mcr = pcmd->Mcr;
   }

   return Mcr;
}

const CRACKINGMOMENTDETAILS* pgsMomentCapacityEngineer::GetCrackingMomentDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   ATLASSERT(poi.GetID() != INVALID_ID);

   return ValidateCrackingMoments(intervalIdx, poi, bPositiveMoment);
}

void pgsMomentCapacityEngineer::GetCrackingMomentDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config, bool bPositiveMoment, CRACKINGMOMENTDETAILS* pcmd) const
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge, pBridge);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   ATLASSERT(config.SegmentKey.IsEqual(segmentKey));

   GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(segmentKey);
   if (poi.GetID() != INVALID_ID && curr_config.IsFlexuralDataEqual(config))
   {
      *pcmd = *GetCrackingMomentDetails(intervalIdx, poi, bPositiveMoment);
   }
   else
   {
      ComputeCrackingMoment(intervalIdx, poi, config, bPositiveMoment, pcmd);
   }
}

std::vector<Float64> pgsMomentCapacityEngineer::GetMinMomentCapacity(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const
{
   std::vector<Float64> Mmin;
   Mmin.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Mmin.push_back(GetMinMomentCapacity(intervalIdx, poi, bPositiveMoment));
   }

   return Mmin;
}

Float64 pgsMomentCapacityEngineer::GetMinMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   const MINMOMENTCAPDETAILS* pmmcd = GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);
   return pmmcd->MrMin;
}

const MINMOMENTCAPDETAILS* pgsMomentCapacityEngineer::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   return ValidateMinMomentCapacity(intervalIdx, poi, bPositiveMoment);
}

void pgsMomentCapacityEngineer::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config, bool bPositiveMoment, MINMOMENTCAPDETAILS* pmmcd) const
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge, pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(segmentKey);
   if (poi.GetID() != INVALID_ID && curr_config.IsFlexuralDataEqual(config))
   {
      *pmmcd = *GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);
   }
   else
   {
      ComputeMinMomentCapacity(intervalIdx, poi, config, bPositiveMoment, pmmcd);
   }
}

std::vector<const MINMOMENTCAPDETAILS*> pgsMomentCapacityEngineer::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const
{
   std::vector<const MINMOMENTCAPDETAILS*> details;
   details.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const MINMOMENTCAPDETAILS* pmmcd = GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);
      details.push_back(pmmcd);
   }

   return details;
}

std::vector<const CRACKINGMOMENTDETAILS*> pgsMomentCapacityEngineer::GetCrackingMomentDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const
{
   std::vector<const CRACKINGMOMENTDETAILS*> details;
   details.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CRACKINGMOMENTDETAILS* pcmd = GetCrackingMomentDetails(intervalIdx, poi, bPositiveMoment);
      details.push_back(pcmd);
   }

   return details;
}

Float64 pgsMomentCapacityEngineer::GetIcr(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   const CRACKEDSECTIONDETAILS* pCSD = GetCrackedSectionDetails(poi, bPositiveMoment);
   return pCSD->Icr;
}

const CRACKEDSECTIONDETAILS* pgsMomentCapacityEngineer::GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   return ValidateCrackedSectionDetails(poi, bPositiveMoment);
}

std::vector<const CRACKEDSECTIONDETAILS*> pgsMomentCapacityEngineer::GetCrackedSectionDetails(const PoiList& vPoi, bool bPositiveMoment) const
{
   std::vector<const CRACKEDSECTIONDETAILS*> details;
   details.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CRACKEDSECTIONDETAILS* pCSD = GetCrackedSectionDetails(poi, bPositiveMoment);
      details.push_back(pCSD);
   }

   return details;
}

////////////////////////////////////////////////////////

MOMENTCAPACITYDETAILS pgsMomentCapacityEngineer::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight); // we just want E so it's ok to use Straight

   GET_IFACE(IPointOfInterest, pPoi);
   bool bIsOnSegment = pPoi->IsOnSegment(poi);
   bool bIsOnGirder = pPoi->IsOnGirder(poi);
   bool bIsInBoundaryPierDiaphragm = pPoi->IsInBoundaryPierDiaphragm(poi);

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bIncludeStrandsWithNegativeMoment = pSpecEntry->IncludeStrandForNegativeMoment();

   Float64 Eps = pStrand->GetE();
   Float64 fpe_ps_all_strands = 0.0; // "average" value for all strands to keep reporting consistent with previous versions
   Float64 eps_initial_all_strands = 0.0; // "average" value for all strands to keep reporting consistent with previous versions
   std::array<std::vector<Float64>, 2> fpe_ps; // effective prestress after all losses
   std::array<std::vector<Float64>, 2> eps_initial; // initial strain in the preressing strands (strain at effective prestress)
   if ( bPositiveMoment || bIncludeStrandsWithNegativeMoment || 0 < nSegmentDucts || 0 < nGirderDucts )
   {
      // only consider strands in positive moment analysis or if there are ducts
      // otherwise, strands are ignored for negative moment analysis
      if (bIsOnSegment || bIsInBoundaryPierDiaphragm)
      {
         GET_IFACE(IStrandGeometry, pStrandGeometry);
         GET_IFACE(IPretensionForce, pPrestressForce);
         // The effective prestress does not include a reduction for prestress transfer, however the prestress force does
         // We need to account for lack of strain in the strand over the transfer length in the analysis
         // so we will compute the effective prestress as fpe = Ppe/A and then eps as fpe/Ep
         Float64 Ps = pPrestressForce->GetPrestressForce(poi, pgsTypes::Straight, intervalIdx, pgsTypes::End, pConfig);
         Float64 Ph = pPrestressForce->GetPrestressForce(poi, pgsTypes::Harped, intervalIdx, pgsTypes::End, pConfig);
         Float64 Ppe = Ps + Ph;

         GET_IFACE(IStrandGeometry, pStrandGeom);
         Float64 Aps = pStrandGeom->GetStrandArea(poi, intervalIdx, pgsTypes::Permanent, pConfig);

         fpe_ps_all_strands = IsZero(Aps) ? 0.0 : Ppe / Aps;
         eps_initial_all_strands = fpe_ps_all_strands / Eps;

         for (int i = 0; i < 2; i++)
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

            // effective prestress does not include reduction for prestress transfer (this is what we want so we can do per-strand reductions)
            Float64 fpe = pPrestressForce->GetEffectivePrestress(poi, strandType, intervalIdx, pgsTypes::End, pConfig);

            StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, strandType, pConfig);
            for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
            {
               Float64 transfer_length_factor = pPrestressForce->GetTransferLengthAdjustment(poi, strandType, strandIdx, pConfig);
               fpe_ps[strandType].push_back(transfer_length_factor * fpe);
               eps_initial[strandType].push_back(fpe_ps[strandType].back() / Eps);
            }
         }
      }
   }

   GET_IFACE_NOCHECK(IPosttensionForce, pPTForce); // not used if there aren't any tendons

   std::vector<Float64> fpe_pt_segment;
   std::vector<Float64> ept_initial_segment;
   const matPsStrand* pTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);
   Float64 EptSegment = pTendon->GetE();
   for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
   {
      Float64 fpe = 0;
      Float64 e = 0;
      if (bIsOnSegment)
      {
         fpe = pPTForce->GetSegmentTendonStress(poi, intervalIdx, pgsTypes::End, ductIdx);
         if (fpe < 0)
         {
            fpe = 0;
         }
         e = fpe / EptSegment;
      }
      fpe_pt_segment.push_back(fpe);
      ept_initial_segment.push_back(e);
   }

   std::vector<Float64> fpe_pt_girder;
   std::vector<Float64> ept_initial_girder;
   pTendon = pMaterial->GetGirderTendonMaterial(segmentKey);
   Float64 EptGirder = pTendon->GetE();
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      Float64 fpe = 0;
      Float64 e = 0;
      if ( bIsOnGirder )
      {
         fpe = pPTForce->GetGirderTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx);
         if ( fpe < 0 )
         {
            fpe = 0;
         }
         e = fpe/ EptGirder;
      }
      fpe_pt_girder.push_back(fpe);
      ept_initial_girder.push_back(e);
   }

   return ComputeMomentCapacity(intervalIdx,poi,pConfig,fpe_ps_all_strands,eps_initial_all_strands,fpe_ps,eps_initial,fpe_pt_segment,ept_initial_segment, fpe_pt_girder, ept_initial_girder,bPositiveMoment);
}

MOMENTCAPACITYDETAILS pgsMomentCapacityEngineer::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64 fpe_ps_all_strands,Float64 eps_initial_all_strands, const std::array<std::vector<Float64>, 2>& fpe_ps, const std::array<std::vector<Float64>, 2>& eps_initial, const std::vector<Float64>& fpe_pt_segment, const std::vector<Float64>& ept_initial_segment, const std::vector<Float64>& fpe_pt_girder,const std::vector<Float64>& ept_initial_girder,bool bPositiveMoment) const
{
   MOMENTCAPACITYDETAILS mcd;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   pgsBondTool bondTool(m_pBroker, poi, pConfig);

   GET_IFACE(ISegmentTendonGeometry,pSegmentTendonGeometry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE_NOCHECK(IStrandGeometry,pStrandGeom); // only used for positive moment

   StrandIndexType Ns = 0;
   StrandIndexType Nh = 0;
   if ( bPositiveMoment || 0 < nSegmentDucts || 0 < nGirderDucts )
   {
      // Strands only modeled for positive moment calculations or all calculations when ducts are present
      Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight,pConfig);
      Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped,pConfig);
   }

   DuctIndexType NptSegment = 0;
   for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
   {
      NptSegment += pSegmentTendonGeometry->GetTendonStrandCount(segmentKey, ductIdx);
   }

   DuctIndexType NptGirder = 0;
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      NptGirder += pGirderTendonGeometry->GetTendonStrandCount(segmentKey,ductIdx);
   }

   bool bIsSplicedGirder = (0 < (nSegmentDucts+nGirderDucts) ? true : false);

   GET_IFACE(IPointOfInterest,pPoi);
   bool bIsOnSegment = pPoi->IsOnSegment(poi);
   bool bIsOnGirder = pPoi->IsOnGirder(poi);

   // create a problem to solve
   CComPtr<IGeneralSection> section;
   CComPtr<IPoint2d> pntCompression; // location of the extreme compression face
   Float64 ec; // compression strain limit for upper-most piece of concrete (typically the deck)
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H; // overall height of section
   Float64 Haunch; // haunch build up that is modeled
   bool bDevelopmentReducedStrainCapacity; // indicates if the strain limit in reinforcement is reduced because of lack of full development length
   BuildCapacityProblem(intervalIdx,poi,pConfig,eps_initial,ept_initial_segment,ept_initial_girder,bondTool,bPositiveMoment,&section,&pntCompression,&ec,&dt,&H,&Haunch,&bDevelopmentReducedStrainCapacity);

   CComPtr<IMomentCapacitySolution> solution;
   if (section)
   {
#if defined _DEBUG_SECTION_DUMP
      DumpSection(poi, section, bond_factors[0], bond_factors[1], bPositiveMoment);
#endif // _DEBUG_SECTION_DUMP

      GET_IFACE(ILibrary, pLib);
      GET_IFACE(ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      bool bConsiderReinforcementStrainLimits = pSpecEntry->ConsiderReinforcementStrainLimitForMomentCapacity();
      IndexType nSlices = pSpecEntry->GetSliceCountForMomentCapacity();

      m_MomentCapacitySolver->putref_Section(section);
      m_MomentCapacitySolver->put_Slices((long)nSlices);
      m_MomentCapacitySolver->put_SliceGrowthFactor(3);
      m_MomentCapacitySolver->put_MaxIterations(50);

      // Set the convergence tolerance to 0.1N. This is more than accurate enough for the
      // output display. Output accuracy for SI = 0.01kN = 10N, for US = 0.01kip = 45N
      m_MomentCapacitySolver->put_AxialTolerance(0.1);

      // determine neutral axis angle
      // compression is on the left side of the neutral axis
      Float64 na_angle = (bPositiveMoment ? 0.00 : M_PI);

#if defined _DEBUG
      CTime startTime = CTime::GetCurrentTime();
#endif // _DEBUG

      HRESULT hr = m_MomentCapacitySolver->Solve(0.00, na_angle, ec, 0.0, smFixedCompressionStrain, &solution);
      if (hr != S_OK && hr != RC_E_MATERIALFAILURE)
      {
         // if the solution doesn't converge, try using more slices
         m_MomentCapacitySolver->put_Slices( 2* (long)nSlices);
         solution.Release();
         hr = m_MomentCapacitySolver->Solve(0.00, na_angle, ec, 0.0, smFixedCompressionStrain, &solution);
      }

      if (hr == S_OK)
      {
         mcd.Controlling = MOMENTCAPACITYDETAILS::ControllingType::Concrete; // capacity was controlled by concrete strain
      }
      else if (hr == RC_E_MATERIALFAILURE)
      {
         hr = S_OK; // reset
         WATCHX(MomCap, 0, _T("Exceeded material strain limit"));
         if (bConsiderReinforcementStrainLimits)
         {
         CComPtr<IGeneralSectionSolution> general_solution;
         solution->get_GeneralSectionSolution(&general_solution);

         Float64 max_overstrain_ratio = 0;
         IndexType controllingOverstrainedSliceIdx = INVALID_INDEX;

         IndexType nSlices;
         general_solution->get_SliceCount(&nSlices);
         for (IndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++)
         {
            CComPtr<IGeneralSectionSlice> slice;
            general_solution->get_Slice(sliceIdx, &slice);
            VARIANT_BOOL vbExceededStrainLimit;
            slice->ExceededStrainLimit(&vbExceededStrainLimit);
            if (vbExceededStrainLimit == VARIANT_TRUE)
            {
               Float64 total_strain;
               slice->get_TotalStrain(&total_strain);
               CComPtr<IStressStrain> fgMaterial;
               slice->get_ForegroundMaterial(&fgMaterial);
               // if fgMaterial is null, slice is a void so we will skip it
               if (fgMaterial)
               {
                   Float64 emin, emax;
                   fgMaterial->StrainLimits(&emin, &emax);
                   ATLASSERT(!IsZero(emin) && !IsZero(emax));
                   Float64 overstrain_ratio = Max(total_strain / emin, total_strain / emax);

                   if (max_overstrain_ratio < overstrain_ratio)
                   {
                       max_overstrain_ratio = overstrain_ratio;
                       controllingOverstrainedSliceIdx = sliceIdx;
                   }
               }
            }
         }

         ATLASSERT(controllingOverstrainedSliceIdx != INVALID_INDEX); // if this is INVALID_INDEX, we didn't find an over-strained slice but should have

         CComPtr<IGeneralSectionSlice> slice;
         general_solution->get_Slice(controllingOverstrainedSliceIdx, &slice);

         IndexType shapeIdx;
         slice->get_ShapeIndex(&shapeIdx);

         CComPtr<IShape> s;
         section->get_Shape(shapeIdx, &s);
         CComPtr<IShapeProperties> props;
         s->get_ShapeProperties(&props);
         CComPtr<IPoint2d> pntCG;
         props->get_Centroid(&pntCG);
         Float64 Xcg, Ycg;
         pntCG->Location(&Xcg, &Ycg);

         // get the foreground model for the shape and get its max usable strain
         CComPtr<IStressStrain> ssModel;
         section->get_ForegroundMaterial(shapeIdx, &ssModel);
         Float64 emin, emax;
         ssModel->StrainLimits(&emin, &emax);

         // get the initial strain
         CComPtr<IPlane3d> initial_strain;
         section->get_InitialStrain(shapeIdx, &initial_strain);
         Float64 ei;
         initial_strain->GetZ(Xcg, Ycg, &ei);

         Float64 e = emax - ei;
         hr = m_MomentCapacitySolver->Solve(0.0, na_angle, e, Ycg, smFixedStrain, &solution.p);
         ATLASSERT(SUCCEEDED(hr));
         }
         mcd.Controlling = bDevelopmentReducedStrainCapacity ? MOMENTCAPACITYDETAILS::ControllingType::Development : MOMENTCAPACITYDETAILS::ControllingType::ReinforcementStrain;
      }

      if (FAILED(hr))
      {
         GET_IFACE(IEAFStatusCenter, pStatusCenter);
         GET_IFACE(IEAFDisplayUnits, pDisplayUnits);

         CString strErrorCode;
         switch (hr)
         {
         case RC_E_INITCONCRETE:          strErrorCode = _T("RC_E_INITCONCRETE");          break;
         case RC_E_SOLUTIONNOTFOUND:      strErrorCode = _T("RC_E_SOLUTIONNOTFOUND");      break;
         case RC_E_BEAMNOTSYMMETRIC:      strErrorCode = _T("RC_E_BEAMNOTSYMMETRIC");      break;
         case RC_E_MATERIALFAILURE:       strErrorCode = _T("RC_E_MATERIALFAILURE");       break;
         case RC_E_NEUTRALAXISNOTBOUNDED: strErrorCode = _T("RC_E_NEUTRALAXISNOTBOUNDED"); break;
         case RC_E_SECTION:               strErrorCode = _T("RC_E_SECTION");               break;
         case RC_E_FGMATERIAL:            strErrorCode = _T("RC_E_FGMATERIAL");            break;
         case RC_E_BGMATERIAL:            strErrorCode = _T("RC_E_BGMATERIAL");            break;
         case E_FAIL:                     strErrorCode = _T("E_FAIL");                     break;
         default:                         strErrorCode.Format(_T("0x%X"), hr);
         }

         const unitmgtLengthData& unit = pDisplayUnits->GetSpanLengthUnit();
         CString msg;
         msg.Format(_T("An unknown error occurred while computing %s moment capacity for %s at %f %s from the left end of the girder.\n(hr = %s)\n(Location ID = %d).\nPlease send your file to technical support."),
            (bPositiveMoment ? _T("positive") : _T("negative")),
            SEGMENT_LABEL(segmentKey),
            ::ConvertFromSysUnits(poi.GetDistFromStart(), unit.UnitOfMeasure),
            unit.UnitOfMeasure.UnitTag().c_str(),
            strErrorCode,
            poi.GetID()
         );
         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidMomentCapacity, msg);
         pStatusCenter->Add(pStatusItem);
      }

#if defined _DEBUG
      CTime endTime = CTime::GetCurrentTime();
      CTimeSpan duration = endTime - startTime;
      WATCHX(MomCap,0,_T("Duration = ") << duration.GetTotalSeconds() << _T(" seconds"));
#endif // _DEBUG
   }

   mcd.Section = section;
   mcd.CapacitySolution = solution;

   Float64 Fz(0.0),Mx(0.0),My(0.0);
   if (solution)
   {
      solution->get_Fz(&Fz);
      solution->get_Mx(&Mx);
      solution->get_My(&My);
   }

   ATLASSERT( IsZero(Fz,0.1) );
   ATLASSERT( Mx != 0.0 ? IsZero(My/Mx,0.30) : true );  // when there is an odd number of harped strands, the strands aren't always symmetrical
                                     // this will cause a small amount of off axis bending.
                                     // Only assert if the ratio of My/Mx is larger that the tolerance for zero
   // RAB: 6/2017 - loosened the tolerance because the deck bulb tees are non-symmetric girders even thought we are assuming unixaial bending. 

   Float64 Mn = -Mx;

   Mn = IsZero(Mn) ? 0.0 : Mn;

   mcd.Mn  = Mn;

   if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
   {
      GET_IFACE_NOCHECK(ILongRebarGeometry, pLongRebarGeom);

      mcd.PPR = (bPositiveMoment ? pLongRebarGeom->GetPPRBottomHalf(poi, pConfig) : 0.0);
   }
   else
   {
      // PPR was removed from LRFD in 6th Edition, 2012. Use 1.0 for positive moments so
      // Phi = PhiRC + (PhiPS-PhiRC)*PPR = PhiRC + PhiPS - PhiRC = PhiPS and use 0.0 for negative moment so
      // Phi = PhiRC + (PhiPS-PhiRC)*0.0 = PhiRC
      //
      // See computation of Phi about 5 lines down
      if ( bIsSplicedGirder )
      {
         mcd.PPR = 1.0;
      }
      else
      {
         mcd.PPR = (bPositiveMoment ? 1.0 : 0.0);
      }
   }

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   matRebar::Type rebarType;
   matRebar::Grade deckRebarGrade;
   pMaterial->GetDeckRebarMaterial(&rebarType,&deckRebarGrade);

   GET_IFACE(IResistanceFactors,pResistanceFactors);
   Float64 PhiRC,PhiPS,PhiSP,PhiC;
   CClosureKey closureKey;
   if ( pPoi->IsInClosureJoint(poi,&closureKey) )
   {
      mcd.Phi = pResistanceFactors->GetClosureJointFlexureResistanceFactor(concType);
   }
   else
   {
      pResistanceFactors->GetFlexureResistanceFactors(concType,&PhiPS,&PhiRC,&PhiSP,&PhiC);
      if ( bIsSplicedGirder )
      {
         mcd.Phi = PhiSP;
      }
      else
      {
         mcd.Phi = PhiRC + (PhiPS-PhiRC)*mcd.PPR; // generalized form of 5.5.4.2.1-3
                                                      // Removed in AASHTO LRFD 6th Edition 2012, however
                                                      // PPR has been computed above to take this into account
      }
   }

   Float64 C(0.0),T(0.0);
   if (solution)
   {
      solution->get_CompressionResultant(&C);
      solution->get_TensionResultant(&T);
   }
   ATLASSERT(IsZero(C+T,0.5)); // equilibrium within 0.5 Newtons
   
   mcd.C = C;
   mcd.T = T;

   CComPtr<IPoint2d> cgC, cgT;
   if (solution)
   {
      solution->get_CompressionResultantLocation(&cgC);
      solution->get_TensionResultantLocation(&cgT);
   }
   else
   {
      cgC.CoCreateInstance(CLSID_Point2d);
      cgT.CoCreateInstance(CLSID_Point2d);
   }

   Float64 fps_avg = 0;
   Float64 fpt_avg_segment = 0;
   Float64 fpt_avg_girder = 0;

   const matPsStrand* pSegmentTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);
   const matPsStrand* pGirderTendon = pMaterial->GetGirderTendonMaterial(segmentKey);

   if ( IsZero(Mn) || solution == nullptr)
   {
      // dimensions have no meaning if no moment capacity
      mcd.c          = 0.0;
      mcd.dc         = 0.0;
      mcd.MomentArm  = 0.0;
      mcd.de         = 0.0;
      mcd.de_shear   = 0.0;
      
      fps_avg         = 0.0;
      fpt_avg_segment = 0.0;
      fpt_avg_girder  = 0.0;
   }
   else
   {
      mcd.MomentArm = fabs(Mn/T);

      Float64 x1,y1, x2,y2;
      pntCompression->get_X(&x1);
      pntCompression->get_Y(&y1);

      cgC->get_X(&x2);
      cgC->get_Y(&y2);

      mcd.dc = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
      mcd.de = mcd.dc + mcd.MomentArm;

      CComPtr<IPlane3d> strainPlane;
      solution->get_StrainPlane(&strainPlane);
      Float64 x,y,z;
      x = 0;
      z = 0;
      strainPlane->GetY(x,z,&y);

      mcd.c = sqrt((x1-x)*(x1-x) + (y1-y)*(y1-y));

      // calculate average resultant stress in strands and tendons at ultimate
      // also determine de (see PCI BDM 8.4.1.2).
      // de = resultant of tension force due to reinforcement on the tension half of the beam
      Float64 H_over_2 = H / 2;
      GET_IFACE(IBridge, pBridge);
      Float64 tSlab = pBridge->GetStructuralSlabDepth(poi);
      Float64 t = 0; // sum of the tension forces 
      Float64 tde = 0; // sum of tension force * depth to reinforcement
      Float64 Aps = 0;
      Float64 Apt_Segment = 0;
      Float64 Apt_Girder = 0;
      enum class ReinforcementType { Strand, SegmentTendon, GirderTendon, GirderRebar, TopMatDeckRebar, BottomMatDeckRebar };
      CComPtr<IGeneralSectionSolution> general_solution;
      solution->get_GeneralSectionSolution(&general_solution);
      IndexType nSlices;
      general_solution->get_SliceCount(&nSlices);
      for (IndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++)
      {
         CComPtr<IGeneralSectionSlice> slice;
         general_solution->get_Slice(sliceIdx, &slice);

         IndexType shapeIdx;
         slice->get_ShapeIndex(&shapeIdx);

         ReinforcementType reinforcementType;
         CComBSTR bstrName;
         mcd.Section->get_Name(shapeIdx, &bstrName);
         CString strName(bstrName);
         if (0 <= strName.Find(_T("Strand"), 0))
         {
            reinforcementType = ReinforcementType::Strand;
         }
         else if (CString(bstrName).Left(14) == _T("Segment Tendon"))
         {
            reinforcementType = ReinforcementType::SegmentTendon;
         }
         else if (CString(bstrName).Left(13) == _T("Girder Tendon"))
         {
            reinforcementType = ReinforcementType::GirderTendon;
         }
         else if (CString(bstrName).Left(5) == _T("Girder Rebar"))
         {
            reinforcementType = ReinforcementType::GirderRebar;
         }
         else if (CString(bstrName) == _T("Top Mat Deck Rebar"))
         {
            reinforcementType = ReinforcementType::TopMatDeckRebar;
         }
         else if (CString(bstrName) == _T("Bottom Mat Deck Rebar"))
         {
            reinforcementType = ReinforcementType::BottomMatDeckRebar;
         }
         else
         {
            continue;
         }

         Float64 slice_area;
         slice->get_Area(&slice_area);

         CComPtr<IPoint2d> pntCG;
         slice->get_CG(&pntCG);

         Float64 cgY;
         pntCG->get_Y(&cgY);

         // We are computing the location of the resultant stress in the reinforcement.
         // The foreground stress times are slice_area is the force in the reinforcement.
         // Subtracting the background stress would be the net stress at the location and 
         // that's not what we need for this calculation.
         Float64 fgStress;
         slice->get_ForegroundStress(&fgStress);

         Float64 F = slice_area * fgStress;
         Float64 d = (Haunch + tSlab) - cgY; // cgY is in girder section coordinates (Y=0 at top of non-composite girder)
         // subtracting from (Haunch+tSlab) makes d a distance from the top of composite girder section

         if (0 < F &&  // tension
            (
               (bPositiveMoment && ::IsGE(H_over_2, d)) || // on bottom half
               (!bPositiveMoment && ::IsLE(d, H_over_2))    // on top half
               )
            )
         {
            t += F;
            tde += F * d;
         }

         if (reinforcementType == ReinforcementType::Strand)
         {
            Aps += slice_area;
            fps_avg += F;
         }
         else if (reinforcementType == ReinforcementType::SegmentTendon)
         {
            Apt_Segment += slice_area;
            fpt_avg_segment += F;
         }
         else if (reinforcementType == ReinforcementType::GirderTendon)
         {
            Apt_Girder += slice_area;
            fpt_avg_girder += F;
         }
      } // next slice

      if (0 < Aps)
      {
         fps_avg /= Aps;
      }

      if (0 < Apt_Segment)
      {
         fpt_avg_segment /= Apt_Segment;
      }

      if (0 < Apt_Girder)
      {
         fpt_avg_girder /= Apt_Girder;
      }

      mcd.de_shear = (IsZero(t) ? 0 : tde / t);

      if (!bPositiveMoment)
      {
         // de_shear is measured from the top of the girder
         // for negative moment, we want it to be measured from the bottom
         mcd.de_shear = H - mcd.de_shear;
      }

      if (IsZero(mcd.de_shear))
      {
         // de_shear is zero when there isn't any reinforcement on the "flexural tension side" of the member defined by
         // the mid-depth of the member. This typically occurs in spliced girders at intermediate piers where the
         // tendons are at the top of the girder and extended reinforcement is not modeled at the bottom of the girder
         // to make the positive moment connection. When de_shear is zero, analysis results are nonsense.
         // We will use the depth to the resultant tension force as de_shear
         mcd.de_shear = mcd.de;
      }
   }


   WATCHX(MomCap,0, _T("X = ") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") << _T("   Mn = ") << ::ConvertFromSysUnits(Mn,unitMeasure::KipFeet) << _T(" kip-ft") << _T(" My/Mx = ") << My/Mn << _T(" fps_avg = ") << ::ConvertFromSysUnits(fps_avg,unitMeasure::KSI) << _T(" KSI"));

   mcd.fps_avg = fps_avg;
   mcd.fpt_avg_segment = fpt_avg_segment;
   mcd.fpt_avg_girder = fpt_avg_girder;
   mcd.dt  = dt;
   mcd.bOverReinforced = false;

   GET_IFACE(ISpecification, pSpec);
   mcd.Method = pSpec->GetMomentCapacityMethod();

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( mcd.Method == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims)
   {
      mcd.bOverReinforced = (mcd.c / mcd.de > 0.42) ? true : false;
      if ( mcd.bOverReinforced )
      {
         GET_IFACE(IMaterials,pMaterial);
         Float64 de = mcd.de;
         Float64 c  = mcd.c;

         Float64 hf;
         Float64 b;
         Float64 bw;
         Float64 fc;
         Float64 Beta1;
         if ( bPositiveMoment )
         {
            GET_IFACE(ISectionProperties,pProps);
            GET_IFACE(IBridge, pBridge);
            GET_IFACE(IGirder, pGdr);

            bw = pGdr->GetWebWidth(poi);

            if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
            {
               b     = pGdr->GetTopFlangeWidth(poi);
               hf    = pGdr->GetMinTopFlangeThickness(poi);
               fc    = pMaterial->GetSegmentDesignFc(segmentKey,intervalIdx);
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
            else
            {
               b     = pProps->GetEffectiveFlangeWidth(poi);
               hf    = pBridge->GetStructuralSlabDepth(poi);
               fc    = pMaterial->GetDeckDesignFc(intervalIdx);
               Beta1 = lrfdConcreteUtil::Beta1(fc);
            }
         }
         else
         {
            GET_IFACE(IGirder, pGdr);
            hf = pGdr->GetMinBottomFlangeThickness(poi);
            b  = pGdr->GetBottomWidth(poi);
            bw = pGdr->GetWebWidth(poi);
            fc = pMaterial->GetSegmentDesignFc(segmentKey,intervalIdx);
            Beta1 = lrfdConcreteUtil::Beta1(fc);
         }

         mcd.FcSlab = fc;
         mcd.b = b;
         mcd.bw = bw;
         mcd.hf = hf;
         mcd.Beta1Slab = Beta1;

         if ( c <= hf )
         {
            mcd.bRectSection = true;
            mcd.MnMin = (0.36*Beta1 - 0.08*Beta1*Beta1)*fc*b*de*de;
         }
         else
         {
            // T-section behavior
            mcd.bRectSection = false;
            mcd.MnMin = (0.36*Beta1 - 0.08*Beta1*Beta1)*fc*bw*de*de 
                        + 0.85*Beta1*fc*(b - bw)*hf*(de - 0.5*hf);
         }

         if ( !bPositiveMoment )
         {
            mcd.MnMin *= -1;
         }
      }
      else
      {
         // Dummy values
         mcd.bRectSection = true;
         mcd.MnMin = 0;
      }
   }
   else
   {
      // WSDOT method 2005... LRFD 2006 and later

      // the method of compute phi based on strains was introduced in WSDOT 2005/LRFD 2006, however it only included
      // prestressing strand and grade 60 rebar. PGSuper can model grade 40-80 rebar. Use the strain based method
      // for computing Phi. This method is in WSDOT BDM 2012 and will be in LRFD 2013
      Float64 ecl, etl;
      if ( bIsSplicedGirder )
      {
         // girder tendons are the primary reinforcement so use the girder tendon properties to get the flexure strain limit
         if (NptGirder == 0)
         {
            // no girder tendon... then use segment tendon
            pResistanceFactors->GetFlexuralStrainLimits(pSegmentTendon->GetGrade(), pSegmentTendon->GetType(), &ecl, &etl);
         }
         else
         {
            pResistanceFactors->GetFlexuralStrainLimits(pGirderTendon->GetGrade(), pGirderTendon->GetType(), &ecl, &etl);
         }
      }
      else
      {
         if ( bPositiveMoment )
         {
            // we just want grade and type so using Straight is fine
            const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Straight);
            pResistanceFactors->GetFlexuralStrainLimits(pStrand->GetGrade(),pStrand->GetType(),&ecl,&etl);
         }
         else
         {
            pResistanceFactors->GetFlexuralStrainLimits(deckRebarGrade,&ecl,&etl);
         }
      }
      mcd.ecl = ecl;
      mcd.etl = etl;

      mcd.et = 0;

      // Compute Phi based on the net tensile strain....
      // This not applicable at closure joints
      CClosureKey closureKey;
      if ( !pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         if ( IsZero(mcd.c) ) 
         {
            if ( bIsSplicedGirder )
            {
               mcd.Phi = PhiSP;
            }
            else
            {
               mcd.Phi = (bPositiveMoment ? PhiPS : PhiRC); // there is no moment capacity, use PhiRC for phi instead of dividing by zero
            }
         }
         else
         {
            mcd.et = (mcd.dt - mcd.c)*0.003/(mcd.c);
            if ( bIsSplicedGirder )
            {
               mcd.Phi = PhiC + (PhiSP - PhiC)*(mcd.et - ecl)/(etl-ecl);
            }
            else
            {
               if ( bPositiveMoment && (0 < Ns+Nh+NptSegment+NptGirder) )
               {
                  // Prestressed case
                  mcd.Phi = PhiC + (PhiPS - PhiC)*(mcd.et - ecl)/(etl-ecl);
               }
               else
               {
                  // Plain reinforced case
                  mcd.Phi = PhiC + (PhiRC - PhiC)*(mcd.et - ecl)/(etl-ecl);
               }
            }
         }

         if ( bIsSplicedGirder )
         {
            mcd.Phi = ForceIntoRange(PhiC,mcd.Phi,PhiSP);
         }
         else
         {
            mcd.Phi = ForceIntoRange(PhiC,mcd.Phi,PhiRC + (PhiPS-PhiRC)*mcd.PPR);
         }
      }
   }



   mcd.fpe_ps      = fpe_ps_all_strands;
   mcd.eps_initial = eps_initial_all_strands;

   mcd.fpe_pt_segment = fpe_pt_segment;
   mcd.ept_initial_segment = ept_initial_segment;

   mcd.fpe_pt_girder      = fpe_pt_girder;
   mcd.ept_initial_girder = ept_initial_girder;

   mcd.Mr = mcd.Phi*mcd.Mn;

#if defined _DEBUG
   pgsMomentCapacityEngineer* pThis = const_cast<pgsMomentCapacityEngineer*>(this);
   pThis->m_Log << _T("Dist from end ") << poi.GetDistFromStart() << endl;
   pThis->m_Log << _T("-------------------------") << endl;
   pThis->m_Log << endl;
#endif

   return mcd;
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const
{
   const MOMENTCAPACITYDETAILS* pmcd = GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

   const CRACKINGMOMENTDETAILS* pcmd = GetCrackingMomentDetails(intervalIdx,poi,bPositiveMoment);

   ComputeMinMomentCapacity(intervalIdx,poi,bPositiveMoment,pmcd,pcmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const
{
   const MOMENTCAPACITYDETAILS* pmcd = GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,&config);

   CRACKINGMOMENTDETAILS cmd;
   GetCrackingMomentDetails(intervalIdx, poi, config, bPositiveMoment, &cmd);

   ComputeMinMomentCapacity(intervalIdx,poi,bPositiveMoment,pmcd,&cmd,pmmcd);
}

void pgsMomentCapacityEngineer::ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS* pmcd,const CRACKINGMOMENTDETAILS* pcmd,MINMOMENTCAPDETAILS* pmmcd) const
{
   Float64 Mr;     // Nominal resistance (phi*Mn)
   Float64 MrMin;  // Minimum nominal resistance - Min(MrMin1,MrMin2)
   Float64 MrMin1; // 1.2Mcr
   Float64 MrMin2; // 1.33Mu
   Float64 Mu_StrengthI;
   Float64 Mu_StrengthII;
   Float64 Mu;     // Limit State Moment
   Float64 Mcr;    // Cracking moment

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable( poi.GetSegmentKey() );

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002  = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   bool bBefore2012 = ( pSpecEntry->GetSpecificationType() <  lrfdVersionMgr::SixthEdition2012 ? true : false );

   GET_IFACE(IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(bPositiveMoment ? pgsTypes::Maximize : pgsTypes::Minimize);

   Mr = pmcd->Phi * pmcd->Mn;

   if ( bAfter2002 && bBefore2012 )
   {
      Mcr = (bPositiveMoment ? Max(pcmd->Mcr,pcmd->McrLimit) : Min(pcmd->Mcr,pcmd->McrLimit));
   }
   else
   {
      Mcr = pcmd->Mcr;
   }
   Mcr = IsZero(Mcr) ? 0 : Mcr;


   if ( bPositiveMoment )
   {
      Float64 MuMin_StrengthI, MuMax_StrengthI;
      pLimitStateForces->GetMoment(intervalIdx,pgsTypes::StrengthI,poi,bat,&MuMin_StrengthI,&MuMax_StrengthI);
      Mu_StrengthI = MuMax_StrengthI;
   }
   else
   {
      Mu_StrengthI = pLimitStateForces->GetSlabDesignMoment(pgsTypes::StrengthI,poi,bat);
   }
      
   if ( bPermit )
   {
      if ( bPositiveMoment )
      {
         Float64 MuMin_StrengthII, MuMax_StrengthII;
         pLimitStateForces->GetMoment(intervalIdx,pgsTypes::StrengthII,poi,bat,&MuMin_StrengthII,&MuMax_StrengthII);
         Mu_StrengthII = MuMax_StrengthII;
      }
      else
      {
         Mu_StrengthII = pLimitStateForces->GetSlabDesignMoment(pgsTypes::StrengthII,poi,bat);
      }
   }
   else
   {
      Mu_StrengthII = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
   }

   pgsTypes::LimitState ls; // limit state of controlling Mu (greatest magnitude)
   if ( bPermit )
   {
      if ( fabs(Mu_StrengthII) < fabs(Mu_StrengthI) )
      {
         Mu = Mu_StrengthI;
         ls = pgsTypes::StrengthI;
      }
      else
      {
         Mu = Mu_StrengthII;
         ls = pgsTypes::StrengthII;
      }
   }
   else
   {
      Mu = Mu_StrengthI;
      ls = pgsTypes::StrengthI;
   }
   Mu = IsZero(Mu) ? 0 : Mu;

   if ( lrfdVersionMgr::SixthEdition2012 <= pSpecEntry->GetSpecificationType() )
   {
      MrMin1 = Mcr;
   }
   else
   {
      MrMin1 = 1.20*Mcr;
   }


   MrMin2 = 1.33*Mu;
   MrMin =  (bPositiveMoment ? Min(MrMin1,MrMin2) : Max(MrMin1,MrMin2));

   if (ls==pgsTypes::StrengthI)
   {
      pmmcd->LimitState = _T("Strength I");
   }
   else 
   {
      ATLASSERT(ls==pgsTypes::StrengthII);
      pmmcd->LimitState = _T("Strength II");
   }

   pmmcd->Mr     = Mr;
   pmmcd->MrMin  = MrMin;
   pmmcd->MrMin1 = MrMin1;
   pmmcd->MrMin2 = MrMin2;
   pmmcd->Mu     = Mu;
   pmmcd->Mcr    = Mcr;
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const
{
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPretensionStresses,pPrestress);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 fcpe = 0; // compressive stress in concrete due to effective prestress forces only
                     // (after allowance for all prestress losses) at extreme fiber of 
                     // section where tensile stress is caused by externally applied loads

   GET_IFACE(IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   pgsTypes::StressLocation stressLocation = (bPositiveMoment ? pgsTypes::BottomGirder : (IsStructuralDeck(deckType) ? pgsTypes::TopDeck : pgsTypes::TopGirder));

   // Compute stress due to prestressing
   Float64 Pps = pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   Float64 eps = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, pgsTypes::Permanent).Y(); // eccentricity of non-composite section

   Float64 dfcpe = pPrestress->GetStress(releaseIntervalIdx, poi,stressLocation,Pps,0.0,eps);
   if ( dfcpe < 0 )
   {
      fcpe += dfcpe; // only want compression stress
   }

   // Compute stress due to tendons
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(bPositiveMoment ? pgsTypes::Maximize : pgsTypes::Minimize);
   Float64 fDummy;
   pProductForces->GetStress(intervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,stressLocation,stressLocation,&dfcpe,&fDummy);
   if ( dfcpe < 0 )
   {
      fcpe += dfcpe;
   }

   // We have computed a compression stress so its sign is < 0. However, in the context of computing
   // the cracking moment, we want the equal and opposite tension stress that must overcome this compression
   // to induce cracking. Change sign of stress for use in LRFD Eq. 5.6.3.3-1 (pre2017: 5.7.3.3.2-1)
   ATLASSERT(fcpe <= 0);
   fcpe *= -1;

   ComputeCrackingMoment(intervalIdx,poi,fcpe,bPositiveMoment,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const
{
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress

#if defined _DEBUG
   // version of this method with GDRCONFIG does not apply to spliced girder bridges
   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   ATLASSERT(pTendonGeom->GetDuctCount(poi.GetSegmentKey()) == 0);
#endif

   if ( bPositiveMoment )
   {
      // Get stress at bottom of girder due to prestressing
      // Using negative because we want the amount tensile stress required to overcome the
      // precompression
      GET_IFACE(IPretensionForce,pPrestressForce);
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(IPretensionStresses,pPrestress);

      Float64 P = pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceI,INVALID_INDEX/*controlling live load*/,&config);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
      Float64 e = pStrandGeom->GetEccentricity( releaseIntervalIdx, poi, pgsTypes::Permanent, &config).Y(); // eccentricity of non-composite section

      fcpe = -pPrestress->GetStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,P,0.0,e);
   }
   else
   {
      // no precompression in the slab
      fcpe = 0;
   }

   ComputeCrackingMoment(intervalIdx,config,poi,fcpe,bPositiveMoment,pcmd);
}

void pgsMomentCapacityEngineer::GetCrackingMomentFactors(bool bPositiveMoment,Float64* pG1,Float64* pG2,Float64* pG3) const
{
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
   {
      *pG1 = 1.6; // all other concrete structures (not-segmental)
      *pG2 = 1.1; // bonded strand/tendon

      if ( bPositiveMoment )
      {
         *pG3 = 1.0; // prestressed concrete
      }
      else
      {
         GET_IFACE(IBridge,pBridge);
         if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
         {
            *pG3 = 1.0; // prestress concrete (no deck, so all we have is the beam)
         }
         else
         {
            GET_IFACE(IMaterials,pMaterials);
            Float64 E,fy,fu;
            pMaterials->GetDeckRebarProperties(&E,&fy,&fu);

            *pG3 = fy/fu;
         }
      }
   }
   else
   {
      *pG1 = 1.0;
      *pG2 = 1.0;
      *pG3 = 1.0;
   }
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(intervalIdx,poi,bPositiveMoment,&config);
   fr = GetModulusOfRupture(intervalIdx,poi,bPositiveMoment,&config);

   GetSectionProperties(intervalIdx,poi,config,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const
{
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get dead load moment on non-composite girder
   Mdnc = GetNonCompositeDeadLoadMoment(intervalIdx,poi,bPositiveMoment);
   fr = GetModulusOfRupture(intervalIdx,poi,bPositiveMoment);

   GetSectionProperties(intervalIdx,poi,bPositiveMoment,&Sb,&Sbc);

   Float64 g1,g2,g3;
   GetCrackingMomentFactors(bPositiveMoment,&g1,&g2,&g3);

   ComputeCrackingMoment(g1,g2,g3,fr,fcpe,Mdnc,Sb,Sbc,pcmd);
}

Float64 pgsMomentCapacityEngineer::GetNonCompositeDeadLoadMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IProductForces,pProductForces);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(bPositiveMoment ? pgsTypes::Maximize : pgsTypes::Minimize);

   Float64 Mdnc = 0; // Dead load moment on non-composite girder

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( bPositiveMoment )
   {
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx;
      if ( IsStructuralDeck(pBridge->GetDeckType()) )
      {
         IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
         intervalIdx = compositeIntervalIdx - 1;
      }
      else
      {
         intervalIdx = pIntervals->GetIntervalCount() - 1;
      }

      // Girder moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftGirder,poi, bat, rtCumulative);

      // Slab moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftSlab,   poi, bat, rtCumulative);
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftSlabPad,poi, bat, rtCumulative);

      // Diaphragm moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftDiaphragm,poi, bat, rtCumulative);

      // Shear Key moment
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftShearKey,poi, bat, rtCumulative);

      // User DC and User DW
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftUserDC,poi, bat, rtCumulative);
      Mdnc += pProductForces->GetMoment(intervalIdx,pgsTypes::pftUserDW,poi, bat, rtCumulative);
   }

   if (pConfig)
   {
      // add effect of different slab offset
      Float64 deltaSlab = pProductForces->GetDesignSlabMomentAdjustment(poi,pConfig);
      Mdnc += deltaSlab;

      Float64 deltaSlabPad = pProductForces->GetDesignSlabPadMomentAdjustment(poi,pConfig);
      Mdnc += deltaSlabPad;
   }

   Mdnc = IsZero(Mdnc) ? 0 : Mdnc;

   return Mdnc;
}

#include <IFace\Allowables.h>
Float64 pgsMomentCapacityEngineer::GetModulusOfRupture(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IMaterials,pMaterial);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest, pPoi);
   CClosureKey closureKey;
   bool bOnSegment = pPoi->IsOnSegment(poi);
   bool bInClosureJoint = pPoi->IsInClosureJoint(poi, &closureKey);
   bool bUHPC = (bOnSegment ? pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC : bInClosureJoint ? pMaterial->GetClosureJointConcreteType(closureKey) == pgsTypes::PCI_UHPC : false);
   
   GET_IFACE(IBridge, pBridge);
   bool bNonstructuralDeck = IsNonstructuralDeck(pBridge->GetDeckType());

   Float64 fr;   // Rupture stress

   if ((bPositiveMoment && bUHPC)  // Positive moment with UHPC segment or closure
      ||  // -OR-
      (!bPositiveMoment && bNonstructuralDeck && bUHPC) // Negative moment with nonstructural deck (moment is taken by girder) and UHPC segment or closure
      )
   {
      // PCI UHPC uses the tensile stress limit at service limit state, ft, instead of modulus of rupture, fr
      GET_IFACE(IAllowableConcreteStress, pAllowables);
      fr = pAllowables->GetAllowableTensionStress(poi, pgsTypes::BottomGirder, StressCheckTask(intervalIdx, pgsTypes::ServiceIII, pgsTypes::Tension), true, true);
   }
   else if (bPositiveMoment)
   {
      if (pConfig)
      {
         fr = pMaterial->GetFlexureModRupture(pConfig->fc28, pConfig->ConcType);
      }
      else
      {
         if (bOnSegment)
         {
            fr = pMaterial->GetSegmentFlexureFr(segmentKey, intervalIdx);
         }
         else if (bInClosureJoint)
         {
            fr = pMaterial->GetClosureJointFlexureFr(closureKey, intervalIdx);
         }
         else
         {
            IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
            fr = pMaterial->GetDeckFlexureFr(deckCastingRegionIdx, intervalIdx);
         }
      }
   }
   else
   {
      // negative moment
      if (bNonstructuralDeck)
      {
         // deck is not structural so moment is carried by the girder
         // NOTE: currently we don't support non-structural and no-deck spliced girders. When that happens, we need to deal with the POI being in a closure joint
         ATLASSERT(bInClosureJoint == false);
         if (pConfig)
         {
            fr = pMaterial->GetFlexureModRupture(pConfig->fc28, pConfig->ConcType);
         }
         else
         {
            fr = pMaterial->GetSegmentFlexureFr(segmentKey, intervalIdx);
         }
      }
      else
      {
         GET_IFACE(IPointOfInterest, pPoi);
         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         fr = pMaterial->GetDeckFlexureFr(deckCastingRegionIdx,intervalIdx);
      }
   }

   return fr;
}

void pgsMomentCapacityEngineer::GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,Float64* pSb,Float64* pSbc) const
{
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // Get the section moduli
   if ( bPositiveMoment )
   {
      GET_IFACE(IPointOfInterest,pPoi);

      Sbc = pSectProp->GetS(intervalIdx,poi,pgsTypes::BottomGirder);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) || pPoi->IsInBoundaryPierDiaphragm(poi) )
      {
         Sb = Sbc;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType idx = pIntervals->GetLastNoncompositeInterval();
         Sb  = pSectProp->GetS(idx,poi,pgsTypes::BottomGirder);
      }
   }
   else
   {

      GET_IFACE(IBridge, pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      pgsTypes::StressLocation stressLocation = (IsStructuralDeck(deckType) ? pgsTypes::TopDeck : pgsTypes::TopGirder);
      Sbc = pSectProp->GetS(intervalIdx,poi,stressLocation);
      Sb  = Sbc;
   }

   *pSb  = Sb;
   *pSbc = Sbc;
}

void pgsMomentCapacityEngineer::GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,Float64* pSb,Float64* pSbc) const
{
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder

   // 90 day strength isn't applicable to strength limit states (only stress limit states, LRFD 5.12.3.2.4)
   // so use 28day properties
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bUse90DayStrength;
   Float64 factor;
   pSpecEntry->Use90DayStrengthForSlowCuringConcrete(&bUse90DayStrength, &factor);

   // Get the section moduli
   if ( bPositiveMoment )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType idx = pIntervals->GetLastNoncompositeInterval();
      Sb = pSectProp->GetS(idx, poi, pgsTypes::BottomGirder, bUse90DayStrength ? config.fc28 : config.fc);
      Sbc = pSectProp->GetS(intervalIdx, poi, pgsTypes::BottomGirder, bUse90DayStrength ? config.fc28 : config.fc);
   }
   else
   {
      GET_IFACE(IBridge, pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      pgsTypes::StressLocation stressLocation = (IsStructuralDeck(deckType) ? pgsTypes::TopDeck : pgsTypes::TopGirder);
      Sbc = pSectProp->GetS(intervalIdx, poi, stressLocation, bUse90DayStrength ? config.fc28 : config.fc);
      Sb  = Sbc;
   }

   *pSb  = Sb;
   *pSbc = Sbc;
}

void pgsMomentCapacityEngineer::ComputeCrackingMoment(Float64 g1,Float64 g2,Float64 g3,Float64 fr,Float64 fcpe,Float64 Mdnc,Float64 Sb,Float64 Sbc,CRACKINGMOMENTDETAILS* pcmd) const
{
   Float64 Mcr = (g1*fr + g2*fcpe)*Sbc + Mdnc; // NOTE this is the same as -Mdnc(Sbc/Sb - 1).   -Mdnc(-1) = +Mdnc
   if (!IsZero(Sb))
   {
      Mcr -= Mdnc*(Sbc / Sb);
   }

   Mcr *= g3;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   if ( bAfter2002 )
   {
      Float64 McrLimit = Sbc*fr;
      pcmd->McrLimit = McrLimit;
   }

   pcmd->Mcr  = Mcr;
   pcmd->Mdnc = Mdnc;
   pcmd->fr   = fr;
   pcmd->fcpe = fcpe;
   pcmd->Sb   = Sb;
   pcmd->Sbc  = Sbc;
   pcmd->g1   = g1;
   pcmd->g2   = g2;
   pcmd->g3   = g3;
}

void pgsMomentCapacityEngineer::AnalyzeCrackedSection(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   pgsBondTool bondTool(m_pBroker,poi);

   // create a problem to solve
   // the cracked section analysis tool uses the same model as the moment capacity tool
   CComPtr<IGeneralSection> beam_section;
   CComPtr<IPoint2d> pntCompression; // needed to figure out the result geometry
   Float64 ec; // concrete strain limit
   Float64 dt; // depth from top of section to extreme layer of tensile reinforcement
   Float64 H; // overall height of section
   Float64 Haunch; // haunch build up that is modeled
   bool bDevelopmentReducedStrainCapacity; // indicates if the strain limit in reinforcement is reduced because of lack of full development length

   // initial strains aren't relavent on cracked section analysis
   // initialize all initial strains with 0
   GET_IFACE(IStrandGeometry, pStrandGeometry);
   std::array<std::vector<Float64>, 2> e_initial_strands;
   e_initial_strands[pgsTypes::Straight].insert(e_initial_strands[pgsTypes::Straight].begin(),pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight),0.0);
   e_initial_strands[pgsTypes::Harped].insert(e_initial_strands[pgsTypes::Harped].begin(),pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped),0.0);

   std::vector<Float64> e_initial_segment_tendons;
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeomemtry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeomemtry->GetDuctCount(segmentKey);
   e_initial_segment_tendons.insert(e_initial_segment_tendons.begin(), nSegmentDucts, 0);

   std::vector<Float64> e_initial_girder_tendons;
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeomemtry);
   DuctIndexType nGirderDucts = pGirderTendonGeomemtry->GetDuctCount(segmentKey);
   e_initial_girder_tendons.insert(e_initial_girder_tendons.begin(), nGirderDucts, 0);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   BuildCapacityProblem(liveLoadIntervalIdx,poi,nullptr,e_initial_strands,e_initial_segment_tendons,e_initial_girder_tendons,bondTool,bPositiveMoment,&beam_section,&pntCompression,&ec,&dt,&H,&Haunch,&bDevelopmentReducedStrainCapacity);

   // determine neutral axis angle
   // compression is on the left side of the neutral axis
   Float64 na_angle = (bPositiveMoment ? 0.00 : M_PI);

   CComPtr<ICrackedSectionSolution> solution;
   m_CrackedSectionSolver->putref_Section(beam_section);
   m_CrackedSectionSolver->put_Slices(1);
   m_CrackedSectionSolver->put_SliceGrowthFactor(2);
   m_CrackedSectionSolver->put_CGTolerance(0.001);
   HRESULT hr = m_CrackedSectionSolver->Solve(na_angle,&solution);
   ATLASSERT(SUCCEEDED(hr));

   pCSD->CrackedSectionSolution = solution;
  

   ///////////////////////////////////////////
   // Compute I-crack
   ///////////////////////////////////////////
   CComPtr<IElasticProperties> elastic_properties;
   solution->get_ElasticProperties(&elastic_properties);

   // transform properties into girder matieral
   GET_IFACE(IMaterials,pMaterials);
   Float64 Eg = pMaterials->GetSegmentEc(segmentKey,liveLoadIntervalIdx);

   CComPtr<IShapeProperties> shape_properties;
   elastic_properties->TransformProperties(Eg,&shape_properties);

   // Icrack for girder
   Float64 Icr;
   shape_properties->get_Ixx(&Icr);
   pCSD->Icr = Icr;

   // distance from top of section to the cracked centroid
   Float64 c;
   shape_properties->get_Ytop(&c);
   pCSD->c = c;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
StrandGradeType GetStrandGradeType(matPsStrand::Grade grade)
{
   StrandGradeType grade_type;
   switch (grade)
   {
   case matPsStrand::Gr1725: grade_type = sgtGrade250; break;
   case matPsStrand::Gr1860: grade_type = sgtGrade270; break;
   case matPsStrand::Gr2070: grade_type = sgtGrade300; break;
   default: ATLASSERT(false); // is there a new strand grade?
   }

   return grade_type;
}

void pgsMomentCapacityEngineer::CreateStrandMaterial(const CSegmentKey& segmentKey,pgsBondTool& bondTool,pgsTypes::StrandType strandType,StrandIndexType strandIdx,Float64 initialStrain,bool* pbDevelopmentReducedStrainCapacity,IStressStrain** ppSS) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   *pbDevelopmentReducedStrainCapacity = false;
   Float64 development_length_factor = 1.0;
   if(pPoi->IsOnSegment(bondTool.GetPOI())) development_length_factor = bondTool.GetDevelopmentLengthFactor(strandIdx, strandType);

   ATLASSERT(!IsZero(development_length_factor));

   auto found = m_StrandMaterial.find(StrandMaterial(strandType, development_length_factor, false, nullptr));
   if (found == m_StrandMaterial.end())
   {
      GET_IFACE(IMaterials, pMaterial);
      const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey, strandType);

      StrandGradeType grade = GetStrandGradeType(pStrand->GetGrade());
      ProductionMethodType type = pStrand->GetType() == matPsStrand::LowRelaxation ? pmtLowRelaxation : pmtStressRelieved;

      CComPtr<IPowerFormula> powerFormula;
      powerFormula.CoCreateInstance(CLSID_PSPowerFormula);
      powerFormula->put_Grade(grade);
      powerFormula->put_ProductionMethod(type);


      CComQIPtr<IStressStrain> ssStrand(powerFormula);

      if (development_length_factor < 1)
      {
         // reduced stress per LRFD Fig C5.9.4.3.2-1, development_length_factor = fpx/fps -> fpx = (development_length_factor)*fps
         *pbDevelopmentReducedStrainCapacity = true; // capacity of strand is being reduced because of development length
         powerFormula->put_ReductionFactor(development_length_factor);
      }

      m_StrandMaterial.insert(StrandMaterial(strandType, development_length_factor, *pbDevelopmentReducedStrainCapacity, ssStrand));

      ssStrand.CopyTo(ppSS);
   }
   else
   {
      *pbDevelopmentReducedStrainCapacity = found->bDevelopmentReducedStrainCapacity;
      *ppSS = found->ssMaterial;
      (*ppSS)->AddRef();
   }
}

void pgsMomentCapacityEngineer::CreateSegmentTendonMaterial(const CSegmentKey& segmentKey,IStressStrain** ppSS) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);

   CreateTendonMaterial(pTendon, ppSS);
}

void pgsMomentCapacityEngineer::CreateGirderTendonMaterial(const CGirderKey& girderKey, IStressStrain** ppSS) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pTendon = pMaterial->GetGirderTendonMaterial(girderKey);

   CreateTendonMaterial(pTendon, ppSS);
}

void pgsMomentCapacityEngineer::CreateTendonMaterial(const matPsStrand* pTendon, IStressStrain** ppSS) const
{
   StrandGradeType grade = GetStrandGradeType(pTendon->GetGrade());
   ProductionMethodType type = pTendon->GetType() == matPsStrand::LowRelaxation ? pmtLowRelaxation : pmtStressRelieved;

   CComPtr<IPowerFormula> powerFormula;
   powerFormula.CoCreateInstance(CLSID_PSPowerFormula);
   powerFormula->put_Grade(grade);
   powerFormula->put_ProductionMethod(type);

   CComQIPtr<IStressStrain> ssStrand(powerFormula);
   (*ppSS) = ssStrand;
   (*ppSS)->AddRef();
}

void pgsMomentCapacityEngineer::BuildCapacityProblem(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,const std::array<std::vector<Float64>, 2>& eps_initial,const std::vector<Float64>& ept_initial_segment,const std::vector<Float64>& ept_initial_girder,pgsBondTool& bondTool,bool bPositiveMoment,IGeneralSection** ppProblem,IPoint2d** pntCompression,Float64* pec,Float64* pdt, Float64* pH,Float64* pHaunch, bool* pbDevelopmentReducedStrainCapacity) const
{
   *pbDevelopmentReducedStrainCapacity = false;

   // beam shape
   GET_IFACE(IShapes, pShapes);
   CComPtr<IShape> shapeBeam;
   pShapes->GetSegmentSectionShape(intervalIdx, poi, false, pgsTypes::scGirder, &shapeBeam);
   if (shapeBeam == nullptr)
   {
      // if the poi is at a location where concrete doesn't exist in the current interval, there isn't a shape to be analyzed
      // this can happen at the centerline of a pier diaphragm before the pier diaphragm is cast
      // there is no moment capacity for this case
      *ppProblem = nullptr;
      *pntCompression = nullptr;
      *pec = 0;
      *pdt = 0;
      *pH = 0;
      *pHaunch = 0;
      return;
   }

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IPointOfInterest,pPoi);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   CComPtr<IPoint2d> pntTension; // location of the extreme tension face

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CClosureKey closureKey;
   bool bIsInClosure   = pPoi->IsInClosureJoint(poi,&closureKey);
   bool bIsOnSegment   = pPoi->IsOnSegment(poi);
   bool bIsOnGirder    = pPoi->IsOnGirder(poi);
   bool bIsInBoundaryPierDiaphragm = pPoi->IsInBoundaryPierDiaphragm(poi);

   bool bUHPC = (bIsOnSegment ? pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC : bIsInClosure ? pMaterial->GetClosureJointConcreteType(closureKey) == pgsTypes::PCI_UHPC : false);

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   Float64 dt = 0; // depth from compression face to extreme layer of tensile reinforcement

   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight,pConfig);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped,pConfig);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bIncludeRebar = pSpecEntry->IncludeRebarForMoment(); // only include rebar if permitted by the project criteria... 
   bool bIncludeStrandsWithNegativeMoment = pSpecEntry->IncludeStrandForNegativeMoment();

   //
   // Create Materials
   //
   // 90 day strength isn't applicable to strength limit states (only stress limit states, LRFD 5.12.3.2.5)
   // so use 28day properties
   bool bUse90DayStrength;
   Float64 factor;
   pSpecEntry->Use90DayStrengthForSlowCuringConcrete(&bUse90DayStrength, &factor);

   // strands
   //
   // because of debonding and development length adjustments, strand materials have to be created on a per-strand basis - see below

   // segment tendon
   CComPtr<IStressStrain> ssSegmentTendon;
   CreateSegmentTendonMaterial(segmentKey, &ssSegmentTendon);

   // girder tendon
   CComPtr<IStressStrain> ssGirderTendon;
   CreateGirderTendonMaterial(segmentKey, &ssGirderTendon);

   // girder concrete
   CComQIPtr<IStressStrain> ssGirder;
   if (!bUHPC)
   {
      CComPtr<IUnconfinedConcrete> matGirder;
      matGirder.CoCreateInstance(CLSID_UnconfinedConcrete);
      if (pConfig)
      {
         matGirder->put_fc(bUse90DayStrength ? pConfig->fc28 /*if 90 day strength option is enabled, force fc to the 28 day value*/ : pConfig->fc);
      }
      else
      {
         if (bIsInClosure)
         {
            // poi is in a closure joint
            if (bUse90DayStrength)
            {
               matGirder->put_fc(pMaterial->GetClosureJointFc28(closureKey));
            }
            else
            {
               matGirder->put_fc(pMaterial->GetClosureJointDesignFc(closureKey, intervalIdx));
            }
         }
         else if (bIsOnSegment)
         {
            if (bUse90DayStrength)
            {
               matGirder->put_fc(pMaterial->GetSegmentFc28(segmentKey));
            }
            else
            {
               matGirder->put_fc(pMaterial->GetSegmentDesignFc(segmentKey, intervalIdx));
            }
         }
         else
         {
            ATLASSERT(bIsInBoundaryPierDiaphragm);
            // poi is not in the segment and isn't in a closure joint
            // this means the POI is in a cast-in-place diaphragm between girder groups
            // LRFD 5.12.3.3.10 (5.14.1.4.10) says if the diaphragm is confined by the girders, the girder strength can be used.
            if (IsDiaphragmConfined(poi))
            {
               if (bUse90DayStrength)
               {
                  matGirder->put_fc(pMaterial->GetSegmentFc28(segmentKey));
               }
               else
               {
                  matGirder->put_fc(pMaterial->GetSegmentDesignFc(segmentKey, intervalIdx));
               }
            }
            else
            {
               // assume deck concrete is used for the diaphragm.
               if (bUse90DayStrength)
               {
                  matGirder->put_fc(pMaterial->GetDeckFc28());
               }
               else
               {
                  matGirder->put_fc(pMaterial->GetDeckDesignFc(intervalIdx));
               }
            }
         }
      }
      matGirder.QueryInterface(&ssGirder);
   }
   else
   {
      CComPtr<IPCIUHPConcrete> matGirder;
      matGirder.CoCreateInstance(CLSID_PCIUHPConcrete);

      if (pConfig)
      {
         matGirder->put_fc(pConfig->fc28);
      }
      else
      {
         if (bIsInClosure)
         {
            // poi is in a closure joint
            matGirder->put_fc(pMaterial->GetClosureJointDesignFc(closureKey, intervalIdx));
         }
         else if (bIsOnSegment)
         {
            matGirder->put_fc(pMaterial->GetSegmentDesignFc(segmentKey, intervalIdx));
         }
         else
         {
            ATLASSERT(bIsInBoundaryPierDiaphragm);
            // poi is not in the segment and isn't in a closure joint
            // this means the POI is in a cast-in-place diaphragm between girder groups
            // LRFD 5.12.3.3.10 (5.14.1.4.10) says if the diaphragm is confined by the girders, the girder strength can be used.
            if (IsDiaphragmConfined(poi))
            {
               matGirder->put_fc(pMaterial->GetSegmentDesignFc(segmentKey, intervalIdx));
            }
            else
            {
               // assume deck concrete is used for the diaphragm.
               matGirder->put_fc(pMaterial->GetDeckDesignFc(intervalIdx));
            }
         }
      }
      matGirder.QueryInterface(&ssGirder);
   }

   Float64 maxStrain;
   ssGirder->StrainLimits(pec, &maxStrain); // compression strain limit of girder concrete

   // longitudinal joint concrete - see below

   // slab concrete
   CComPtr<IUnconfinedConcrete> matSlab;
   matSlab.CoCreateInstance(CLSID_UnconfinedConcrete);
   matSlab->put_fc( pMaterial->GetDeckDesignFc(intervalIdx) );
   CComQIPtr<IStressStrain> ssSlab(matSlab);

   // girder rebar
   CComPtr<IRebarModel> matGirderRebar;
   matGirderRebar.CoCreateInstance(CLSID_RebarModel);
   Float64 E, Fy, Fu;
   if ( bIsInClosure )
   {
      pMaterial->GetClosureJointLongitudinalRebarProperties(closureKey,&E,&Fy,&Fu);
   }
   else if ( bIsOnSegment )
   {
      pMaterial->GetSegmentLongitudinalRebarProperties(segmentKey,&E,&Fy,&Fu);
   }
   else
   {
      ATLASSERT(bIsInBoundaryPierDiaphragm);
      pMaterial->GetDeckRebarProperties(&E,&Fy,&Fu);
   }

   matRebar::Type barType;
   matRebar::Grade barGrade;
   pMaterial->GetSegmentLongitudinalRebarMaterial(segmentKey, &barType, &barGrade);
   matGirderRebar->Init(Fy, E, matRebar::GetElongation(barType, barGrade, matRebar::bs3) );
   CComQIPtr<IStressStrain> ssGirderRebar(matGirderRebar);

   // slab rebar
   CComPtr<IRebarModel> matSlabRebar;
   matSlabRebar.CoCreateInstance(CLSID_RebarModel);
   pMaterial->GetDeckRebarProperties(&E,&Fy,&Fu);
   pMaterial->GetDeckRebarMaterial(&barType, &barGrade);
   matSlabRebar->Init( Fy, E, matRebar::GetElongation(barType, barGrade, matRebar::bs3) );
   CComQIPtr<IStressStrain> ssSlabRebar(matSlabRebar);

   //
   // Build the section
   //
   CComPtr<IGeneralSection> section;
   section.CoCreateInstance(CLSID_GeneralSection);

   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);


   ModelShape(section, shapeBeam, ssGirder, VARIANT_FALSE);

   // so far there is no deck in the model.... 
   // if this is for positive moment the compression point is top center, otherwise bottom center
   CComQIPtr<IXYPosition> posBeam(shapeBeam);
   if ( bPositiveMoment )
   {
      posBeam->get_LocatorPoint(lpTopCenter,pntCompression);
      posBeam->get_LocatorPoint(lpBottomCenter,&pntTension);
   }
   else
   {
      posBeam->get_LocatorPoint(lpBottomCenter,pntCompression);
      posBeam->get_LocatorPoint(lpTopCenter,&pntTension);
   }

   Float64 Yc; // elevation of the extreme compression fiber
   (*pntCompression)->get_Y(&Yc);

   // strand and rebar are measured from down from the top of the precast section
   // If we have a no-deck girder, we have to adjust the depth to the bar/strand
   // by the sacrifical depth
   Float64 sacDepth = 0;
   if (deckType == pgsTypes::sdtNone )
   {
      sacDepth = pBridge->GetSacrificalDepth();
   }

   if ( bPositiveMoment || bIncludeStrandsWithNegativeMoment || 0 < nSegmentDucts || 0 < nGirderDucts) // only model strands for positive moment, the spec says to use them, or if there are tendons in the model
   {
      // strands
      if ( bIsOnSegment || bIsInBoundaryPierDiaphragm || bIsInClosure)
      {
         GET_IFACE(IBridgeDescription, pIBridgeDesc);
         const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
         pgsTypes::AdjustableStrandType adj_type = pSegment->Strands.GetAdjustableStrandType();
         std::array<CString, 2> strStrandType{ _T("Straight"),pgsTypes::asHarped == adj_type ? _T("Harped") : _T("Adj. Straight") };

         GET_IFACE(IStrandGeometry, pStrandGeom);
         for ( int i = 0; i < 2; i++ ) // straight and harped strands
         {
            StrandIndexType nStrands = (i == 0 ? Ns : Nh);
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
          
            const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey, strandType);
            Float64 aps = pStrand->GetNominalArea();
            Float64 dps = pStrand->GetNominalDiameter();
         
            CComPtr<IPoint2dCollection> points; // strand points are in Girder Section Coordinates
            if ( pConfig )
            {
               pStrandGeom->GetStrandPositionsEx(poi, pConfig->PrestressConfig, strandType, &points);
            }
            else
            {
               pStrandGeom->GetStrandPositions(poi, strandType, &points);
            }

            for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
            {
               // if the strand has a very small transfer length factor, it's not bonded yet, don't consider it
               // if the strand does have a very small transfer length factor but is an extended strand, consider it
               //    because an extended strand, even though prestress force isn't transfered yet, the strand is anchored and contributes to capacity
               // if the transfer length is small (but non-zero) and the strand is debonded ignore it because the section capacity will be governed by debonding of these strands
               //    even though the capacity without these strands is greater - which is what we want
               bool bIsExtendedStrand = pStrandGeom->IsExtendedStrand(poi, strandIdx, strandType, pConfig);
               Float64 bond_start, bond_end;
               bool bIsDebonded = pStrandGeom->IsStrandDebonded(segmentKey, strandIdx, strandType, pConfig, &bond_start, &bond_end);
               Float64 transfer_length_factor = bondTool.GetTransferLengthFactor(strandIdx, strandType);
               if ((bIsDebonded && transfer_length_factor < 0.5) || (IsZero(transfer_length_factor) && !bIsExtendedStrand)) continue;

               CComPtr<IGenericShape> strand_shape;
               strand_shape.CoCreateInstance(CLSID_GenericShape);
               strand_shape->put_Area(aps);

               // get one strand from the row and get it's Y value (all strands in the row are at the same elevation)
               CComPtr<IPoint2d> point;
               points->get_Item(strandIdx, &point);
               Float64 X, Y;
               point->Location(&X, &Y);
               point.Release();

               // this Y value is measured from the top of the precast section
               // if there isn't a deck and we are using a sacrificial wearing surface
               // we need to deducted the sacrifical depth
               Y += sacDepth;

               // position the "strand" shape
               CComPtr<IPoint2d> pntCG;
               strand_shape->get_Centroid(&pntCG);
               pntCG->Move(X, Y);
               strand_shape->putref_Centroid(pntCG);

               CComQIPtr<IShape> shape(strand_shape);
               Float64 Le = 1.0; // elongation length (unity)

               // if the POI is off the segment and the strand is extended, it's not stressed, use a decompression strain of 0.0
               // otherwise use the provided value
               Float64 epsi = (!bIsOnSegment && bIsExtendedStrand) ? 0.0 : eps_initial[strandType][strandIdx];

               bool bDevelopmentReducedStrainCapacity;
               CComPtr<IStressStrain> ssStrand;
               CreateStrandMaterial(segmentKey, bondTool, strandType, strandIdx, epsi, &bDevelopmentReducedStrainCapacity, &ssStrand);
               if (!(*pbDevelopmentReducedStrainCapacity) && bDevelopmentReducedStrainCapacity)
               {
                  *pbDevelopmentReducedStrainCapacity = true;
               }


               // determine depth to lowest layer of strand
               Float64 cy;
               pntCG->get_Y(&cy);
               dt = Max(dt, fabs(Yc - cy));

               CComPtr<IPlane3d> strand_initial_strain;
               strand_initial_strain.CoCreateInstance(CLSID_Plane3d);
               strand_initial_strain->ThroughAltitude(epsi);

               CString strName;
               strName.Format(_T("%s%s Strand %d"), bIsExtendedStrand ? _T("Extended ") : _T(""), strStrandType[strandType], LABEL_INDEX(strandIdx));
               AddShape2Section(CComBSTR(strName), section, shape, ssStrand, ssGirder, strand_initial_strain, Le, false);
            } // next strand
         } // next strand type
      } // bIsOnSegment
   } // bPositiveMoment


     // Segment PT Tendons
   if (bIsOnSegment)
   {
      ATLASSERT(ept_initial_segment.size() == nSegmentDucts);
      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         if (pSegmentTendonGeometry->IsOnDuct(poi))
         {
            CComPtr<IPoint2d> point;
            pSegmentTendonGeometry->GetSegmentDuctPoint(poi, ductIdx, &point);

            Float64 area = pSegmentTendonGeometry->GetSegmentTendonArea(segmentKey, intervalIdx, ductIdx);

            CComPtr<IGenericShape> tendon_shape;
            tendon_shape.CoCreateInstance(CLSID_GenericShape);
            tendon_shape->put_Area(area);
            CComPtr<IPoint2d> pntCG;
            tendon_shape->get_Centroid(&pntCG);
            pntCG->MoveEx(point);
            tendon_shape->putref_Centroid(pntCG);

            // determine depth to lowest layer of strand
            Float64 cy;
            pntCG->get_Y(&cy);
            dt = Max(dt, fabs(Yc - cy));

            CComPtr<IPlane3d> initial_strain;
            initial_strain.CoCreateInstance(CLSID_Plane3d);
            initial_strain->ThroughAltitude(ept_initial_segment[ductIdx]);
            Float64 Le = 1.0; // elongation length (unity)

            CComQIPtr<IShape> shape(tendon_shape);

            CString strName;
            strName.Format(_T("Segment Tendon %d"), LABEL_DUCT(ductIdx));
            AddShape2Section(CComBSTR(strName),section, shape, ssSegmentTendon, ssGirder, initial_strain, Le, false);
         }
      }
   }

   // Girder PT Tendons
   if ( bIsOnGirder )
   {
      ATLASSERT(ept_initial_girder.size() == nGirderDucts);
      for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
      {
         if (pGirderTendonGeometry->IsOnDuct(poi, ductIdx))
         {
            CComPtr<IPoint2d> point;
            pGirderTendonGeometry->GetGirderDuctPoint(poi, ductIdx, &point);

            Float64 area = pGirderTendonGeometry->GetGirderTendonArea(segmentKey, intervalIdx, ductIdx);

            CComPtr<IGenericShape> tendon_shape;
            tendon_shape.CoCreateInstance(CLSID_GenericShape);
            tendon_shape->put_Area(area);
            CComPtr<IPoint2d> pntCG;
            tendon_shape->get_Centroid(&pntCG);
            pntCG->MoveEx(point);
            tendon_shape->putref_Centroid(pntCG);

            // determine depth to lowest layer of strand
            Float64 cy;
            pntCG->get_Y(&cy);
            dt = Max(dt, fabs(Yc - cy));

            CComPtr<IPlane3d> initial_strain;
            initial_strain.CoCreateInstance(CLSID_Plane3d);
            initial_strain->ThroughAltitude(ept_initial_girder[ductIdx]);

            Float64 Le = 1.0; // elongation length (unity)

            CString strName;
            strName.Format(_T("Girder Tendon %d"), LABEL_DUCT(ductIdx));

            CComQIPtr<IShape> shape(tendon_shape);
            AddShape2Section(CComBSTR(strName), section, shape, ssGirderTendon, ssGirder, initial_strain, Le, false);
         }
      }
   }

   // girder rebar
   if ( bIncludeRebar // only include rebar if permitted by the project criteria... 
        ||   // ... EXCEPT...
        (IsNonstructuralDeck(deckType) && !bPositiveMoment) // it must be included for negative moment capacity of "no deck" bridge systems (the girder has the only reinforcement available to develop capacity)
      )
   {
      GET_IFACE(ILongRebarGeometry, pRebarGeom);

      pgsPointOfInterest barCutPoi(poi);
      if (IsNonstructuralDeck(deckType) && !bPositiveMoment && !bIsOnSegment)
      {
         // we are doing girder rebar for negative moment for a "no deck" bridge and the current POI is not on the segment
         // this happens when the POI is in the pier... In this case, use the POI at the face of the segment
         // The development length factor will be zero at the face of the segment, so the only bars
         // that will get modeled are extended bars, which is what we want
         if (poi.HasAttribute(POI_SPAN | POI_0L))
         {
            PoiList vPoi;
            pPoi->GetPointsOfInterest(poi.GetSegmentKey(), POI_START_FACE, &vPoi);
            ATLASSERT(vPoi.size() == 1);
            barCutPoi = vPoi.front();
            ATLASSERT(barCutPoi.HasAttribute(POI_START_FACE));
         }
         else if (poi.HasAttribute(POI_SPAN | POI_10L))
         {
            PoiList vPoi;
            pPoi->GetPointsOfInterest(poi.GetSegmentKey(), POI_END_FACE, &vPoi);
            ATLASSERT(vPoi.size() == 1);
            barCutPoi = vPoi.front();
            ATLASSERT(barCutPoi.HasAttribute(POI_END_FACE));
         }
      }

      CComPtr<IRebarSection> rebar_section;
      pRebarGeom->GetRebars(barCutPoi,&rebar_section);
      
      CComPtr<IEnumRebarSectionItem> enumItems;
      rebar_section->get__EnumRebarSectionItem(&enumItems);

      CComPtr<IRebarSectionItem> item;
      while ( enumItems->Next(1,&item,nullptr) != S_FALSE )
      {
         CComPtr<IPoint2d> location;
         item->get_Location(&location);

         Float64 x,y;
         location->get_X(&x);
         location->get_Y(&y);

         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);
         Float64 as;
         rebar->get_NominalArea(&as);

         Float64 dev_length_factor = pRebarGeom->GetDevLengthFactor(barCutPoi,item);

         CComPtr<IGenericShape> bar_shape;
         bar_shape.CoCreateInstance(CLSID_GenericShape);
         bar_shape->put_Area(dev_length_factor* as);
         CComPtr<IPoint2d> pntCG;
         bar_shape->get_Centroid(&pntCG);
         pntCG->MoveEx(location);
         pntCG->Offset(0, sacDepth);

         Float64 cy;
         pntCG->get_Y(&cy);
         dt = Max(dt, fabs(Yc - cy));

         CComQIPtr<IShape> shape(bar_shape);
         AddShape2Section(CComBSTR("Rebar"), section, shape, ssGirderRebar, ssGirder, nullptr, 1.0, false);

         item.Release();
      }
   }

   // add the longitudinal joints to the model
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   if (pIBridgeDesc->GetBridgeDescription()->HasStructuralLongitudinalJoints())
   {
      CComPtr<IStressStrain> ssLongitudinalJoints;
      if (pMaterial->GetLongitudinalJointConcreteType() == pgsTypes::PCI_UHPC)
      {
         CComPtr<IPCIUHPConcrete> matLongitudinalJoints;
         matLongitudinalJoints.CoCreateInstance(CLSID_PCIUHPConcrete);
         matLongitudinalJoints->put_fc(pMaterial->GetLongitudinalJointFc(intervalIdx));
         matLongitudinalJoints.QueryInterface(&ssLongitudinalJoints);
      }
      else
      {
         CComPtr<IUnconfinedConcrete> matLongitudinalJoints;
         matLongitudinalJoints.CoCreateInstance(CLSID_UnconfinedConcrete);
         matLongitudinalJoints->put_fc(pMaterial->GetLongitudinalJointFc(intervalIdx));
         matLongitudinalJoints.QueryInterface(&ssLongitudinalJoints);
      }

      CComPtr<IShape> leftJointShape, rightJointShape;
      pShapes->GetJointShapes(intervalIdx, poi, false, pgsTypes::scGirder, &leftJointShape, &rightJointShape);

      if (leftJointShape)
      {
         AddShape2Section(CComBSTR("Left Long. Joint"), section, leftJointShape, ssLongitudinalJoints, nullptr, nullptr, 1.0, false);
      }

      if (rightJointShape)
      {
         AddShape2Section(CComBSTR("Right Long. Joint"), section, rightJointShape, ssLongitudinalJoints, nullptr, nullptr, 1.0, false);
      }

      if (leftJointShape || rightJointShape)
      {
         Float64 minStrain, maxStrain;
         ssLongitudinalJoints->StrainLimits(&minStrain, &maxStrain);
         if (*pec < minStrain)
            *pec = minStrain; // longitudinal joint compressive strain limit governs over the girder's limit
      }
   }

   // add the deck to the model
   GET_IFACE(ISectionProperties, pSectProp);
   Float64 Weff = pSectProp->GetEffectiveFlangeWidth(poi);
   Float64 Dslab = pBridge->GetStructuralSlabDepth(poi);
   if ( (IsStructuralDeck(deckType) && // the deck is structural
         compositeDeckIntervalIdx <= intervalIdx && // interval at or after deck is composite
         0 < Weff*Dslab) // the geometry of the bridge is good and the deck actually has area over this girder
      )
   {
      pgsTypes::HaunchAnalysisSectionPropertiesType hatype = pSectProp->GetHaunchAnalysisSectionPropertiesType();
      // Determine whether haunch is required and its depth
      if ( bPositiveMoment && hatype==pgsTypes::hspZeroHaunch )
      {
         // No haunch. Put the bottom center of the deck rectangle right on the top center of the beam
         *pHaunch = 0; // not modeling haunch buildup for positive moment
      }
      else
      {
        // put slab in correct location to account for additional moment arm due to haunch depth
        Float64 haunch_depth; // aka, top_girder_to_bottom_slab

        if (!bPositiveMoment && hatype != pgsTypes::hspVariableParabolic)
        {
           // Until 4/2018, the haunch depth for negative moment capacity was always computed using the computed excess camber
           // Retain this behavior and extend it to pgsTypes::hspFillet
           Float64 top_girder_to_top_slab = pBridge->GetTopSlabToTopGirderChordDistance(poi); // does not account for camber
           Float64 excess_camber;
           GET_IFACE(ICamber, pCamber);
           if (pConfig)
           {
              excess_camber = pCamber->GetExcessCamber(poi, CREEP_MAXTIME, pConfig);
           }
           else
           {
              excess_camber = pCamber->GetExcessCamber(poi, CREEP_MAXTIME);
           }

           haunch_depth = top_girder_to_top_slab - Dslab - excess_camber;
        }
        else
        {
           // for the other cases, use the assumed excess camber and roadway from the wbfl model
           ATLASSERT(hatype==pgsTypes::hspVariableParabolic || hatype==pgsTypes::hspConstFilletDepth);
           GET_IFACE(ISectionProperties,pSectProps);

           haunch_depth = pSectProps->GetStructuralHaunchDepth(poi, hatype);
        }

        // don't allow negative haunch depths
        if ( haunch_depth < 0 )
        {
           haunch_depth = 0;
        }

        *pHaunch = haunch_depth;
      }

      // Get slab shape in girder coord's
      GET_IFACE_NOCHECK(IGirder, pGirder);
      CComPtr<IShape> shapeDeck;
      pShapes->GetSlabAnalysisShape(intervalIdx, poi, *pHaunch, pBridge->IsAsymmetricGirder(segmentKey) && IsZero(pGirder->GetOrientation(segmentKey)), &shapeDeck);

      CComQIPtr<IXYPosition> posDeck(shapeDeck);
      // if this is positive moment and we have a deck, the extreme compression point is top center
      if (bPositiveMoment)
      {
         (*pntCompression)->Release();
         posDeck->get_LocatorPoint(lpTopCenter,pntCompression);

         // for positive moments, the deck is on top of the girder so
         // the deck is furthest from the neutral axis and it's compression strain limit
         // is what we want
         ssSlab->StrainLimits(pec, &maxStrain);
      }
      else
      {
         pntTension.Release();
         posDeck->get_LocatorPoint(lpTopCenter,&pntTension);
      }

      AddShape2Section(CComBSTR("Deck"), section, shapeDeck, ssSlab, nullptr, nullptr, 1.0, false);

      // deck rebar if this is for negative moment
      if ( !bPositiveMoment )
      {
#pragma Reminder("UPDATE: use the correct location of deck mat rebar")
         // NOTE: The value return from GetCoverTop/BottomMat() is the raw input. The raw input
         // is the cover to the center of the lump sum input and the CLEAR COVER to the individual bar input.
         // The CG of the individual and lump sum input are not at the same location.
         // Use the GetTop/BottomMatLocation() function to get the CG location measured from the bottom of the deck.
         // 
         // NOTE: NOTE: NOTE *****!!!!!!******!!!!
         // 
         // Don't make this change until all of the regression tests from the 2.x PGSuper branch are validated
         // This change will slightly alter the negative moment capacity results. We don't want to be dealing with
         // changed results while doing the initial validating.
         //
         // THIS MUST BE ONE OF THE LAST CHANGES TO MAKE BEFORE MOVING EVERYTHING TO THE 3.0 BRANCH
         GET_IFACE(ILongRebarGeometry, pRebarGeom);
         Float64 AsTop = pRebarGeom->GetAsTopMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);

         if ( !IsZero(AsTop) )
         {
            Float64 coverTop = pRebarGeom->GetCoverTopMat();
            
            CComPtr<IGenericShape> bar_shape;
            bar_shape.CoCreateInstance(CLSID_GenericShape);
            bar_shape->put_Area(AsTop);

            // move the center of the rebar shape below the top of the deck rectangle by the cover amount.
            // center it horizontally
            CComPtr<IPoint2d> pntDeck;
            posDeck->get_LocatorPoint(lpTopCenter, &pntDeck);
            pntDeck->Offset(0, -coverTop);
            bar_shape->putref_Centroid(pntDeck);

            Float64 cy;
            pntDeck->get_Y(&cy);
            dt = Max(dt, fabs(Yc - cy));

            CComQIPtr<IShape> shape(bar_shape);
            AddShape2Section(CComBSTR("Top Mat Deck Rebar"), section, shape, ssSlabRebar, ssSlab, nullptr, 1.0, false);
         }


         Float64 AsBottom = pRebarGeom->GetAsBottomMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
         if ( !IsZero(AsBottom) )
         {
            Float64 coverBottom = pRebarGeom->GetCoverBottomMat();

            CComPtr<IGenericShape> bar_shape;
            bar_shape.CoCreateInstance(CLSID_GenericShape);
            bar_shape->put_Area(AsBottom);

            // move the center of the rebar shape above the bottom of the deck rectangle by the cover amount.
            // center it horizontally
            CComPtr<IPoint2d> pntDeck;
            posDeck->get_LocatorPoint(lpTopCenter, &pntDeck);
            pntDeck->Offset(0, -Dslab + coverBottom);
            bar_shape->putref_Centroid(pntDeck);

            Float64 cy;
            pntDeck->get_Y(&cy);
            dt = Max(dt, fabs(Yc - cy));


            CComQIPtr<IShape> shape(bar_shape);
            AddShape2Section(CComBSTR("Bottom Mat Deck Rebar"), section, shape, ssSlabRebar, ssSlab, nullptr, 1.0, false);
         }
      }
   }
   else
   {
      // no deck, or deck is not composite or installed yet
      *pHaunch = 0.0;
   }

   // so far, dt is measured from top of girder (if positive moment)
   // since we have a deck, add Dslab+Haunch so that dt is measured from top of slab
   // If dt is zero, there wasn't any reinforcement so don't add Dslab
   if (bPositiveMoment && !IsZero(dt))
   {
      dt += Dslab +(*pHaunch);
   }

   // measure from bottom of beam to top of deck to get height
   pntTension->DistanceEx(*pntCompression,pH);

   *pdt = dt;

   (*ppProblem) = section;
   (*ppProblem)->AddRef();
}

//-----------------------------------------------------------------------------
const MINMOMENTCAPDETAILS* pgsMomentCapacityEngineer::ValidateMinMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,  bool bPositiveMoment) const
{
   ATLASSERT(poi.GetID() != INVALID_ID);

   std::map<PoiIDType, MINMOMENTCAPDETAILS>* pMap;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   pMap = (intervalIdx < compositeIntervalIdx) ? &m_NonCompositeMinMomentCapacity[bPositiveMoment]
      : &m_CompositeMinMomentCapacity[bPositiveMoment];

   auto found = pMap->find(poi.GetID());
   if (found != pMap->end())
   {
      return &((*found).second); // capacities have already been computed
   }

   MINMOMENTCAPDETAILS mmcd;
   ComputeMinMomentCapacity(intervalIdx, poi, bPositiveMoment, &mmcd);

   auto retval = pMap->insert(std::make_pair(poi.GetID(), mmcd));
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const CRACKINGMOMENTDETAILS* pgsMomentCapacityEngineer::ValidateCrackingMoments(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   ATLASSERT(poi.GetID() != INVALID_ID);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   auto pMap = (intervalIdx < compositeIntervalIdx) ? const_cast<CrackingMomentContainer*>(&m_NonCompositeCrackingMoment[bPositiveMoment]) : const_cast<CrackingMomentContainer*>(&m_CompositeCrackingMoment[bPositiveMoment]);
   const CRACKINGMOMENTDETAILS* pMcrDetails = nullptr;

   auto found = pMap->find(poi.GetID());
   if (found != pMap->end())
   {
      pMcrDetails = &((*found).second); // capacities have already been computed
   }

   if (pMcrDetails == nullptr)
   {
      CRACKINGMOMENTDETAILS cmd;
      ComputeCrackingMoment(intervalIdx, poi, bPositiveMoment, &cmd);

      auto retval = pMap->insert(std::make_pair(poi.GetID(), cmd));
      pMcrDetails = &((*(retval.first)).second);
   }

   return pMcrDetails;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* pgsMomentCapacityEngineer::ValidateMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   MOMENTCAPACITYDETAILS mcd = ComputeMomentCapacity(intervalIdx, poi, bPositiveMoment, pConfig);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   if (pConfig)
   {
      return StoreMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, mcd, intervalIdx < compositeIntervalIdx ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
   }
   else
   {
      return StoreMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, mcd, intervalIdx < compositeIntervalIdx ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
   }
}

//-----------------------------------------------------------------------------
pgsPointOfInterest pgsMomentCapacityEngineer::GetEquivalentPointOfInterest(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const
{
   const CGirderKey& girderKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest, pPOI);
   Float64 Xg = pPOI->ConvertPoiToGirderCoordinate(poi);

   pgsPointOfInterest search_poi(poi);

   GET_IFACE(IGirder, pGirder);

   // check for symmetry
   if (pGirder->IsSymmetric(intervalIdx, girderKey))
   {
      GET_IFACE(IBridge, pBridge);
      Float64 girder_length = pBridge->GetGirderLength(girderKey);

      if (girder_length / 2 < Xg)
      {
         // we are past mid-point of a symmetric girder
         // get the poi that is a mirror about the centerline of the girder

         Xg = girder_length - Xg;
         search_poi = pPOI->ConvertGirderCoordinateToPoi(girderKey, Xg);

         if (search_poi.GetID() == INVALID_ID) // a symmetric POI was not actually found
         {
            search_poi = poi;
         }
      }
   }

   return search_poi;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* pgsMomentCapacityEngineer::GetCachedMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // if the stored config is not equal to the requesting config, flush all the cached results
   if (pConfig && !pConfig->IsFlexuralDataEqual(m_TempGirderConfig))
   {
      m_TempNonCompositeMomentCapacity[0].clear();
      m_TempNonCompositeMomentCapacity[1].clear();

      m_TempCompositeMomentCapacity[0].clear();
      m_TempCompositeMomentCapacity[1].clear();

      m_TempGirderConfig = *pConfig;

      return nullptr;
   }

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   if (pConfig)
   {
      return GetStoredMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, intervalIdx < compositeIntervalIdx ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
   }
   else
   {
      return GetStoredMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, intervalIdx < compositeIntervalIdx ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
   }
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* pgsMomentCapacityEngineer::GetStoredMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentCapacityDetailsContainer& container) const
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   // don't do this for negative moment... the symmetry check just isn't working right


   pgsPointOfInterest search_poi((bPositiveMoment ? GetEquivalentPointOfInterest(intervalIdx, poi) : poi));

   // if this is a real POI, then see if we've already computed results
   if (search_poi.GetID() != INVALID_ID)
   {
      auto found = container.find(search_poi.GetID());
      if (found != container.end())
      {
         return &((*found).second); // capacities have already been computed
      }
   }

   return nullptr;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* pgsMomentCapacityEngineer::StoreMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const MOMENTCAPACITYDETAILS& mcd, MomentCapacityDetailsContainer& container) const
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   pgsPointOfInterest search_poi((bPositiveMoment ? GetEquivalentPointOfInterest(intervalIdx, poi) : poi));

   auto retval = container.insert(std::make_pair(search_poi.GetID(), mcd));

   // insert failed
   if (!retval.second)
   {
      // this shouldn't be happening unless we are out of memory or something really bad like that
      // if there is already something stored with 'key' the insert will fail and that is
      // bad because it indicates a bug elsewhere
      ATLASSERT(false);
      return nullptr;
   }


   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
void pgsMomentCapacityEngineer::InvalidateMomentCapacity()
{
   for (int i = 0; i < 2; i++)
   {
      m_NonCompositeMomentCapacity[i].clear();
      m_CompositeMomentCapacity[i].clear();
      m_NonCompositeCrackingMoment[i].clear();
      m_CompositeCrackingMoment[i].clear();
      m_NonCompositeMinMomentCapacity[i].clear();
      m_CompositeMinMomentCapacity[i].clear();
   }
}

void pgsMomentCapacityEngineer::InvalidateCrackingMoments()
{
   for (int i = 0; i < 2; i++)
   {
      m_NonCompositeCrackingMoment[i].clear();
      m_CompositeCrackingMoment[i].clear();
   }
}

void pgsMomentCapacityEngineer::InvalidateMinMomentCapacity()
{
   for (int i = 0; i < 2; i++)
   {
      m_NonCompositeMinMomentCapacity[i].clear();
      m_CompositeMinMomentCapacity[i].clear();
   }
}

//-----------------------------------------------------------------------------
void pgsMomentCapacityEngineer::InvalidateCrackedSectionDetails()
{
   for (int i = 0; i < 2; i++)
   {
      m_CrackedSectionDetails[i].clear();
   }
}

const CRACKEDSECTIONDETAILS* pgsMomentCapacityEngineer::ValidateCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   int idx = (bPositiveMoment ? 0 : 1);
   if (poi.GetID() != INVALID_ID)
   {
      auto found = m_CrackedSectionDetails[idx].find(poi.GetID());

      if (found != m_CrackedSectionDetails[idx].end())
      {
         return &((*found).second); // cracked section has already been computed
      }
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
   std::_tostringstream os;
   os << _T("Analyzing cracked section for ") << SEGMENT_LABEL(segmentKey)
      << _T(" at ") << (LPCTSTR)FormatDimension(poi.GetDistFromStart(), pDisplayUnits->GetSpanLengthUnit())
      << _T(" from start of girder") << std::ends;

   pProgress->UpdateMessage(os.str().c_str());

   CRACKEDSECTIONDETAILS csd;
   AnalyzeCrackedSection(poi, bPositiveMoment, &csd);

   auto retval = m_CrackedSectionDetails[idx].insert(std::make_pair(poi.GetID(), csd));
   return &((*(retval.first)).second);
}

bool pgsMomentCapacityEngineer::IsDiaphragmConfined(const pgsPointOfInterest& poi) const
{
   // LRFD 5.12.3.3.10 (5.14.1.1.10) says we can use the girder concrete strength at the intermediate diaphragm if the
   // diaphragm is confined by the girder ends and the diaphragm extends beyond the girders.
   GET_IFACE(IPointOfInterest, pPoi);
   PierIndexType pierIdx = pPoi->GetPier(poi);
   ATLASSERT(pierIdx != INVALID_INDEX);

   if (pierIdx == INVALID_INDEX)
      return false;

   GET_IFACE(IBridge, pBridge);
   pgsTypes::BoundaryConditionType bc = pBridge->GetBoundaryConditionType(pierIdx);
   if (!IsContinuousBoundaryCondition(bc))
      return false; // boundary condition must be continuous

   GroupIndexType backGroupIdx, aheadGroupIdx;
   pBridge->GetGirderGroupIndex(pierIdx, &backGroupIdx, &aheadGroupIdx);


   // First check that the diaphragm extends beyond the piers

   // 1) Get the diaphragm width
   Float64 Wb, Hb, Wa, Ha;
   pBridge->GetPierDiaphragmSize(pierIdx, pgsTypes::Back, &Wb, &Hb);
   pBridge->GetPierDiaphragmSize(pierIdx, pgsTypes::Ahead, &Wa, &Ha);
   Float64 W = Wb + Wa;

   // 2) Get the distance between girder ends
   GirderIndexType nGirdersBack = pBridge->GetGirderCount(backGroupIdx);
   GirderIndexType nGirdersAhead = pBridge->GetGirderCount(aheadGroupIdx);
   GirderIndexType nGirders = Max(nGirdersBack, nGirdersAhead);
   SegmentIndexType nSegmentsBack = pBridge->GetSegmentCount(backGroupIdx, 0);
   GirderIndexType gdrIdx = poi.GetSegmentKey().girderIndex;
   GirderIndexType backGdrIdx = (nGirdersBack <= gdrIdx ? nGirdersBack - 1 : gdrIdx);
   GirderIndexType aheadGdrIdx = (nGirdersAhead <= gdrIdx ? nGirdersAhead - 1 : gdrIdx);
   CSegmentKey backSegmentKey(backGroupIdx, backGdrIdx,nSegmentsBack-1);
   CSegmentKey aheadSegmentKey(aheadGroupIdx, aheadGdrIdx,0);

   GET_IFACE(IGirder, pGirder);
   Float64 dummy, backEndDist, backBrgOffset, aheadEndDist, aheadBrgOffset;
   pGirder->GetSegmentEndDistance(backSegmentKey, &dummy, &backEndDist);
   pGirder->GetSegmentBearingOffset(backSegmentKey, &dummy, &backBrgOffset);
   pGirder->GetSegmentEndDistance(aheadSegmentKey, &aheadEndDist, &dummy);
   pGirder->GetSegmentBearingOffset(aheadSegmentKey, &aheadBrgOffset, &dummy);

   // clear distance between ends of girders on back/ahead side of pier
   Float64 end_to_end_of_girders = backBrgOffset - backEndDist + aheadBrgOffset - aheadEndDist;

   // the width of the diaphragm must be wider than end_to_end_of_girders by some margin to
   // say that the diaphragm extends beyond the piers. We'll use 5%

   if (::IsLE(W, 1.05*end_to_end_of_girders))
      return false; // diaphragm width is not 5% more than the end to end distance between girders

   // OK... diaphragm extends beyond the girders

   // Now, check that the girders confine the diaphragm concrete.
   // We will consider the diaphragm concrete confined if the girder framing on
   // both sides of the pier nearly aligned. Project the girder lines onto
   // the centerline of the pier. If the distance between the points where the 
   // girderlines intersect the CL pier line is less 5% of the bottom width
   // of the girders, we will consider them to be aligned and confining
   // the diaphragm concrete. If the girders on either side of the pier
   // have different bottom widths, we'll base the 5% on the lessor bottom width

   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   pGirder->GetSegmentEndPoints(backSegmentKey, pgsTypes::pcLocal, &pntPier1, &pntEnd1, &pntBrg1, &pntBrg2, &pntEnd2, &pntPier2);
   CComPtr<IPoint2d> pntA(pntPier2);

   pntPier1.Release();
   pntEnd1.Release();
   pntBrg1.Release();
   pntBrg2.Release();
   pntEnd2.Release();
   pntPier2.Release();
   pGirder->GetSegmentEndPoints(aheadSegmentKey, pgsTypes::pcLocal, &pntPier1, &pntEnd1, &pntBrg1, &pntBrg2, &pntEnd2, &pntPier2);
   CComPtr<IPoint2d> pntB(pntPier1);

   Float64 dist;
   pntA->DistanceEx(pntB, &dist);

   PoiList vPoi;
   pPoi->GetPointsOfInterest(backSegmentKey, POI_END_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiA = vPoi.front();
   vPoi.clear();
   pPoi->GetPointsOfInterest(aheadSegmentKey, POI_START_FACE, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiB = vPoi.front();

   Float64 Wba = pGirder->GetBottomWidth(poiA);
   Float64 Wbb = pGirder->GetBottomWidth(poiB);
   W = Min(Wa, Wb);

   if (0.05*W < dist)
   {
      return false;
   }
   return true;
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsMomentCapacityEngineer::AssertValid() const
{
   return true;
}

void pgsMomentCapacityEngineer::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsMomentCapacityEngineer") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsMomentCapacityEngineer::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsMomentCapacityEngineer");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsMomentCapacityEngineer");

   TESTME_EPILOG("MomentCapacityEngineer");
}
#endif // _UNITTEST

#if defined _DEBUG_SECTION_DUMP
void pgsMomentCapacityEngineer::DumpSection(const pgsPointOfInterest& poi,IGeneralSection* section, std::map<StrandIndexType,Float64> ssBondFactors,std::map<StrandIndexType,Float64> hsBondFactors,bool bPositiveMoment) const
{
   std::_tostringstream os;
   std::_tstring strMn(bPositiveMoment ? _T("+M") : _T("-M"));
   os << _T("GeneralSection_") << strMn << _T("_Span_") << LABEL_SPAN(poi.GetSegmentKey().groupIndex) << _T("_Girder_") << LABEL_GIRDER(poi.GetSegmentKey().segmentIndex) << _T("_") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << ".txt";
   std::_tofstream file(os.str().c_str());

   IndexType shape_count;
   section->get_ShapeCount(&shape_count);
   for (IndexType idx = 0; idx < shape_count; idx++ )
   {
      file << (idx+1) << std::endl;

      CComPtr<IShape> shape;
      section->get_Shape(idx,&shape);

      CComPtr<IPoint2dCollection> points;
      shape->get_PolyPoints(&points);

      CComPtr<IPoint2d> point;
      CComPtr<IEnumPoint2d> enum_points;
      points->get__Enum(&enum_points);
      while ( enum_points->Next(1,&point,0) == S_OK )
      {
         Float64 x,y;
         point->get_X(&x);
         point->get_Y(&y);

         file << ::ConvertFromSysUnits(x,unitMeasure::Inch) << "," << ::ConvertFromSysUnits(y,unitMeasure::Inch) << std::endl;

         point.Release();
      }
   }

   file << "done" << std::endl;

   file << "Straight Strand Bond Factors" << std::endl;
   std::map<long,Float64>::iterator iter;
   for ( iter = ssBondFactors.begin(); iter != ssBondFactors.end(); iter++ )
   {
      file << iter->first << ", " << iter->second << std::endl;
   }

   file << "Harped Strand Bond Factors" << std::endl;
   for ( iter = hsBondFactors.begin(); iter != hsBondFactors.end(); iter++ )
   {
      file << iter->first << ", " << iter->second << std::endl;
   }

   file.close();
}
#endif // _DEBUG_SECTION_DUMP

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pgsMomentCapacityEngineer::pgsBondTool::pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig)
{
   m_pBroker    = pBroker;
   m_Poi        = poi;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   GET_IFACE(IBridge, pBridge);

   m_pConfig = pConfig;

   m_pBroker->GetInterface(IID_IPretensionForce,(IUnknown**)&m_pPrestressForce);
   m_pBroker->GetInterface(IID_IStrandGeometry, (IUnknown**)&m_pStrandGeometry);

   m_GirderLength = pBridge->GetSegmentLength(segmentKey);

   m_DistFromStart = m_Poi.GetDistFromStart();

   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ASSERT( vPoi.size() == 1 );
   m_PoiMidSpan = vPoi.front();

   /////// -- NOTE -- //////
   // Development length, and hence the development length adjustment factor, require the result
   // of a moment capacity analysis. fps is needed to compute development length, yet, development
   // length is needed to adjust the effectiveness of the strands for moment capacity analysis.
   //
   // This causes a circular dependency. However, the development length calculation only needs
   // fps for the capacity at the mid-span section. Unless the bridge is extremely short, the
   // strands will be fully developed at mid-span.
   //
   // If the poi is around mid-span, assume a development length factor of 1.0 otherwise compute it.
   //
   Float64 fra = 0.25; // 25% either side of centerline
   m_bNearMidSpan = false;
   if ( InRange(fra*m_GirderLength,m_DistFromStart,(1-fra)*m_GirderLength))
   {
      m_bNearMidSpan = true;
   }
}

Float64 pgsMomentCapacityEngineer::pgsBondTool::GetTransferLengthFactor(StrandIndexType strandIdx, pgsTypes::StrandType strandType) const
{
   return m_pPrestressForce->GetTransferLengthAdjustment(m_Poi, strandType, strandIdx, m_pConfig);
}

Float64 pgsMomentCapacityEngineer::pgsBondTool::GetDevelopmentLengthFactor(StrandIndexType strandIdx,pgsTypes::StrandType strandType) const
{
   // NOTE: More tricky code here (see note above)
   //
   // If we have a section that isn't near mid-span, we have to compute a bond factor. The bond factor is
   // a function of development length and development length needs fps and fpe which are computed in the
   // moment capacity analysis... again, circular dependency here.
   //
   // To work around this, the bond factors for moment capacity analysis are computed based on fps and fpe
   // at mid span.
   Float64 development_length_factor = 1.0;
   bool bIsExtendedStrand = m_pStrandGeometry->IsExtendedStrand(m_Poi, strandIdx, strandType, m_pConfig);

   if ( !m_bNearMidSpan && !bIsExtendedStrand)
   {
      bool bDebonded = IsDebonded(strandIdx,strandType);
      const std::shared_ptr<pgsDevelopmentLength> pDevLength = m_pPrestressForce->GetDevelopmentLengthDetails(m_PoiMidSpan,strandType,bDebonded, m_pConfig);
      Float64 fps = pDevLength->GetFps();
      Float64 fpe = pDevLength->GetFpe();

      development_length_factor = m_pPrestressForce->GetDevelopmentLengthAdjustment(m_Poi,strandIdx,strandType,fps,fpe, m_pConfig);
   }

   return development_length_factor;
}

bool pgsMomentCapacityEngineer::pgsBondTool::IsDebonded(StrandIndexType strandIdx,pgsTypes::StrandType strandType) const
{
   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 Lstart, Lend;
   const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();
   bool bIsDebonded = pStrandGeom->IsStrandDebonded(segmentKey, strandIdx, strandType, m_pConfig, &Lstart, &Lend);
   if (bIsDebonded)
   {
      // the strand has debonding, but is it bonded at our location?
      if (Lstart <= m_DistFromStart && m_DistFromStart <= Lend)
      {
         bIsDebonded = false;
      }
   }

   return bIsDebonded;
}


void pgsMomentCapacityEngineer::ModelShape(IGeneralSection* pSection, IShape* pShape, IStressStrain* pMaterial, VARIANT_BOOL bIsVoid) const
{
   CComQIPtr<ICompositeShape> compShape(pShape);
   if (compShape)
   {
      CollectionIndexType shapeCount;
      compShape->get_Count(&shapeCount);

      for (CollectionIndexType idx = 0; idx < shapeCount; idx++)
      {
         CComPtr<ICompositeShapeItem> csItem;
         compShape->get_Item(idx, &csItem);

         CComPtr<IShape> shape;
         csItem->get_Shape(&shape);

         VARIANT_BOOL bVoid;
         csItem->get_Void(&bVoid);

         ModelShape(pSection, shape, pMaterial, bVoid);
      } // next shape
   }
   else
   {
      // beam shape isn't composite so just add it
      if (bIsVoid == VARIANT_TRUE)
      {
         // void shape... use only a background material (backgrounds are subtracted)
         AddShape2Section(CComBSTR("Void"), pSection, pShape, nullptr, pMaterial, nullptr, 1.0, false);
      }
      else
      {
         AddShape2Section(CComBSTR("Girder"), pSection, pShape, pMaterial, nullptr, nullptr, 1.0, true);
      }
   }
}
