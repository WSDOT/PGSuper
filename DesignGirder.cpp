///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnDesignGirder::txnDesignGirder( std::vector<const pgsDesignArtifact*>& artifacts, pgsTypes::SlabOffsetType slabOffsetType)
{
   for (std::vector<const pgsDesignArtifact*>::iterator it= artifacts.begin(); it != artifacts.end(); it++)
   {
      DesignData data;
      data.m_DesignArtifact = *(*it);
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
   std::vector<const pgsDesignArtifact*> artifacts;
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
   os << "Design for (Span, Girder) =";
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      os <<" (" << LABEL_SPAN(iter->m_DesignArtifact.GetSpan()) << ", " << LABEL_GIRDER(iter->m_DesignArtifact.GetGirder())<<")";
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

      if (rdata.m_DesignArtifact.GetDesignOptions().doDesignForFlexure != dtNoDesign)
      {
         CacheFlexureDesignResults(rdata);
      }

      if (rdata.m_DesignArtifact.GetDesignOptions().doDesignForShear)
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
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   // Loop over all girder designs
   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

      SpanIndexType span  = rdata.m_DesignArtifact.GetSpan();
      GirderIndexType gdr = rdata.m_DesignArtifact.GetGirder();
      arDesignOptions design_options = rdata.m_DesignArtifact.GetDesignOptions();

      if (design_options.doDesignForFlexure != dtNoDesign)
      {
         pGirderData->SetGirderData(rdata.m_GirderData[i], span, gdr);

         if ( rdata.m_SlabOffsetType[i] == pgsTypes::sotBridge )
         {
            pIBridgeDesc->SetSlabOffset(rdata.m_SlabOffset[pgsTypes::metStart][i]);
         }
         else
         {
            pIBridgeDesc->SetSlabOffset( span, gdr,rdata. m_SlabOffset[pgsTypes::metStart][i], rdata.m_SlabOffset[pgsTypes::metEnd][i]);
         }
      }

      if (design_options.doDesignForShear)
      {
         GET_IFACE2(pBroker,IShear,pShear);
         pShear->SetShearData(rdata.m_ShearData[i], span, gdr);
      }
   }

  pEvents->FirePendingEvents();
}

void txnDesignGirder::CacheFlexureDesignResults(DesignData& rdata)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

   SpanIndexType   span  = rdata.m_DesignArtifact.GetSpan();
   GirderIndexType gdr   = rdata.m_DesignArtifact.GetGirder();

   arDesignOptions design_options = rdata.m_DesignArtifact.GetDesignOptions();

   // old (existing) girder data
   rdata.m_GirderData[0] = pGirderData->GetGirderData(span, gdr);

   // new girder data (start with the old and tweak it)
   rdata.m_GirderData[1] = rdata.m_GirderData[0];

   // Convert Harp offset data
   // offsets are absolute measure in the design artifact
   // convert them to the measurement basis that the CGirderData object is using
   rdata.m_GirderData[1].HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, 
                                                                                       rdata.m_DesignArtifact.GetNumHarpedStrands(), 
                                                                                       rdata.m_GirderData[1].HsoEndMeasurement, 
                                                                                       rdata.m_DesignArtifact.GetHarpStrandOffsetEnd());

   rdata.m_GirderData[1].HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                                     rdata.m_DesignArtifact.GetNumHarpedStrands(), 
                                                                                     rdata.m_GirderData[1].HsoHpMeasurement, 
                                                                                     rdata.m_DesignArtifact.GetHarpStrandOffsetHp());

   // see if strand design data fits in grid
   bool fills_grid=false;
   StrandIndexType num_permanent = rdata.m_DesignArtifact.GetNumHarpedStrands() + rdata.m_DesignArtifact.GetNumStraightStrands();
   if (design_options.doStrandFillType == ftGridOrder)
   {
      // we asked design to fill using grid, but this may be a non-standard design - let's check
      StrandIndexType ns, nh;
      if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
      {
         if (ns == rdata.m_DesignArtifact.GetNumStraightStrands() && nh ==  rdata.m_DesignArtifact.GetNumHarpedStrands() )
         {
            fills_grid = true;
         }
      }
   }

   if ( fills_grid )
   {
      rdata.m_GirderData[1].NumPermStrandsType = NPS_TOTAL_NUMBER;

      rdata.m_GirderData[1].Nstrands[pgsTypes::Permanent]            = num_permanent;
      rdata.m_GirderData[1].Pjack[pgsTypes::Permanent]               = rdata.m_DesignArtifact.GetPjackStraightStrands() + rdata.m_DesignArtifact.GetPjackHarpedStrands();
      rdata.m_GirderData[1].bPjackCalculated[pgsTypes::Permanent]    = rdata.m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   }
   else
   {
      rdata.m_GirderData[1].NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }

   rdata.m_GirderData[1].Nstrands[pgsTypes::Harped]            = rdata.m_DesignArtifact.GetNumHarpedStrands();
   rdata.m_GirderData[1].Nstrands[pgsTypes::Straight]          = rdata.m_DesignArtifact.GetNumStraightStrands();
   rdata.m_GirderData[1].Nstrands[pgsTypes::Temporary]         = rdata.m_DesignArtifact.GetNumTempStrands();
   rdata.m_GirderData[1].Pjack[pgsTypes::Harped]               = rdata.m_DesignArtifact.GetPjackHarpedStrands();
   rdata.m_GirderData[1].Pjack[pgsTypes::Straight]             = rdata.m_DesignArtifact.GetPjackStraightStrands();
   rdata.m_GirderData[1].Pjack[pgsTypes::Temporary]            = rdata.m_DesignArtifact.GetPjackTempStrands();
   rdata.m_GirderData[1].bPjackCalculated[pgsTypes::Harped]    = rdata.m_DesignArtifact.GetUsedMaxPjackHarpedStrands();
   rdata.m_GirderData[1].bPjackCalculated[pgsTypes::Straight]  = rdata.m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   rdata.m_GirderData[1].bPjackCalculated[pgsTypes::Temporary] = rdata.m_DesignArtifact.GetUsedMaxPjackTempStrands();
   rdata.m_GirderData[1].LastUserPjack[pgsTypes::Harped]       = rdata.m_DesignArtifact.GetPjackHarpedStrands();
   rdata.m_GirderData[1].LastUserPjack[pgsTypes::Straight]     = rdata.m_DesignArtifact.GetPjackStraightStrands();
   rdata.m_GirderData[1].LastUserPjack[pgsTypes::Temporary]    = rdata.m_DesignArtifact.GetPjackTempStrands();

   rdata.m_GirderData[1].TempStrandUsage = rdata.m_DesignArtifact.GetTemporaryStrandUsage();

   // Get debond information from design artifact
   rdata.m_GirderData[1].ClearDebondData();
   rdata.m_GirderData[1].bSymmetricDebond = true;  // design is always symmetric

   DebondInfoCollection dbcoll = rdata.m_DesignArtifact.GetStraightStrandDebondInfo();
   // TRICKY: Mapping from DEBONDINFO to CDebondInfo is tricky because
   //         former designates individual strands and latter stores symmetric strands
   //         in pairs.
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondInfoIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDINFO& rdbrinfo = *dbit;

      CDebondInfo cdbi;
      cdbi.idxStrand1 = rdbrinfo.strandIdx;

      // if the difference between the current and next number of strands is 2, this is a pair
      StrandIndexType currnum = rdbrinfo.strandIdx;
      StrandIndexType nextnum = pStrandGeometry->GetNextNumStrands(span, gdr, pgsTypes::Straight, currnum);
      if (nextnum-currnum == 2)
      {
         dbit++; // increment counter to account for a pair
         cdbi.idxStrand2 = dbit->strandIdx;

         // some asserts to ensure we got things right
         ATLASSERT(cdbi.idxStrand1+1 == cdbi.idxStrand2);
         ATLASSERT(rdbrinfo.LeftDebondLength == dbit->LeftDebondLength);
         ATLASSERT(rdbrinfo.RightDebondLength == dbit->RightDebondLength);
      }
      else
      {
         // not a pair
         cdbi.idxStrand2 = INVALID_INDEX;
      }
      cdbi.Length1    = rdbrinfo.LeftDebondLength;
      cdbi.Length2    = rdbrinfo.RightDebondLength;

      rdata.m_GirderData[1].Debond[pgsTypes::Straight].push_back(cdbi);
   }
   
   // concrete
   rdata.m_GirderData[1].Material.Fci = rdata.m_DesignArtifact.GetReleaseStrength();
   if (!rdata.m_GirderData[1].Material.bUserEci)
   {
      rdata.m_GirderData[1].Material.Eci = lrfdConcreteUtil::ModE( rdata.m_GirderData[1].Material.Fci, 
                                                             rdata.m_GirderData[1].Material.StrengthDensity, 
                                                             false  ); // ignore LRFD range checks 
      rdata.m_GirderData[1].Material.Eci *= (rdata.m_GirderData[1].Material.EcK1*rdata.m_GirderData[1].Material.EcK2);
   }

   rdata.m_GirderData[1].Material.Fc  = rdata.m_DesignArtifact.GetConcreteStrength();
   if (!rdata.m_GirderData[1].Material.bUserEc)
   {
      rdata.m_GirderData[1].Material.Ec = lrfdConcreteUtil::ModE( rdata.m_GirderData[1].Material.Fc, 
                                                            rdata.m_GirderData[1].Material.StrengthDensity, 
                                                            false );// ignore LRFD range checks 
      rdata.m_GirderData[1].Material.Ec *= (rdata.m_GirderData[1].Material.EcK1*rdata.m_GirderData[1].Material.EcK2);
   }

   // deck offset
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderTypes* pGirderTypes = pBridgeDesc->GetSpan(span)->GetGirderTypes();
   rdata.m_SlabOffset[pgsTypes::metStart][0] = pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart);
   rdata.m_SlabOffset[pgsTypes::metEnd][0]   = pGirderTypes->GetSlabOffset(gdr,pgsTypes::metEnd);
   rdata.m_SlabOffset[pgsTypes::metStart][1] = rdata.m_DesignArtifact.GetSlabOffset(pgsTypes::metStart);
   rdata.m_SlabOffset[pgsTypes::metEnd][1]   = rdata.m_DesignArtifact.GetSlabOffset(pgsTypes::metEnd);

   // get old deck offset type... new type was set in constructor
   rdata.m_SlabOffsetType[0] = pBridgeDesc->GetSlabOffsetType();

   // lifting
   if ( design_options.doDesignLifting )
   {
      rdata.m_GirderData[1].HandlingData.LeftLiftPoint  = rdata.m_DesignArtifact.GetLeftLiftingLocation();
      rdata.m_GirderData[1].HandlingData.RightLiftPoint = rdata.m_DesignArtifact.GetRightLiftingLocation();
   }

   // shipping
   if ( design_options.doDesignHauling )
   {
      rdata.m_GirderData[1].HandlingData.LeadingSupportPoint  = rdata.m_DesignArtifact.GetLeadingOverhang();
      rdata.m_GirderData[1].HandlingData.TrailingSupportPoint = rdata.m_DesignArtifact.GetTrailingOverhang();
   }
}

void txnDesignGirder::CacheShearDesignResults(DesignData& rdata)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IShear,pShear);
   // get the current data
   SpanIndexType   span  = rdata.m_DesignArtifact.GetSpan();
   GirderIndexType gdr   = rdata.m_DesignArtifact.GetGirder();

   rdata.m_ShearData[0] = pShear->GetShearData(span, gdr);

   // get the design data
   rdata.m_ShearData[1] = rdata.m_ShearData[0]; // start with the old and tweak
   rdata.m_ShearData[1].ShearZones.clear();

   ZoneIndexType nShearZones = rdata.m_DesignArtifact.GetNumberOfStirrupZonesDesigned();
   if (0 < nShearZones)
   {
      for (ZoneIndexType i =0; i < nShearZones; i++)
      {
         rdata.m_ShearData[1].ShearZones.push_back( rdata.m_DesignArtifact.GetShearZoneData(i) );
      }

      rdata.m_ShearData[1].ConfinementBarSize  = rdata.m_DesignArtifact.GetConfinementBarSize();

      rdata.m_ShearData[1].NumConfinementZones = rdata.m_DesignArtifact.GetLastConfinementZone()+1;
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData dat;
      rdata.m_ShearData[1].ShearZones.push_back(dat);
      rdata.m_ShearData[1].ConfinementBarSize = 0;
      rdata.m_ShearData[1].NumConfinementZones = 0;
   }
}
