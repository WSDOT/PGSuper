///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "PsForceEng.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>
#include <IFace\RatingSpecification.h>

#include <PGSuperException.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsPsForceEng
****************************************************************************/


pgsPsForceEng::pgsPsForceEng()
{
   // Set to bogus value so we can update after our agents are set up
   m_PrestressTransferComputationType=(pgsTypes::PrestressTransferComputationType)-1; 
}

pgsPsForceEng::pgsPsForceEng(const pgsPsForceEng& rOther)
{
   MakeCopy(rOther);
}

pgsPsForceEng::~pgsPsForceEng()
{
}

pgsPsForceEng& pgsPsForceEng::operator= (const pgsPsForceEng& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsPsForceEng::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsPsForceEng::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;
}

void pgsPsForceEng::CreateLossEngineer(const CGirderKey& girderKey) const
{
   if (m_LossEngineer )
   {
      return;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGdr = pGirder->GetGirderLibraryEntry();

   CComPtr<IBeamFactory> beamFactory;
   pGdr->GetBeamFactory(&beamFactory);

   beamFactory->CreatePsLossEngineer(m_pBroker,m_StatusGroupID,girderKey,&m_LossEngineer);
}

void pgsPsForceEng::Invalidate()
{
   m_LossEngineer.Release();

   // Set transfer comp type to invalid
   m_PrestressTransferComputationType=(pgsTypes::PrestressTransferComputationType)-1; 
}     

void pgsPsForceEng::ClearDesignLosses()
{
   if (m_LossEngineer)
   {
      m_LossEngineer->ClearDesignLosses();
   }
}

void pgsPsForceEng::ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   CreateLossEngineer(girderKey);
   m_LossEngineer->BuildReport(girderKey,pChapter,pDisplayUnits);
}

void pgsPsForceEng::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   CreateLossEngineer(girderKey);
   m_LossEngineer->ReportFinalLosses(girderKey,pChapter,pDisplayUnits);
}

const ANCHORSETDETAILS* pgsPsForceEng::GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   return m_LossEngineer->GetGirderTendonAnchorSetDetails(girderKey,ductIdx);
}

const ANCHORSETDETAILS* pgsPsForceEng::GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   return m_LossEngineer->GetSegmentTendonAnchorSetDetails(segmentKey, ductIdx);
}

Float64 pgsPsForceEng::GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const
{
   CreateLossEngineer(girderKey);
   return m_LossEngineer->GetGirderTendonElongation(girderKey,ductIdx,endType);
}

Float64 pgsPsForceEng::GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const
{
   CreateLossEngineer(segmentKey);
   return m_LossEngineer->GetSegmentTendonElongation(segmentKey, ductIdx, endType);
}

Float64 pgsPsForceEng::GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetGirderTendonAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);
   return dfpF;
}

Float64 pgsPsForceEng::GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetSegmentTendonAverageFrictionAndAnchorSetLoss(segmentKey, ductIdx, &dfpF, &dfpA);
   return dfpF;
}

Float64 pgsPsForceEng::GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   CreateLossEngineer(girderKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetGirderTendonAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);
   return dfpA;
}

Float64 pgsPsForceEng::GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   CreateLossEngineer(segmentKey);
   Float64 dfpF, dfpA;
   m_LossEngineer->GetSegmentTendonAverageFrictionAndAnchorSetLoss(segmentKey, ductIdx, &dfpF, &dfpA);
   return dfpA;
}

const LOSSDETAILS* pgsPsForceEng::GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   CreateLossEngineer(poi.GetSegmentKey());
   return m_LossEngineer->GetLosses(poi,intervalIdx);
}

const LOSSDETAILS* pgsPsForceEng::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) const
{
   CreateLossEngineer(poi.GetSegmentKey());
   return m_LossEngineer->GetLosses(poi,config,intervalIdx);
}

Float64 pgsPsForceEng::GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const
{
   GET_IFACE(ISegmentData,pSegmentData);
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);
   ATLASSERT(pStrand != 0);

   return GetPjackMax(segmentKey,*pStrand,nStrands);
}

Float64 pgsPsForceEng::GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands) const
{
   GET_IFACE( ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();

   GET_IFACE( ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 Pjack = 0.0;
   if ( pSpecEntry->CheckStrandStress(CSS_AT_JACKING) )
   {
      Float64 coeff;
      if ( strand.GetType() == matPsStrand::LowRelaxation )
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,LOW_RELAX);
      }
      else
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_AT_JACKING,STRESS_REL);
      }

      Float64 fpu;
      Float64 aps;

      fpu = strand.GetUltimateStrength();
      aps = strand.GetNominalArea();

      Float64 Fu = fpu * aps * nStrands;

      Pjack = coeff*Fu;
   }
   else
   {
      Float64 coeff;
      if ( strand.GetType() == matPsStrand::LowRelaxation )
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,LOW_RELAX);
      }
      else
      {
         coeff = pSpecEntry->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,STRESS_REL);
      }

      // fake up some data so losses are computed before transfer
      GET_IFACE(IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      const pgsPointOfInterest& poi( vPoi.front() );

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
      Float64 loss = GetEffectivePrestressLoss(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End);

      Float64 fpu = strand.GetUltimateStrength();
      Float64 aps = strand.GetNominalArea();

      Pjack = (coeff*fpu + loss) * aps * nStrands;
   }

   return Pjack;
}

XFERLENGTHDETAILS pgsPsForceEng::GetXferLengthDetails(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   // Make sure our computation type is valid before we try to use it
   if ( (pgsTypes::PrestressTransferComputationType)-1 == m_PrestressTransferComputationType)
   {
      // Get value from libary if it hasn't been set up 
      GET_IFACE( ISpecification, pSpec );
      std::_tstring spec_name = pSpec->GetSpecification();
      GET_IFACE( ILibrary, pLib );
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

      m_PrestressTransferComputationType = pSpecEntry->GetPrestressTransferComputationType();
   }

   if (m_PrestressTransferComputationType==pgsTypes::ptMinuteValue)
   {
      // Model zero prestress transfer length. 0.1 inches seems to give
      // good designs and spec checks. This does not happen if the value is reduced to 0.0;
      // because pgsuper is not set up to deal with moment point loads.
      //
      XFERLENGTHDETAILS details;
      details.bMinuteValue = true;
      details.lt = ::ConvertToSysUnits(0.1,unitMeasure::Inch);
      return details;
   }
   else
   {
      XFERLENGTHDETAILS details;
      details.bMinuteValue = false;

      GET_IFACE(ISegmentData,pSegmentData);
      const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);
      ATLASSERT(pStrand != nullptr);

      // for epoxy coated strand see PCI "Guidelines for the use of Epoxy-Coated Strand"
      // PCI Journal, July-August 1993
      // otherwise, LRFD 5.9.4.3.1 (pre2017: 5.11.4.1)
      details.bEpoxy = (pStrand->GetCoating() == matPsStrand::None ? false : true);
      details.db = pStrand->GetNominalDiameter();
      details.ndb = (details.bEpoxy ? 50 : 60);

      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      if (pMaterial->Concrete.Type == pgsTypes::UHPC)
      {
         // it is conservative to use the same transfer length for bare and epoxy coated 
         // strand in UHPC
         details.ndb = 20;
      }

      details.lt = details.ndb * details.db;
      return details;
   }
}

Float64 pgsPsForceEng::GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   XFERLENGTHDETAILS details = GetXferLengthDetails(segmentKey,strandType);
   return details.lt;
}

Float64 pgsPsForceEng::GetXferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE_NOCHECK(IStrandGeometry, pStrandGeom); // not used if pConfig != nullptr

   StrandIndexType Ns = INVALID_INDEX;
   if (pConfig)
   {
      Ns = (strandType == pgsTypes::Permanent ? pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight) + pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped) : pConfig->PrestressConfig.GetStrandCount(strandType));
   }
   else
   {
      Ns = pStrandGeom->GetStrandCount(segmentKey, strandType);
   }

   // Quick check to make sure there is even an adjustment to be made. If there are no strands, just leave
   if (Ns == 0)
   {
      return 1.0;
   }

   StrandIndexType nDebond = INVALID_INDEX;
   if (pConfig)
   {
      nDebond = (strandType == pgsTypes::Permanent ? pConfig->PrestressConfig.Debond[pgsTypes::Straight].size() + pConfig->PrestressConfig.Debond[pgsTypes::Harped].size() : pConfig->PrestressConfig.Debond[strandType].size());
   }
   else
   {
      nDebond = pStrandGeom->GetNumDebondedStrands(segmentKey, strandType, pgsTypes::dbetEither);

   }
   ATLASSERT(nDebond <= Ns); // must be true!

   // set up loop counters for below. stand type assumptions are same as above
   pgsTypes::StrandType st1, st2;
   if ( strandType == pgsTypes::Permanent )
   {
      st1 = pgsTypes::Straight;
      st2 = pgsTypes::Harped;
   }
   else
   {
      st1 = strandType;
      st2 = strandType;
   }

   // Compute a scaling factor to apply to the basic prestress force to adjust for transfer length
   // and debonded strands
   Float64 xfer_length = GetXferLength(segmentKey,strandType);

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length;
   Float64 left_end_size;
   Float64 right_end_size;
   Float64 dist_from_left_end;
   Float64 dist_from_right_end;

   gdr_length      = pBridge->GetSegmentLength(segmentKey);
   left_end_size   = pBridge->GetSegmentStartEndDistance(segmentKey);
   right_end_size  = pBridge->GetSegmentEndEndDistance(segmentKey);

   dist_from_left_end  = poi.GetDistFromStart();
   dist_from_right_end = gdr_length - dist_from_left_end;


   // Determine effectiveness of the debonded strands
   Float64 nDebondedEffective = 0; // number of effective debonded strands

   for ( int i = (int)st1; i <= (int)st2; i++ )
   {
      pgsTypes::StrandType st = (pgsTypes::StrandType)i;

      if (pConfig)
      {
         for(const auto& debond_info : pConfig->PrestressConfig.Debond[st])
         {
            Float64 right_db_from_left = gdr_length - debond_info.DebondLength[pgsTypes::metEnd];

            // see if bonding occurs at all here
            if (debond_info.DebondLength[pgsTypes::metStart] < dist_from_left_end && dist_from_left_end < right_db_from_left)
            {
               // compute minimum bonded length from poi
               Float64 left_len = dist_from_left_end - debond_info.DebondLength[pgsTypes::metStart];
               Float64 rgt_len = right_db_from_left - dist_from_left_end;
               Float64 min_db_len = Min(left_len, rgt_len);

               if (min_db_len < xfer_length)
               {
                  nDebondedEffective += min_db_len / xfer_length;
               }
               else
               {
                  nDebondedEffective += 1.0;
               }

            }
            else
            {
               // strand is not bonded at the location of the POI
               nDebondedEffective += 0.0;
            }
         }
      }
      else
      {
         GET_IFACE(ISegmentData, pSegmentData);
         const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
         if (pStrands->GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
         {
            const auto& strandRows = pStrands->GetStrandRows();
            for (const auto& strandRow : strandRows)
            {
               if (strandRow.m_StrandType != (StrandType)st)
               {
                  continue;
               }

               if (strandRow.m_bIsDebonded[etStart] || strandRow.m_bIsDebonded[etEnd])
               {
                  Float64 Ldb = strandRow.m_bIsDebonded[etStart] ? strandRow.m_DebondLength[etStart] : 0;
                  Float64 Rdb = strandRow.m_bIsDebonded[etEnd] ? strandRow.m_DebondLength[etEnd] : 0;
                  Float64 right_db_from_left = gdr_length - Rdb;

                  // see if bonding occurs at all here
                  if (Ldb < dist_from_left_end && dist_from_left_end < right_db_from_left)
                  {
                     // compute minimum bonded length from poi
                     Float64 left_len = dist_from_left_end - Ldb;
                     Float64 rgt_len = right_db_from_left - dist_from_left_end;
                     Float64 min_db_len = Min(left_len, rgt_len);

                     if (min_db_len < xfer_length)
                     {
                        nDebondedEffective += (min_db_len / xfer_length)*strandRow.m_nStrands;
                     }
                     else
                     {
                        nDebondedEffective += strandRow.m_nStrands;
                     }

                  }
                  else
                  {
                     // strand is not bonded at the location of the POI
                     nDebondedEffective += 0.0;
                  }
               }
            }
         }
         else
         {
            const auto& debonding = pStrands->GetDebonding(st);
            for (const auto& debond_data : debonding)
            {
               Float64 Ldb = debond_data.Length[pgsTypes::metStart];
               Float64 Rdb = debond_data.Length[pgsTypes::metEnd];
               Float64 right_db_from_left = gdr_length - Rdb;

               // see if bonding occurs at all here
               if (Ldb < dist_from_left_end && dist_from_left_end < right_db_from_left)
               {
                  // compute minimum bonded length from poi
                  Float64 left_len = dist_from_left_end - Ldb;
                  Float64 rgt_len = right_db_from_left - dist_from_left_end;
                  Float64 min_db_len = Min(left_len, rgt_len);

                  StrandIndexType strandIdx1, strandIdx2;
                  pStrandGeom->GridPositionToStrandPosition(segmentKey, st, debond_data.strandTypeGridIdx, &strandIdx1, &strandIdx2);
                  ATLASSERT(strandIdx1 != INVALID_INDEX);
                  StrandIndexType nStrands = 1;
                  if (strandIdx2 != INVALID_INDEX)
                  {
                     nStrands++;
                  }

                  if (min_db_len < xfer_length)
                  {
                     nDebondedEffective += (min_db_len / xfer_length)*nStrands;
                  }
                  else
                  {
                     nDebondedEffective += nStrands;
                  }
               }
               else
               {
                  // strand is not bonded at the location of the POI
                  nDebondedEffective += 0.0;
               }
            }
         }
      }
   }

   // Determine effectiveness of bonded strands
   Float64 nBondedEffective = 0; // number of effective bonded strands
   if ( InRange(0.0, dist_from_left_end, xfer_length) )
   {
      // from the left end of the girder, POI is in transfer zone
      nBondedEffective = dist_from_left_end / xfer_length;
   }
   else if ( InRange(0.0, dist_from_right_end,xfer_length) )
   {
      // from the right end of the girder, POI is in transfer zone
      nBondedEffective = dist_from_right_end / xfer_length;
   }
   else
   {
      // strand is fully bonded at the location of the POI
      nBondedEffective = 1.0;
   }

   // nBondedEffective is for 1 strand... make it for all the bonded strands
   nBondedEffective *= (Ns-nDebond);

   Float64 adjust = (nBondedEffective + nDebondedEffective)/Ns;

   return adjust;
}

Float64 pgsPsForceEng::GetDevLength(const pgsPointOfInterest& poi,bool bDebonded,bool bUHPC,const GDRCONFIG* pConfig) const
{
   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,bDebonded,bUHPC,pConfig);
   return details.ld;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,bool bUHPC,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType intervalIdx = nIntervals-1;
   pgsTypes::LimitState limitState = pgsTypes::ServiceIII;

   GET_IFACE(ISegmentData,pSegmentData);
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   Float64 mbrDepth = pGirder->GetHeight(poi);

   GET_IFACE(IPretensionForce,pPrestressForce);
   Float64 fpe = pPrestressForce->GetEffectivePrestressWithLiveLoad(poi,pgsTypes::Permanent,limitState,INVALID_INDEX/*controlling live load*/,pConfig);

   GET_IFACE(IMomentCapacity,pMomCap);
   const MOMENTCAPACITYDETAILS* pmcd = pMomCap->GetMomentCapacityDetails(intervalIdx,poi,true/*positive moment*/,pConfig);

   STRANDDEVLENGTHDETAILS details;
   details.db = pStrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = pmcd->fps_avg;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded, bUHPC);
   details.ld = lrfdPsStrand::GetDevLength( *pStrand, details.fps, details.fpe, mbrDepth, bDebonded, bUHPC);

   details.ltDetails = GetXferLengthDetails(segmentKey,pgsTypes::Permanent);

   return details;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,bool bUHPC,Float64 fps,Float64 fpe,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ISegmentData,pSegmentData);
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   Float64 mbrDepth = pGirder->GetHeight(poi);

   STRANDDEVLENGTHDETAILS details;
   details.db = pStrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = fps;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded, bUHPC);
   details.ld = lrfdPsStrand::GetDevLength( *pStrand, details.fps, details.fpe, mbrDepth, bDebonded, bUHPC);

   details.ltDetails = GetXferLengthDetails(segmentKey,pgsTypes::Permanent);

   return details;
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig) const
{
   GET_IFACE(ISegmentData, pSegmentData);
   bool bUHPC = pSegmentData->GetSegmentMaterial(poi.GetSegmentKey())->Concrete.Type == pgsTypes::UHPC ? true : false;

   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,false,bUHPC,pConfig);
   return GetDevLengthAdjustment(poi,strandIdx,strandType,details.fps,details.fpe,pConfig);
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe, const GDRCONFIG* pConfig) const
{
   // Compute a scaling factor to apply to the basic prestress force to
   // adjust for prestress force in the development
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Xpoi = poi.GetDistFromStart();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 bond_start, bond_end;
   bool bDebonded = pStrandGeom->IsStrandDebonded(segmentKey,strandIdx,strandType,pConfig,&bond_start,&bond_end);
   bool bExtendedStrand = pStrandGeom->IsExtendedStrand(poi,strandIdx,strandType,pConfig);

   GET_IFACE(ISegmentData, pSegmentData);
   bool bUHPC = pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC ? true : false;

   // determine minimum bonded length from poi
   Float64 left_bonded_length, right_bonded_length;
   if ( bDebonded )
   {
      // measure bonded length
      left_bonded_length = Xpoi - bond_start;
      right_bonded_length = bond_end - Xpoi;
   }
   else if ( bExtendedStrand )
   {
      // strand is extended into end diaphragm... the development length adjustment is 1.0
      return 1.0;
   }
   else
   {
      // no debonding, bond length is to ends of girder
      GET_IFACE(IBridge,pBridge);
      Float64 gdr_length  = pBridge->GetSegmentLength(segmentKey);

      left_bonded_length  = Xpoi;
      right_bonded_length = gdr_length - Xpoi;
   }

   Float64 lpx = Min(left_bonded_length, right_bonded_length);

   if (lpx <= 0.0)
   {
      // strand is unbonded at location, no more to do
      return 0.0;
   }
   else
   {
      STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,bDebonded,bUHPC,fps,fpe,pConfig);
      Float64 xfer_length = details.ltDetails.lt;
      Float64 dev_length  = details.ld;

      Float64 adjust = -999; // dummy value, helps with debugging

      if ( IsLE(lpx,xfer_length) )
      {
         adjust = (IsLE(fpe,fps) ? (lpx*fpe) / (xfer_length*fps) : 1.0);
      }
      else if ( IsLE(lpx,dev_length) )
      {
         adjust = (fpe + (lpx - xfer_length)*(fps-fpe)/(dev_length - xfer_length))/fps;
      }
      else
      {
         adjust = 1.0;
      }

      adjust = IsZero(adjust) ? 0 : adjust;
      adjust = ::ForceIntoRange(0.0,adjust,1.0);
      return adjust;
   }
}

Float64 pgsPsForceEng::GetHoldDownForce(const CSegmentKey& segmentKey, bool bTotal, Float64* pSlope, pgsPointOfInterest* pPoi, const GDRCONFIG* pConfig) const
{
   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped, pConfig);
   if (0 < Nh)
   {
      GET_IFACE(IPointOfInterest,pIPoi);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
   
      // no hold down force if there aren't any harped strands
      if ( vPoi.size() == 0 )
      {
         if (pPoi)
         {
            *pPoi = pgsPointOfInterest(segmentKey, 0.0);
         }

         if (pSlope)
         {
            *pSlope = 0;
         }
         return 0;
      }

      const pgsPointOfInterest& poi(vPoi.front());
   
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
      // The slope of the strands may be different at each harp point... need to compute the
      // hold down force for each harp point and return the maximum value

      Float64 s = Float64_Max; // slope associated with governing hold down force
      Float64 F = 0; // governing hold down force
      if (pPoi)
      {
         *pPoi = vPoi.front();
      }
      for (const pgsPointOfInterest& poi : vPoi)
      {
         Float64 harped = GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,pgsTypes::Start,pConfig);

         // Adjust for slope
         Float64 slope;
         if (bTotal)
         {
            slope = pStrandGeom->GetAvgStrandSlope(poi, pConfig);
         }
         else
         {
            harped /= Nh; // per strand force

            // maximum hold down force is associated with the maximum strand slope
            slope = pStrandGeom->GetMaxStrandSlope(poi, pConfig);
         }


         Float64 Fhd;
         Fhd = harped / sqrt(1 + slope*slope);

         if (F < Fhd)
         {
            F = Fhd;
            s = slope;
            if (pPoi)
            {
               *pPoi = poi;
            }
         }
      }

      // NOTE: increase the force by some percentage to account for friction
      // in the hold down device. harped *= 1.05 (for a 5% increase)
      // See PCI BDM Example 9.1a,pg 9.1a-28
      // Also see LRFD 5.9.3.2.2a (pre2017: 5.9.5.2.2a)
      //
      // Do this computation outside of the loop for efficiency
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

      bool bCheck, bDesign;
      int holdDownForceType;
      Float64 maxHoldDownForce, friction;
      pSpecEntry->GetHoldDownForce(&bCheck, &bDesign, &holdDownForceType, &maxHoldDownForce, &friction);
      ATLASSERT(bTotal == (holdDownForceType == HOLD_DOWN_TOTAL));

      F *= (1 + friction);

      if (pSlope)
      {
         *pSlope = s;
      }
      return F;
   }
   else
   {
      if (pPoi)
      {
         *pPoi = pgsPointOfInterest(segmentKey, 0);
      }

      if (pSlope)
      {
         *pSlope = 0;
      }

      return 0;
   }
}

void pgsPsForceEng::MakeCopy(const pgsPsForceEng& rOther)
{
   m_pBroker = rOther.m_pBroker;
   m_StatusGroupID = rOther.m_StatusGroupID;

   m_PrestressTransferComputationType = rOther.m_PrestressTransferComputationType;
}

void pgsPsForceEng::MakeAssignment(const pgsPsForceEng& rOther)
{
   MakeCopy( rOther );
}

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType N = pStrandGeom->GetStrandCount(segmentKey, strandType, pConfig);
   if (N == 0)
   {
      return 0;
   }
   
   GET_IFACE(ISegmentData,pSegmentData );
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   GET_IFACE(ISectionProperties,pSectProps);
   bool bIncludeElasticEffects = (pSectProps->GetSectionPropertiesMode() == pgsTypes::spmGross || intervalTime == pgsTypes::End ? true : false);
   Float64 fpe = GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,pConfig,bIncludeElasticEffects);

   Float64 aps = pStrand->GetNominalArea();

   Float64 P = aps*N*fpe;

   return P;
}

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType N = pStrandGeom->GetStrandCount(segmentKey,strandType);
   if (N == 0)
   {
      return 0;
   }

   GET_IFACE(ISegmentData,pSegmentData );
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   Float64 fpe = GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,nullptr,bIncludeElasticEffects);

   Float64 aps = pStrand->GetNominalArea();

   Float64 P = aps*N*fpe;

   return P;
}

Float64 pgsPsForceEng::GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,pConfig,true/*include elastic effects*/);
}

Float64 pgsPsForceEng::GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, const GDRCONFIG* pConfig, bool bIncludeElasticEffects) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType N = pStrandGeom->GetStrandCount(segmentKey, strandType, pConfig);
   if (N == 0)
   {
      return 0;
   }

   GET_IFACE(ISegmentData,pSegmentData );
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   Float64 fpe = GetEffectivePrestressWithLiveLoad(poi,strandType,limitState,vehicleIndex,pConfig,bIncludeElasticEffects);

   Float64 aps = pStrand->GetNominalArea();

   Float64 P = aps*N*fpe;

   return P;
}

Float64 pgsPsForceEng::GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig);
   Float64 instantaneous_effects = GetInstantaneousEffects(poi,strandType,intervalIdx,intervalTime,pConfig);
   Float64 effective_loss = time_dependent_loss - instantaneous_effects;
   return effective_loss;
}

Float64 pgsPsForceEng::GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig,bool bIncludeElasticEffects) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if ( IsRatingLimitState(limitState) )
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }
   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;
   Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,liveLoadIntervalIdx,intervalTime,pConfig);
   Float64 instantaneous_effects = 0;
   if (bIncludeElasticEffects)
   {
      instantaneous_effects = GetInstantaneousEffectsWithLiveLoad(poi, strandType, limitState, vehicleIdx, pConfig);
   }
   Float64 effective_loss = time_dependent_loss - instantaneous_effects;
   return effective_loss;
}

Float64 pgsPsForceEng::GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
#pragma Reminder("REVIEW = why does on version of GetLosses use the intervalIdx and the other doesnt?")
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      pDetails = pLosses->GetLossDetails(poi,intervalIdx);
   }

   return GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   // NOTE: Losses are just time-dependent change in prestress force.
   // Losses do not include elastic effects include elastic shortening or elastic gains due to external loads
   //
   // fpe = fpj - loss + gains

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // if losses were computed with the time-step method
   // look up change in stress in strands due to creep, shrinkage, and relaxation
   if ( pDetails->LossMethod == pgsTypes::TIME_STEP )
   {
      if ( intervalIdx == 0 && intervalTime == pgsTypes::Start )
      {
         return 0; // wanting losses at the start of the first interval.. nothing has happened yet
      }

      // time step losses are computed for the end of an interval
      IntervalIndexType theIntervalIdx = intervalIdx;
      switch(intervalTime)
      {
      case pgsTypes::Start:
         theIntervalIdx--; // losses at start of this interval are equal to losses at end of previous interval
         break;

      case pgsTypes::Middle:
         // drop through so we just use the end
      case pgsTypes::End:
         break; // do nothing... theIntervalIdx is correct
      }

#if !defined LUMP_STRANDS
      GET_IFACE(IStrandGeometry,pStrandGeom);
#endif

      if ( strandType == pgsTypes::Permanent) 
      {
#if defined LUMP_STRANDS
         StrandIndexType Ns, Nh;
         if ( pConfig )
         {
            Ns = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight);
            Nh =  pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped);
         }
         else
         {
            GET_IFACE(IStrandGeometry,pStrandGeom);
            Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
            Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
         }

         if ( Ns + Nh == 0 )
         {
            return 0;
         }

         Float64 dfpe_creep_straight      = 0;
         Float64 dfpe_shrinkage_straight  = 0;
         Float64 dfpe_relaxation_straight = 0;
         Float64 dfpe_creep_harped      = 0;
         Float64 dfpe_shrinkage_harped  = 0;
         Float64 dfpe_relaxation_harped = 0;
         for ( IntervalIndexType i = 0; i <= theIntervalIdx; i++ )
         {
            dfpe_creep_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftCreep];
            dfpe_creep_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftCreep];

            dfpe_shrinkage_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftShrinkage];
            dfpe_shrinkage_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftShrinkage];

            dfpe_relaxation_straight += pDetails->TimeStepDetails[i].Strands[pgsTypes::Straight].dfpei[pgsTypes::pftRelaxation];
            dfpe_relaxation_harped   += pDetails->TimeStepDetails[i].Strands[pgsTypes::Harped  ].dfpei[pgsTypes::pftRelaxation];
         }

         return -(Ns*(dfpe_creep_straight + dfpe_shrinkage_straight + dfpe_relaxation_straight) + Nh*(dfpe_creep_harped + dfpe_shrinkage_harped + dfpe_relaxation_harped))/(Ns + Nh);
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
      else
      {
#if defined LUMP_STRANDS
         Float64 dfpe_creep      = 0;
         Float64 dfpe_shrinkage  = 0;
         Float64 dfpe_relaxation = 0;
         for ( IntervalIndexType i = 0; i <= theIntervalIdx; i++ )
         {
            dfpe_creep += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftCreep];

            dfpe_shrinkage += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftShrinkage];

            dfpe_relaxation += pDetails->TimeStepDetails[i].Strands[strandType].dfpei[pgsTypes::pftRelaxation];
         }

         return -(dfpe_creep + dfpe_shrinkage + dfpe_relaxation);
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
   }
   else
   {
      // some method other than Time Step
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandIntervalIdx  = pIntervals->GetStressStrandInterval(segmentKey);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      IntervalIndexType noncompositeCastingIntervalIdx = pIntervals->GetLastNoncompositeInterval();
      IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      GET_IFACE(IBridge,pBridge);
      bool bIsFutureOverlay = pBridge->IsFutureOverlay();

      Float64 loss = 0;
      if ( intervalIdx == stressStrandIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            loss = 0.0;
         }
         else if ( intervalTime == pgsTypes::Middle )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }

            loss /= 2.0;
         }
         else if ( intervalTime == pgsTypes::End )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }
         }
      }
      else if ( intervalIdx == releaseIntervalIdx || intervalIdx == storageIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTransfer();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTransfer();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTransfer();
            }
         }
      }
      else if ( intervalIdx == liftSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_AtLifting();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_AtLifting();
         }
      }
      else if ( intervalIdx == haulSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_AtShipping();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_AtShipping();
         }
      }
      else if ( intervalIdx == tsInstallationIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AtShipping();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AtShipping();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandInstallation();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandInstallation();
            }
         }
      }
      else if ( intervalIdx == erectSegmentIntervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();
         }
      }
      else if ( intervalIdx == tsRemovalIntervalIdx )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandRemoval();
            }
         }
      }
      else if ( intervalIdx == noncompositeCastingIntervalIdx || (intervalIdx == compositeIntervalIdx && compositeIntervalIdx != railingSystemIntervalIdx) )
      {
         if ( intervalTime == pgsTypes::Start )
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterTemporaryStrandRemoval();
            }
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               loss = pDetails->pLosses->TemporaryStrand_AfterDeckPlacement();
            }
            else
            {
               loss = pDetails->pLosses->PermanentStrand_AfterDeckPlacement();
            }
         }
      }
      else
      {
         if ( strandType == pgsTypes::Temporary )
         {
            loss = pDetails->pLosses->TemporaryStrand_Final();
         }
         else
         {
            loss = pDetails->pLosses->PermanentStrand_Final();
         }
      }

      return loss;
   }
}

Float64 pgsPsForceEng::GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
#pragma Reminder("REVIEW = why does one version of GetLossDetails use the intervalIdx and the other doesnt?")
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      pDetails = pLosses->GetLossDetails(poi,intervalIdx);
   }

   return GetInstantaneousEffects(poi,strandType,intervalIdx,intervalTime,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 gain = 0;

   if ( pDetails->LossMethod == pgsTypes::GENERAL_LUMPSUM )
   {
      // all changes to effective prestress are included in general lump sum losses
      // since the TimeDependentEffects returns the user input value for loss,
      // we return 0 here.
      return 0;
   }
   else if ( pDetails->LossMethod == pgsTypes::TIME_STEP )
   {
      // effective loss = time-dependent loss - elastic effects
      // effective loss is on the strand objects, just look it up
      // elastic effects = time-dependent loss - effective loss
      if ( intervalIdx == 0 && intervalTime == pgsTypes::Start )
      {
         return 0; // wanting losses at the start of the first interval.. nothing has happened yet
      }

      Float64 time_dependent_loss = GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig,pDetails);

      // time step losses are computed for the end of an interval
      IntervalIndexType theIntervalIdx = intervalIdx;
      switch(intervalTime)
      {
      case pgsTypes::Start:
         theIntervalIdx--; // losses at start of this interval are equal to losses at end of previous interval
         break;

      case pgsTypes::Middle:
         // drop through so we just use the end
      case pgsTypes::End:
         break; // do nothing... theIntervalIdx is correct
      }

      if ( strandType == pgsTypes::Permanent )
      {
#if defined LUMP_STRANDS
         StrandIndexType Ns, Nh;
         if ( pConfig )
         {
            Ns = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight);
            Nh =  pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped);
         }
         else
         {
            GET_IFACE(IStrandGeometry,pStrandGeom);
            Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
            Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
         }

         if ( Ns + Nh == 0 )
         {
            return 0;
         }

         Float64 effective_loss = (Ns*pDetails->TimeStepDetails[theIntervalIdx].Strands[pgsTypes::Straight].loss
                                  + Nh*pDetails->TimeStepDetails[theIntervalIdx].Strands[pgsTypes::Harped  ].loss)/(Ns+Nh);

         return time_dependent_loss - effective_loss;
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
      else
      {
#if defined LUMP_STRANDS
         Float64 effective_loss = pDetails->TimeStepDetails[theIntervalIdx].Strands[strandType].loss;
         return time_dependent_loss - effective_loss;
#else
#pragma Reminder("IMPLEMENT")
#endif
      }
   }
   else
   {
      // some method other than Time Step
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType stressStrandIntervalIdx  = pIntervals->GetStressStrandInterval(segmentKey);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      IntervalIndexType noncompositeCastingIntervalIdx      = pIntervals->GetLastNoncompositeInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      Float64 gain = 0;

      if ( releaseIntervalIdx <= intervalIdx )
      {
         if ( intervalIdx == releaseIntervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += -pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
            }
            else
            {
               gain += -pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
            }
         }
      }

      if ( tsInstallationIntervalIdx <= intervalIdx )
      {
         if ( strandType == pgsTypes::Temporary )
         {
            GET_IFACE(ISegmentData,pSegmentData);
            const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

            if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
            {
               gain += -(pDetails->pLosses->FrictionLoss() + pDetails->pLosses->AnchorSetLoss() + pDetails->pLosses->GetDeltaFptAvg());
            }
         }
         else
         {
            gain += -pDetails->pLosses->GetDeltaFpp();
         }
      }

      if ( tsRemovalIntervalIdx <= intervalIdx )
      {
         if ( strandType != pgsTypes::Temporary && intervalTime != pgsTypes::Start )
         {
            gain += -pDetails->pLosses->GetDeltaFptr();
         }
      }

      if (noncompositeCastingIntervalIdx <= intervalIdx )
      {
         if ( intervalIdx == noncompositeCastingIntervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += 0;
            }
            else
            {
               gain += pDetails->pLosses->ElasticGainDueToDeckPlacement();
            }
         }
      }

      if ( railingSystemIntervalIdx <= intervalIdx )
      {
         if ( railingSystemIntervalIdx == intervalIdx && intervalTime == pgsTypes::Start )
         {
            gain += 0;
         }
         else
         {
            if ( strandType == pgsTypes::Temporary )
            {
               gain += 0;
            }
            else
            {
               gain += pDetails->pLosses->ElasticGainDueToSIDL();
            }
         }
      }

      return gain;
   }
}

Float64 pgsPsForceEng::GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig) const
{
   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails;
   if ( pConfig )
   {
      pDetails = pLosses->GetLossDetails(poi,*pConfig);
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx;
      if ( IsRatingLimitState(limitState) )
      {
         liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
      }
      else
      {
         liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      }
      pDetails = pLosses->GetLossDetails(poi,liveLoadIntervalIdx);
   }

   return GetInstantaneousEffectsWithLiveLoad(poi,strandType,limitState,vehicleIdx,pConfig,pDetails);
}

Float64 pgsPsForceEng::GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const
{
   ATLASSERT(!IsStrengthLimitState(limitState)); // limit state must be servie or fatigue

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   Float64 gLL;
   if ( IsRatingLimitState(limitState) )
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();

      GET_IFACE(IRatingSpecification, pRatingSpec);
      gLL = pRatingSpec->GetLiveLoadFactor(limitState, true);
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

	   GET_IFACE(ILoadFactors,pLoadFactors);
	   const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
	   gLL = pLF->GetLLIMMax(limitState);
   }
   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;

   Float64 gain = GetInstantaneousEffects(poi,strandType,liveLoadIntervalIdx,intervalTime,pConfig,pDetails);
   gain += GetElasticGainDueToLiveLoad(poi, strandType, limitState, vehicleIdx, pConfig, pDetails);
   return gain;
}

Float64 pgsPsForceEng::GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig,bool bIncludeElasticEffects) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Get the prestressing input information
   GET_IFACE(ISegmentData,pSegmentData );
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx        = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsStressingIntervalIdx = pIntervals->GetTemporaryStrandStressingInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx      = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 Pj = pStrandGeom->GetPjack(segmentKey,strandType,pConfig);
   StrandIndexType N = pStrandGeom->GetStrandCount(segmentKey,strandType,pConfig);

   if ( strandType == pgsTypes::Temporary &&
         ( intervalIdx < tsStressingIntervalIdx ||
            tsRemovalIntervalIdx < intervalIdx      || 
           (tsRemovalIntervalIdx == intervalIdx && intervalTime != pgsTypes::Start) 
         )
      )
   {
      N = 0;
      Pj = 0;
   }

   if ( strandType == pgsTypes::Temporary && N == 0)
   {
      // if we are after temporary strand stress and Nt is zero... the result is zero
      return 0;
   }

   if ( IsZero(Pj) && N == 0 )
   {
      // no strand, no jack force... the strand stress is 0
      return 0;
   }

   Float64 aps = pStrand->GetNominalArea();

   // Compute the jacking stress 
   Float64 fpj = Pj/(aps*N);

   // Compute the requested strand stress
   Float64 loss;
   if ( bIncludeElasticEffects )
   {
      loss = GetEffectivePrestressLoss(poi,strandType,intervalIdx,intervalTime,pConfig);
   }
   else
   {
      loss = GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig);
   }
   Float64 fps = fpj - loss;

   ATLASSERT( 0 <= fps ); // strand stress must be greater than or equal to zero.

   // Reduce for transfer effect (no transfer effect if the strands aren't released
   if ( releaseIntervalIdx <= intervalIdx )
   {
      Float64 adjust = GetXferLengthAdjustment(poi,strandType,pConfig);
      fps *= adjust;
   }

   return fps;
}

Float64 pgsPsForceEng::GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, const GDRCONFIG* pConfig, bool bIncludeElasticEffects) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Get the prestressing input information
   GET_IFACE(ISegmentData,pSegmentData );
   const matPsStrand* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);

   Float64 Pj;
   StrandIndexType N;

   if ( pConfig )
   {
      if ( strandType == pgsTypes::Permanent )
      {
         Pj = pConfig->PrestressConfig.Pjack[pgsTypes::Straight]          + pConfig->PrestressConfig.Pjack[pgsTypes::Harped];
         N  = pConfig->PrestressConfig.GetStrandCount(pgsTypes::Straight) + pConfig->PrestressConfig.GetStrandCount(pgsTypes::Harped);
      }
      else
      {
         Pj = pConfig->PrestressConfig.Pjack[strandType];
         N  = pConfig->PrestressConfig.GetStrandCount(strandType);
      }
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Pj = pStrandGeom->GetPjack(segmentKey,strandType);
      N  = pStrandGeom->GetStrandCount(segmentKey,strandType);
   }

   if ( strandType == pgsTypes::Temporary )
   {
      N = 0;
      Pj = 0;
   }

   if ( strandType == pgsTypes::Temporary && N == 0)
   {
      // if we are after temporary strand stress and Nt is zero... the result is zero
      return 0;
   }

   if ( IsZero(Pj) && N == 0 )
   {
      // no strand, no jack force... the strand stress is 0
      return 0;
   }

   Float64 aps = pStrand->GetNominalArea();

   // Compute the jacking stress 
   Float64 fpj = Pj/(aps*N);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   if ( IsRatingLimitState(limitState) )
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   }


   // Compute the requested strand stress
   Float64 loss = GetEffectivePrestressLossWithLiveLoad(poi,strandType,limitState,vehicleIndex,pConfig,bIncludeElasticEffects);

   Float64 fps = fpj - loss;

   ATLASSERT( 0 <= fps ); // strand stress must be greater than or equal to zero.

   // Reduce for transfer effect
   Float64 adjust = GetXferLengthAdjustment(poi, strandType, pConfig);

   fps *= adjust;

   return fps;
}

Float64 pgsPsForceEng::GetElasticGainDueToLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, const GDRCONFIG* pConfig, const LOSSDETAILS* pDetails) const
{
   ATLASSERT(!IsStrengthLimitState(limitState)); // limit state must be service or fatigue

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx;
   Float64 gLL;
   if (IsRatingLimitState(limitState))
   {
      liveLoadIntervalIdx = pIntervals->GetLoadRatingInterval();

      GET_IFACE(IRatingSpecification, pRatingSpec);
      gLL = pRatingSpec->GetLiveLoadFactor(limitState, true);
   }
   else
   {
      liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      GET_IFACE(ILoadFactors, pLoadFactors);
      const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
      gLL = pLF->GetLLIMMax(limitState);
   }
   pgsTypes::IntervalTimeType intervalTime = pgsTypes::End;

   GET_IFACE(IProductForces, pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
   Float64 Mmin, Mmax;

   if (vehicleIndex == INVALID_INDEX)
   {
      pProductForces->GetLiveLoadMoment(liveLoadIntervalIdx, llType, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
   }
   else
   {
      pProductForces->GetVehicularLiveLoadMoment(liveLoadIntervalIdx, llType, vehicleIndex, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
   }


   Float64 gain = 0;
   if (pDetails->LossMethod == pgsTypes::TIME_STEP)
   {
#if defined LUMP_STRANDS
      GET_IFACE(IStrandGeometry, pStrandGeom);
      GET_IFACE(ISectionProperties, pSectProps);
      Float64 neff;
      Float64 e = pStrandGeom->GetEccentricity(liveLoadIntervalIdx, poi, strandType, &neff);
      Float64 Ixx = pSectProps->GetIxx(liveLoadIntervalIdx, poi);
      gain = gLL * Mmax * e / Ixx;
#else
      GET_IFACE(IStrandGeometry, pStrandGeom);
      for (int i = 0; i < 2; i++) // straight and harped only, temp strands have been removed
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey, strandType);
         for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            THIS IS NOT CORRECT FOR INDIVIDUAL STRANDS
            const TIME_STEP_STRAND& strand(pDetails->TimeStepDetails[liveLoadIntervalIdx].Strands[strandType][strandIdx]);
            gain += gLL*strand.dFllMax;
         }
      }
#endif // LUMP_STRANDS
   }
   else
   {
      Float64 llGain;
      if (pDetails->LossMethod == pgsTypes::GENERAL_LUMPSUM)
      {
         llGain = 0.0;
      }
      else
      {
         GET_IFACE(ISpecification, pSpec);
         GET_IFACE(ILibrary, pLibrary);
         const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
         Float64 K_liveload = pSpecEntry->GetLiveLoadElasticGain();

         llGain = pDetails->pLosses->ElasticGainDueToLiveLoad(Mmax);
         llGain *= K_liveload;
      }
      gain += gLL*llGain;
   }
   return gain;
}
