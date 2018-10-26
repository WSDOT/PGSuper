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

#include "StdAfx.h"
#include "PsForceEng.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\MomentCapacity.h>
#include "..\PGSuperException.h"

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\BridgeDescription.h>

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

void pgsPsForceEng::CreateLossEngineer(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   if (m_LossEngineer )
      return;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGdr = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

   CComPtr<IBeamFactory> beamFactory;
   pGdr->GetBeamFactory(&beamFactory);

   beamFactory->CreatePsLossEngineer(m_pBroker,m_StatusGroupID,spanIdx,gdrIdx,&m_LossEngineer);
}

void pgsPsForceEng::Invalidate()
{
   m_LossEngineer.Release();

   // Set transfer comp type to invalid
   m_PrestressTransferComputationType=(pgsTypes::PrestressTransferComputationType)-1; 
}     

void pgsPsForceEng::ReportLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   CreateLossEngineer(span,gdr);
   m_LossEngineer->BuildReport(span,gdr,pChapter,pDisplayUnits);
}

void pgsPsForceEng::ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   CreateLossEngineer(span,gdr);
   m_LossEngineer->ReportFinalLosses(span,gdr,pChapter,pDisplayUnits);
}

void pgsPsForceEng::ComputeLosses(const pgsPointOfInterest& poi,LOSSDETAILS* pLosses)
{
   CreateLossEngineer(poi.GetSpan(),poi.GetGirder());
   *pLosses = m_LossEngineer->ComputeLosses(poi);
}

void pgsPsForceEng::ComputeLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   CreateLossEngineer(poi.GetSpan(),poi.GetGirder());
   *pLosses = m_LossEngineer->ComputeLossesForDesign(poi,config);
}

Float64 pgsPsForceEng::GetPjackMax(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType,StrandIndexType nStrands)
{
   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr,strandType);
   CHECK(pstrand!=0);

   return GetPjackMax( span, gdr, *pstrand, nStrands);
}

Float64 pgsPsForceEng::GetPjackMax(SpanIndexType span,GirderIndexType gdr,const matPsStrand& strand,StrandIndexType nStrands)
{
   GET_IFACE( ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();

   GET_IFACE( ILibrary, pLib );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 Pjack = 0.0;
   if ( pSpecEntry->CheckStrandStress(AT_JACKING) )
   {
      Float64 coeff;
      if ( strand.GetType() == matPsStrand::LowRelaxation )
         coeff = pSpecEntry->GetStrandStressCoefficient(AT_JACKING,LOW_RELAX);
      else
         coeff = pSpecEntry->GetStrandStressCoefficient(AT_JACKING,STRESS_REL);

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
         coeff = pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,LOW_RELAX);
      else
         coeff = pSpecEntry->GetStrandStressCoefficient(BEFORE_TRANSFER,STRESS_REL);

      // fake up some data so losses are computed before transfer
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest( span, gdr, pgsTypes::CastingYard, POI_MIDSPAN);
      pgsPointOfInterest poi = vPoi[0];

      GET_IFACE(ILosses,pLosses);
      Float64 loss = pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent); // should be the same for all girders in all spans

      Float64 fpu = strand.GetUltimateStrength();
      Float64 aps = strand.GetNominalArea();

      Pjack = (coeff*fpu + loss) * aps * nStrands;
   }

   return Pjack;
}

Float64 pgsPsForceEng::GetXferLength(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
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
      return ::ConvertToSysUnits(0.1,unitMeasure::Inch);
   }
   else
   {
      GET_IFACE(IGirderData,pGirderData);
      const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr,strandType);
      ATLASSERT(pstrand!=0);

      return lrfdPsStrand::GetXferLength( *pstrand );
   }
}

Float64 pgsPsForceEng::GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   ////////////////////////////////////////////////////////////////////////////////
   //******************************************************************************
   // NOTE: This method is almost identical to the other GetXferLengthAdjustment method.
   // Any changes done here will likely need to be done there as well
   //******************************************************************************
   ////////////////////////////////////////////////////////////////////////////////
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Ns = pStrandGeom->GetNumStrands(span,gdr,strandType);

   // Quick check to make sure there is even an adjustment to be made
   // If there are no strands, just leave
   if ( Ns == 0 )
      return 1.0;

   // Compute a scaling factor to apply to the basic prestress force to adjust for transfer length
   // and debonded strands
   Float64 xfer_length = GetXferLength(span,gdr,strandType);

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length;
   Float64 left_end_size;
   Float64 right_end_size;
   Float64 dist_from_left_end;
   Float64 dist_from_right_end;

   gdr_length      = pBridge->GetGirderLength(span,gdr);
   left_end_size   = pBridge->GetGirderStartConnectionLength(span,gdr);
   right_end_size  = pBridge->GetGirderEndConnectionLength(span,gdr);

   dist_from_left_end  = poi.GetDistFromStart();
   dist_from_right_end = gdr_length - dist_from_left_end;

   StrandIndexType nDebond = pStrandGeom->GetNumDebondedStrands(span,gdr,strandType);
   ATLASSERT(nDebond <= Ns); // must be true!

   // Determine effectiveness of the debonded strands
   Float64 nDebondedEffective = 0; // number of effective debonded strands
   std::vector<DEBONDINFO>::const_iterator iter;

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

   const GDRCONFIG& config = pBridge->GetGirderConfiguration(span,gdr);
   for ( int i = (int)st1; i <= (int)st2; i++ )
   {
      pgsTypes::StrandType st = (pgsTypes::StrandType)i;

      for ( iter = config.Debond[st].begin(); iter != config.Debond[st].end(); iter++ )
      {
         const DEBONDINFO& debond_info = *iter;

         Float64 right_db_from_left = gdr_length - debond_info.RightDebondLength;

         // see if bonding occurs at all here
         if ( debond_info.LeftDebondLength<dist_from_left_end && dist_from_left_end<right_db_from_left)
         {
            // compute minimum bonded length from poi
            Float64 left_len = dist_from_left_end - debond_info.LeftDebondLength;
            Float64 rgt_len  = right_db_from_left - dist_from_left_end;
            Float64 min_db_len = min(left_len, rgt_len);

            if (min_db_len<xfer_length)
               nDebondedEffective += min_db_len/xfer_length;
            else
               nDebondedEffective += 1.0;

         }
         else
         {
            // strand is not bonded at the location of the POI
            nDebondedEffective += 0.0;
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

Float64 pgsPsForceEng::GetXferLengthAdjustment(const pgsPointOfInterest& poi,
                                               pgsTypes::StrandType strandType,
                                               const GDRCONFIG& config)
{
   ////////////////////////////////////////////////////////////////////////////////
   //******************************************************************************
   // NOTE: This method is almost identical to the other GetXferLengthAdjustment method.
   // Any changes done here will likely need to be done there as well
   //******************************************************************************
   ////////////////////////////////////////////////////////////////////////////////
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // Compute a scaling factor to apply to the basic prestress force to adjust for transfer length
   // and debonded strands
   Float64 xfer_length = GetXferLength(span,gdr,strandType);

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length;
   Float64 left_end_size;
   Float64 right_end_size;
   Float64 dist_from_left_end;
   Float64 dist_from_right_end;

   gdr_length      = pBridge->GetGirderLength(span,gdr);
   left_end_size   = pBridge->GetGirderStartConnectionLength(span,gdr);
   right_end_size  = pBridge->GetGirderEndConnectionLength(span,gdr);

   dist_from_left_end  = poi.GetDistFromStart();
   dist_from_right_end = gdr_length - dist_from_left_end;

   StrandIndexType Ns = (strandType == pgsTypes::Permanent ? config.Nstrands[pgsTypes::Straight] + config.Nstrands[pgsTypes::Harped] : config.Nstrands[strandType]);
   StrandIndexType nDebond = (strandType == pgsTypes::Permanent ? config.Debond[pgsTypes::Straight].size() + config.Debond[pgsTypes::Harped].size() : config.Debond[strandType].size());
   ATLASSERT(nDebond <= Ns); // must be true!

   // Quick check to make sure there is even an adjustment to be made
   // If there are no strands, just leave
   if ( Ns == 0 )
      return 1.0;

   // Determine effectiveness of the debonded strands
   Float64 nDebondedEffective = 0; // number of effective debonded strands
   std::vector<DEBONDINFO>::const_iterator iter;

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

   for ( int i = (int)st1; i <= (int)st2; i++ )
   {
      pgsTypes::StrandType st = (pgsTypes::StrandType)i;

      for ( iter = config.Debond[st].begin(); iter != config.Debond[st].end(); iter++ )
      {
         const DEBONDINFO& debond_info = *iter;

         Float64 right_db_from_left = gdr_length - debond_info.RightDebondLength;

         // see if bonding occurs at all here
         if ( debond_info.LeftDebondLength<dist_from_left_end && dist_from_left_end<right_db_from_left)
         {
            // compute minimum bonded length from poi
            Float64 left_len = dist_from_left_end - debond_info.LeftDebondLength;
            Float64 rgt_len  = right_db_from_left - dist_from_left_end;
            Float64 min_db_len = min(left_len, rgt_len);

            if (min_db_len<xfer_length)
               nDebondedEffective += min_db_len/xfer_length;
            else
               nDebondedEffective += 1.0;

         }
         else
         {
            // strand is not bonded at the location of the POI
            nDebondedEffective += 0.0;
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

Float64 pgsPsForceEng::GetDevLength(const pgsPointOfInterest& poi,bool bDebonded)
{
   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,bDebonded);
   return details.ld;
}

Float64 pgsPsForceEng::GetDevLength(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config)
{
   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,bDebonded,config);
   return details.ld;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded)
{
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(spanIdx,gdrIdx,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   double mbrDepth = pGirder->GetHeight(poi);

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 fpe = pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterLosses);

   GET_IFACE(IMomentCapacity,pMomCap);
   MOMENTCAPACITYDETAILS mcd;
   pMomCap->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mcd); // positive moment

   STRANDDEVLENGTHDETAILS details;
   details.db = pstrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = mcd.fps;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded);
   details.ld = lrfdPsStrand::GetDevLength( *pstrand, details.fps, details.fpe, mbrDepth, bDebonded );
   details.lt = GetXferLength(spanIdx,gdrIdx,pgsTypes::Permanent);

   return details;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,Float64 fps,Float64 fpe)
{
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(spanIdx,gdrIdx,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   double mbrDepth = pGirder->GetHeight(poi);

   STRANDDEVLENGTHDETAILS details;
   details.db = pstrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = fps;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded);
   details.ld = lrfdPsStrand::GetDevLength( *pstrand, details.fps, details.fpe, mbrDepth, bDebonded );
   details.lt = GetXferLength(spanIdx,gdrIdx,pgsTypes::Permanent);

   return details;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config)
{
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(spanIdx,gdrIdx,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   double mbrDepth = pGirder->GetHeight(poi);

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 fpe = pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,config,pgsTypes::AfterLosses);

   GET_IFACE(IMomentCapacity,pMomCap);
   MOMENTCAPACITYDETAILS mcd;
   pMomCap->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,config,true,&mcd); // positive moment

   STRANDDEVLENGTHDETAILS details;
   details.db = pstrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = mcd.fps;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded);
   details.ld = lrfdPsStrand::GetDevLength( *pstrand, details.fps, details.fpe, mbrDepth, bDebonded );
   details.lt = GetXferLength(spanIdx,gdrIdx,pgsTypes::Permanent);

   return details;
}

STRANDDEVLENGTHDETAILS pgsPsForceEng::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config,Float64 fps,Float64 fpe)
{
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(spanIdx,gdrIdx,pgsTypes::Permanent);

   GET_IFACE(IGirder,pGirder);
   double mbrDepth = pGirder->GetHeight(poi);

   STRANDDEVLENGTHDETAILS details;
   details.db = pstrand->GetNominalDiameter();
   details.fpe = fpe;
   details.fps = fps;
   details.k = lrfdPsStrand::GetDevLengthFactor(mbrDepth,bDebonded);
   details.ld = lrfdPsStrand::GetDevLength( *pstrand, details.fps, details.fpe, mbrDepth, bDebonded );
   details.lt = GetXferLength(spanIdx,gdrIdx,pgsTypes::Permanent);

   return details;
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,false);
   return GetDevLengthAdjustment(poi,strandIdx,strandType,details.fps,details.fpe);
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe)
{
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration( poi.GetSpan(), poi.GetGirder() );
   return GetDevLengthAdjustment(poi,strandIdx,strandType,config,fps,fpe);
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,false,config);
   return GetDevLengthAdjustment(poi,strandIdx,strandType,config,details.fps,details.fpe);
}

Float64 pgsPsForceEng::GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64 fps,Float64 fpe)
{
   // Compute a scaling factor to apply to the basic prestress force to
   // adjust for prestress force in the development
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);

   Float64 poi_loc = poi.GetDistFromStart();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 bond_start, bond_end;
   bool bDebonded = pStrandGeom->IsStrandDebonded(span,gdr,strandIdx,strandType,config,&bond_start,&bond_end);

   bool bExtendedStrand = pStrandGeom->IsExtendedStrand(poi,strandIdx,strandType,config);

   // determine minimum bonded length from poi
   Float64 left_bonded_length, right_bonded_length;
   if ( bDebonded )
   {
      // measure bonded length
      left_bonded_length = poi_loc - bond_start;
      right_bonded_length = bond_end - poi_loc;
   }
   else if ( bExtendedStrand )
   {
      // strand is extended into end diaphragm... the development lenght adjustment is 1.0
      return 1.0;
   }
   else
   {
      // no debonding, bond length is to ends of girder
      Float64 gdr_length      = pBridge->GetGirderLength(span,gdr);

      left_bonded_length  = poi_loc;
      right_bonded_length = gdr_length - poi_loc;
   }

   Float64 lpx = min(left_bonded_length, right_bonded_length);

   if (lpx <= 0.0)
   {
      // strand is unbonded at location, no more to do
      return 0.0;
   }
   else
   {
      STRANDDEVLENGTHDETAILS details = GetDevLengthDetails(poi,bDebonded,config,fps,fpe);
      Float64 xfer_length = details.lt;
      Float64 dev_length  = details.ld;

      Float64 adjust = -999; // dummy value, helps with debugging

      if ( lpx <= xfer_length)
      {
         adjust = (fpe < fps ? (lpx*fpe) / (xfer_length*fps) : 1.0);
      }
      else if ( lpx < dev_length )
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

Float64 pgsPsForceEng::GetHoldDownForce(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType Nh = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Harped);
   if (0 < Nh)
   {
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_HARPINGPOINT);
   
      // no hold down force if there aren't any harped strands
      if ( vPOI.size() == 0 )
         return 0;

      pgsPointOfInterest poi = vPOI[0];
   
      // move the POI just before the harping point so we are sure
      // that we are using a point on the sloped section of the strands
      poi.SetDistFromStart( poi.GetDistFromStart() - 0.01 );

      Float64 harped = GetPrestressForce(poi,pgsTypes::Harped,pgsTypes::Jacking);

      // Adjust for slope
      Float64 slope = pStrandGeom->GetAvgStrandSlope( poi );

      Float64 F;
      F = harped / sqrt( 1*1 + slope*slope );

      return F;
   }
   else
   {
      return 0;
   }
}

Float64 pgsPsForceEng::GetHoldDownForce(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config)
{
   if (config.Nstrands[pgsTypes::Harped] > 0)
   {
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_HARPINGPOINT);
   
      // no hold down force if there aren't any harped strands
      if ( vPOI.size() == 0 )
         return 0;

      pgsPointOfInterest poi = vPOI[0];
   
      // move the POI just before the harping point so we are sure
      // that we are using a point on the sloped section of the strands
      poi.SetDistFromStart( poi.GetDistFromStart() - 0.01 );

      Float64 harped = GetPrestressForce(poi,pgsTypes::Harped,pgsTypes::Jacking,config);

      // Adjust for slope
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 slope = pStrandGeom->GetAvgStrandSlope( poi,config.Nstrands[pgsTypes::Harped],config.EndOffset,config.HpOffset );

      Float64 F;
      F = harped / sqrt( 1*1 + slope*slope );

      return F;
   }
   else
   {
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

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,
                                         pgsTypes::StrandType strandType,
                                         pgsTypes::LossStage stage)
{
   ////////////////////////////////////////////////////////////////////////////////
   //******************************************************************************
   // NOTE: This method is almost identical to the other GetPrestressForce method.
   // Any changes done here will likely need to be done there as well
   //******************************************************************************
   ////////////////////////////////////////////////////////////////////////////////
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Ns = pStrandGeom->GetNumStrands(span,gdr,strandType);
   if (Ns == 0)
      return 0;


   GET_IFACE(IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   const matPsStrand* pstrand = girderData.Material.pStrandMaterial[strandType == pgsTypes::Permanent ? pgsTypes::Straight : strandType];
   CHECK(pstrand!=0);
   
   // Get the strand stress
   Float64 fps = GetStrandStress(poi,strandType,stage); // ( accounts for losses and adjusts for lack of development length)

   pgsTypes::TTSUsage tempStrandUsage = girderData.TempStrandUsage;

   if ( strandType == pgsTypes::Temporary )
   {
      switch( tempStrandUsage )
      {
      case pgsTypes::ttsPretensioned:
         if ( pgsTypes::AfterTemporaryStrandRemoval <= stage && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;
         break;

      case pgsTypes::ttsPTBeforeLifting:
         if ( (stage < pgsTypes::AtLifting || pgsTypes::AfterTemporaryStrandRemoval <= stage) && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;

         break;
         
      case pgsTypes::ttsPTAfterLifting:
      case pgsTypes::ttsPTBeforeShipping:
         if ( (stage < pgsTypes::AtShipping || pgsTypes::AfterTemporaryStrandRemoval <= stage) && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;
         break;
      }
   }

   Float64 aps = pstrand->GetNominalArea();

   // determine the strand area we need to use
   Float64 Aps;
   Aps = aps * Ns;

   // Compute the requested prestress force
   Float64 Fps;
   Fps = Aps * fps;

   return Fps;
}

Float64 pgsPsForceEng::GetPrestressForce(const pgsPointOfInterest& poi,
                                         pgsTypes::StrandType strandType,
                                         pgsTypes::LossStage stage,
                                         const GDRCONFIG& config)
{
   ////////////////////////////////////////////////////////////////////////////////
   //******************************************************************************
   // NOTE: This method is almost identical to the other GetPrestressForce method.
   // Any changes done here will likely need to be done there as well
   //******************************************************************************
   StrandIndexType Ns = (strandType == pgsTypes::Permanent ? config.Nstrands[pgsTypes::Straight] + config.Nstrands[pgsTypes::Harped] : config.Nstrands[strandType]);

   if ( Ns == 0 )
      return 0;

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(IGirderData,pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr,strandType);
   CHECK(pstrand!=0);
   
   // Get the strand stress
   Float64 fps = GetStrandStress(poi,strandType,stage,config); // ( accounts for losses and transfer length adjustment)

   if ( strandType == pgsTypes::Temporary )
   {
      switch( config.TempStrandUsage )
      {
      case pgsTypes::ttsPretensioned:
         if ( pgsTypes::AfterTemporaryStrandRemoval <= stage && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;
         break;

      case pgsTypes::ttsPTBeforeLifting:
         if ( (stage < pgsTypes::AtLifting || pgsTypes::AfterTemporaryStrandRemoval <= stage) && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;

         break;
         
      case pgsTypes::ttsPTAfterLifting:
      case pgsTypes::ttsPTBeforeShipping:
         if ( (stage < pgsTypes::AtShipping || pgsTypes::AfterTemporaryStrandRemoval <= stage) && stage != pgsTypes::AfterTemporaryStrandInstallation )
            Ns = 0;
         break;
      }
   }

   Float64 aps = pstrand->GetNominalArea();

   // determine the strand area we need to use
   Float64 Aps;
   Aps = aps * Ns;

   // Compute the requested prestress force
   Float64 Fps;
   Fps = Aps * fps;

   return Fps;
}

Float64 pgsPsForceEng::GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage stage)
{
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
   return GetStrandStress(poi,strandType,stage,config);
}

Float64 pgsPsForceEng::GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage stage,const GDRCONFIG& config)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // Get the prestressing input information
   GET_IFACE(ILosses,pLosses);
   GET_IFACE(IGirderData, pGirderData );
   const matPsStrand* pStrand = pGirderData->GetStrandMaterial(span,gdr,strandType);

   Float64 Pj;
   StrandIndexType N;

   if ( strandType == pgsTypes::Permanent )
   {
      Pj = config.Pjack[pgsTypes::Straight]    + config.Pjack[pgsTypes::Harped];
      N  = config.Nstrands[pgsTypes::Straight] + config.Nstrands[pgsTypes::Harped];
   }
   else
   {
      Pj = config.Pjack[strandType];
      N  = config.Nstrands[strandType];
   }

   if ( strandType == pgsTypes::Temporary )
   {
      if ( 
           ( (stage == pgsTypes::Jacking || stage == pgsTypes::BeforeXfer || stage == pgsTypes::AfterXfer) && config.TempStrandUsage != pgsTypes::ttsPretensioned ) ||
           (  stage == pgsTypes::AtLifting && (config.TempStrandUsage == pgsTypes::ttsPTAfterLifting || config.TempStrandUsage == pgsTypes::ttsPTBeforeShipping) )   ||
           (  stage == pgsTypes::AfterTemporaryStrandRemoval || stage == pgsTypes::AfterDeckPlacement || stage == pgsTypes::AfterSIDL || stage == pgsTypes::AfterLosses )
         )
      {
         N = 0;
         Pj = 0;
      }
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
   Float64 fps;
   Float64 loss;

   switch ( stage )
   {
   case pgsTypes::Jacking:
      loss = 0.0;
      break;

   case pgsTypes::BeforeXfer:
      loss = pLosses->GetBeforeXferLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterXfer:
      loss = pLosses->GetAfterXferLosses(poi,strandType,config);
      break;

   case pgsTypes::AtLifting:
      loss = pLosses->GetLiftingLosses(poi,strandType,config);
      break;

   case pgsTypes::AtShipping:
      loss = pLosses->GetShippingLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterTemporaryStrandInstallation:
      loss = pLosses->GetAfterTemporaryStrandInstallationLosses(poi,strandType,config);
      break;

   case pgsTypes::BeforeTemporaryStrandRemoval:
      loss = pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterTemporaryStrandRemoval:
      loss = pLosses->GetAfterTemporaryStrandRemovalLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterDeckPlacement:
      loss = pLosses->GetDeckPlacementLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterSIDL:
      loss = pLosses->GetSIDLLosses(poi,strandType,config);
      break;

   case pgsTypes::AfterLosses:
      loss = pLosses->GetFinal(poi,strandType,config);
      // Final losses are relative to stress immedately before transfer (LRFD 5.9.5.1)
      break;

   default:
      ATLASSERT(false);
   }

   fps = fpj - loss;

   ATLASSERT( 0 <= fps ); // strand stress must be greater than or equal to zero.

   // Reduce for transfer effect
   Float64 adjust = GetXferLengthAdjustment(poi,strandType,config);
   fps *= adjust;

   return fps;
}
