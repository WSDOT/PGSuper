///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "DesignGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <LRFD\ConcreteUtil.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnDesignGirder::txnDesignGirder( std::vector<const pgsGirderDesignArtifact*>& artifacts, pgsTypes::SlabOffsetType slabOffsetType)
{
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iter(artifacts.begin());
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iterEnd(artifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsGirderDesignArtifact& gdrDesignArtifact(*(*iter));
      DesignData data(gdrDesignArtifact);
      data.m_SlabOffsetType[1] = slabOffsetType;

      m_DesignDataColl.push_back(data);
   }

   m_bInit = false;
}

txnDesignGirder::~txnDesignGirder()
{
}

bool txnDesignGirder::Execute()
{
   if ( !m_bInit )
      Init();

   DoExecute(1);
   return true;
}

void txnDesignGirder::Undo()
{
   DoExecute(0);
}

txnTransaction* txnDesignGirder::CreateClone() const
{
   pgsTypes::SlabOffsetType slabtype;
   std::vector<const pgsGirderDesignArtifact*> artifacts;
   bool first = true;
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      artifacts.push_back(&(iter->m_DesignArtifact));

      if (first)
         slabtype = iter->m_SlabOffsetType[1];

      first = false;
   }

   return new txnDesignGirder(artifacts, slabtype);
}

std::_tstring txnDesignGirder::Name() const
{
   std::_tostringstream os;
   if ( m_DesignDataColl.size() == 1 )
   {
      const CGirderKey& girderKey = m_DesignDataColl.front().m_DesignArtifact.GetGirderKey();
      SpanIndexType spanIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;
      os << _T("Design for Span ") << LABEL_SPAN(spanIdx) << _T(", Girder ") << LABEL_GIRDER(gdrIdx);
   }
   else
   {
	   os << _T("Design for (Span, Girder) =");
	   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
	   {
	      const CGirderKey& girderKey = iter->m_DesignArtifact.GetGirderKey();
	      SpanIndexType spanIdx = girderKey.groupIndex;
	      GirderIndexType gdrIdx = girderKey.girderIndex;
	      os << _T(" (") << LABEL_SPAN(spanIdx) << _T(", ") << LABEL_GIRDER(gdrIdx)<< _T(")");
	   }
   }

   return os.str();
}

bool txnDesignGirder::IsUndoable()
{
   return true;
}

bool txnDesignGirder::IsRepeatable()
{
   return false;
}

void txnDesignGirder::Init()
{
   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);


      if (pArtifact->GetDesignOptions().doDesignForFlexure != dtNoDesign)
      {
         CacheFlexureDesignResults(rdata);
      }

      if (pArtifact->GetDesignOptions().doDesignForShear)
      {
         CacheShearDesignResults(rdata);
      }
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   // Loop over all girder designs
   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey segmentKey = pArtifact->GetSegmentKey();

      arDesignOptions design_options = pArtifact->GetDesignOptions();

      if (design_options.doDesignForFlexure != dtNoDesign)
      {
         pSegmentData->SetSegmentMaterial(segmentKey,rdata.m_Material[i]);
         pSegmentData->SetStrandData(segmentKey,rdata.m_Strands[i]);
         pSegmentData->SetHandlingData(segmentKey,rdata.m_HandlingData[i]);

         if ( rdata.m_SlabOffsetType[i] == pgsTypes::sotBridge )
         {
            pIBridgeDesc->SetSlabOffset(rdata.m_SlabOffset[pgsTypes::metStart][i]);
         }
         else
         {
            GET_IFACE2(pBroker,IBridge,pBridge);
            PierIndexType startPierIdx, endPierIdx;
            pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
            pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex,rdata.m_SlabOffset[pgsTypes::metStart][i]);
            pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex,rdata.m_SlabOffset[pgsTypes::metEnd][i]);
         }
      }

      if (design_options.doDesignForShear)
      {
         GET_IFACE2(pBroker,IShear,pShear);
         pShear->SetSegmentShearData(segmentKey,rdata.m_ShearData[i]);

#pragma Reminder("BUG: this may be a bug")
         // RAB: commented this block out when merging RDP's shear design code over
         // I think the call to SetShearData above is enough for this version of PGSuper/PGSplice

         // RAB: 9/9/2013... on the Patches and Head branches, the block of code below
         // was moved above the SetSegemntShearData call above because it was wiping out
         // the shear data.


         //if (design_options.doDesignForFlexure==dtNoDesign)
         //{
         //   // Need to set girder data in order to pick up long reinf for shear design
         //   // and possible increase in concrete strength for shear stress
         //   pGirderData->SetGirderData(rdata.m_GirderData[i], span, gdr);
         //}
      }
   }

  pEvents->FirePendingEvents();
}

void txnDesignGirder::CacheFlexureDesignResults(DesignData& rdata)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

#pragma Reminder("UPDATE: assuming precast girder bridge")
   SegmentIndexType segIdx = 0;
   const pgsSegmentDesignArtifact* pArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
   const CSegmentKey segmentKey = pArtifact->GetSegmentKey();

   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   std::_tstring gdrName = pGirder->GetGirderName();

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 HgStart, HgHp1, HgHp2, HgEnd;
   pStrandGeom->GetHarpedStrandControlHeights(segmentKey,&HgStart,&HgHp1,&HgHp2,&HgEnd);


   arDesignOptions design_options = pArtifact->GetDesignOptions();

   if (dtDesignFullyBondedRaised  != design_options.doDesignForFlexure &&
       dtDesignForDebondingRaised != design_options.doDesignForFlexure)
   {
   // old (existing) girder data
   rdata.m_Material[0]     = *pSegmentData->GetSegmentMaterial(segmentKey);
   rdata.m_Strands[0]      = *pSegmentData->GetStrandData(segmentKey);
   rdata.m_HandlingData[0] = *pSegmentData->GetHandlingData(segmentKey);

   // new girder data (start with the old and tweak it)
   rdata.m_Material[1]     = rdata.m_Material[0];
   rdata.m_Strands[1]      = rdata.m_Strands[0];
   rdata.m_HandlingData[1] = rdata.m_HandlingData[0];

      // Strand Designs using continuous fill for all stands
      pgsTypes::AdjustableStrandType adjType;
      if(dtDesignForHarping == design_options.doDesignForFlexure)
      {
         adjType = pgsTypes::asHarped;
      }
      else
      {
         adjType = pgsTypes::asStraight;
      }

      rdata.m_Strands[1].SetAdjustableStrandType(adjType);


      ConfigStrandFillVector harpfillvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, pArtifact->GetNumHarpedStrands());

      // Convert Adjustable strand offset data
      // offsets are absolute measure in the design artifact
      // Convert them to the measurement basis that the CGirderData object is using, unless it's the default,
      // then let's use a favorite
      if (hsoLEGACY == rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtEnd())
      {
         rdata.m_Strands[1].SetHarpStrandOffsetMeasurementAtEnd(hsoBOTTOM2BOTTOM);
      }

      rdata.m_Strands[1].SetHarpStrandOffsetAtEnd(pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), adjType,
                                                                                          HgStart, HgHp1, HgHp2, HgEnd,
                                                                                          harpfillvec, 
                                                                                          rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                          pArtifact->GetHarpStrandOffsetEnd()));

      if (hsoLEGACY == rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtHarpPoint())
      {
         rdata.m_Strands[1].SetHarpStrandOffsetMeasurementAtHarpPoint(hsoBOTTOM2BOTTOM);
      }

      rdata.m_Strands[1].SetHarpStrandOffsetAtHarpPoint(pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), adjType,
                                                                                        HgStart, HgHp1, HgHp2, HgEnd,
                                                                                        harpfillvec, 
                                                                                        rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                        pArtifact->GetHarpStrandOffsetHp()));

      // See if strand design data fits in grid
      bool fills_grid=false;
      StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
      StrandIndexType ns(0), nh(0);
      if (design_options.doStrandFillType == ftGridOrder)
      {
         // we asked design to fill using grid, but this may be a non-standard design - let's check
         if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, segmentKey, &ns, &nh))
         {
            if (ns == pArtifact->GetNumStraightStrands() && nh == pArtifact->GetNumHarpedStrands() )
            {
               fills_grid = true;
            }
         }
      }

      if ( fills_grid )
      {
         ATLASSERT(num_permanent==ns+nh);
         rdata.m_Strands[1].SetTotalPermanentNstrands(num_permanent, ns, nh);
         rdata.m_Strands[1].SetPjack(pgsTypes::Permanent,pArtifact->GetPjackStraightStrands() + pArtifact->GetPjackHarpedStrands());
         rdata.m_Strands[1].IsPjackCalculated(pgsTypes::Permanent, pArtifact->GetUsedMaxPjackStraightStrands());
      }
      else
      {
         rdata.m_Strands[1].SetHarpedStraightNstrands(pArtifact->GetNumStraightStrands(), pArtifact->GetNumHarpedStrands());
      }

      rdata.m_Strands[1].SetTemporaryNstrands(pArtifact->GetNumTempStrands());

   }
   else
   {
      // Raised straight design
      rdata.m_Strands[1].SetAdjustableStrandType(pgsTypes::asStraight);

      // Raised straight adjustable strands are filled directly, but others use fill order.
      // must convert all to DirectStrandFillCollection
      ConfigStrandFillVector strvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Straight, pArtifact->GetNumStraightStrands());
      CDirectStrandFillCollection strfill = ConvertConfigToDirectStrandFill(strvec);
      rdata.m_Strands[1].SetDirectStrandFillStraight(strfill);

      CDirectStrandFillCollection harpfill = ConvertConfigToDirectStrandFill(pArtifact->GetRaisedAdjustableStrands());
      rdata.m_Strands[1].SetDirectStrandFillHarped(harpfill);

      ConfigStrandFillVector tempvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Temporary, pArtifact->GetNumTempStrands());
      CDirectStrandFillCollection tempfill = ConvertConfigToDirectStrandFill(tempvec);
      rdata.m_Strands[1].SetDirectStrandFillTemporary(tempfill);

      // Convert Adjustable strand offset data. This is typically zero from library, but must be converted to input datum
      // offsets are absolute measure in the design artifact
      // convert them to the measurement basis that the CGirderData object is using
      ConfigStrandFillVector harpfillvec = pArtifact->GetRaisedAdjustableStrands();
         
      rdata.m_Strands[1].SetHarpStrandOffsetAtEnd(pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(gdrName.c_str(), pgsTypes::asStraight,
                                                                                          HgStart, HgHp1, HgHp2, HgEnd,
                                                                                          harpfillvec, 
                                                                                          rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtEnd(), 
                                                                                          pArtifact->GetHarpStrandOffsetEnd()));

      rdata.m_Strands[1].SetHarpStrandOffsetAtHarpPoint(pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(gdrName.c_str(), pgsTypes::asStraight,
                                                                                        HgStart, HgHp1, HgHp2, HgEnd,
                                                                                        harpfillvec, 
                                                                                        rdata.m_Strands[1].GetHarpStrandOffsetMeasurementAtHarpPoint(), 
                                                                                        pArtifact->GetHarpStrandOffsetHp()));
   }

   rdata.m_Strands[1].SetPjack(pgsTypes::Harped,             pArtifact->GetPjackHarpedStrands());
   rdata.m_Strands[1].SetPjack(pgsTypes::Straight,           pArtifact->GetPjackStraightStrands());
   rdata.m_Strands[1].SetPjack(pgsTypes::Temporary,          pArtifact->GetPjackTempStrands());
   rdata.m_Strands[1].IsPjackCalculated(pgsTypes::Harped,    pArtifact->GetUsedMaxPjackHarpedStrands());
   rdata.m_Strands[1].IsPjackCalculated(pgsTypes::Straight,  pArtifact->GetUsedMaxPjackStraightStrands());
   rdata.m_Strands[1].IsPjackCalculated(pgsTypes::Temporary, pArtifact->GetUsedMaxPjackTempStrands());
   rdata.m_Strands[1].SetLastUserPjack(pgsTypes::Harped,     pArtifact->GetPjackHarpedStrands());
   rdata.m_Strands[1].SetLastUserPjack(pgsTypes::Straight,   pArtifact->GetPjackStraightStrands());
   rdata.m_Strands[1].SetLastUserPjack(pgsTypes::Temporary,  pArtifact->GetPjackTempStrands());

   rdata.m_Strands[1].SetTemporaryStrandUsage( pArtifact->GetTemporaryStrandUsage() );

   // Get debond information from design artifact
   rdata.m_Strands[1].ClearDebondData();
   rdata.m_Strands[1].IsSymmetricDebond(true);  // design is always symmetric


   // TRICKY: Mapping from DEBONDCONFIG to CDebondInfo is tricky because
   //         former designates individual strands and latter stores strands
   //         in grid order.
   // Use utility tool to make the strand indexing conversion
   ConfigStrandFillVector strtfillvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Straight, pArtifact->GetNumStraightStrands());
   ConfigStrandFillTool fillTool( strtfillvec );

   DebondConfigCollection dbcoll = pArtifact->GetStraightStrandDebondInfo();
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondConfigConstIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDCONFIG& rdbrinfo = *dbit;

      CDebondData cdbi;

      StrandIndexType gridIndex, otherPos;
      fillTool.StrandPositionIndexToGridIndex(rdbrinfo.strandIdx, &gridIndex, &otherPos);

      cdbi.strandTypeGridIdx = gridIndex;

      // If there is another position, this is a pair. Increment to next position
      if (otherPos != INVALID_INDEX)
      {
         dbit++;

#ifdef _DEBUG
         const DEBONDCONFIG& ainfo = *dbit;
         StrandIndexType agrid;
         fillTool.StrandPositionIndexToGridIndex(ainfo.strandIdx, &agrid, &otherPos);
         ATLASSERT(agrid==gridIndex); // must have the same grid index
#endif
      }

      cdbi.Length[pgsTypes::metStart] = rdbrinfo.DebondLength[pgsTypes::metStart];
      cdbi.Length[pgsTypes::metEnd]   = rdbrinfo.DebondLength[pgsTypes::metEnd];

      rdata.m_Strands[1].GetDebonding(pgsTypes::Straight).push_back(cdbi);
   }
   
   // concrete
   rdata.m_Material[1].Concrete.Fci = pArtifact->GetReleaseStrength();
   if (!rdata.m_Material[1].Concrete.bUserEci)
   {
      rdata.m_Material[1].Concrete.Eci = lrfdConcreteUtil::ModE( rdata.m_Material[1].Concrete.Fci, 
                                                             rdata.m_Material[1].Concrete.StrengthDensity, 
                                                             false  ); // ignore LRFD range checks 
      rdata.m_Material[1].Concrete.Eci *= (rdata.m_Material[1].Concrete.EcK1*rdata.m_Material[1].Concrete.EcK2);
   }

   rdata.m_Material[1].Concrete.Fc  = pArtifact->GetConcreteStrength();
   if (!rdata.m_Material[1].Concrete.bUserEc)
   {
      rdata.m_Material[1].Concrete.Ec = lrfdConcreteUtil::ModE( rdata.m_Material[1].Concrete.Fc, 
                                                            rdata.m_Material[1].Concrete.StrengthDensity, 
                                                            false );// ignore LRFD range checks 
      rdata.m_Material[1].Concrete.Ec *= (rdata.m_Material[1].Concrete.EcK1*rdata.m_Material[1].Concrete.EcK2);
   }

   // deck offset
   rdata.m_SlabOffset[pgsTypes::metStart][0] = pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metStart),segmentKey.girderIndex);
   rdata.m_SlabOffset[pgsTypes::metEnd][0]   = pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metEnd),  segmentKey.girderIndex);
   rdata.m_SlabOffset[pgsTypes::metStart][1] = pArtifact->GetSlabOffset(pgsTypes::metStart);
   rdata.m_SlabOffset[pgsTypes::metEnd][1]   = pArtifact->GetSlabOffset(pgsTypes::metEnd);

   // get old deck offset type... new type was set in constructor
   rdata.m_SlabOffsetType[0] = pBridgeDesc->GetSlabOffsetType();

   // lifting
   if ( design_options.doDesignLifting )
   {
      rdata.m_HandlingData[1].LeftLiftPoint  = pArtifact->GetLeftLiftingLocation();
      rdata.m_HandlingData[1].RightLiftPoint = pArtifact->GetRightLiftingLocation();
   }

   // shipping
   if ( design_options.doDesignHauling )
   {
      rdata.m_HandlingData[1].LeadingSupportPoint  = pArtifact->GetLeadingOverhang();
      rdata.m_HandlingData[1].TrailingSupportPoint = pArtifact->GetTrailingOverhang();
   }
}

void txnDesignGirder::CacheShearDesignResults(DesignData& rdata)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IShear,pShear);

#pragma Reminder("UPDATE: assuming precast girder bridge")
   SegmentIndexType segIdx = 0;
   const pgsSegmentDesignArtifact* pArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
   const CSegmentKey segmentKey = pArtifact->GetSegmentKey();

   rdata.m_ShearData[0] = *pShear->GetSegmentShearData(segmentKey);

   // get the design data
   rdata.m_ShearData[1] = rdata.m_ShearData[0]; // start with the old and tweak
   rdata.m_ShearData[1].ShearZones.clear();

   ZoneIndexType nShearZones = pArtifact->GetNumberOfStirrupZonesDesigned();
   if (0 < nShearZones)
   {
      rdata.m_ShearData[1] =  *pArtifact->GetShearData();
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData2 dat;
      rdata.m_ShearData[1].ShearZones.push_back(dat);
   }

#pragma Reminder("BUG: this may be a bug")
   // RAB: Commented this code out when merging RDP's shear design code
   // I think the couple lines above gets the job done

   //if(rdata.m_DesignArtifact.GetWasLongitudinalRebarForShearDesigned())
   //{
   //   // Rebar data was changed during shear design
   //   rdata.m_GirderData[1].LongitudinalRebarData  = rdata.m_DesignArtifact.GetLongitudinalRebarData();
   //}
///////////////// END OF COMMENT BLOCK RELATED TO BUG REMINDER /////////////////////

   // It is possible for shear stress to control final concrete strength
   // Make sure it is updated if no flexural design was requested
   if (pArtifact->GetDesignOptions().doDesignForFlexure == dtNoDesign)
   {
      rdata.m_Material[1].Concrete.Fc = pArtifact->GetConcreteStrength();
      if (!rdata.m_Material[1].Concrete.bUserEc)
      {
         rdata.m_Material[1].Concrete.Ec = lrfdConcreteUtil::ModE( rdata.m_Material[1].Concrete.Fc, 
                                                               rdata.m_Material[1].Concrete.StrengthDensity, 
                                                               false );// ignore LRFD range checks 
         rdata.m_Material[1].Concrete.Ec *= (rdata.m_Material[1].Concrete.EcK1*rdata.m_Material[1].Concrete.EcK2);
      }
   }
}
