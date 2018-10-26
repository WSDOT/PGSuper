///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <DesignConfigUtil.h>
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
   if ( m_DesignDataColl.size() == 1 )
   {
      DesignDataConstIter iter = m_DesignDataColl.begin();
      os << _T("Design for Span ") << LABEL_SPAN(iter->m_DesignArtifact.GetSpan()) << _T(", Girder ") << LABEL_GIRDER(iter->m_DesignArtifact.GetGirder());
   }
   else
   {
      os << _T("Design for (Span, Girder) =");
      for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
      {
         os << _T(" (") << LABEL_SPAN(iter->m_DesignArtifact.GetSpan()) << _T(", ") << LABEL_GIRDER(iter->m_DesignArtifact.GetGirder())<< _T(")");
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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);

   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

      SpanIndexType   span  = rdata.m_DesignArtifact.GetSpan();
      GirderIndexType gdr   = rdata.m_DesignArtifact.GetGirder();

      // old (existing) girder data
      rdata.m_GirderData[0] = *(pGirderData->GetGirderData(span, gdr));

      // new girder data (start with the old and tweak it)
      rdata.m_GirderData[1] = rdata.m_GirderData[0];

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

         if (design_options.doDesignForFlexure==dtNoDesign &&
             rdata.m_DesignArtifact.GetWasLongitudinalRebarForShearDesigned())
         {
            // Need to set girder data in order to pick up long reinf for shear design
            pGirderData->SetGirderData(rdata.m_GirderData[i], span, gdr);
         }
      }
   }

  pEvents->FirePendingEvents();
}

void txnDesignGirder::CacheFlexureDesignResults(DesignData& rdata)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IStrandGeometry, pStrandGeometry );

   SpanIndexType   span  = rdata.m_DesignArtifact.GetSpan();
   GirderIndexType gdr   = rdata.m_DesignArtifact.GetGirder();

   arDesignOptions design_options = rdata.m_DesignArtifact.GetDesignOptions();

   // Convert Harp offset data
   // offsets are absolute measure in the design artifact
   // convert them to the measurement basis that the CGirderData object is using
   ConfigStrandFillVector harpfillvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Harped, rdata.m_DesignArtifact.GetNumHarpedStrands());


   rdata.m_GirderData[1].PrestressData.HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, 
                                                                                       harpfillvec, 
                                                                                       rdata.m_GirderData[1].PrestressData.HsoEndMeasurement, 
                                                                                       rdata.m_DesignArtifact.GetHarpStrandOffsetEnd());

   rdata.m_GirderData[1].PrestressData.HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr,
                                                                                     harpfillvec, 
                                                                                     rdata.m_GirderData[1].PrestressData.HsoHpMeasurement, 
                                                                                     rdata.m_DesignArtifact.GetHarpStrandOffsetHp());

   // NOTE: Strand Designs ALWAYS use continuous fill
   //    See if strand design data fits in grid
   bool fills_grid=false;
   StrandIndexType num_permanent = rdata.m_DesignArtifact.GetNumHarpedStrands() + rdata.m_DesignArtifact.GetNumStraightStrands();
   StrandIndexType ns(0), nh(0);
   if (design_options.doStrandFillType == ftGridOrder)
   {
      // we asked design to fill using grid, but this may be a non-standard design - let's check
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
      ATLASSERT(num_permanent==ns+nh);
      rdata.m_GirderData[1].PrestressData.SetTotalPermanentNstrands(num_permanent, ns, nh);
      rdata.m_GirderData[1].PrestressData.Pjack[pgsTypes::Permanent]               = rdata.m_DesignArtifact.GetPjackStraightStrands() + rdata.m_DesignArtifact.GetPjackHarpedStrands();
      rdata.m_GirderData[1].PrestressData.bPjackCalculated[pgsTypes::Permanent]    = rdata.m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   }
   else
   {
      rdata.m_GirderData[1].PrestressData.SetHarpedStraightNstrands(rdata.m_DesignArtifact.GetNumStraightStrands(), rdata.m_DesignArtifact.GetNumHarpedStrands());
   }

   rdata.m_GirderData[1].PrestressData.SetTemporaryNstrands(rdata.m_DesignArtifact.GetNumTempStrands());

   rdata.m_GirderData[1].PrestressData.Pjack[pgsTypes::Harped]               = rdata.m_DesignArtifact.GetPjackHarpedStrands();
   rdata.m_GirderData[1].PrestressData.Pjack[pgsTypes::Straight]             = rdata.m_DesignArtifact.GetPjackStraightStrands();
   rdata.m_GirderData[1].PrestressData.Pjack[pgsTypes::Temporary]            = rdata.m_DesignArtifact.GetPjackTempStrands();
   rdata.m_GirderData[1].PrestressData.bPjackCalculated[pgsTypes::Harped]    = rdata.m_DesignArtifact.GetUsedMaxPjackHarpedStrands();
   rdata.m_GirderData[1].PrestressData.bPjackCalculated[pgsTypes::Straight]  = rdata.m_DesignArtifact.GetUsedMaxPjackStraightStrands();
   rdata.m_GirderData[1].PrestressData.bPjackCalculated[pgsTypes::Temporary] = rdata.m_DesignArtifact.GetUsedMaxPjackTempStrands();
   rdata.m_GirderData[1].PrestressData.LastUserPjack[pgsTypes::Harped]       = rdata.m_DesignArtifact.GetPjackHarpedStrands();
   rdata.m_GirderData[1].PrestressData.LastUserPjack[pgsTypes::Straight]     = rdata.m_DesignArtifact.GetPjackStraightStrands();
   rdata.m_GirderData[1].PrestressData.LastUserPjack[pgsTypes::Temporary]    = rdata.m_DesignArtifact.GetPjackTempStrands();

   rdata.m_GirderData[1].PrestressData.TempStrandUsage = rdata.m_DesignArtifact.GetTemporaryStrandUsage();

   // Get debond information from design artifact
   rdata.m_GirderData[1].PrestressData.ClearDebondData();
   rdata.m_GirderData[1].PrestressData.bSymmetricDebond = true;  // design is always symmetric

   // TRICKY: Mapping from DEBONDCONFIG to CDebondInfo is tricky because
   //         former designates individual strands and latter stores strands
   //         in grid order.
   // Use utility tool to make the strand indexing conversion
   ConfigStrandFillVector strtfillvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Straight, rdata.m_DesignArtifact.GetNumStraightStrands());
   ConfigStrandFillTool fillTool( strtfillvec );

   DebondConfigCollection dbcoll = rdata.m_DesignArtifact.GetStraightStrandDebondInfo();
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondConfigConstIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDCONFIG& rdbrinfo = *dbit;

      CDebondInfo cdbi;

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

      cdbi.Length1    = rdbrinfo.LeftDebondLength;
      cdbi.Length2    = rdbrinfo.RightDebondLength;

      rdata.m_GirderData[1].PrestressData.Debond[pgsTypes::Straight].push_back(cdbi);
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
      rdata.m_ShearData[1] =  rdata.m_DesignArtifact.GetShearData();
   }
   else
   {
      // if no shear zones were designed, we had a design failure.
      // create a single zone with no stirrups in it.
      CShearZoneData dat;
      rdata.m_ShearData[1].ShearZones.push_back(dat);
   }

   if(rdata.m_DesignArtifact.GetWasLongitudinalRebarForShearDesigned())
   {
      // Rebar data was changed during shear design
      rdata.m_GirderData[1].LongitudinalRebarData  = rdata.m_DesignArtifact.GetLongitudinalRebarData();
   }
}
