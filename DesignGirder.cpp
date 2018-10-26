///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

txnDesignGirder::txnDesignGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,arDesignOptions designOptions,const pgsDesignArtifact& artifact,pgsTypes::SlabOffsetType slabOffsetType)
{
   m_SpanIdx = spanIdx;
   m_GirderIdx = gdrIdx;
   m_DesignOptions = designOptions;
   m_DesignArtifact = artifact;

   m_SlabOffsetType[1] = slabOffsetType;

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
   return new txnDesignGirder(m_SpanIdx, m_GirderIdx,m_DesignOptions,m_DesignArtifact,m_SlabOffsetType[1]);
}

std::_tstring txnDesignGirder::Name() const
{
   std::_tostringstream os;
   os << "Design Span " << LABEL_SPAN(m_SpanIdx) << " Girder " << LABEL_GIRDER(m_GirderIdx);
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
   if (m_DesignOptions.doDesignForFlexure != dtNoDesign)
   {
      CacheFlexureDesignResults();
   }

   if (m_DesignOptions.doDesignForShear)
   {
      CacheShearDesignResults();
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   if (m_DesignOptions.doDesignForFlexure != dtNoDesign)
   {
      GET_IFACE2(pBroker,IGirderData,pGirderData);
      pGirderData->SetGirderData(m_GirderData[i],m_SpanIdx,m_GirderIdx);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      if ( m_SlabOffsetType[i] == pgsTypes::sotBridge )
      {
         pIBridgeDesc->SetSlabOffset(m_SlabOffset[pgsTypes::metStart][i]);
      }
      else
      {
         pIBridgeDesc->SetSlabOffset(m_SpanIdx,m_GirderIdx,m_SlabOffset[pgsTypes::metStart][i],m_SlabOffset[pgsTypes::metEnd][i]);
      }
   }

   if (m_DesignOptions.doDesignForShear)
   {
      GET_IFACE2(pBroker,IShear,pShear);
      pShear->SetShearData(m_ShearData[i], m_SpanIdx, m_GirderIdx);
   }

   pEvents->FirePendingEvents();
}

void txnDesignGirder::CacheFlexureDesignResults()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

   // old (existing) girder data
   m_GirderData[0] = pGirderData->GetGirderData(m_SpanIdx,m_GirderIdx);

   // new girder data (start with the old and tweak it)
   m_GirderData[1] = m_GirderData[0];

   // Convert Harp offset data
   // offsets are absolute measure in the design artifact
   // convert them to the measurement basis that the CGirderData object is using
   m_GirderData[1].HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(m_SpanIdx,m_GirderIdx, 
                                                                                       m_DesignArtifact.GetNumHarpedStrands(), 
                                                                                       m_GirderData[1].HsoEndMeasurement, 
                                                                                       m_DesignArtifact.GetHarpStrandOffsetEnd());

   m_GirderData[1].HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(m_SpanIdx, m_GirderIdx,
                                                                                     m_DesignArtifact.GetNumHarpedStrands(), 
                                                                                     m_GirderData[1].HsoHpMeasurement, 
                                                                                     m_DesignArtifact.GetHarpStrandOffsetHp());



   // see if strand design data fits in grid
   bool fills_grid=false;
   StrandIndexType num_permanent = m_DesignArtifact.GetNumHarpedStrands() + m_DesignArtifact.GetNumStraightStrands();
   if (m_DesignOptions.doStrandFillType == ftGridOrder)
   {
      // we asked design to fill using grid, but this may be a non-standard design - let's check
      StrandIndexType ns, nh;
      if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, m_SpanIdx, m_GirderIdx, &ns, &nh))
      {
         if (ns == m_DesignArtifact.GetNumStraightStrands() && nh ==  m_DesignArtifact.GetNumHarpedStrands() )
         {
            fills_grid = true;
         }
      }
   }

   if ( fills_grid )
   {
      m_GirderData[1].NumPermStrandsType = NPS_TOTAL_NUMBER;

      m_GirderData[1].Nstrands[pgsTypes::Permanent]            = num_permanent;
      m_GirderData[1].Pjack[pgsTypes::Permanent]               = m_DesignArtifact.GetPjackStraightStrands() + m_DesignArtifact.GetPjackHarpedStrands();
      m_GirderData[1].bPjackCalculated[pgsTypes::Permanent]    = m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   }
   else
   {
      m_GirderData[1].NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }

   m_GirderData[1].Nstrands[pgsTypes::Harped]            = m_DesignArtifact.GetNumHarpedStrands();
   m_GirderData[1].Nstrands[pgsTypes::Straight]          = m_DesignArtifact.GetNumStraightStrands();
   m_GirderData[1].Nstrands[pgsTypes::Temporary]         = m_DesignArtifact.GetNumTempStrands();
   m_GirderData[1].Pjack[pgsTypes::Harped]               = m_DesignArtifact.GetPjackHarpedStrands();
   m_GirderData[1].Pjack[pgsTypes::Straight]             = m_DesignArtifact.GetPjackStraightStrands();
   m_GirderData[1].Pjack[pgsTypes::Temporary]            = m_DesignArtifact.GetPjackTempStrands();
   m_GirderData[1].bPjackCalculated[pgsTypes::Harped]    = m_DesignArtifact.GetUsedMaxPjackHarpedStrands();
   m_GirderData[1].bPjackCalculated[pgsTypes::Straight]  = m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   m_GirderData[1].bPjackCalculated[pgsTypes::Temporary] = m_DesignArtifact.GetUsedMaxPjackTempStrands();
   m_GirderData[1].LastUserPjack[pgsTypes::Harped]       = m_DesignArtifact.GetPjackHarpedStrands();
   m_GirderData[1].LastUserPjack[pgsTypes::Straight]     = m_DesignArtifact.GetPjackStraightStrands();
   m_GirderData[1].LastUserPjack[pgsTypes::Temporary]    = m_DesignArtifact.GetPjackTempStrands();

   m_GirderData[1].TempStrandUsage = m_DesignArtifact.GetTemporaryStrandUsage();

   // Get debond information from design artifact
   m_GirderData[1].ClearDebondData();
   m_GirderData[1].bSymmetricDebond = true;  // design is always symmetric

   DebondInfoCollection dbcoll = m_DesignArtifact.GetStraightStrandDebondInfo();
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
      StrandIndexType nextnum = pStrandGeometry->GetNextNumStrands(m_SpanIdx, m_GirderIdx, pgsTypes::Straight, currnum);
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

      m_GirderData[1].Debond[pgsTypes::Straight].push_back(cdbi);
   }
   
   // concrete
   m_GirderData[1].Material.Fci = m_DesignArtifact.GetReleaseStrength();
   if (!m_GirderData[1].Material.bUserEci)
   {
      m_GirderData[1].Material.Eci = lrfdConcreteUtil::ModE( m_GirderData[1].Material.Fci, 
                                                             m_GirderData[1].Material.StrengthDensity, 
                                                             false /* ignore LRFD range checks */ );
      m_GirderData[1].Material.Eci *= (m_GirderData[1].Material.EcK1*m_GirderData[1].Material.EcK2);
   }

   m_GirderData[1].Material.Fc  = m_DesignArtifact.GetConcreteStrength();
   if (!m_GirderData[1].Material.bUserEc)
   {
      m_GirderData[1].Material.Ec = lrfdConcreteUtil::ModE( m_GirderData[1].Material.Fc, 
                                                            m_GirderData[1].Material.StrengthDensity, 
                                                            false /* ignore LRFD range checks */ );
      m_GirderData[1].Material.Ec *= (m_GirderData[1].Material.EcK1*m_GirderData[1].Material.EcK2);
   }

   // deck offset
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderTypes* pGirderTypes = pBridgeDesc->GetSpan(m_SpanIdx)->GetGirderTypes();
   m_SlabOffset[pgsTypes::metStart][0] = pGirderTypes->GetSlabOffset(m_GirderIdx,pgsTypes::metStart);
   m_SlabOffset[pgsTypes::metEnd][0]   = pGirderTypes->GetSlabOffset(m_GirderIdx,pgsTypes::metEnd);
   m_SlabOffset[pgsTypes::metStart][1] = m_DesignArtifact.GetSlabOffset(pgsTypes::metStart);
   m_SlabOffset[pgsTypes::metEnd][1]   = m_DesignArtifact.GetSlabOffset(pgsTypes::metEnd);

   // get old deck offset type... new type was set in constructor
   m_SlabOffsetType[0] = pBridgeDesc->GetSlabOffsetType();

   // lifting
   if ( m_DesignOptions.doDesignLifting )
   {
      m_GirderData[1].HandlingData.LeftLiftPoint  = m_DesignArtifact.GetLeftLiftingLocation();
      m_GirderData[1].HandlingData.RightLiftPoint = m_DesignArtifact.GetRightLiftingLocation();
   }

   // shipping
   if ( m_DesignOptions.doDesignHauling )
   {
      m_GirderData[1].HandlingData.LeadingSupportPoint  = m_DesignArtifact.GetLeadingOverhang();
      m_GirderData[1].HandlingData.TrailingSupportPoint = m_DesignArtifact.GetTrailingOverhang();
   }
}

void txnDesignGirder::CacheShearDesignResults()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IShear,pShear);
   // get the current data
   m_ShearData[0] = pShear->GetShearData(m_SpanIdx,m_GirderIdx);

   // get the design data
   m_ShearData[1] = m_ShearData[0]; // start with the old and tweak
   m_ShearData[1].ShearZones.clear();

   ZoneIndexType nShearZones = m_DesignArtifact.GetNumberOfStirrupZonesDesigned();
   if (0 < nShearZones)
   {
      for (ZoneIndexType i =0; i < nShearZones; i++)
      {
         m_ShearData[1].ShearZones.push_back( m_DesignArtifact.GetShearZoneData(i) );
      }

      m_ShearData[1].ConfinementBarSize = m_DesignArtifact.GetConfinementBarSize();

      m_ShearData[1].NumConfinementZones = m_DesignArtifact.GetLastConfinementZone()+1;
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData dat;
      m_ShearData[1].ShearZones.push_back(dat);
      m_ShearData[1].ConfinementBarSize = 0;
      m_ShearData[1].NumConfinementZones = 0;
   }
}
