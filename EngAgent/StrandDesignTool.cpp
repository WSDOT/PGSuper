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

#include "StdAfx.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Allowables.h>
#include <IFace\GirderHandlingPointOfInterest.h>

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription.h>

#include "PsForceEng.h"
#include "StrandDesignTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStrandDesignTool
****************************************************************************/


////// local functions
//////////////////////
inline static Float64 GetPointY(StrandIndexType idx, IPoint2dCollection* points)
{
   CComPtr<IPoint2d> point;
   points->get_Item(idx,&point);
   Float64 curr_y;
   point->get_Y(&curr_y);
   return curr_y;
}


static void SortDebondLevels(std::vector<Int16>& rDebondLevelsAtSections)
{
   Int16 max_level=0;
   for(std::vector<Int16>::reverse_iterator rit=rDebondLevelsAtSections.rbegin(); rit!=rDebondLevelsAtSections.rend(); rit++)
   {
      Int16& rlvl = *rit;

      if (rlvl<0)
      {
         ATLASSERT(0); // some design functions set negative levels, this should have been handled earlier
         rlvl *= -1;
      }
 
      if (rlvl<max_level)
      {
         rlvl = max_level;
      }
      else
      {
         max_level = rlvl;
      }
   }
}

// starting default to use about 2/3 straight and 1/3 harped, and if not possible; use whatever fits
static const Float64 DefaultHarpedRatio = 1.0/3.0;

pgsStrandDesignTool::pgsStrandDesignTool(SHARED_LOGFILE lf):
LOGFILE(lf),
m_pArtifact(NULL),
m_pBroker(NULL),
m_StatusGroupID(NULL),
m_DoDesignForStrandSlope(false),
m_AllowableStrandSlope(0.0),
m_DoDesignForHoldDownForce(false),
m_AllowableHoldDownForce(0.0),
m_MinimumFinalMzEccentricity(Float64_Max),
m_HarpedRatio(DefaultHarpedRatio),
m_MinPermanentStrands(0),
m_MinSlabOffset(0.0),
m_ConcreteAccuracy(::ConvertToSysUnits(100,unitMeasure::PSI))
{
}

void pgsStrandDesignTool::Initialize(IBroker* pBroker, long statusGroupID, pgsDesignArtifact* pArtif)
{
   ATLASSERT(pBroker);

   // Cache a whole bunch of stuff that does not change during design
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_pArtifact = pArtif;
   m_Span = m_pArtifact->GetSpan();
   m_Girder = m_pArtifact->GetGirder();
   m_DesignOptions = pArtif->GetDesignOptions();

   m_StrandFillType       = m_DesignOptions.doStrandFillType;
   m_HarpedRatio = DefaultHarpedRatio;

   // Every design starts from the same point regardless of the current state of bridge
   // description input
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirderData,pGirderData);
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(m_Span);

   const CGirderMaterial* pGirderMaterial = pGirderData->GetGirderMaterial(m_Span,m_Girder);

   // Initialize concrete type with a minimum strength
   Float64 slab_fc = pDeck->SlabFc;

   // Set concrete strength 
   Float64 ifc = GetMinimumConcreteStrength();
   ifc = max(slab_fc, ifc);

   matConcreteEx conc("Design Concrete", ifc, pGirderMaterial->StrengthDensity, 
                      pGirderMaterial->WeightDensity, lrfdConcreteUtil::ModE(ifc,  pGirderMaterial->StrengthDensity, false ));
   conc.SetMaxAggregateSize(pGirderMaterial->MaxAggregateSize);
   conc.SetType((matConcrete::Type)pGirderMaterial->Type);
   conc.HasAggSplittingStrength(pGirderMaterial->bHasFct);
   conc.SetAggSplittingStrength(pGirderMaterial->Fct);

   m_pArtifact->SetConcrete(conc);

   if (pGirderMaterial->bUserEc)
   {
      m_pArtifact->SetUserEc(pGirderMaterial->Ec);
   }

   if (pGirderMaterial->bUserEci)
   {
      m_pArtifact->SetUserEci(pGirderMaterial->Eci);
   }

   m_FcControl.Init(ifc);

   // Initialize release strength
   Float64 ifci = ::ConvertToSysUnits(4.0,unitMeasure::KSI);
   m_ReleaseStrengthResult = ConcSuccess; // assume that no rebar is needed to make 4 ksi
   m_pArtifact->SetReleaseStrength( ifci );
   m_FciControl.Init(ifci);

   if ( pBridge->GetDeckType() == pgsTypes::sdtNone || !(m_DesignOptions.doDesignSlabOffset) )
   {
      // if there is no deck, set the artifact value to the current value
      m_pArtifact->SetSlabOffset( pgsTypes::metStart, pBridge->GetSlabOffset(m_Span,m_Girder,pgsTypes::metStart) );
      m_pArtifact->SetSlabOffset( pgsTypes::metEnd,   pBridge->GetSlabOffset(m_Span,m_Girder,pgsTypes::metEnd)   );
   }
   else
   {
      Float64 tSlab = pBridge->GetGrossSlabDepth( pgsPointOfInterest(m_Span,m_Girder,0.00) );
      m_pArtifact->SetSlabOffset( pgsTypes::metStart, 1.5*tSlab );
      m_pArtifact->SetSlabOffset( pgsTypes::metEnd,   1.5*tSlab );
   }

   // Initialize Prestressing
   m_pArtifact->SetNumStraightStrands( 0 );
   m_pArtifact->SetPjackStraightStrands( 0 );
   m_pArtifact->SetNumHarpedStrands( 0 );
   m_pArtifact->SetPjackHarpedStrands( 0 );
   m_pArtifact->SetNumTempStrands( 0 );
   m_pArtifact->SetPjackTempStrands( 0 );
   m_pArtifact->SetUsedMaxPjackStraightStrands( false );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( false );
   m_pArtifact->SetUsedMaxPjackTempStrands( false );

   m_MinSlabOffset = 0.0;

   InitDebondData();

   // Get area of an individual prestressing strand
   m_Aps[pgsTypes::Straight]  = pGirderMaterial->pStrandMaterial[pgsTypes::Straight]->GetNominalArea();
   m_Aps[pgsTypes::Harped]    = pGirderMaterial->pStrandMaterial[pgsTypes::Harped]->GetNominalArea();
   m_Aps[pgsTypes::Temporary] = pGirderMaterial->pStrandMaterial[pgsTypes::Temporary]->GetNominalArea();

   m_GirderLength          = pBridge->GetGirderLength(m_Span,m_Girder);
   m_SpanLength            = pBridge->GetSpanLength(m_Span,m_Girder);
   m_StartConnectionLength = pBridge->GetGirderStartConnectionLength(m_Span,m_Girder);
   m_XFerLength[pgsTypes::Straight]  = pPrestressForce->GetXferLength(m_Span,m_Girder,pgsTypes::Straight);
   m_XFerLength[pgsTypes::Harped]    = pPrestressForce->GetXferLength(m_Span,m_Girder,pgsTypes::Harped);
   m_XFerLength[pgsTypes::Temporary] = pPrestressForce->GetXferLength(m_Span,m_Girder,pgsTypes::Temporary);

   // harped offsets, hold-down and max strand slopes
   InitHarpedPhysicalBounds(pGirderMaterial->pStrandMaterial[pgsTypes::Harped]);

   // Compute and Cache mid-zone boundaries
   ComputeMidZoneBoundaries();

   // lay out possible debond levels
   ComputeDebondLevels(pPrestressForce);

   // locate and cache points of interest for design
   ValidatePointsOfInterest();

   // Compute minimum number of strands to start design from
   ComputeMinStrands();
}

void pgsStrandDesignTool::InitReleaseStrength(Float64 fci)
{
   m_FciControl.Init(fci);
   m_pArtifact->SetReleaseStrength(fci);
}

void pgsStrandDesignTool::RestoreDefaults(bool retainProportioning)
{
   // set fill order back to user-requested while attempting to retain current number of strands
   StrandIndexType np = GetNumPermanentStrands();

   if (!retainProportioning && 0 < np)
   {
      if (m_StrandFillType != m_DesignOptions.doStrandFillType)
      {
         m_StrandFillType = m_DesignOptions.doStrandFillType;
         m_HarpedRatio = DefaultHarpedRatio;

         // match current strands as close as possible
         StrandIndexType npnew = GetNextNumPermanentStrands(np-1);
         ATLASSERT(0 < npnew);

         SetNumPermanentStrands(npnew);
      }
      else if (m_StrandFillType == ftMinimizeHarping && m_HarpedRatio != DefaultHarpedRatio)
      {
         m_HarpedRatio = DefaultHarpedRatio;

         // match current strands as close as possible
         StrandIndexType npnew = GetNextNumPermanentStrands(np-1);
         ATLASSERT(0 < npnew);

         SetNumPermanentStrands(npnew);
      }
   }
}

Float64 pgsStrandDesignTool::GetGirderLength() const
{
   return m_GirderLength;
}

arFlexuralDesignType pgsStrandDesignTool::GetFlexuralDesignType() const
{
   return m_DesignOptions.doDesignForFlexure;
}

bool pgsStrandDesignTool::SetNumTempStrands(StrandIndexType num)
{
   m_pArtifact->SetNumTempStrands(num);
   UpdateJackingForces();
   return true;
}

bool pgsStrandDesignTool::SetNumPermanentStrands(StrandIndexType numPerm)
{
   GET_IFACE(IStrandGeometry,pStrandGeom);
   ATLASSERT(m_MinPermanentStrands <= numPerm);

   if (GetMaxPermanentStrands() < numPerm)
   {
      ATLASSERT(0);
      return false;
   }
   else if (numPerm == 0)
   {
      m_pArtifact->SetNumHarpedStrands(0);
      m_pArtifact->SetNumStraightStrands(0);
      UpdateJackingForces();
      LOG("** Set Np=0");
      return true;
   }
   else
   {
      // proportion strands
      StrandIndexType ns, nh;
      if (m_StrandFillType == ftGridOrder)
      {
         StrandIndexType uns, unh;
         if (!pStrandGeom->ComputeNumPermanentStrands(numPerm,m_Span, m_Girder, &uns, &unh))
         {
            ATLASSERT(0); // caller should have figured out if numPerm is valid
            return false;
         }

         ns = uns;
         nh = unh;

         LOG ("Using grid fill order to make Ns="<<ns<<", Nh="<<nh<<" from "<< numPerm);
      }
      else if (m_StrandFillType == ftMinimizeHarping)
      {
         if (ComputeNextNumProportionalStrands(numPerm-1, &ns, &nh) == Uint32_Max)
         {
            ATLASSERT(0);
            return false;
         }

         LOG ("Using "<<m_HarpedRatio<<" harped ratio fill order to make Ns="<<ns<<", Nh="<<nh<<" from "<< numPerm);
      }
      else
      {
         ATLASSERT(0);
         return false;
      }

      StrandIndexType nh_old = m_pArtifact->GetNumHarpedStrands();

      m_pArtifact->SetNumHarpedStrands(nh);
      m_pArtifact->SetNumStraightStrands(ns);

      // if we had no harped strands and now we have some, set offsets to maximize harp
      if (nh_old==0 && nh>0)
      {
         ResetHarpedStrandConfiguration();
      }

      // update jacking forces
      UpdateJackingForces();

      // Make sure we are within offset bounds. Force if necessary
      return KeepHarpedStrandsInBounds();

   } 

   LOG("** Set Np="<<GetNumPermanentStrands()<<", Ns="<<GetNs()<<", Nh="<<GetNh());

   return true;
}

bool pgsStrandDesignTool::SetNumStraightHarped(StrandIndexType ns, StrandIndexType nh)
{
   ATLASSERT(0 <= ns && 0 <= nh);
   LOG ("SetNumStraightHarped:: Ns = "<<ns<<" Nh = "<<nh);
   ATLASSERT(ns+nh>=m_MinPermanentStrands);

   // If this is being called, we are probably changing our strand fill. adjust likewise
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // make sure numbers of strands are valid
   if (0 < ns && ns != pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Straight, ns-1))
   {
      ATLASSERT(0);
      LOG ("Ns is invalid");
      return false;
   }

   if (0 < nh && nh != pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Harped, nh-1))
   {
      ATLASSERT(0);
      LOG ("Nh is invalid");
      return false;
   }

   // change fill order for future strand setting and set our numbers of strands
   if (m_StrandFillType != ftMinimizeHarping)
   {
      LOG ("Changing fill type to ftMinimizeHarping");
      m_StrandFillType = ftMinimizeHarping;
   }

   if (nh == 0)
   {
      m_HarpedRatio = 0.0;
   }
   else if (ns == 0)
   {
      m_HarpedRatio=Float64_Max;
   }
   else
   {
      m_HarpedRatio = (Float64)(nh)/(Float64)(ns+nh);
   }

   LOG ("Setting harped ratio to "<<m_HarpedRatio);

   StrandIndexType nh_old = m_pArtifact->GetNumHarpedStrands(); 

   m_pArtifact->SetNumHarpedStrands(nh);
   m_pArtifact->SetNumStraightStrands(ns);


   // if we had no harped strands and now we have some, set offsets to maximize harp
   if (nh_old == 0 && 0 < nh)
   {
      ResetHarpedStrandConfiguration();
   }

   // update jacking forces
   UpdateJackingForces();

   // Make sure we are within offset bounds. Force if necessary
   LOG("** Set Np="<<GetNumPermanentStrands()<<", Ns="<<GetNs()<<", Nh="<<GetNh());
   return KeepHarpedStrandsInBounds();
}


StrandIndexType pgsStrandDesignTool::GetNumPermanentStrands()
{
   return m_pArtifact->GetNumStraightStrands() + m_pArtifact->GetNumHarpedStrands();
}

StrandIndexType pgsStrandDesignTool::GetNumTotalStrands()
{
   return GetNumPermanentStrands() + GetNt();
}

StrandIndexType pgsStrandDesignTool::GetMaxPermanentStrands()
{
   GET_IFACE(IStrandGeometry,pStrandGeom);
   return pStrandGeom->GetMaxNumPermanentStrands(m_Span, m_Girder);
}

StrandIndexType pgsStrandDesignTool::GetNh()
{
   return m_pArtifact->GetNumHarpedStrands();
}

StrandIndexType pgsStrandDesignTool::GetNs()
{
   return m_pArtifact->GetNumStraightStrands();
}

StrandIndexType pgsStrandDesignTool::GetNt()
{
   return m_pArtifact->GetNumTempStrands();;
}

void pgsStrandDesignTool::SetConcreteAccuracy(Float64 accuracy)
{
   m_ConcreteAccuracy = accuracy;
}

Float64 pgsStrandDesignTool::GetConcreteAccuracy() const
{
   return m_ConcreteAccuracy;
}

GDRCONFIG pgsStrandDesignTool::GetGirderConfiguration()
{
   return m_pArtifact->GetGirderConfiguration();
}

StrandIndexType pgsStrandDesignTool::GetNextNumPermanentStrands(StrandIndexType prevNum)
{
   if (prevNum == Uint32_Max)
   {
      return 0; // trivial case
   }
   else if ( GetMaxPermanentStrands() <= prevNum)
   {
      return Uint32_Max;
   }
   else if (m_StrandFillType==ftGridOrder)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      return pStrandGeom->GetNextNumPermanentStrands(m_Span, m_Girder, prevNum);
   }
   else if (m_StrandFillType==ftMinimizeHarping)
   {
      StrandIndexType ns, nh;
      return ComputeNextNumProportionalStrands(prevNum, &ns, &nh);
   }
   else
   {
      ATLASSERT(0);
      return Uint32_Max;
   }
}

StrandIndexType pgsStrandDesignTool::GetPreviousNumPermanentStrands(StrandIndexType nextNum)
{
   StrandIndexType maxNum = GetMaxPermanentStrands();

   if (nextNum == 0 || nextNum == Uint32_Max)
   {
      ATLASSERT(0);
      return Uint32_Max; // trivial case
   }
   else if ( maxNum < nextNum )
   {
      return maxNum;
   }
   else if (m_StrandFillType==ftGridOrder)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      return pStrandGeom->GetPreviousNumPermanentStrands(m_Span, m_Girder, nextNum);
   }
   else if (m_StrandFillType==ftMinimizeHarping)
   {
      StrandIndexType befNum = nextNum-1;
      StrandIndexType answer;
      do
      {
         StrandIndexType ns, nh;
         answer =  ComputeNextNumProportionalStrands(befNum, &ns, &nh);
         befNum--;
      }
      while (answer==nextNum);

      return answer;
   }
   else
   {
      ATLASSERT(0);
      return Uint32_Max;
   }
}


StrandIndexType pgsStrandDesignTool::ComputeNextNumProportionalStrands(StrandIndexType prevNum, StrandIndexType* pns, StrandIndexType* pnh)
{
   if (prevNum < 0 || GetMaxPermanentStrands() <= prevNum)
   {
      ATLASSERT(prevNum>0);
      *pnh = INVALID_INDEX;
      *pns = INVALID_INDEX;
      return INVALID_INDEX;
   }

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // try to use current strand ratio, and if not possible; use whatever fits
   StrandIndexType nh_max = pStrandGeom->GetMaxStrands(m_Span, m_Girder,pgsTypes::Harped);
   StrandIndexType ns_max = pStrandGeom->GetMaxStrands(m_Span, m_Girder,pgsTypes::Straight);

   if ( nh_max == 0 )
   {
      StrandIndexType nst = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Straight, prevNum);
      if (0 < nst)
      {
         *pnh = 0;
         *pns = nst;
         return nst;
      }
      else
      {
         *pnh = INVALID_INDEX;
         *pns = INVALID_INDEX;
         return INVALID_INDEX;
      }
   }
   else if ( ns_max == 0 )
   {
      StrandIndexType nsh = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Harped, prevNum);
      if (0 < nsh)
      {
         *pnh = nsh;
         *pns = 0;
         return nsh;
      }
      else
      {
         *pnh = INVALID_INDEX;
         *pns = INVALID_INDEX;
         return INVALID_INDEX;
      }
   }
   else
   {
      StrandIndexType ns;
      // use current ratio to compute strands
      if (m_HarpedRatio == Float64_Max)
      {
         ns = 0;
      }
      else
      {
         if (m_HarpedRatio==0.0)
         {
            ns = min(prevNum, ns_max-1);
         }
         else
         {
            double fra = 1.0 - m_HarpedRatio;
            StrandIndexType s = (StrandIndexType)ceil(fra*prevNum);
            ns = min( max(s, 1), ns_max-1);
         }

         ns = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Straight, ns-1 );
         ATLASSERT(0 < ns && ns != INVALID_INDEX); 
      }

      StrandIndexType nh = (prevNum < ns ? INVALID_INDEX : prevNum - ns);
      if ( nh != INVALID_INDEX )
      {
         // need some harped strands
         if ( nh < nh_max )
         {
            *pnh = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Harped, nh);
            *pns = ns;
            ATLASSERT(*pnh+ns > prevNum); // new must always be more
            return *pnh + ns;
         }
         else
         {
            // not enough harped strands to make it, must take from straight
            *pnh = nh_max;
            ATLASSERT(nh_max <= prevNum);
            ns = prevNum - nh_max;
            *pns = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Straight, ns);
            if (0 < *pns)
            {
               ATLASSERT(prevNum < (*pns + *pnh)); // new must always be more
               return *pns + *pnh;
            }
            else
            {
               // not enough strands available
               ATLASSERT(0);
               return INVALID_INDEX;
            }
         }
      }
      else
      {
         // got all we need from straight strands
         *pnh = 0;
         *pns = ns;
         ATLASSERT(prevNum < *pns); // new must always be more
         return *pns;
      }
   }
}



bool pgsStrandDesignTool::IsValidNumPermanentStrands(StrandIndexType num)
{
   ATLASSERT(num != INVALID_INDEX);
   if (num == 0)
   {
      return true;
   }
   else
   {
      return num == GetNextNumPermanentStrands(num-1);
   }
}

void pgsStrandDesignTool::SetMinimumPermanentStrands(StrandIndexType num)
{
   ATLASSERT(0 <= num && num != INVALID_INDEX);
   m_MinPermanentStrands = num;
}

StrandIndexType pgsStrandDesignTool::GetMinimumPermanentStrands() const
{
   return m_MinPermanentStrands;
}


Float64 pgsStrandDesignTool::ComputeEccentricity(const pgsPointOfInterest& poi,pgsTypes::Stage eccStage)
{
   ATLASSERT(poi.GetSpan() == m_Span);
   ATLASSERT(poi.GetGirder() == m_Girder);

   // temp strands only in casting yard
   StrandIndexType nt = eccStage <pgsTypes::BridgeSite1 ? m_pArtifact->GetNumTempStrands() : 0;

   GDRCONFIG guess = GetGirderConfiguration();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 neff;
   bool bIncTemp = (eccStage < pgsTypes::BridgeSite1 ? true : false);
   return pStrandGeom->GetEccentricity(poi, guess, bIncTemp, &neff);
}

Float64 pgsStrandDesignTool::GetTransferLength(pgsTypes::StrandType strandType) const
{
   if ( strandType == pgsTypes::Permanent )
      strandType = pgsTypes::Straight;

   return m_XFerLength[strandType];
}

StrandIndexType pgsStrandDesignTool::ComputePermanentStrandsRequiredForPrestressForce(const pgsPointOfInterest& poi,Float64 Force)
{
   LOG("Compute number of strands required to handle a force of "<<::ConvertFromSysUnits(Force,unitMeasure::Kip) << " kip");

   GDRCONFIG guess = GetGirderConfiguration();

   // Compute the maximum allowable jacking force for the trial number of strands
   Float64 fpjMax, PjMax;

   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = GetNt();

   if ( ns + nh + nt == 0)
   {
      PjMax                            = 0.0;
      fpjMax                           = 0.0;
   }
   else
   {
      GET_IFACE(IPrestressForce,pPrestressForce);

      guess.Pjack[pgsTypes::Straight]  = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Straight,ns);
      guess.Pjack[pgsTypes::Harped]    = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Harped,nh);
      guess.Pjack[pgsTypes::Temporary] = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Temporary,nt);
      
      PjMax                            = guess.Pjack[pgsTypes::Straight] + 
                                         guess.Pjack[pgsTypes::Harped]   + 
                                         guess.Pjack[pgsTypes::Temporary];

      fpjMax                           = PjMax/(m_Aps[pgsTypes::Straight]*ns + m_Aps[pgsTypes::Harped]*nh + m_Aps[pgsTypes::Temporary]*nt);
   }

   LOG("Maximum jacking stress for this strand configuration = " << ::ConvertFromSysUnits(fpjMax,unitMeasure::KSI) << " KSI");

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   LOSSDETAILS losses;
   ATLASSERT(m_Span == poi.GetSpan() && m_Girder == poi.GetGirder());
   psfeng.ComputeLosses(poi,guess,&losses);
   Float64 loss = losses.pLosses->PermanentStrand_Final();

#if defined _DEBUG
   GET_IFACE(ILosses,pILosses);
   Float64 check_loss = pILosses->GetFinal(poi,pgsTypes::Permanent,guess);
   ATLASSERT(IsEqual(loss,check_loss));
#endif // _DEBUG

   LOG("Estimated losses for this strand configuration = " << ::ConvertFromSysUnits(loss,unitMeasure::KSI) << " KSI");

   // Required strand stress
   Float64 fstrand = fpjMax - loss;
   LOG("Required strand stress = " << ::ConvertFromSysUnits(fstrand,unitMeasure::KSI) << " KSI");

   // Estimate number of prestressing strands
   Float64 Aps;
   if (fstrand==0.0)
      Aps = 0.0;
   else
      Aps = -Force/fstrand; // Total required area of prestressing

   LOG("Strand Area = " << ::ConvertFromSysUnits(m_Aps[pgsTypes::Straight],unitMeasure::Inch2) << " in^2 per strand");
   LOG("Required area of prestressing = " << ::ConvertFromSysUnits(Aps,unitMeasure::Inch2) << " in^2");

   ATLASSERT(IsEqual(m_Aps[pgsTypes::Straight],m_Aps[pgsTypes::Harped])); // must be the same for this algorithm to work
   Float64 fN = Aps/m_Aps[pgsTypes::Straight];
   StrandIndexType N = (StrandIndexType)ceil(fN);
   N = max(N,1); // Must be zero or more strands

   LOG("Required number of permanent strands (float) = " << fN);
   LOG("Required number of permanent strands = " << N);

   N = GetNextNumPermanentStrands(N-1);
   LOG("Actual number of permanent strands = " << N);

   ATLASSERT(fN <= N || Uint32_Max == N);

   return N;
}

StrandIndexType pgsStrandDesignTool::GuessInitialStrands()
{
   // Intialize with low number of strands to force tension to control
   StrandIndexType Np = GetNextNumPermanentStrands(m_MinPermanentStrands);

   if (Np < 1)
   {
      Np = 0;
      LOG("No permanent strands defined in section");
   }

   LOG("Make initial guess of permanent strands using a couple (WAGOTA) Np = " << Np);
   
   StrandIndexType ns = SetNumPermanentStrands(Np) ? Np : Uint32_Max;
   if (0 <= ns && ns < Uint32_Max)
   {
      bool st = ResetEndZoneStrandConfig(); 
      ATLASSERT(st);
   }

   return ns;
}

// return artifact updated with current design information
void pgsStrandDesignTool::UpdateJackingForces()
{
   // Compute Jacking Force
   GET_IFACE(IPrestressForce,pPrestressForce);

   Float64 PjS, PjH, PjT;
   PjS  = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Straight, GetNs());
   PjH  = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Harped,   GetNh());
   PjT  = pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Temporary,GetNt());

   // Save the initial design in the design artifact.
   m_pArtifact->SetPjackStraightStrands( PjS );
   m_pArtifact->SetUsedMaxPjackStraightStrands( true );

   m_pArtifact->SetPjackHarpedStrands( PjH );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( true );

   m_pArtifact->SetPjackTempStrands( PjT );
   m_pArtifact->SetUsedMaxPjackTempStrands( true );
}

bool pgsStrandDesignTool::ResetEndZoneStrandConfig()
{
   if (m_DesignOptions.doDesignForFlexure == dtDesignForHarping)
   {
      return ResetHarpedStrandConfiguration();
   }
   else if(m_DesignOptions.doDesignForFlexure == dtDesignForDebonding)
   {
      bool bResult = MaximizeDebonding();
      if (!m_DesignOptions.doDesignLifting )
         bResult = LayoutDebonding(m_MaxPhysicalDebondLevels);

      return bResult;
   }
   else
   {
      // straight design, nothing to do
      return true;
   }
}

bool pgsStrandDesignTool::ResetHarpedStrandConfiguration()
{
   LOG("Raising harped strands to maximum height at girder ends");

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (nh == 0)
   {
      m_pArtifact->SetHarpStrandOffsetEnd(0.0);
      m_pArtifact->SetHarpStrandOffsetHp(0.0);
   }
   else
   {
      // if allowed, put end strands at top and harp point strands at lowest input position
      Float64 end_offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(m_Span,m_Girder);

      Float64 end_lower_bound, end_upper_bound;
      // cant adjust if we're not allowed.
      if (0.0 < end_offset_inc)
      {
         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_Span,m_Girder, nh, &end_lower_bound, &end_upper_bound);
         m_pArtifact->SetHarpStrandOffsetEnd(end_upper_bound);
      }

      Float64 hp_offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(m_Span,m_Girder);

      // cant adjust if we're not allowed.
      Float64 hp_lower_bound, hp_upper_bound;
      if (0.0 < hp_offset_inc)
      {
         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_Span,m_Girder, nh, &hp_lower_bound, &hp_upper_bound);
         m_pArtifact->SetHarpStrandOffsetHp(hp_lower_bound);
      }

      if (m_DoDesignForStrandSlope)
      {
         if ( !this->AdjustForStrandSlope() )
         {
            return false;
         }
      }

      if (m_DoDesignForHoldDownForce)
      {
         if ( !this->AdjustForHoldDownForce() )
         {
            return false;
         }
      }

   }

   return true;
}

void pgsStrandDesignTool::ComputeMinStrands()
{
   // It's possible for users to enter strands at the beginning of the fill sequence that have
   // negative eccentricity. This is typically for hanging stirrups. If this occurs then design will crap out

   m_MinPermanentStrands=1; // resonable starting point

   GET_IFACE(IStrandGeometry,pStrandGeom);

   LOG("Compute m_MinPermanentStrands so next num strands give positive ecc");

   m_MinPermanentStrands = GetNextNumPermanentStrands(0);
   if(m_MinPermanentStrands<0)
      return;

   std::vector<pgsPointOfInterest> mid_pois = GetDesignPoi(pgsTypes::CastingYard,POI_MIDSPAN);
   if (mid_pois.empty())
   {
      ATLASSERT(0); // no-midspan? this shouldn't happen, but take a default and carry on
   }
   else
   {
      GDRCONFIG config = m_pArtifact->GetGirderConfiguration();

      StrandIndexType ns_prev = 1;
      StrandIndexType ns_curr = GetNextNumPermanentStrands(ns_prev);

      // Compute min number of strands in order to get a positive ecc
      int nIter=0;
      while (ns_curr>0)
      {
         StrandIndexType ns, nh;
         if (m_StrandFillType == ftGridOrder)
         {
            if (!pStrandGeom->ComputeNumPermanentStrands(ns_curr,m_Span, m_Girder, &ns, &nh))
            {
               ATLASSERT(0); // caller should have figured out if numPerm is valid
               m_MinPermanentStrands = 0;
               return;
            }
         }
         else if (m_StrandFillType == ftMinimizeHarping)
         {
            if (ComputeNextNumProportionalStrands(ns_curr-1, &ns, &nh) == Uint32_Max)
            {
               ATLASSERT(0);
               m_MinPermanentStrands = 0;
               return;
            }
         }
         else
         {
            ATLASSERT(0);
         }

         config.Nstrands[pgsTypes::Straight] = ns;
         config.Nstrands[pgsTypes::Harped] = nh;

         Float64 neff;
         Float64 ecc = pStrandGeom->GetEccentricity(mid_pois[0],config,false,&neff);
         LOG("Computed ecc = "<<ecc<<" for ns="<<ns<<" nh="<<nh);
         if (0. < ecc)
         {
            if (nIter == 0)
            {
               // Setting strand to a minimal number seems to give optimal results for cases without top strands
               m_MinPermanentStrands = 1;
               LOG("Eccentricity positive on first iteration - m_MinPermanentStrands = " << m_MinPermanentStrands << "Success");
            }
            else
            {
               // TRICKY: Just finding the point where eccentricity is postitive turns out not to be enough.
               //         The design algorithm will likely get stuck. So we double it.
               m_MinPermanentStrands = GetNextNumPermanentStrands(2*ns_prev);
               LOG("Found m_MinPermanentStrands = " << ns_prev << "Success");
            }

            break;
         }
         else
         {
            ns_prev = ns_curr;
            ns_curr = GetNextNumPermanentStrands(ns_prev);

            if (ns_curr < 0)
            {
               LOG("**WARNING: Could not find number of strands to create positive eccentricity. The end is likely near...");
            }
         }

         nIter++;
      }
   }
}


bool pgsStrandDesignTool::AdjustForStrandSlope()
{
   ATLASSERT(m_DoDesignForStrandSlope); // should not be calling this

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   Float64 hp_offset = GetHarpStrandOffsetHp();
   Float64 end_offset = GetHarpStrandOffsetEnd();

   GET_IFACE(IStrandGeometry,pStrandGeom);

   Float64 slope = pStrandGeom->GetMaxStrandSlope(pgsPointOfInterest(m_Span,m_Girder,0.00), nh,
                                               end_offset, hp_offset );

   LOG("Design for Maximum Strand Slope");
   LOG("Maximum Strand Slope 1 : " << m_AllowableStrandSlope);
   LOG("Actual  Strand Slope 1 : " << slope);

   Float64 adj = 0.0;
   if ( slope < m_AllowableStrandSlope)
   {
      LOG("Strand slope needs adjusting");
      LOG("Current End offset = "<< ::ConvertFromSysUnits(end_offset,unitMeasure::Inch) << " in");
      LOG("Current HP  offset = "<< ::ConvertFromSysUnits(hp_offset,unitMeasure::Inch) << " in");

      if (! AdjustStrandsForSlope(m_AllowableStrandSlope, slope, nh, pStrandGeom))
      {
         LOG("** DESIGN FAILED ** We cannot adjust Strands to design for allowable strand slope");
         m_pArtifact->SetOutcome(pgsDesignArtifact::StrandSlopeOutOfRange);
         return false;
      }

      LOG("** Adjusted End offset = "<< ::ConvertFromSysUnits(end_offset = GetHarpStrandOffsetEnd() ,unitMeasure::Inch) << " in");
      LOG("** Adjusted HP  offset = "<< ::ConvertFromSysUnits(hp_offset = GetHarpStrandOffsetHp() ,unitMeasure::Inch) << " in");
      LOG("New slope is 1 : "<< pStrandGeom->GetMaxStrandSlope( pgsPointOfInterest(m_Span,m_Girder,0.00), nh, end_offset, hp_offset) );
   }

   return true;
}

bool pgsStrandDesignTool::AdjustForHoldDownForce()
{
   LOG("Design for Maximum Hold Down Force");

   ATLASSERT(m_DoDesignForHoldDownForce); // should not be calling this

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (nh == 0)
   {
      LOG("No harped strands, no hold down force");
      return true;
   }

   // could use interface to get hdf directly, but need raw values to adjust
   GDRCONFIG config = GetGirderConfiguration();
   Float64 hp_offset = GetHarpStrandOffsetHp();
   Float64 end_offset = GetHarpStrandOffsetEnd();

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(m_Span,m_Girder,pgsTypes::BridgeSite3,POI_HARPINGPOINT);

   // no hold down force if there aren't any harped strands
   if ( vPOI.size() == 0 )
   {
      ATLASSERT(0); // we have harped strands and no harping points?
      return 0;
   }

   pgsPointOfInterest poi = vPOI[0];

   // move the POI just before the harping point so we are sure
   // that we are using a point on the sloped section of the strands
   poi.SetDistFromStart( poi.GetDistFromStart() - 0.01 );

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 slope = pStrandGeom->GetAvgStrandSlope( poi, nh, end_offset, hp_offset);
   LOG("Average Strand Slope = 1 : " << slope);

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 strand_force = pPrestressForce->GetPrestressForce(poi,config,pgsTypes::Harped,pgsTypes::Jacking);
   LOG("PS Force in harped strands " << ::ConvertFromSysUnits(strand_force,unitMeasure::Kip) << " kip");

   // finally, the hold down force
   Float64 hft = strand_force / sqrt( 1*1 + slope*slope );

   LOG("Maximum HD " << ::ConvertFromSysUnits(m_AllowableHoldDownForce,unitMeasure::Kip) << " kip");
   LOG("Actual  HD " << ::ConvertFromSysUnits(hft,unitMeasure::Kip) << " kip");

   Float64 adj = 0.0;
   if ( m_AllowableHoldDownForce < hft )
   {
      LOG("Hold down force exceeds max, strands need adjustment");

      // slope required for allowable hold down
      Float64 sl_reqd = sqrt( (strand_force*strand_force)/(m_AllowableHoldDownForce*m_AllowableHoldDownForce)-1.0 );
      ATLASSERT(sl_reqd > slope);

      LOG("Slope required = 1 : "<< sl_reqd);
      LOG("Current End offset = "<< ::ConvertFromSysUnits(end_offset,unitMeasure::Inch) << " in");
      LOG("Current HP  offset = "<< ::ConvertFromSysUnits(hp_offset,unitMeasure::Inch) << " in");

      if (! AdjustStrandsForSlope(sl_reqd, slope, nh, pStrandGeom))
      {
         LOG("** DESIGN FAILED ** We cannot adjust Strands to design for allowable hold down");
         m_pArtifact->SetOutcome(pgsDesignArtifact::ExceededMaxHoldDownForce);
         return false;
      }

      LOG("** Adjusted End offset = "<< ::ConvertFromSysUnits(end_offset = GetHarpStrandOffsetEnd() ,unitMeasure::Inch) << " in");
      LOG("** Adjusted HP  offset = "<< ::ConvertFromSysUnits(hp_offset = GetHarpStrandOffsetHp() ,unitMeasure::Inch) << " in");
      LOG("New slope is 1 : "<< pStrandGeom->GetAvgStrandSlope( poi, nh, end_offset, hp_offset) );
   }

   return true;
}

bool pgsStrandDesignTool::AdjustStrandsForSlope(Float64 sl_reqd, Float64 slope, StrandIndexType nh, IStrandGeometry* pStrandGeom)
{
   // compute adjustment distance
   Float64 lhp, rhp;
   pStrandGeom->GetHarpingPointLocations(m_Span,m_Girder, &lhp, &rhp);
   Float64 adj = lhp * (1/slope - 1/sl_reqd);

   LOG("Vertical adjustment required to acheive slope = "<< ::ConvertFromSysUnits(adj,unitMeasure::Inch) << " in");

   // try to adjust end first
   Float64 end_offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(m_Span,m_Girder);
   if (0.0 < end_offset_inc)
   {
      LOG("Attempt to adjust hold down by lowering at ends");
      Float64 curr_adj = m_pArtifact->GetHarpStrandOffsetEnd();

      Float64 end_lower_bound, end_upper_bound;
      pStrandGeom->GetHarpedEndOffsetBoundsEx(m_Span, m_Girder, nh, &end_lower_bound, &end_upper_bound);

      // adjust by round increment
      Float64 end_adj = ::CeilOff(adj, end_offset_inc);
      LOG("End adjustment rounded to increment = "<< ::ConvertFromSysUnits(end_adj,unitMeasure::Inch) << " in");

      Float64 max_adj = curr_adj - end_lower_bound;
      if (0 < max_adj)
      {
         if (end_adj <= max_adj)
         {
            LOG("Entire adjustment for slope can be taken at girder end - doing so");
            m_pArtifact->SetHarpStrandOffsetEnd(curr_adj - end_adj);
            adj = 0.0;
         }
         else
         {
            LOG("Partial adjustment for slope can be taken at girder end. Adjusting to "<< ::ConvertFromSysUnits(end_lower_bound,unitMeasure::Inch) << " in");
            m_pArtifact->SetHarpStrandOffsetEnd(end_lower_bound);
            adj -= (curr_adj-end_lower_bound);
            LOG("reminder of adjustment required = "<< ::ConvertFromSysUnits(adj,unitMeasure::Inch) << " in");
         }
      }
      else
      {
         LOG("Strands at end already ajusted as low as possible");
      }
   }

   // we've done what we can at the end. see if we need to adjust at hp
   Float64 hp_offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(m_Span,m_Girder);
   if (0.0 < adj && 0.0 < hp_offset_inc)
   {
      LOG("Attempt to adjust Strand slope by rasing at HP's");
      Float64 curr_adj = m_pArtifact->GetHarpStrandOffsetHp();
      Float64 hp_lower_bound, hp_upper_bound;
      pStrandGeom->GetHarpedHpOffsetBoundsEx(m_Span,m_Girder, nh, &hp_lower_bound, &hp_upper_bound);

      Float64 max_adj = hp_upper_bound - curr_adj;
      if (0 < max_adj)
      {
         if (adj <= max_adj)
         {
            LOG("Entire adjustment for slope can be taken at girder HP - doing so");
            m_pArtifact->SetHarpStrandOffsetHp(curr_adj + adj);
            adj = 0.0;
         }
      }
      else
      {
         LOG("Strands at HP already ajusted as high as possible - cannot acheive target slope reduction");
      }
   }

   // see if we made our adjustment
   return (adj==0.0);
}

bool pgsStrandDesignTool::SwapStraightForHarped()
{
   LOG("Attempting to change strand proportions by moving straight strands into harped pattern");
   StrandIndexType Ns = GetNs();
   StrandIndexType Nh = GetNh();
   if ( Ns < 2 )
      return false;

   return SetNumStraightHarped(Ns-2,Nh+2);
}

bool pgsStrandDesignTool::AddStrands()
{
   LOG("Attempting to add permanent strands");

   StrandIndexType Ns = GetNs();
   StrandIndexType Nh = GetNh();
   StrandIndexType Np = Ns + Nh;


   LOG("Current configuration -> Ns = " << Ns << " Nh = " << Nh <<" Nperm = " <<Np);

   StrandIndexType nextNp = this->GetNextNumPermanentStrands(Np);
   if ( 0 < nextNp)
   {
      LOG("Adding " << (nextNp - Np) << " permanent strands");
      if (!this->SetNumPermanentStrands(nextNp))
      {
         return false;
      }

   }
   else
   {
      LOG("Number of strands exceed maximum for this girder");
      return false;
   }

   Ns = GetNs();
   Nh = GetNh();

   LOG("** Successfully added strands -> Ns = " << Ns << " Nh = " << Nh);

   return KeepHarpedStrandsInBounds();
}

bool pgsStrandDesignTool::AddTempStrands()
{
   LOG("Attempting to add temporary strands");

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType Nt = m_pArtifact->GetNumTempStrands();

   LOG("Current configuration -> Nt = " << Nt);
   StrandIndexType nextNt = pStrandGeom->GetNextNumStrands(m_Span,m_Girder,pgsTypes::Temporary,Nt);
   if ( nextNt == 0 || nextNt == Uint32_Max )
   {
      LOG("Number of temp strand exceeds maximum for this girder");
      return false;
   }
   else
   {
      GET_IFACE(IPrestressForce,pPrestressForce);

      LOG("Adding " << (nextNt - Nt) << " temporary strands");
      LOG("** Successfully added strands -> Nt = " << nextNt);
      m_pArtifact->SetNumTempStrands( nextNt );
      m_pArtifact->SetPjackTempStrands( pPrestressForce->GetPjackMax(m_Span,m_Girder,pgsTypes::Temporary,nextNt) );
      m_pArtifact->SetUsedMaxPjackTempStrands( true );

      return true;
   }
}

// predicate class to sort debond info by location
class DebondInfoSorter
{
public:
   int operator() (const DEBONDINFO& db1,const DEBONDINFO& db2)
   {
      if (db1.LeftDebondLength == db2.LeftDebondLength)
      {
         return db1.strandIdx < db2.strandIdx;
      }
      else
      {
         return db1.LeftDebondLength < db2.LeftDebondLength;
      }
   }
};

void pgsStrandDesignTool::DumpDesignParameters()
{
#ifdef _DEBUG

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType Nh = m_pArtifact->GetNumHarpedStrands();

   Float64 end_offset = pStrandGeom->ComputeHarpedOffsetFromAbsoluteEnd(m_Span,m_Girder,Nh, hsoTOP2TOP, m_pArtifact->GetHarpStrandOffsetEnd());
   Float64 hp_offset  = pStrandGeom->ComputeHarpedOffsetFromAbsoluteHp(m_Span,m_Girder,Nh,  hsoBOTTOM2BOTTOM, m_pArtifact->GetHarpStrandOffsetHp());

   LOG("");
   LOG("---------------------------------------------------------------");
   LOG("Current design parameters");
   LOG("");
   LOG("f'c  = " << ::ConvertFromSysUnits(m_pArtifact->GetConcreteStrength(),unitMeasure::KSI) << " KSI");
   LOG("f'ci = " << ::ConvertFromSysUnits(m_pArtifact->GetReleaseStrength(),unitMeasure::KSI) << (m_ReleaseStrengthResult==ConcSuccessWithRebar?" KSI - Min Rebar Required ":" KSI") );
   LOG("Np = " << m_pArtifact->GetNumStraightStrands()+ m_pArtifact->GetNumHarpedStrands());
   LOG("Ns = " << m_pArtifact->GetNumStraightStrands() << "   Pjack = " << ::ConvertFromSysUnits(m_pArtifact->GetPjackStraightStrands(), unitMeasure::Kip) << " Kip");
   LOG("Nh = " << m_pArtifact->GetNumHarpedStrands() << "   Pjack = " << ::ConvertFromSysUnits(m_pArtifact->GetPjackHarpedStrands(), unitMeasure::Kip) << " Kip");
   LOG("Nt = " << m_pArtifact->GetNumTempStrands() << "   Pjack = " << ::ConvertFromSysUnits(m_pArtifact->GetPjackTempStrands(), unitMeasure::Kip) << " Kip");
   LOG("HP Offset at End = " << ::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetEnd(),unitMeasure::Inch) << " in" << " (From top = " << ::ConvertFromSysUnits(end_offset,unitMeasure::Inch) << " in)");
   LOG("HP Offset at HP  = " << ::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetHp(),unitMeasure::Inch) << " in" << " (From bottom = " << ::ConvertFromSysUnits(hp_offset,unitMeasure::Inch) << " in)");
   LOG("Slab Offset at Start = " << ::ConvertFromSysUnits(m_pArtifact->GetSlabOffset(pgsTypes::metStart),unitMeasure::Inch) << " in");
   LOG("Slab Offset at End   = " << ::ConvertFromSysUnits(m_pArtifact->GetSlabOffset(pgsTypes::metEnd),unitMeasure::Inch) << " in");
   LOG("Pick Point = " << ::ConvertFromSysUnits(m_pArtifact->GetLeftLiftingLocation(), unitMeasure::Feet) << " ft");
   LOG("Leading Overhang  = " << ::ConvertFromSysUnits(m_pArtifact->GetLeadingOverhang(), unitMeasure::Feet) << " ft");
   LOG("Trailing Overhang = " << ::ConvertFromSysUnits(m_pArtifact->GetTrailingOverhang(), unitMeasure::Feet) << " ft");

   DebondInfoCollection dbinfo = m_pArtifact->GetStraightStrandDebondInfo();
   if (!dbinfo.empty())
   {
      LOG("Debonding Information:");
      // sort debonding by section location. no need to output right and left ends because we are always symmetric
      std::sort(dbinfo.begin(), dbinfo.end(), DebondInfoSorter());

      bool loop = true;
      DebondInfoIterator it = dbinfo.begin();
      while(loop)
      {
         Float64 curr_loc = it->LeftDebondLength;
         Float64 last_loc = curr_loc;
         std::stringstream os;
         os<<"    Strands Debonded at "<< ::ConvertFromSysUnits(curr_loc, unitMeasure::Feet) << " ft: ";
         while(curr_loc == last_loc)
         {
            os<<it->strandIdx<<", ";

            it++;
            if (it==dbinfo.end())
            {
               loop = false;
               break;
            }
            else
            {
               last_loc = curr_loc;
               curr_loc = it->LeftDebondLength;
            }
         }

         std::string str(os.str());
         Int16 n = str.size();
         if (n>0)
            str.erase(n-2,2); // get rid of trailing ", "

         LOG( str );
      }
   }
   LOG("---------------------------------------------------------------");
   LOG("");
#endif
}

void pgsStrandDesignTool::FillArtifactWithFlexureValues()
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IBridgeMaterial,pMaterial);

   m_pArtifact->SetNumStraightStrands(pStrandGeom->GetNumStrands(m_Span,m_Girder,pgsTypes::Straight));
   m_pArtifact->SetNumHarpedStrands(pStrandGeom->GetNumStrands(m_Span,m_Girder,pgsTypes::Harped));

   m_pArtifact->SetPjackHarpedStrands(pStrandGeom->GetPjack(m_Span,m_Girder,pgsTypes::Harped));
   m_pArtifact->SetPjackStraightStrands(pStrandGeom->GetPjack(m_Span,m_Girder,pgsTypes::Straight));
   m_pArtifact->SetUsedMaxPjackStraightStrands( true );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( true );

   Float64 offsetEnd, offsetHP;
   pStrandGeom->GetHarpStrandOffsets(m_Span,m_Girder,&offsetEnd,&offsetHP);
   m_pArtifact->SetHarpStrandOffsetHp(offsetHP);
   m_pArtifact->SetHarpStrandOffsetEnd(offsetEnd);

   m_pArtifact->SetReleaseStrength(pMaterial->GetFciGdr(m_Span,m_Girder));

   m_pArtifact->SetSlabOffset(pgsTypes::metStart,pBridge->GetSlabOffset(m_Span,m_Girder,pgsTypes::metStart));
   m_pArtifact->SetSlabOffset(pgsTypes::metEnd,  pBridge->GetSlabOffset(m_Span,m_Girder,pgsTypes::metEnd)  );

   GDRCONFIG config = pBridge->GetGirderConfiguration(m_Span,m_Girder);
   m_pArtifact->SetStraightStrandDebondInfo( config.Debond[pgsTypes::Straight] );

}

bool pgsStrandDesignTool::UpdateConcreteStrength(Float64 fcRequired,pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation)
{
   Float64 fc_current = m_pArtifact->GetConcreteStrength();
   LOG("Update Final Concrete Strength if needed. f'c required = "<< ::ConvertFromSysUnits(fcRequired,unitMeasure::KSI) << " KSI f'c current = "<< ::ConvertFromSysUnits(fc_current,unitMeasure::KSI) << " KSI");;

   // round up to nearest 100psi
   fcRequired = CeilOff(fcRequired, m_ConcreteAccuracy );
   LOG("Round up to nearest 100psi. New Required value is now = "<< ::ConvertFromSysUnits(fcRequired,unitMeasure::KSI) << " KSI");;

   Float64 fc_max = GetMaximumConcreteStrength();
   if (fcRequired>fc_max)
   {
      ATLASSERT(0); // should be checked by caller
      LOG("FAILED - f'c cannot exceed "<< ::ConvertFromSysUnits(fc_max,unitMeasure::KSI) << " KSI");
      return false;
   }

   Float64 fc_min = GetMinimumConcreteStrength();
   if (fcRequired<fc_min)
   {
      LOG("f'c required less than minimum.  No need to update f'c");
      return false;
   }

   Float64 fci = m_pArtifact->GetReleaseStrength();
   if ( fcRequired < fci )
   {
      LOG("f'c required less than f'ci. Release controls - no need to update f'c");
      return false;
   }

   Float64 newfc;
   if ( m_FcControl.DoUpdate(fcRequired, stage, stressType,limitState,stressLocation,&newfc) )
   {
      m_pArtifact->SetConcreteStrength(newfc);
      LOG("** Updated Final Concrete Strength to "<< ::ConvertFromSysUnits(newfc,unitMeasure::KSI) << " KSI");
   }
   else
   {
      LOG("A higher concrete strength is required for a different design element. Don't update f'c");
      return false; // nothing changed
   }

   return true;
}


bool pgsStrandDesignTool::UpdateReleaseStrength(Float64 fciRequired,ConcStrengthResultType strengthResult,pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation)
{
   LOG("Update Concrete Strength if needed. f'ci required = "<< ::ConvertFromSysUnits(fciRequired,unitMeasure::KSI) << " KSI");;

   Float64 fci_current = m_pArtifact->GetConcreteStrength();

   Float64 fci_min = GetMinimumReleaseStrength();
   if( fciRequired < fci_min )
   {
      LOG("f'c min = " << ::ConvertFromSysUnits(fci_min,unitMeasure::KSI) << " KSI");
      LOG("f'ci cannot be less than min");

      fciRequired = fci_min;
      LOG("f'ci required now = " << ::ConvertFromSysUnits(fciRequired,unitMeasure::KSI) << " KSI");
   }


   fciRequired = CeilOff(fciRequired, m_ConcreteAccuracy );
   LOG("Round up to nearest 100psi. Required value is now = "<< ::ConvertFromSysUnits(fciRequired,unitMeasure::KSI) << " KSI");;

   LOG("Required fully adjusted f'ci now = "<< ::ConvertFromSysUnits(fciRequired,unitMeasure::KSI) << " KSI, Current = "<< ::ConvertFromSysUnits(m_pArtifact->GetReleaseStrength(), unitMeasure::KSI) << " KSI");

   Float64 fci;
   if ( m_FciControl.DoUpdate(fciRequired,stage,stressType,limitState,stressLocation,&fci) )
   {
      LOG("** Setting new release strength to  = "<< ::ConvertFromSysUnits(fci, unitMeasure::KSI) << " KSI");;
      m_pArtifact->SetReleaseStrength(fci);

      ATLASSERT(strengthResult!=ConcFailed); // this should always be blocked

      // new compression controlled values cannot override the need for minimum rebar
      if (stressType==pgsTypes::Tension)
         m_ReleaseStrengthResult = strengthResult;
   }
   else
   {
      // allow tension to override the need to use min rebar even if concrete strenth doesn't need change
      if (stressType==pgsTypes::Tension && strengthResult==ConcSuccessWithRebar)
      {
         if (m_ReleaseStrengthResult!=ConcSuccessWithRebar)
         {
            // Note: This logic here might require more treatment if designs are coming up requiring
            //       min rebar when is is not desired. If this is the case, some serious thought must
            //       be put into m_FciControl.
            LOG("Tensile demand requires minimum rebar - allow it, even though current strength does not require");
            m_ReleaseStrengthResult = strengthResult;
         }
      }

      LOG("A higher release strength is required for a different design element. Don't update f'ci");
      return false; // nothing changed
   }

   // Release strength can drive final.
   if (UpdateConcreteStrength(fci,stage,limitState,stressType,stressLocation))
   {
      LOG("** Concrete strength changed by change in release strength");
   }

   return true;
}

ConcStrengthResultType pgsStrandDesignTool::ComputeRequiredConcreteStrength(double fControl,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType stressType,double* pfc)
{
   LOG("Entering ComputeRequiredConcreteStrength");
   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   Float64 fc_reqd;

   ConcStrengthResultType result = ConcSuccess;

   if ( stressType == pgsTypes::Compression )
   {
      Float64 c = -pAllowStress->GetAllowableCompressiveStressCoefficient(stage,ls);
      fc_reqd = fControl/c;
      LOG(c << "F demand (compression) = " << ::ConvertFromSysUnits(fControl,unitMeasure::KSI) << " KSI" << " --> f'c (req'd unrounded) = " << ::ConvertFromSysUnits(fc_reqd,unitMeasure::KSI) << " KSI");
   }
   else
   {
      fc_reqd = -1;
      if ( 0 < fControl )
      {
         Float64 t, fmax;
         bool bfMax;

         pAllowStress->GetAllowableTensionStressCoefficient(stage,ls,&t,&bfMax,&fmax);
         if (0 < t)
         {
            LOG("f allow coeff = " << ::ConvertFromSysUnits(t,unitMeasure::SqrtKSI) << "_/f'c = " << ::ConvertFromSysUnits(fControl,unitMeasure::KSI));
            fc_reqd = pow(fControl/t,2);

            if ( bfMax && fmax < fControl) 
            {
               // allowable stress is limited to value lower than needed
               if ( stage == pgsTypes::CastingYard )
               {
                  // try getting the alternative allowable if rebar is used
                  double talt = pAllowStress->GetCastingYardAllowableTensionStressCoefficientWithRebar();
                  fc_reqd = pow(fControl/talt,2);
                  result = ConcSuccessWithRebar;
                  LOG("Min rebar is required to acheive required strength");
               }
               else
               {
                  LOG("Required strength is greater than user-input max of "<<::ConvertFromSysUnits(fmax,unitMeasure::KSI)<<" cannot achieve strength");
                  fc_reqd = -1;
                  return ConcFailed;
               }
            }
         }
         else
         {
            // stress coeff is zero, no tensile capacity
            fc_reqd = -1;
            LOG("WARNING: Have applied tension with zero tension allowed - Should not happen");
            ATLASSERT(0);
            return ConcFailed;
         }

         LOG("F demand (tension) = " << ::ConvertFromSysUnits(fControl,unitMeasure::KSI) << " KSI" << " --> f'c (req'd unrounded) = " << ::ConvertFromSysUnits(fc_reqd,unitMeasure::KSI) << (result==ConcSuccessWithRebar ?" KSI, min rebar required":" KSI"));

      }
      else
      {
         // this is a tension case, but the controlling stress is compressive
         // the lowest required concrete strength is 0.
         fc_reqd = 0;
      }
   }

   Float64 fc_min = (stage==pgsTypes::CastingYard) ? GetMinimumReleaseStrength() : GetMinimumConcreteStrength();

   Float64 fc_max = GetMaximumConcreteStrength();
   if ( fc_reqd < fc_min )
   {
      fc_reqd = fc_min;
      LOG("Required strength less than minumum... setting f'c = " << ::ConvertFromSysUnits(fc_reqd,unitMeasure::KSI) << " KSI");
   }
   else if (fc_reqd > fc_max )
   {
      // try setting to max if current strength is not already there
      if (GetConcreteStrength()<fc_max)
      {
         fc_reqd = fc_max;
         LOG("Required strength exceeds that allowed by 5.4.2.1 - try setting f'c to max for one interation = " << ::ConvertFromSysUnits(fc_reqd,unitMeasure::KSI) << " KSI");
      }
      else
      {
         // we've tried the max and it didn't work, time to punt
         LOG("*** Required strength exceeds that allowed by 5.4.2.1 ***- and we've tried max - Punt");
         return ConcFailed;
      }
   }

   *pfc = fc_reqd;

   LOG("Exiting ComputeRequiredConcreteStrength");
   return result;
}


bool  pgsStrandDesignTool::Bump500(pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation)
{
   LOG("Bump 500psi");
   Float64 five_ksi = ::ConvertToSysUnits(0.5,unitMeasure::KSI);
   Float64 fc = GetConcreteStrength();
   Float64 fci = GetReleaseStrength();
   LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI" );
   LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );
   fc  += five_ksi;
   LOG("target f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI" );

   Float64 fc_max = GetMaximumConcreteStrength();
   if (fc>fc_max)
   {
      LOG("Final Strength Exceeds Maximum of "<<::ConvertFromSysUnits(fc_max,unitMeasure::KSI) << " KSI - Bump 500 failed" );
      return false;
   }

   if (!UpdateConcreteStrength(fc,stage,limitState,stressType,stressLocation))
   {
      LOG("Failed increasing concrete strength");
      return false;
   }

   if (stage==pgsTypes::CastingYard ||stage==pgsTypes::Lifting)
   {
      fci += five_ksi;
      LOG("target f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

      Float64 fci_max = GetMaximumReleaseStrength();
      if (fci>fci_max)
      {
         LOG("Release Strength Exceeds Maximum of "<<::ConvertFromSysUnits(fci_max,unitMeasure::KSI) << " KSI - Bump 500 failed" );
         return false;
      }
      else if (!UpdateReleaseStrength(fci,m_ReleaseStrengthResult,stage,limitState,stressType,stressLocation))
      {
         LOG("Failed increasing concrete release strength");
         return false;
      }
   }

   LOG("** Bump 500psi Succeeded");
   return true;
}

void pgsStrandDesignTool::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
   ATLASSERT(m_MinSlabOffset <= offset);

   LOG("** Set slab offset to " <<::ConvertFromSysUnits(offset,unitMeasure::Inch) << (end == pgsTypes::metStart ? " at Start of Girder" : " at End of Girder"));
   m_pArtifact->SetSlabOffset(end,offset);
}

Float64 pgsStrandDesignTool::GetSlabOffset(pgsTypes::MemberEndType end) const
{
   return m_pArtifact->GetSlabOffset(end);
}

void pgsStrandDesignTool::SetMinimumSlabOffset(Float64 offset)
{
   ATLASSERT(0.0 <= offset);
   m_MinSlabOffset = offset;
}

Float64 pgsStrandDesignTool::GetMinimumSlabOffset() const
{
   return m_MinSlabOffset;
}


void pgsStrandDesignTool::SetLiftingLocations(Float64 left,Float64 right)
{
   LOG("** Lifting locations set to left = "<<::ConvertFromSysUnits(left,unitMeasure::Feet)<<", right = "<<::ConvertFromSysUnits(right,unitMeasure::Feet)<< " ft" );
   m_pArtifact->SetLiftingLocations(left, right);
}

Float64 pgsStrandDesignTool::GetLeftLiftingLocation() const
{
   return m_pArtifact->GetLeftLiftingLocation();
}

Float64 pgsStrandDesignTool::GetRightLiftingLocation() const
{
   return m_pArtifact->GetRightLiftingLocation();
}

void pgsStrandDesignTool::SetTruckSupportLocations(Float64 left,Float64 right)
{
   LOG("** Hauling locations set to left = "<<::ConvertFromSysUnits(left,unitMeasure::Feet)<<", right = "<<::ConvertFromSysUnits(right,unitMeasure::Feet)<< " ft" );
   m_pArtifact->SetTruckSupportLocations(left, right);
}

Float64 pgsStrandDesignTool::GetLeadingOverhang() const
{
   return m_pArtifact->GetLeadingOverhang();
}

Float64 pgsStrandDesignTool::GetTrailingOverhang() const
{
   return m_pArtifact->GetTrailingOverhang();
}


Float64 pgsStrandDesignTool::GetConcreteStrength() const
{
   return m_pArtifact->GetConcreteStrength();
}

Float64 pgsStrandDesignTool::GetReleaseStrength() const
{
   return m_pArtifact->GetReleaseStrength();
}

Float64 pgsStrandDesignTool::GetReleaseStrength(ConcStrengthResultType* pStrengthResult) const
{
   *pStrengthResult = m_ReleaseStrengthResult;
   return m_pArtifact->GetReleaseStrength();
}


Float64 pgsStrandDesignTool::GetPjackStraightStrands() const
{
   return m_pArtifact->GetPjackStraightStrands();
}

Float64 pgsStrandDesignTool::GetPjackTempStrands() const
{
   return m_pArtifact->GetPjackTempStrands();
}

Float64 pgsStrandDesignTool::GetPjackHarpedStrands() const
{
   return m_pArtifact->GetPjackHarpedStrands();
}

void pgsStrandDesignTool::SetOutcome(pgsDesignArtifact::Outcome outcome)
{
   m_pArtifact->SetOutcome(outcome);
}

Float64 pgsStrandDesignTool::GetHarpStrandOffsetEnd() const
{
   return m_pArtifact->GetHarpStrandOffsetEnd();
}

Float64 pgsStrandDesignTool::GetHarpStrandOffsetHp() const
{
   return m_pArtifact->GetHarpStrandOffsetHp();
}

void pgsStrandDesignTool::SetHarpStrandOffsetEnd(Float64 off)
{
   // set offset, but make sure it stays within bounds
   LOG("Attempt to offset harped strands at ends to   = " << ::ConvertFromSysUnits(off, unitMeasure::Inch) << " in");
   m_pArtifact->SetHarpStrandOffsetEnd(off);

   bool st = KeepHarpedStrandsInBounds();
   ATLASSERT(st);

   LOG("** Strands Actually offset to  = " << ::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetEnd(), unitMeasure::Inch) << " in");
}

void pgsStrandDesignTool::SetHarpStrandOffsetHp(Float64 off)
{
   LOG("Attempt to offset harped strands at HPs to   = " << ::ConvertFromSysUnits(off, unitMeasure::Inch) << " in");
   m_pArtifact->SetHarpStrandOffsetHp(off);

   bool st = KeepHarpedStrandsInBounds();
   ATLASSERT(st);

   LOG("** Strands Actually offset to  = " << ::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetHp(), unitMeasure::Inch) << " in");
}

bool pgsStrandDesignTool::KeepHarpedStrandsInBounds()
{
   LOG("Make sure harped strand patterns stay in bounds");

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (0 < nh)
   {
      Float64 end_offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(m_Span,m_Girder);

      Float64 end_lower_bound, end_upper_bound;
      // cant adjust if we're not allowed.
      if (0.0 <= end_offset_inc)
      {
         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_Span,m_Girder, nh, &end_lower_bound, &end_upper_bound);

         Float64 end_offset = m_pArtifact->GetHarpStrandOffsetEnd();
         if (end_offset>end_upper_bound)
         {
            m_pArtifact->SetHarpStrandOffsetEnd(end_upper_bound);
         }
         else if (end_offset<end_lower_bound)
         {
            m_pArtifact->SetHarpStrandOffsetEnd(end_lower_bound);
         }
      }

      Float64 hp_offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(m_Span,m_Girder);

      // cant adjust if we're not allowed.
      Float64 hp_lower_bound, hp_upper_bound;
      if (0.0 < hp_offset_inc)
      {
         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_Span,m_Girder, nh, &hp_lower_bound, &hp_upper_bound);

         Float64 hp_offset = m_pArtifact->GetHarpStrandOffsetHp();
         if (hp_upper_bound < hp_offset)
         {
            m_pArtifact->SetHarpStrandOffsetHp(hp_upper_bound);
         }
         else if (hp_offset < hp_lower_bound)
         {
            m_pArtifact->SetHarpStrandOffsetHp(hp_lower_bound);
         }
      }

      if (m_DoDesignForStrandSlope)
      {
         if ( !this->AdjustForStrandSlope() )
         {
            return false;
         }
      }

      if (m_DoDesignForHoldDownForce)
      {
         if ( !this->AdjustForHoldDownForce() )
         {
            return false;
         }
      }
   }

   return true;
}

void pgsStrandDesignTool::GetEndOffsetBounds(Float64* pLower, Float64* pUpper) const
{
   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (0 < nh)
   {
      Float64 end_offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(m_Span,m_Girder);
      if (0.0 < end_offset_inc)
      {
         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_Span,m_Girder, nh, pLower, pUpper);
      }
      else
      {
         // adjustment not allowed
         *pLower=0;
         *pUpper=0;
      }
   }
   else
   {
      // should be smart enough not to call this with no hs
      ATLASSERT(0);
      *pLower=0;
      *pUpper=0;
   }
}

void pgsStrandDesignTool::GetHpOffsetBounds(Float64* pLower, Float64* pUpper) const
{
   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (0 < nh)
   {
      Float64 offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(m_Span,m_Girder);
      if (0.0 < offset_inc)
      {
         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_Span,m_Girder, nh, pLower, pUpper);
      }
      else
      {
         // adjustment not allowed
         *pLower=0;
         *pUpper=0;
      }
   }
   else
   {
      // should be smart enough not to call this with no hs
      ATLASSERT(0);
      *pLower=0;
      *pUpper=0;
   }
}


Float64 pgsStrandDesignTool::GetPrestressForceAtLifting(const GDRCONFIG &guess,const pgsPointOfInterest& poi)
{
   Float64 distFromStart = poi.GetDistFromStart();
   ATLASSERT( !IsZero(distFromStart) && !IsEqual(distFromStart,m_GirderLength));

   LOG("Compute total prestessing force at Release in end-zone for the current configuration at " << ::ConvertFromSysUnits(distFromStart, unitMeasure::Feet) << " ft along girder");

   Float64 xFerFactor;
   Float64 xferLength = GetTransferLength(pgsTypes::Permanent);
   if (distFromStart < xferLength)
   {
      xFerFactor = distFromStart/xferLength;
   }
   else if (m_GirderLength-distFromStart < xferLength)
   {
      xFerFactor = (m_GirderLength-distFromStart)/xferLength;
   }
   else
   {
      xFerFactor = 1.0;
   }
   LOG("Transfer Length Factor = "<<xFerFactor);

   // Compute the maximum allowable jacking force for the trial number of strands
   StrandIndexType ns = guess.Nstrands[pgsTypes::Straight];
   StrandIndexType nh = guess.Nstrands[pgsTypes::Harped];
   StrandIndexType nt = guess.Nstrands[pgsTypes::Temporary];
   StrandIndexType np = ns + nh + nt;

   if ( np == 0)
   {
      ATLASSERT(0); // this really shouldn't happen in a design algoritm
      return 0.0;
   }

   Float64 pj  =  guess.Pjack[pgsTypes::Straight] + 
                  guess.Pjack[pgsTypes::Harped]   + 
                  guess.Pjack[pgsTypes::Temporary];

   Float64 Aps = m_Aps[pgsTypes::Straight]*ns + m_Aps[pgsTypes::Harped]*nh + m_Aps[pgsTypes::Temporary]*nt;
   Float64 fpj =  pj/Aps;

   LOG("Average jacking stress for this strand configuration = " << ::ConvertFromSysUnits(fpj,unitMeasure::KSI) << " KSI");

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   LOSSDETAILS losses;
   ATLASSERT(m_Span == poi.GetSpan() && m_Girder == poi.GetGirder());
   psfeng.ComputeLosses(poi,guess,&losses);
   Float64 loss = losses.pLosses->PermanentStrand_AtLifting();

   LOG("Estimated losses at lifting for this strand configuration = "
      << ::ConvertFromSysUnits(loss,unitMeasure::KSI) << " KSI");

   // Average stress after losses
   Float64 fstrand = fpj - loss;
   LOG("Average strand stress at lifting = " << ::ConvertFromSysUnits(fstrand,unitMeasure::KSI) << " KSI");

   // reduction due to strand slope
   Float64 hp_offset  = GetHarpStrandOffsetHp();
   Float64 end_offset = GetHarpStrandOffsetEnd();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 ss = pStrandGeom->GetAvgStrandSlope(pgsPointOfInterest(m_Span,m_Girder,distFromStart), nh,
                                               end_offset, hp_offset );

   LOG("Adjust for a strand slope of: " << ss);

   Float64 hz = 1.0;
//   if (ss < Float64_Max)
//      hz = ss/sqrt(1*1 + ss*ss);

   Float64 force = fstrand * xFerFactor * (m_Aps[pgsTypes::Straight]*ns + m_Aps[pgsTypes::Harped]*nh*hz + m_Aps[pgsTypes::Temporary]*nt);
   LOG("Total force at lifting = (" << ::ConvertFromSysUnits(fstrand,unitMeasure::KSI) << " ksi)(" << xFerFactor << ")"
      << "[" << ::ConvertFromSysUnits(m_Aps[pgsTypes::Straight],unitMeasure::Inch2) << " in^2)(" << ns << ") + (" 
      << ::ConvertFromSysUnits(m_Aps[pgsTypes::Harped],unitMeasure::Inch2) << " in^2)(" << hz << ")(" << nh << ") + " 
      << ::ConvertFromSysUnits(m_Aps[pgsTypes::Temporary],unitMeasure::Inch2) << "in^2)(" << nt << ")] = " << ::ConvertFromSysUnits(force,unitMeasure::Kip) << " kip");

   return force;
}

Float64 pgsStrandDesignTool::GetPrestressForceMz(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   LOG("Compute total prestessing force in mid-zone for the current configuration");

   UpdateJackingForces();
   GDRCONFIG guess = GetGirderConfiguration();

   // Compute the maximum allowable jacking force for the trial number of strands
   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = stage<pgsTypes::BridgeSite2 ? GetNt() : 0;
   StrandIndexType np = ns + nh + nt;

   if ( np == 0)
   {
      ATLASSERT(0); // this really shouldn't happen in a design algoritm
      return 0.0;
   }

   Float64 pj  =  guess.Pjack[pgsTypes::Straight] + guess.Pjack[pgsTypes::Harped];

   if (stage<pgsTypes::BridgeSite2)
   {
      pj += guess.Pjack[pgsTypes::Temporary];
   }

   Float64 Aps = m_Aps[pgsTypes::Straight]*ns + m_Aps[pgsTypes::Harped]*nh + m_Aps[pgsTypes::Temporary]*nt;
   Float64 fpj =  pj/Aps;

   LOG("Average jacking stress for this strand configuration = " << ::ConvertFromSysUnits(fpj,unitMeasure::KSI) << " KSI");

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   LOSSDETAILS losses;
   ATLASSERT(m_Span == poi.GetSpan() && m_Girder == poi.GetGirder());
   psfeng.ComputeLosses(poi,guess,&losses);
   Float64 loss;
   if (stage==pgsTypes::CastingYard)
   {
      loss = losses.pLosses->PermanentStrand_AfterTransfer();
      LOG("Estimated Release losses for this strand configuration = " << ::ConvertFromSysUnits(loss,unitMeasure::KSI) << " KSI");
   }
   else
   {
      loss = losses.pLosses->PermanentStrand_Final();
      LOG("Estimated Final losses for this strand configuration = " << ::ConvertFromSysUnits(loss,unitMeasure::KSI) << " KSI");
   }


   // Average stress after losses
   Float64 fstrand = fpj - loss;
   LOG("average strand stress after losses = " << ::ConvertFromSysUnits(fstrand,unitMeasure::KSI) << " KSI");

   Float64 force = fstrand*Aps;
   LOG("Total force at final = " << ::ConvertFromSysUnits(force,unitMeasure::Kip) << " kip");

   return force;
}


Float64 pgsStrandDesignTool::ComputeEndOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc)
{
   ATLASSERT(poi.GetSpan() == m_Span);
   ATLASSERT(poi.GetGirder() == m_Girder);

   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = GetNt();

   GDRCONFIG guess = GetGirderConfiguration();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   Float64 neff_ss, neff_ts, neff_hs;

   Float64 ecc_ss = pStrandGeom->GetSsEccentricity(poi, guess, &neff_ss);
   Float64 ecc_ts = pStrandGeom->GetTempEccentricity(poi, guess, &neff_ts);

   // compute hs eccentricities for end offsets of 1.0 and -1.0, and extrapolate the required offset
   guess.EndOffset = 1.0;
   Float64 ecc_hs_p1 = pStrandGeom->GetHsEccentricity(poi, guess, &neff_hs);

   guess.EndOffset = -1.0;
   Float64 ecc_hs_m1 = pStrandGeom->GetHsEccentricity(poi, guess, &neff_hs);

   Float64 neff = neff_ss + neff_hs + neff_ts;
   ATLASSERT(neff>0.0);

   Float64 as_straight  = m_Aps[pgsTypes::Straight]*neff_ss;
   Float64 as_harped    = m_Aps[pgsTypes::Harped]*neff_hs;
   Float64 as_temporary = m_Aps[pgsTypes::Temporary]*neff_ts;
   Float64 as = (as_straight + as_harped + as_temporary);

   Float64 ecc_p1 = (as_straight*ecc_ss + as_temporary*ecc_ts + as_harped*ecc_hs_p1)/as;
   Float64 ecc_m1 = (as_straight*ecc_ss + as_temporary*ecc_ts + as_harped*ecc_hs_m1)/as;

   mathCoordMapper1d mapper(-1.0, ecc_m1, 1.0, ecc_p1);

   Float64 off = mapper.GetA(ecc);

   guess.EndOffset = off;

   Float64 new_ecc = pStrandGeom->GetEccentricity(poi, guess, true, &neff);
   ATLASSERT(IsEqual(ecc,new_ecc,0.01));

   return off;
}

Float64 pgsStrandDesignTool::ComputeHpOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc,pgsTypes::Stage eccStage)
{
   ATLASSERT(poi.GetSpan() == m_Span);
   ATLASSERT(poi.GetGirder() == m_Girder);

   if (ecc == Float64_Max)
   {
      // Eating our own dog food here. If above is true, then we are dealing with ecc as an uninitialized value of 
      // m_MinimumFinalMzEccentricity(Float64_Max), set above in our constructor. This means that we are having trouble
      // determining a minimum number of strands to satisfy bottom mid-span tensile stresses, and the main algorithm
      // is grasping for straws. At any rate, lowering the strand bundle at the HP is not an option.
      return -Float64_Max;
   }
   else
   {
      bool include_temp = eccStage<pgsTypes::BridgeSite1;  // temp strands only before bridgesite
      GDRCONFIG guess = GetGirderConfiguration();

      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 neff_ss(0.0), neff_ts(0.0), neff_hs(0.0);

      Float64 ecc_ss = pStrandGeom->GetSsEccentricity(poi, guess, &neff_ss);

      Float64 ecc_ts = include_temp ? pStrandGeom->GetTempEccentricity(poi, guess, &neff_ts) : 0.0;

      // compute hs eccentricities for hp offsets of +1.0 and -1.0, and extrapolate the required offset
      guess.HpOffset = 1.0;
      Float64 ecc_hs_p1 = pStrandGeom->GetHsEccentricity(poi, guess, &neff_hs);
      guess.HpOffset = -1.0;
      Float64 ecc_hs_m1 = pStrandGeom->GetHsEccentricity(poi, guess, &neff_hs);

      Float64 neff = neff_ss + neff_hs + neff_ts;
      ATLASSERT(neff>0.0);

      Float64 as_straight  = m_Aps[pgsTypes::Straight]*neff_ss;
      Float64 as_harped    = m_Aps[pgsTypes::Harped]*neff_hs;
      Float64 as_temporary = include_temp ? m_Aps[pgsTypes::Temporary]*neff_ts : 0;
      Float64 as = (as_straight + as_harped + as_temporary);

      Float64 ecc_p1 = (as_straight*ecc_ss + as_temporary*ecc_ts + as_harped*ecc_hs_p1)/as;
      Float64 ecc_m1 = (as_straight*ecc_ss + as_temporary*ecc_ts + as_harped*ecc_hs_m1)/as;

      mathCoordMapper1d mapper(-1.0, ecc_m1, 1.0, ecc_p1);

      Float64 off = mapper.GetA(ecc);

      guess.HpOffset = off;
      Float64 new_ecc = pStrandGeom->GetEccentricity(poi, guess, include_temp, &neff);
      ATLASSERT(IsEqual(ecc,new_ecc,0.01));

      return off;
   }
}

bool pgsStrandDesignTool::ComputeMinHarpedForEzEccentricity(const pgsPointOfInterest& poi, Float64 eccTarget, pgsTypes::Stage eccStage, StrandIndexType* pNs, StrandIndexType* pNh)
{
   // don't do anything if we aren't in minimize harped mode
   if (m_DesignOptions.doStrandFillType != ftMinimizeHarping)
   {
      LOG("We do not minimize harped strands unless m_StrandFillType==ftMinimizeHarping");
      return false;
   }

   ATLASSERT(poi.GetSpan()   == m_Span);
   ATLASSERT(poi.GetGirder() == m_Girder);

   LOG("Attempting to swap harped for straight to acheive an ecc = "<< ::ConvertFromSysUnits(eccTarget, unitMeasure::Inch) << " in");
   LOG("At "<< ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Feet) << " feet from left end of girder");

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // get the current eccentricity and make sure our target is lower (bigger)
   Float64 curr_ecc = ComputeEccentricity(poi,eccStage);
   LOG("Current ecc = "<< ::ConvertFromSysUnits(curr_ecc, unitMeasure::Inch) << " in");
   if (eccTarget < curr_ecc)
   {
      // If code hits here, we have a bug upstream computing lifing concrete strength - need to revisit
      ATLASSERT(0); // this should never happen
      LOG("Warning - current eccentricity is already larger than requested");
      return false;
   }

   bool include_temp = (eccStage == pgsTypes::CastingYard || 
                        eccStage == pgsTypes::Lifting     || 
                        eccStage == pgsTypes::Hauling ? true : false);  

   GDRCONFIG guess = GetGirderConfiguration();

   StrandIndexType ns_orig = GetNs();
   StrandIndexType nh_orig = GetNh();
   StrandIndexType ns_prev = ns_orig;
   StrandIndexType nh_prev = nh_orig;
   StrandIndexType Np = ns_prev+nh_prev;  // need to maintain this or be one bigger
   LOG("Current Ns = " << ns_prev << " Nh = " << nh_prev << " Np = " << Np);

   pgsPointOfInterest ms_poi(m_Span,m_Girder,m_GirderLength/2.0);

   // smallest number of harped strands we can have
   StrandIndexType min_nh = pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Harped, 0);

   if (nh_orig <= min_nh)
   {
      LOG("No harped strands to lose - try another method");
      return false;
   }

   Float64 neff_ss(0.0), neff_ts(0.0), neff_hs(0.0);

   // loop until our eccentricity gets larger than the target
   StrandIndexType Ns, Nh;
   Float64 eccDiffPrev = eccTarget - curr_ecc; // difference between the target eccentricity and the eccentricy
                                               // for the previous guess
   while(curr_ecc < eccTarget)
   {
      // try to swap harped for straight
      Ns = pStrandGeom->GetNextNumStrands(m_Span, m_Girder, pgsTypes::Straight, ns_prev);
      if (Ns == Uint32_Max)
      {
         //no more straight strands left to fill
         break;
      }

      Nh = Np - Ns;
      Nh = pStrandGeom->GetPrevNumStrands(m_Span, m_Girder,pgsTypes::Harped, Nh+1);
      if (Nh == Uint32_Max)
      {
         //no more harped strands left
         break;
      }

      // we must end up with at least Np strands
      StrandIndexType remain = Np - (Ns+Nh);
      if (0 < remain)
      {
         ATLASSERT(remain==1);
         Ns = pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Straight, Ns);
         if (Ns == Uint32_Max)
         {
            // last ditch is to see if there is a harped increment we missed
            StrandIndexType nhd = pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Harped, Nh);
            if (nhd != nh_prev)
            {
               Nh = nhd;
            }
            else
            {
               break;
            }
         }
      }

      // finally can compute our new eccentricity
      guess.Nstrands[pgsTypes::Straight] = Ns;
      guess.Nstrands[pgsTypes::Harped]   = Nh;
      Float64 new_ecc = pStrandGeom->GetEccentricity(poi, guess, include_temp, &neff_ss);

      ATLASSERT(neff_ss>0.0);

      LOG("Try ns = "<< Ns <<" nh = "<< Nh <<" np = "<< Nh+Ns);
      LOG("New ecc = "<< ::ConvertFromSysUnits(new_ecc, unitMeasure::Inch) << " in, Target ecc = "<< ::ConvertFromSysUnits(eccTarget, unitMeasure::Inch) << " in");

      // our new ecc had better be more than our previous, or this strategy isn't working
      if (new_ecc < curr_ecc)
      {
         LOG("Swapping Harped strands for straight does not increase eccentricity - quit this waste of time!");
         return false;
      }
      
      // we have to guard against causing mid-zone Bottom Service tension to go out of bounds. Odd case, but it happens for WF42G
      Float64 neff;
      Float64 ms_ecc = pStrandGeom->GetEccentricity(ms_poi, guess, false, &neff);
      LOG("New Eccentricity in mid-zone, without temp strands, is " <<::ConvertFromSysUnits( ms_ecc , unitMeasure::Inch)<< " in");
      LOG("Minimum ecc for release tension mz = " <<::ConvertFromSysUnits( GetMinimumFinalMzEccentricity() , unitMeasure::Inch)<< " in");

      if (ms_ecc < GetMinimumFinalMzEccentricity())
      {
         LOG("Swapping harped for straight voilates minimum mid-zone eccentricity for bottom service tension. Abort this strategy");
         return false;
      }

      if ( eccTarget < new_ecc )
      {
         // we've overshot the target eccentricity (basically, we've found the answer)
         // we are after the eccentricity that best matches the target. compute the
         // difference between the eccentricity using the new strand configuration (new_ecc)
         // and the target eccentricity. Compare this to the difference between the
         // previous eccentricity (curr_ecc) and the target. Use the number of strands associated
         // with the eccentricity that is closest to the target
         if ( new_ecc - eccTarget < eccTarget - curr_ecc )
         {
            LOG("We overshot the target eccentricity, but we are closer than the previous guess so keep the result");
            ns_prev = Ns;
            nh_prev = Nh;
         }
#if defined _DEBUG
         else
         {
            // the previous guess was better than this one so don't update the "prev" values.
            // the result will be the actual previous number of straight and harped strands
            LOG("Increased Ecc by reducing harped strands - Success");
         }
#endif

         break;
      }


      // update control variables for the next loop
      ns_prev = Ns;
      nh_prev = Nh;
      curr_ecc = new_ecc;

      if (nh_prev <= min_nh)
      {
         LOG("No more harped strands to lose");
         break;
      }
   }

   if (ns_prev != GetNs() || nh_prev != GetNh())
   {
      // we updated 
      LOG("Succeeded reducing harped strands ns = "<<ns_prev<<" nh = "<<nh_prev<<" np = "<< ns_prev+nh_prev);
      *pNs = ns_prev;
      *pNh = nh_prev;
      return true;
   }
   else
   {
      LOG("Number of harped strands not reduced - try another method");
      return false; // nothing changed
   }
}

bool pgsStrandDesignTool::ComputeAddHarpedForMzReleaseEccentricity(const pgsPointOfInterest& poi, Float64 eccMax, Float64 eccMin, StrandIndexType* pNs, StrandIndexType* pNh)
{
   ATLASSERT(poi.GetSpan()   == m_Span);
   ATLASSERT(poi.GetGirder() == m_Girder);

   LOG("Attempting to swap straight for harped to raise ecc to at least = "<< ::ConvertFromSysUnits(eccMax, unitMeasure::Inch) << " in");
   LOG("Attempting to swap straight for harped to raise ecc to at most  = "<< ::ConvertFromSysUnits(eccMin, unitMeasure::Inch) << " in");
   LOG("At "<< ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Inch) << " in from left end of girder");

#pragma Reminder("If TxDOT starts designing for lifting, we need to change UI to allow non-standard fill and lifting")
   // Lifting analysis swaps the other way - 
   if (m_DesignOptions.doDesignLifting)
   {
      LOG("Lifting analysis Enabled - Attempting to swap straight for harped to raise ecc is not compatible with lifting goals");
      return false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      GDRCONFIG guess = GetGirderConfiguration();

      // get the current eccentricity and make sure our target is higher (smaller)
      Float64 neff;
      Float64 curr_ecc = pStrandGeom->GetEccentricity(poi, guess, true, &neff);
      LOG("Current ecc = "<< ::ConvertFromSysUnits(curr_ecc, unitMeasure::Inch) << " in");
      if (curr_ecc<eccMin)
      {
         ATLASSERT(0); // this probably should never happen
         LOG("Warning - current eccentricity is already smaller than requested");
         return false;
      }

      StrandIndexType ns_orig = GetNs();
      StrandIndexType nh_orig = GetNh();
      StrandIndexType ns_prev = ns_orig;
      StrandIndexType nh_prev = nh_orig;
      StrandIndexType nhs = ns_prev+nh_prev;  // need to maintain this or be one bigger
      StrandIndexType nt = GetNt();
      LOG("Current ns = "<<ns_prev<<" nh = "<<nh_prev<<" nt = "<<nt<<" np = "<< nhs);

      // largest number of harped strands we can have
      StrandIndexType max_nh = pStrandGeom->GetMaxStrands(m_Span, m_Girder,pgsTypes::Harped);
      if (nh_orig>=max_nh)
      {
         LOG("Harped pattern is full - cannot add any more");
         return false;
      }
      else if (ns_orig<=0)
      {
         LOG("No straight strands to trade");
         return false;
      }

      Float64 off_hp = this->GetHarpStrandOffsetHp();
      Float64 off_end= this->GetHarpStrandOffsetEnd();

      Float64 neff_ss(0.0), neff_ts(0.0), neff_hs(0.0);
      Float64 ecc_ts = nt>0 ? pStrandGeom->GetTempEccentricity(poi, guess, &neff_ts) : 0.0;

      // loop until our eccentricity gets smaller than the target, then step back
      StrandIndexType ns, nh;
      bool succeeded=false;
      while(curr_ecc>eccMin)
      {
         // try to swap straight to harped
         nh = pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Harped, nh_prev);
         if (nh>=max_nh)
         {
            //no more harped locations left to fill
            LOG("All harped locations are full");
            break;
         }

         ns = nhs - nh;
         ns = pStrandGeom->GetPrevNumStrands(m_Span, m_Girder,pgsTypes::Straight, ns+1);
         if (ns<0)
         {
            //no more straight strands left
            LOG("No straight strands left to trade");
            break;
         }

         // we must end up with at least nhs strands
         StrandIndexType remain = nhs - (ns+nh);
         if (remain>0)
         {
            ATLASSERT(remain==1);
            nh = pStrandGeom->GetNextNumStrands(m_Span, m_Girder,pgsTypes::Harped, nh);
            if (nh<0)
            {
               LOG("No harped locations left");
               break;
            }
         }

         // finally can compute our new eccentricity
         guess.Nstrands[pgsTypes::Straight] = ns;
         guess.Nstrands[pgsTypes::Harped]   = nh;
         Float64 new_ecc = pStrandGeom->GetEccentricity(poi, guess, true, &neff);

         ATLASSERT(neff>0.0);

         LOG("Try ns = "<<ns<<" nh = "<<nh<<" np = "<< nh+ns);
         LOG("New ecc = "<< ::ConvertFromSysUnits(new_ecc, unitMeasure::Inch) << " in, Target ecc = "<< ::ConvertFromSysUnits(eccMax, unitMeasure::Inch) << " in");

         // our new ecc had better be less than our previous, or this strategy isn't working
         if (new_ecc>curr_ecc)
         {
            LOG("Swapping Straight strands for Harped does not decrease eccentricity - quit this waste of time!");
            return false;
         }

         curr_ecc = new_ecc;

         if (curr_ecc<eccMax)
         {
            // We have achieved our objective. Might be able to go further, but not needed
            LOG("Target ecc achieved at ns = "<<ns<<" nh = "<<nh);
            succeeded = true;
            ns_prev = ns;
            nh_prev = nh;
            break;
         }

         if (curr_ecc<eccMin)
         {
            LOG("New eccentricity is below minimum - this is as far as we can go");
            if (ns_prev==ns_orig && nh_prev==nh_orig)
            {
                  LOG("First try to increase harped strands overshot min - strategy Failed");
            }
            else
            {
               LOG("Increased Ecc by reducing harped strands. But did not reach target - Limited Success");
            }

            break;
         }

         ns_prev = ns;
         nh_prev = nh;

         if (nh>=max_nh)
         {
            LOG("No more harped strands to add");
            break;
         }
         else if (ns<=0)
         {
            LOG("No more straight strands to trade from");
            break;
         }
      }

      if (ns_prev!=GetNs() || nh_prev!=GetNh())
      {
         // we updated 
         if (succeeded)
         {
            LOG("Target ecc was achieved");
         }

         LOG("Succeeded Adding harped strands ns = "<<ns_prev<<" nh = "<<nh_prev<<" np = "<< ns_prev+nh_prev);
         *pNs = ns_prev;
         *pNh = nh_prev;
         return true;
      }
      else
      {
         LOG("Number of harped strands not increased - try another method");
         return false; // nothing changed
      }
   }
}


Float64 pgsStrandDesignTool::GetMinimumFinalMzEccentricity()
{
   return m_MinimumFinalMzEccentricity;
}

void pgsStrandDesignTool::SetMinimumFinalMzEccentricity(Float64 ecc)
{
   m_MinimumFinalMzEccentricity = ecc;
}

Float64 pgsStrandDesignTool::GetMinimumReleaseStrength() const
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   return IS_SI_UNITS(pDisplayUnits) ? ::ConvertToSysUnits(28.0,unitMeasure::MPa) 
                                     : ::ConvertToSysUnits( 4.0,unitMeasure::KSI); // minimum per LRFD 5.4.2.1
}

Float64 pgsStrandDesignTool::GetMaximumReleaseStrength() const
{
   // for lack of a better value
   return GetMaximumConcreteStrength();
}


Float64 pgsStrandDesignTool::GetMinimumConcreteStrength() const
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   return IS_SI_UNITS(pDisplayUnits) ? ::ConvertToSysUnits(34.5,unitMeasure::MPa) 
                                     : ::ConvertToSysUnits( 5.0,unitMeasure::KSI); // agreed by wsdot and txdot
}

Float64 pgsStrandDesignTool::GetMaximumConcreteStrength() const
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   return IS_SI_UNITS(pDisplayUnits) ? ::ConvertToSysUnits(105.0,unitMeasure::MPa) 
                                     : ::ConvertToSysUnits( 15.0,unitMeasure::KSI);
}

arDesignStrandFillType pgsStrandDesignTool::GetOriginalStrandFillType() const
{
   return m_DesignOptions.doStrandFillType;
}

pgsDesignArtifact::ConcreteStrengthDesignState pgsStrandDesignTool::GetReleaseConcreteDesignState() const
{
   pgsDesignArtifact::ConcreteStrengthDesignState state;

   bool ismin = GetReleaseStrength() == GetMinimumReleaseStrength();

   state.SetState(ismin, m_FciControl.Stage(), m_FciControl.StressType(), m_FciControl.LimitState(),m_FciControl.StressLocation());

   return state;
}

pgsDesignArtifact::ConcreteStrengthDesignState pgsStrandDesignTool::GetFinalConcreteDesignState() const
{
   pgsDesignArtifact::ConcreteStrengthDesignState state;

   bool ismin = GetConcreteStrength() == GetMinimumConcreteStrength();

   state.SetState(ismin, m_FcControl.Stage(), m_FcControl.StressType(), m_FcControl.LimitState(),m_FcControl.StressLocation());

   return state;
}

void pgsStrandDesignTool::GetMidZoneBoundaries(Float64* leftEnd, Float64* rightEnd)
{
   // values are cached at initialization
   *leftEnd  = m_lftMz;
   *rightEnd = m_rgtMz;
}

std::vector<pgsPointOfInterest> pgsStrandDesignTool::GetDesignPoi(pgsTypes::Stage stage,PoiAttributeType attrib)
{
   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(m_Span, m_Girder, stage, attrib, POIMGR_OR, &vPoi);
   return vPoi;
}

std::vector<pgsPointOfInterest> pgsStrandDesignTool::GetDesignPoiEndZone(pgsTypes::Stage stage,PoiAttributeType attrib)
{
   std::vector<pgsPointOfInterest> pois = GetDesignPoi(stage, attrib);

   Float64 rgt_end, lft_end;
   GetMidZoneBoundaries(&lft_end, &rgt_end);

   std::vector<pgsPointOfInterest> end_pois;
   end_pois.reserve(pois.size());

   for (std::vector<pgsPointOfInterest>::iterator it=pois.begin(); it!=pois.end(); it++)
   {
      const pgsPointOfInterest& rpoi = *it;
      Float64 dist = rpoi.GetDistFromStart();

      // accept only pois in end zones
      if (dist<=lft_end || dist>=rgt_end)
      {
         end_pois.push_back(rpoi);
      }
   }

   return end_pois;
}

pgsPointOfInterest pgsStrandDesignTool::GetDebondSamplingPOI(pgsTypes::Stage stage) const
{
   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(m_Span, m_Girder, stage, POI_ALL, POIMGR_OR, &vPoi);

   // grab first poi past transfer length
   Float64 xferLength = GetTransferLength(pgsTypes::Permanent);
   Float64 bound = max( xferLength, ::ConvertToSysUnits(2.0,unitMeasure::Inch)); // value is fairly arbitrary, we just don't want end poi

   for (std::vector<pgsPointOfInterest>::iterator it=vPoi.begin(); it!=vPoi.end(); it++)
   {
      const pgsPointOfInterest& rpoi = *it;
      Float64 dist = rpoi.GetDistFromStart();

      // accept only pois in end zones
      if (dist >= bound)
      {
         return rpoi;
      }
   }

   ATLASSERT(0); // impossible?
   return pgsPointOfInterest(m_Span, m_Girder, 1.0);
}


std::vector<pgsPointOfInterest> pgsStrandDesignTool::GetLiftingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 overhang,PoiAttributeType attrib,Uint32 mode)
{
   Float64 left_pick_point_loc = overhang;
   bool do_add_left = true;
   Float64 right_pick_point_loc = this->m_GirderLength-overhang;
   bool do_add_right = true;

   // Remove any current pick point poi's if not at different location
   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Lifting,POI_PICKPOINT,POIMGR_AND,&vPoi);
   ATLASSERT(vPoi.size() == 0 || vPoi.size() == 2);

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      Float64 poi_loc = poi.GetDistFromStart();
      if (IsEqual(poi_loc,left_pick_point_loc) )
      {
         // poi is already there, no need to re-add
         do_add_left = false; 
      }
      else if ( IsEqual(poi_loc,right_pick_point_loc) )
      {
         do_add_right = false; 
      }
      else
      {
         // We need to remove or unset pick point at existing location
         // Use this form because it only evaluations location and not attributes
         m_PoiMgr.RemovePointOfInterest(poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart());

         PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Lifting);
         if ( attributes != POI_PICKPOINT )
         {
            // if the only flag that is set is pick point, remove it
            // otherwise, clear the POI_PICKPOINT bit and add it back
            sysFlags<PoiAttributeType>::Clear(&attributes,POI_PICKPOINT);
            poi.SetAttributes(pgsTypes::Lifting,attributes);
            m_PoiMgr.AddPointOfInterest(poi);
         }
      }
   }


   if (do_add_left)
   {
      pgsPointOfInterest left_pick_point(span,gdr,left_pick_point_loc);
      left_pick_point.AddStage(pgsTypes::Lifting,POI_PICKPOINT|attrib);
      m_PoiMgr.AddPointOfInterest(left_pick_point);
   }

   if (do_add_right)
   {
      pgsPointOfInterest right_pick_point(span,gdr,right_pick_point_loc);
      right_pick_point.AddStage(pgsTypes::Lifting,POI_PICKPOINT|attrib);
      m_PoiMgr.AddPointOfInterest(right_pick_point);
   }

   std::vector<pgsPointOfInterest> vPoi2;
   m_PoiMgr.GetPointsOfInterest(span, gdr, pgsTypes::Lifting, attrib, mode, &vPoi2);

   return vPoi2;
}

std::vector<pgsPointOfInterest> pgsStrandDesignTool::GetHaulingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode)
{
   Float64 left_bunk_point_loc = leftOverhang;
   bool do_add_left = true;
   Float64 right_bunk_point_loc = this->m_GirderLength-rightOverhang;
   bool do_add_right = true;

   // Remove any current pick point poi's if not at different location
   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Hauling,POI_BUNKPOINT,POIMGR_AND,&vPoi);

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      Float64 poi_loc = poi.GetDistFromStart();
      if (IsEqual(poi_loc,left_bunk_point_loc) )
      {
         // poi is already there, no need to re-add
         do_add_left = false; 
      }
      else if ( IsEqual(poi_loc,right_bunk_point_loc) )
      {
         do_add_right = false; 
      }
      else
      {
         // We need to remove or unset pick point at existing location
         m_PoiMgr.RemovePointOfInterest(poi);

         PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Hauling);
         if ( attributes != POI_BUNKPOINT )
         {
            // if the only flag that is set is pick point, remove it
            // otherwise, clear the POI_PICKPOINT bit and add it back
            sysFlags<PoiAttributeType>::Clear(&attributes,POI_BUNKPOINT);
            poi.SetAttributes(pgsTypes::Hauling,attributes);
            m_PoiMgr.AddPointOfInterest(poi);
         }
      }
   }

   if (do_add_left)
   {
      pgsPointOfInterest left_bunk_point(pgsTypes::Hauling,span,gdr,left_bunk_point_loc,POI_BUNKPOINT|attrib);
      m_PoiMgr.AddPointOfInterest(left_bunk_point);
   }

   if (do_add_right)
   {
      pgsPointOfInterest right_bunk_point(pgsTypes::Hauling,span,gdr,right_bunk_point_loc,POI_BUNKPOINT|attrib);
      m_PoiMgr.AddPointOfInterest(right_bunk_point);
   }

   std::vector<pgsPointOfInterest> vPoi2;
   m_PoiMgr.GetPointsOfInterest(span, gdr, pgsTypes::Hauling, attrib, mode, &vPoi2);

   return vPoi2;
}



void pgsStrandDesignTool::ValidatePointsOfInterest()
{
   // POI's are managed locally because the global list can be polluted with information about the current design
   m_PoiMgr.RemoveAll();

   // Get full list of POIs for the current model. For straight and harped designs, this is all we need.
   // However, for debond designs, we need to remove any POI's added for debonding and then
   // add our own POI's at all possible debond locations

   // start by putting pois into a temporary vector
   GET_IFACE(IPointOfInterest,pIPOI);

   // Get all points of interest, regardless of stage and attributes
   std::vector<pgsPointOfInterest> pois = pIPOI->GetPointsOfInterest(m_Span,m_Girder);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   if (m_DesignOptions.doDesignForFlexure == dtDesignForHarping || 
       m_DesignOptions.doDesignForFlexure == dtNoDesign)
   {
      // For Harped designs, add all pois 
      // The none option is also considered here because if we are designing for shear only (no flexure design)
      // we need the same POI.
      std::vector<pgsPointOfInterest>::iterator iter;
      for (iter = pois.begin(); iter != pois.end(); iter++)
      {
         pgsPointOfInterest& poi = *iter;

         // Clear any lifting or hauling attributes
         PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Lifting);
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_PICKPOINT);
         poi.SetAttributes(pgsTypes::Lifting,attributes);

         attributes = poi.GetAttributes(pgsTypes::Hauling);
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_BUNKPOINT);
         poi.SetAttributes(pgsTypes::Hauling,attributes);

         Float64 loc = poi.GetDistFromStart();
         if (0.0 < loc && loc < m_GirderLength) // locations at ends of girder are of no interest to design
         {
            m_PoiMgr.AddPointOfInterest(poi);
         }
      }

      // It is possible that the psxfer location is outside of the girder span.
      // This can cause oddball end conditions to control, so add an 
      // artifical xfer poi at the support locations if this is the case
      GET_IFACE(IPrestressForce,pPrestress);
      GET_IFACE(IBridge,pBridge);

      const PoiAttributeType attrib_xfer = POI_ALLACTIONS | POI_ALLOUTPUT | POI_PSXFER;

      Float64 xfer_length = pPrestress->GetXferLength(m_Span,m_Girder,pgsTypes::Permanent);

      Float64 start_conn = pBridge->GetGirderStartConnectionLength(m_Span,m_Girder);
      if (xfer_length < start_conn )
      {
         pgsPointOfInterest pxfer(stages,m_Span,m_Girder,start_conn,attrib_xfer);
         m_PoiMgr.AddPointOfInterest(pxfer);
      }

      Float64 end_conn = pBridge->GetGirderEndConnectionLength(m_Span,m_Girder);

      if (end_conn > xfer_length)
      {
         pgsPointOfInterest pxfer(stages,m_Span,m_Girder,m_GirderLength-end_conn,attrib_xfer);
         m_PoiMgr.AddPointOfInterest(pxfer);
      }

   }
   else if ( m_DesignOptions.doDesignForFlexure == dtDesignForDebonding || 
             m_DesignOptions.doDesignForFlexure == dtDesignFullyBonded )
   {
      ATLASSERT(m_DesignOptions.doDesignForFlexure!=dtNoDesign);
      // Debonding or straight strands: add all pois except those at at debond and transfer locations
      std::vector<pgsPointOfInterest>::iterator iter;
      for (iter = pois.begin(); iter != pois.end(); iter++)
      {
         pgsPointOfInterest& poi = *iter;

         // Clear any lifting or hauling attributes
         PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Lifting);
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_PICKPOINT);
         poi.SetAttributes(pgsTypes::Lifting,attributes);

         attributes = poi.GetAttributes(pgsTypes::Hauling);
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_BUNKPOINT);
         poi.SetAttributes(pgsTypes::Hauling,attributes);

         Float64 loc = poi.GetDistFromStart();
         if ( 0.0 < loc && loc < m_GirderLength )
         {
            // if POI does not have the POI_DEBOND or POI_PSXFER attribute in the casting yard stage
            // then add it
            if ( ! ( poi.HasAttribute(pgsTypes::CastingYard,POI_DEBOND) || 
                     poi.HasAttribute(pgsTypes::CastingYard,POI_PSXFER) ) )
            {
               m_PoiMgr.AddPointOfInterest(poi);
            }
         }
      }

      // now add pois at all possible debond locations and their transfer points
      PoiAttributeType attrib_debond = POI_ALLACTIONS | POI_ALLOUTPUT | POI_DEBOND;
      PoiAttributeType attrib_xfer   = POI_ALLACTIONS | POI_ALLOUTPUT | POI_PSXFER;

      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IPrestressForce,pPrestress);
      GET_IFACE(IStrandGeometry,pStrandGeometry);

      Float64 xfer_length = pPrestress->GetXferLength(m_Span,m_Girder,pgsTypes::Permanent);

      Float64 start_supp = pBridge->GetGirderStartConnectionLength(m_Span,m_Girder);
      Float64 end_supp   = m_GirderLength - pBridge->GetGirderEndConnectionLength(m_Span,m_Girder);

      // left and right xfer from ends
      pgsPointOfInterest lxfer(m_Span,m_Girder,xfer_length);
      AddPOI(lxfer, start_supp, end_supp,attrib_xfer);

      pgsPointOfInterest rxfer(m_Span,m_Girder,m_GirderLength-xfer_length);
      AddPOI(rxfer, start_supp, end_supp,attrib_xfer);

      if (m_DesignOptions.doDesignForFlexure == dtDesignForDebonding)
      {
         // debonding at left and right ends
         Float64 leftEnd, rightEnd;
         GetMidZoneBoundaries(&leftEnd, &rightEnd);

         Float64 db_incr = pStrandGeometry->GetDefaultDebondLength(m_Span,m_Girder);
      
         Int16 nincs = (Int16)floor((leftEnd + 1.0e-05)/db_incr); // we know both locs are equidistant from ends

         Float64 ldb_loc=0.0;
         Float64 rdb_loc=m_GirderLength;
         for (Int32 inc=0; inc<nincs; inc++)
         {
            if (inc+1==nincs)
            {
               // At end of debond zone.
               // This is a bit of a hack, but treat final debond point as a harping point for design considerations
                attrib_debond |= POI_HARPINGPOINT;
            }

            // left debond and xfer
            ldb_loc += db_incr;
            pgsPointOfInterest ldbpo(m_Span,m_Girder,ldb_loc);
            AddPOI(ldbpo, start_supp, end_supp,attrib_debond);

            Float64 lxferloc = ldb_loc + xfer_length;
            pgsPointOfInterest lxfpo(m_Span,m_Girder,lxferloc);
            AddPOI(lxfpo, start_supp, end_supp,attrib_xfer);

            // right debond and xfer
            rdb_loc -= db_incr;
            pgsPointOfInterest rdbpo(m_Span,m_Girder,rdb_loc);
            AddPOI(rdbpo, start_supp, end_supp,attrib_debond);

            Float64 rxferloc = rdb_loc - xfer_length;
            pgsPointOfInterest rxfpo(m_Span,m_Girder,rxferloc);
            AddPOI(rxfpo, start_supp, end_supp,attrib_xfer);
         }
      }
   }
}

void pgsStrandDesignTool::AddPOI(pgsPointOfInterest& rpoi, Float64 lft_conn, Float64 rgt_conn,PoiAttributeType attribute)
{
   rpoi.AddStage(pgsTypes::CastingYard,attribute);

   // bridgesite pois must be between supports
   Float64 loc = rpoi.GetDistFromStart();

   if ( lft_conn < loc && loc < rgt_conn )
   {
      rpoi.AddStage(pgsTypes::Lifting,attribute);
      rpoi.AddStage(pgsTypes::Hauling,attribute);
      rpoi.AddStage(pgsTypes::GirderPlacement,attribute);
      rpoi.AddStage(pgsTypes::TemporaryStrandRemoval,attribute);
      rpoi.AddStage(pgsTypes::BridgeSite1,attribute);
      rpoi.AddStage(pgsTypes::BridgeSite2,attribute);
      rpoi.AddStage(pgsTypes::BridgeSite3,attribute);
   }

   m_PoiMgr.AddPointOfInterest(rpoi);
}


void pgsStrandDesignTool::ComputeMidZoneBoundaries()
{
   LOG("Entering ComputeMidZoneBoundaries");
   // Mid-zone length along beam where positive bending typically controls
   // Harped designs use harping points. User debond rules for all others
   if (m_DesignOptions.doDesignForFlexure==dtDesignForHarping)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 lhp, rhp;
      pStrandGeom->GetHarpingPointLocations(m_Span,m_Girder,&lhp,&rhp);

      m_lftMz  = lhp;
      m_rgtMz = rhp;

      LOG("Mid-Zone boundaries are at harping points. Left = "<< ::ConvertFromSysUnits(m_lftMz,unitMeasure::Feet) << " ft, Right = "<< ::ConvertFromSysUnits(m_rgtMz,unitMeasure::Feet) << " ft");

   }
   else
   {
      // Mid zone for debonding is envelope of girderLength/2 - dev length, and user-input limits
      GET_IFACE(IStrandGeometry, pStrandGeometry);
      GET_IFACE(ILibrary, pLib);
      GET_IFACE(IDebondLimits,pDebondLimits);
      GET_IFACE(IGirderData,pGirderData);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);

      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const GirderLibraryEntry* pGirderEntry = pBridgeDesc->GetSpan(m_Span)->GetGirderTypes()->GetGirderLibraryEntry(m_Girder);

      // Need development length for design and this is a chicken and egg kinda thing
      // make some basic assumptions;

      // According to Article 5.7.3.1.1, fpe must be at least 50% of fps  (where fps is fpu)
      // Substitute into (5.11.4.2-1)
      // k = 2.0  (from 5.11.4.3)
      // We get:
      // ld = 2.0(fps - 2/3 * fps/2.0)db   = 4/3(fpu)db
      const matPsStrand* pstrand = pGirderData->GetStrandMaterial(m_Span,m_Girder,pgsTypes::Permanent);
      CHECK(pstrand!=0);

      // use US units
      Float64 db = ::ConvertFromSysUnits(pstrand->GetNominalDiameter(),unitMeasure::Inch);
      Float64 fpu = ::ConvertFromSysUnits(pstrand->GetUltimateStrength(),unitMeasure::KSI);

      Float64 dev_len = fpu*db*4.0/3.0;

      dev_len = ::ConvertToSysUnits(dev_len,unitMeasure::Inch);
      LOG("Approximate upper bound of development length = "<< ::ConvertFromSysUnits(dev_len,unitMeasure::Inch)<<" in");

      Float64 mz_end_len = m_GirderLength/2.0 - dev_len;
      LOG("Max debond length based on development length = "<< ::ConvertFromSysUnits(mz_end_len,unitMeasure::Inch)<<" in");

      bool bSpanFraction, buseHard;
      Float64 spanFraction, hardDistance;
      pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

      if (bSpanFraction)
      {
         Float64 sflen = m_GirderLength * spanFraction;
         LOG("User-input min MZ fractional length = "<< ::ConvertFromSysUnits(sflen,unitMeasure::Inch)<<" in");

         mz_end_len = min(mz_end_len, sflen);
      }

      if (buseHard)
      {
         mz_end_len = min(mz_end_len, hardDistance);
         LOG("User-input min MZ hard length = "<< ::ConvertFromSysUnits(hardDistance,unitMeasure::Inch)<<" in");
      }

      mz_end_len = max(mz_end_len, 0.0); // can't be less than zero
      LOG("Raw MZ end length = "<< ::ConvertFromSysUnits(mz_end_len,unitMeasure::Inch)<<" in");
      LOG("Girder length = "<< ::ConvertFromSysUnits(m_GirderLength,unitMeasure::Inch)<<" in");
 
      Float64 db_incr = pStrandGeometry->GetDefaultDebondLength(m_Span,m_Girder);
      LOG("Debond spacing increment = "<< ::ConvertFromSysUnits(db_incr,unitMeasure::Inch)<<" in");
   
      if (mz_end_len < db_incr)
      {
         // we can't debond because there is no room
         LOG("**** No room for Debonding and no use trying - switch to straight strand design ****");
         m_DesignOptions.doDesignForFlexure = dtDesignFullyBonded;
         m_lftMz = 0.0;
         m_rgtMz = m_GirderLength;

         // cache debond section information
         m_NumDebondSections   = 1; // always have a section at beam ends
         m_DebondSectionLength = 0.0;
      }
      else
      {
         // mid-zone length must be a multiple of debond increment 
         Int16 nincs = (Int16)floor((mz_end_len + 1.0e-05)/db_incr);

         m_lftMz = nincs*db_incr;
         m_rgtMz = m_GirderLength - m_lftMz;

         // cache debond section information
         m_NumDebondSections   = nincs+1; // always have a section at beam ends
         m_DebondSectionLength = db_incr;

         LOG("Number of debond increments to MZ  = "<< nincs);
      }
 
      LOG("Left MZ location = "<< ::ConvertFromSysUnits(m_lftMz,unitMeasure::Inch)<<" in");
      LOG("Right MZ location = "<< ::ConvertFromSysUnits(m_rgtMz,unitMeasure::Inch)<<" in");
      LOG("Number of debond sections to MZ  = "<< m_NumDebondSections);
      LOG("Debond section length for design  = "<< ::ConvertFromSysUnits(m_DebondSectionLength,unitMeasure::Inch)<<" in");

      ATLASSERT(m_lftMz<m_GirderLength/2.0);
   }
   LOG("Exiting ComputeMidZoneBoundaries");
}

void pgsStrandDesignTool::InitHarpedPhysicalBounds(const matPsStrand* pstrand)
{
   // Initialize strand offsets
   m_pArtifact->SetHarpStrandOffsetEnd(0.00);
   m_pArtifact->SetHarpStrandOffsetHp(0.00);

   // intialize data for strand slope and hold down checks
   GET_IFACE(IStrandGeometry,pStrandGeom);
   // no use if there are no harped strands
   StrandIndexType nh_max = pStrandGeom->GetMaxStrands(m_Span, m_Girder,pgsTypes::Harped);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck,bDesign;
   Float64 s50, s60, s70;
   pSpecEntry->GetMaxStrandSlope(&bCheck,&bDesign,&s50,&s60,&s70);

   m_DoDesignForStrandSlope = bDesign && (0 < nh_max ? true : false);

   if ( m_DoDesignForStrandSlope )
   {
      if ( pstrand->GetSize() == matPsStrand::D1778  )
         m_AllowableStrandSlope = s70;
      else if ( pstrand->GetSize() == matPsStrand::D1524 )
         m_AllowableStrandSlope = s60;
      else
         m_AllowableStrandSlope = s50;

      LOG("We will be designing for an allowable strand slope of 1:" << m_AllowableStrandSlope);
   }
   else
   {
      LOG("Strand slope not a design criteria");
   }

   // hold down
   pSpecEntry->GetHoldDownForce(&bCheck,&bDesign,&m_AllowableHoldDownForce);

   m_DoDesignForHoldDownForce = bDesign && nh_max>0;

   if (m_DoDesignForHoldDownForce)
      LOG("We will be designing for harped strand hold down allowable: " << m_AllowableHoldDownForce);
   else
      LOG("Hold down force not a design criteria");
}

//////////////////////////////// Debond Design //////////////////////////

// utility struct to temporarily store and sort rows
struct Row
{
   double Elevation;
   StrandIndexType   MaxInRow;
   std::vector<StrandIndexType> StrandsFilled;
   std::vector<StrandIndexType> StrandsDebonded;

   Row():
      Elevation(0.0),
      MaxInRow(0)
      {;}

   Row(Float64 elev):
      Elevation(elev),
      MaxInRow(0)
      {;}

   bool operator==(const Row& rOther) const 
   { 
      return ::IsEqual(Elevation,rOther.Elevation); 
   }

   bool operator<(const Row& rOther) const 
   { 
      return Elevation < rOther.Elevation; 
   }
};
typedef std::set<Row> RowSet;
typedef RowSet::iterator RowIter;


// utility struct to temporarily store debond level information
struct TempLevel
{
   std::pair<StrandIndexType,StrandIndexType> DebondsAtLevel;
   StrandIndexType MinStrandsForLevel;

   TempLevel(StrandIndexType firstStrand, StrandIndexType secondStrand, StrandIndexType totalForLevel):
      DebondsAtLevel(firstStrand, secondStrand), 
      MinStrandsForLevel(totalForLevel)
   {;}
};
typedef std::vector<TempLevel> TempLevelList;
typedef TempLevelList::iterator TempLevelIterator;


void pgsStrandDesignTool::InitDebondData()
{
   m_pArtifact->ClearDebondInfo();

   m_DebondLevels.clear();

   m_NumDebondSections = 0;
   m_DebondSectionLength = 0;
   m_MaxPercentDebondSection = 0;
   m_MaxDebondSection = 0;
}

void pgsStrandDesignTool::ComputeDebondLevels(IPrestressForce* pPrestressForce)
{
   LOG("");
   LOG("Enter ComputeDebondLevels");
   LOG("*************************");

   if (m_DesignOptions.doDesignForFlexure != dtDesignForDebonding)
   {
      LOG("Exiting ComputeDebondLevels - this is not a debond design");
      return;
   }

   if (m_NumDebondSections<=1)
   {
      // no mid-zone means no debonding
      LOG("No mid-zone - Cannot build debond levels");
      ATLASSERT(0);
      return;
   }

   GET_IFACE(IDebondLimits,pDebondLimits);
   // Debond levels represent all of the possible debonding schemes for the current section
   // Debonding is added in the same sequence as strand fill order. However, we must abide by the
   // debonding limit rules from the specification
   // First get the rules

   Float64 db_max_percent_total = pDebondLimits->GetMaxDebondedStrands(m_Span,m_Girder);
   Float64 db_max_percent_row   = pDebondLimits->GetMaxDebondedStrandsPerRow(m_Span,m_Girder);
   m_MaxPercentDebondSection    = pDebondLimits->GetMaxDebondedStrandsPerSection(m_Span,m_Girder);
   m_MaxDebondSection           = pDebondLimits->GetMaxNumDebondedStrandsPerSection(m_Span,m_Girder);

   LOG("db_max_percent_total = "<<db_max_percent_total);
   LOG("db_max_percent_row = "<<db_max_percent_row);
   LOG("m_MaxPercentDebondSection = "<<m_MaxPercentDebondSection);
   LOG("m_MaxDebondSection = "<<m_MaxDebondSection);

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // coordinates of all straight strands
   StrandIndexType max_ss = pStrandGeom->GetMaxStrands(m_Span, m_Girder,pgsTypes::Straight);

   pgsPointOfInterest poi_ms(m_Span, m_Girder, m_GirderLength/2.0);
   CComPtr<IPoint2dCollection> ss_locations;
   pStrandGeom->GetStrandPositionsEx(poi_ms,max_ss, pgsTypes::Straight, &ss_locations); 

   // First chore is to build a raw list of possible strands that can be debonded
   // Debondable list will be paired down by total, and row constraints in the next step
   CComPtr<ILongArray> debondables;
   pStrandGeom->ListDebondableStrands(m_Span, m_Girder, pgsTypes::Straight, &debondables);

   // Build rows as we fill strands, and compute max available number of strands in any row.
   RowSet rows;

   typedef std::pair<StrandIndexType,StrandIndexType> StrandPair;
   std::vector<StrandPair> debondable_list;

   LOG("Building list of debondable strands:");
   Uint32 num_debondable = 0;
   StrandIndexType currnum=0;
   while( currnum < max_ss )
   {
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(m_Span, m_Girder, pgsTypes::Straight, currnum);

      StrandIndexType strands_in_lift = nextnum-currnum; // number of strands in current increment

      Int32 is_debondable;

      // nextnum is a count. need to subtract 1 to get index
      debondables->get_Item(nextnum-1, &is_debondable);

      if (is_debondable != 0)
      {
         // could be one or two strands
         if ( strands_in_lift == 1 )
         {
            num_debondable++;
            debondable_list.push_back( StrandPair(nextnum-1, Uint32_Max) );
            LOG(nextnum-1);
         }
         else
         {
            num_debondable += 2;
            debondable_list.push_back( StrandPair(nextnum-2, nextnum-1) );
            LOG(nextnum-2<<", "<<nextnum-1);
         }
      }
      // count max number of strands in each row
      Float64 curr_y = GetPointY(nextnum-1, ss_locations);

      // fill row with current strands
      // note that logic here is same whether we insert a new row or find an existing one
      Row fill_test_row(curr_y);
      std::pair<RowIter, bool> curr_row_it = rows.insert( fill_test_row );

      Row& curr_row = *(curr_row_it.first);
      curr_row.MaxInRow += strands_in_lift;

      // back to top
      currnum = nextnum;
   }

   if (debondable_list.empty())
   {
      LOG("No debondable strands - Cannot build debond levels");
      ATLASSERT(0); // this should probably be vetted before here ?
      return;
   }
   else
   {
#ifdef _DEBUG
      LOG("Finished building debondable list of "<<num_debondable<<" strands");
      LOG("Max Strands per row in "<<rows.size()<<" rows as follows:");
      for (RowIter riter=rows.begin(); riter!=rows.end(); riter++)
      {
         LOG("elev = "<<::ConvertFromSysUnits(riter->Elevation,unitMeasure::Inch)<<" max strands = "<<riter->MaxInRow);
      }
#endif
   }

   // Second step is to build temporary list containing all debond levels
   // that can be created
   TempLevelList temp_levels;

   // max total number that can be debonded
   StrandIndexType max_debondable_strands = (StrandIndexType)floor(max_ss * db_max_percent_total);

   // Set queue of possible strands to be debonded 
   std::vector<StrandPair>::iterator db_iter = debondable_list.begin();

   Int16 num_debonded=0;

   // Simulate filling all straight strands and debond when possible
   LOG("Build row information and debond levels");
   currnum=0;
   while( currnum < max_ss )
   {
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(m_Span, m_Girder, pgsTypes::Straight, currnum);

      Float64 curr_y = GetPointY(nextnum-1, ss_locations);

      // get row for current strand(s) and add to number filled
      Row fill_test_row(curr_y);
      RowIter curr_row_it = rows.find( fill_test_row );
      if ( curr_row_it == rows.end() )
      {
         ATLASSERT(0); // All rows should have been created in previous loop - this is a bug!!!
         return;
      }

      Row& curr_row = *curr_row_it;
      if (nextnum-currnum == 2)
      {
         curr_row.StrandsFilled.push_back(nextnum-2);
      }

      curr_row.StrandsFilled.push_back(nextnum-1);

      LOG("nextnum = "<<nextnum<<" Y = "<<::ConvertFromSysUnits(curr_y,unitMeasure::Inch)<<" to fill = "<< nextnum-currnum );

      // TRICKY: A nested loop here to try and debond as many strands on the queue as possible for the current fill
      //         Most of the time we will break out of this loop before our queus is depleted
      //         If not for this loop, we could end up with unused debondable strands
      while( db_iter != debondable_list.end() )
      {
         // See if we can debond current strand(s) on the queue
         LOG("Attempt to debond "<<db_iter->first<<", "<<db_iter->second);

         Int16 num_to_db = (db_iter->second == Uint32_Max) ? 1 : 2;
         // first check percentage of total
         Float64 percent_of_total = Float64(num_debonded+num_to_db) / nextnum;
         if ( percent_of_total > db_max_percent_total)
         {
            // not enough total straight strands yet. continue
            // break from inner loop because we need more strands
            LOG("Debonding of strands "<<db_iter->first<<","<<db_iter->second<<" cannot occur until more strands are added for % total");
            break;
         }
         else
         {
            // we can add strands to the total, can we add them to this row?
            // get row elevation of strand(s) to be debonded
            Float64 curr_db_y = GetPointY(db_iter->first, ss_locations);

            Row db_test_row(curr_db_y);
            RowIter db_row_it = rows.find( db_test_row );
            if ( db_row_it != rows.end() )
            {
               Row& db_row = *db_row_it;
               StrandIndexType num_ss_in_row = db_row.StrandsFilled.size();
               StrandIndexType num_db_in_row = db_row.StrandsDebonded.size();

               Float64 new_db_percent = Float64(num_to_db + num_db_in_row) / num_ss_in_row;
               if (new_db_percent <= db_max_percent_row )
               {
                  // We can debond these strands, we have a new debond level
                  temp_levels.push_back( TempLevel(db_iter->first, db_iter->second, nextnum) ); 
                  LOG("Created debond level "<<temp_levels.size()<<" with strands "<<db_iter->first<<","<<db_iter->second<<" and "<<nextnum<<" minimum strands");
                  db_row.StrandsDebonded.push_back(db_iter->first);
                  num_debonded++;
                  if (num_to_db==2)
                  {
                     db_row.StrandsDebonded.push_back(db_iter->second);
                     num_debonded++;
                  }

                  db_iter++; // pop from queue
               }
               else
               {
                  // See if we can ever debond any more strands in this row.
                  // If not, jump to next in queue
                  Float64 db_percent_max = Float64(num_to_db + num_db_in_row) / db_row.MaxInRow;
                  if (db_percent_max > db_max_percent_row )
                  {
                     LOG("Row at "<<::ConvertFromSysUnits(curr_db_y,unitMeasure::Inch)<<" (in), is full try next debondable in queue.");
                  
                     db_iter++;
                  }
                  else
                  {
                     // break from inner loop because we need more strands
                     LOG("Cannot debond in row at "<<::ConvertFromSysUnits(curr_db_y,unitMeasure::Inch)<<" (in), until more strands are added");
                     break;
                  }
               }
            }
            else
            {
               ATLASSERT(0); // bug all rows should have been created by now
            }
         }
      }

      if (db_iter==debondable_list.end())
      {
         LOG("No more debondable strands - exiting debond level loop after filling "<<nextnum<<" strands");
         break;
      }

      currnum = nextnum;
   }
   LOG(temp_levels.size()<<" temporary debond levels created, "<<num_debonded<<" strands can be debonded");

   // now we can build our "real" debond levels data structure
   // First gather some information required to compute debond level information

   // add an empty debond level at head of list. This is debond level 0
   DebondLevel db_level_z;
   db_level_z.Init(ss_locations);
   m_DebondLevels.push_back(db_level_z);

   // now add the rest
   std::vector<StrandIndexType> running_debonds;
   running_debonds.reserve(num_debonded);
   for (TempLevelIterator temp_iter = temp_levels.begin(); temp_iter!=temp_levels.end(); temp_iter++)
   {
      const TempLevel& tl = *temp_iter;

      // running list of debonded strands for each level
      running_debonds.push_back(tl.DebondsAtLevel.first);
      if (tl.DebondsAtLevel.second != -1)
      {
         running_debonds.push_back(tl.DebondsAtLevel.second);
      }

      DebondLevel db_level;
      db_level.MinTotalStrandsRequired = tl.MinStrandsForLevel;
      // starnds for each debond level are cummulative
      db_level.StrandsDebonded.assign(running_debonds.begin(), running_debonds.end());

      // compute values for each debond level
      db_level.Init(ss_locations);

      m_DebondLevels.push_back(db_level);
   }
   // finished creating debond levels - dump them and get outta here
#ifdef _DEBUG
   DumpDebondLevels();
#endif

   LOG("Exiting ComputeDebondLevels");
   LOG("****************************");
}

void pgsStrandDesignTool::DebondLevel::Init(IPoint2dCollection* strandLocations)
{
   // Compute and store eccentricity
   StrandIndexType ns = StrandsDebonded.size();
   if (0 < ns)
   {
      Float64 strand_cg = 0.0;
      for (StrandIndexType strandIdx = 0; strandIdx < ns; strandIdx++)
      {
         StrandIndexType ssn = StrandsDebonded[strandIdx];
         Float64 curr_y = GetPointY(ssn, strandLocations);
         strand_cg += curr_y;
      }

      m_DebondedStrandsCg = strand_cg/ns;
   }
   else
   {
      m_DebondedStrandsCg = 0.0;
   }
}

Float64 pgsStrandDesignTool::DebondLevel::ComputeReliefStress(Float64 psForcePerStrand,Float64 Yb, Float64 Ag, Float64 S) const
{
   StrandIndexType ns = StrandsDebonded.size();
   Float64 e = Yb - m_DebondedStrandsCg;

   Float64 stress = ns * psForcePerStrand * (-1.0/Ag - e/S);

   return stress;
}

void pgsStrandDesignTool::DumpDebondLevels()
{
   LOG("Dump of "<<m_DebondLevels.size()<<" debond levels:");
   Int16 levn = 0;
   for (DebondLevelIterator it=m_DebondLevels.begin(); it!=m_DebondLevels.end(); it++)
   {
      LOG("Debond Level "<<levn);
      DebondLevel& level = *it;
      LOG("   MinTotalStrandsRequired = "<<level.MinTotalStrandsRequired);

      std::stringstream stream;
      for (std::vector<StrandIndexType>::iterator itss = level.StrandsDebonded.begin(); itss!=level.StrandsDebonded.end(); itss++)
      {
         stream <<*itss<<", ";
      }
      // chop  final ,
      std::string str(stream.str());
      Int16 n = str.size();
      if (n>0)
         str.erase(n-2,2);

      LOG("   Debonded Strands = "<<str);
      LOG("   DebondedStrandsCg = "<<level.m_DebondedStrandsCg);
      levn++;
   }
}

bool pgsStrandDesignTool::MaximizeDebonding()
{
   LOG("Entering MaximizeDebonding");
   // Basic idea here is to apply the maximum amount of debonding we can based
   // on physical constraints. We then apply it in order
   // to minimize release and final concrete strength during the early design
   // stages. Maximizing debonding has the same affect as raising harped strands at ends.
   //
   // Once strengths are picked we can return and do "real" debond design

   // Get maximum debond levels for current number of straight strands, and sections
   StrandIndexType ns = GetNs();

   LOG("max_sections = "<< m_NumDebondSections<<", ns = "<<ns);

   std::vector<DebondLevelType> debond_levels;
   debond_levels.reserve(m_NumDebondSections);

   for (StrandIndexType strandIdx = 0; strandIdx < m_NumDebondSections; strandIdx++)
   {
      // number of leading sections is number of sections inboard from current is
      SectionIndexType num_leading_sections;

      // Can't actually debond at location zero (end of girder), so this can only have as
      // many leading sections as section 1
      if (strandIdx == 0)
      {
         ATLASSERT(2 <= m_NumDebondSections);
         num_leading_sections = m_NumDebondSections-2;
      }
      else
      {
         ATLASSERT(strandIdx+1 <= m_NumDebondSections);
         num_leading_sections = m_NumDebondSections-1 - strandIdx;
      }

      DebondLevelType db_level = GetMaxDebondLevel(ns, num_leading_sections);
      debond_levels.push_back(db_level);
   }

   LOG("Debond levels before layout = "<<DumpIntVector(debond_levels));

   // save our max levels
   m_MaxPhysicalDebondLevels = debond_levels;

   LOG("Maximum Debond Layout = "<<DumpIntVector(m_MaxPhysicalDebondLevels));
   LOG("Exiting MaximizeDebonding");
   return true;
}

Float64 pgsStrandDesignTool::GetDebondSectionLength() const
{
   return m_DebondSectionLength;
}

SectionIndexType pgsStrandDesignTool::GetMaxNumberOfDebondSections() const
{
   return m_NumDebondSections;
}

Float64 pgsStrandDesignTool::GetDebondSectionLocation(SectionIndexType sectionIdx, DebondEndType end) const
{
   ATLASSERT(sectionIdx>=0 && sectionIdx<m_NumDebondSections);

   if (end==dbLeft)
   {
      return m_DebondSectionLength*(sectionIdx);
   }
   else
   {
      return m_GirderLength - m_DebondSectionLength*(sectionIdx);
   }
}

void pgsStrandDesignTool::GetDebondSectionForLocation(Float64 location, SectionIndexType* pOutBoardSectionIdx, SectionIndexType* pInBoardSectionIdx, Float64* pOutToInDistance)
{
   Float64 max_deb_loc = (m_NumDebondSections-1) * m_DebondSectionLength;
   Float64 rgt_loc = m_GirderLength - max_deb_loc;

   const Float64 fudge = 1.0e-5;
   // get distance from end
   if (rgt_loc < location+fudge)
   {
      // at right end
      location = m_GirderLength - location;
   }

   if (0.0 <= location && location < max_deb_loc+fudge)
   {
      if (location <= fudge)
      {
         // cover the case where flooring an int below gets us into trouble
         *pInBoardSectionIdx  = 1;
         *pOutBoardSectionIdx = 0;
      }
      else
      {
         *pInBoardSectionIdx = SectionIndexType((location-fudge)/m_DebondSectionLength + 1);
         *pOutBoardSectionIdx = *pInBoardSectionIdx - 1;
      }
   }
   else
   {
      ATLASSERT(0); // asking for a location outside of debond zone
      *pInBoardSectionIdx  = INVALID_INDEX;
      *pOutBoardSectionIdx = INVALID_INDEX;
   }

   // distance location to outboard section
   *pOutToInDistance = location - (*pOutBoardSectionIdx) * m_DebondSectionLength;

   // save some later grief due to tolerancing
   if (IsEqual(*pOutToInDistance,m_DebondSectionLength, fudge))
   {
      *pOutToInDistance = m_DebondSectionLength;
   }
   else if (IsZero(*pOutToInDistance, fudge))
   {
      *pOutToInDistance = 0.0;
   }

   ATLASSERT(*pOutToInDistance >= 0.0);
   ATLASSERT(*pOutToInDistance <= m_DebondSectionLength);
}


DebondLevelType pgsStrandDesignTool::GetMaxDebondLevel(StrandIndexType numStrands, SectionIndexType numLeadingSections )
{
   ATLASSERT(0 <= numStrands);
   ATLASSERT(0 <= numLeadingSections);
//   LOG("Entering GetMaxDebondLevel, numStrands = "<<numStrands<<" numLeadingSections = "<<numLeadingSections);

   Uint32 num_levels = m_DebondLevels.size();
   DebondLevelType level = 0;
   if ( 0 < numStrands && 0 < num_levels )
   {
      level = DebondLevelType(num_levels-1);
      // Find max level based on raw number of strands, and number of leading sections
      // Going backwards, as levels increase min number of strands
      for (DebondLevelReverseIterator dbit=m_DebondLevels.rbegin(); dbit!=m_DebondLevels.rend(); dbit++)
      {
         const DebondLevel& rlevel = *dbit;

         if (rlevel.MinTotalStrandsRequired <= numStrands)
         {
            // Have enough strands to work at this level, see if we have room section-wise
            StrandIndexType num_debonded = rlevel.StrandsDebonded.size();
            StrandIndexType max_debonds_at_section = max(m_MaxDebondSection, Int32(num_debonded*m_MaxPercentDebondSection) );

            // allow int to floor
            SectionIndexType leading_sections_required = (num_debonded == 0 ? 0 : SectionIndexType((num_debonded-1)/max_debonds_at_section));
            if (leading_sections_required <= numLeadingSections)
            {
               break;
            }
         }

         level--;
      }
   }
   
//   LOG("Exiting GetMaxDebondLevel, max level is = "<<level);
   ATLASSERT(0 <= level);
   return level;
}

std::vector<DebondLevelType> pgsStrandDesignTool::GetMaxPhysicalDebonding()
{
   return m_MaxPhysicalDebondLevels;
}

void pgsStrandDesignTool::RefineDebondLevels(std::vector<DebondLevelType>& rDebondLevelsAtSections)
{
   SectionIndexType num_sects = GetMaxNumberOfDebondSections();
   LOG("Entering RefineDebondLevels, max debond sections = "<<num_sects);
   LOG("List of raw levels at start "<<DumpIntVector(rDebondLevelsAtSections));
   Uint32 test_size = rDebondLevelsAtSections.size();

   if (test_size != num_sects)
   {
      // should probably never happen
      ATLASSERT(0);
      rDebondLevelsAtSections.resize(num_sects, 0);
   }

   if (0 < test_size)
   {

      // levels must increase as we approach the end of beam, make this so, if it is not
      SortDebondLevels(rDebondLevelsAtSections);

      // make sure debond termination rules are enforced
      if (! SmoothDebondLevelsAtSections(rDebondLevelsAtSections) )
      {
         LOG("Unable to debond within section rules"); //  could we increase conc strength here?
         rDebondLevelsAtSections.clear();
         return;
      }

      LOG("List of levels after smoothing     "<<DumpIntVector(rDebondLevelsAtSections));
      LOG("List of max physical debond levels "<<DumpIntVector(m_MaxPhysicalDebondLevels));

      // check levels against physical max computed in MaximizeDebonding
      ATLASSERT(m_MaxPhysicalDebondLevels.size()==num_sects);
      std::vector<DebondLevelType>::iterator mit = m_MaxPhysicalDebondLevels.begin();
      SectionIndexType sectno = 0;
      for(std::vector<DebondLevelType>::iterator it = rDebondLevelsAtSections.begin(); it != rDebondLevelsAtSections.end(); it++)
      {
         DebondLevelType debond_level_at_section = *it;
         DebondLevelType max_debond_level = *mit;

         if (max_debond_level < debond_level_at_section)
         {
            ATLASSERT(0); // remove this after testing
            LOG(" A debond level exceeds the maximum physical allowable at section "<<sectno<<" Design abort");
            rDebondLevelsAtSections.clear();
            break;
         }

         mit++;
         sectno++;
      }
   }
   else
   {
      ATLASSERT(0); // should probably always of at least one debond section by the time we get here
   }

   LOG("Exiting RefineDebondLevels");
}

bool pgsStrandDesignTool::SmoothDebondLevelsAtSections(std::vector<Int16>& rDebondLevelsAtSections)
{
   // make sure we abide to max bond terminations at a section
   // Get number of debonded strands (max level will be at end). Assuming that levels have been sorted
   DebondLevelType debond_level_at_girder_end = rDebondLevelsAtSections[0];
   StrandIndexType num_debonded = m_DebondLevels[debond_level_at_girder_end].StrandsDebonded.size();

   StrandIndexType max_db_term_at_section = max(m_MaxDebondSection, StrandIndexType(floor(num_debonded*m_MaxPercentDebondSection)));
   LOG("Max allowable debond terminations at a section = "<<max_db_term_at_section);

   // iterate from mid-girder toward end
   std::vector<DebondLevelType>::reverse_iterator rit = rDebondLevelsAtSections.rbegin();
   StrandIndexType last_num_db=0;
   while( rit != rDebondLevelsAtSections.rend())
   {
      DebondLevelType debond_level = *rit;
      ATLASSERT(0 <= debond_level && debond_level < (DebondLevelType)m_DebondLevels.size());

      StrandIndexType num_db_at_lvl = m_DebondLevels[debond_level].StrandsDebonded.size();

      StrandIndexType num_db_term = num_db_at_lvl - last_num_db; // num debonds terminated at this section

      if (max_db_term_at_section < num_db_term)
      {
         // too many strands terminated here, see if we can debond upstream
         if ( rit != rDebondLevelsAtSections.rbegin() )
         {
            // What minimum debond level can fix out problem at the adjacent (inward) section
            DebondLevelType min_adjacent_level = GetMinAdjacentDebondLevel(debond_level, max_db_term_at_section); 

            std::vector<DebondLevelType>::reverse_iterator rit2 = rit;
            rit2--;
            DebondLevelType& rlvl_prv = *rit2;

            if (rlvl_prv < min_adjacent_level)
            {
               // set previous section's level and restart loop at previous location
               rlvl_prv = min_adjacent_level;

               rit = rit2;

               if (rit2==rDebondLevelsAtSections.rbegin())
               {
                  last_num_db = 0;
               }
               else
               {
                  rit2--;
                  DebondLevelType lvl2 = *rit2;
                  last_num_db = m_DebondLevels[lvl2].StrandsDebonded.size();
               }
            }
            else
            {
               ATLASSERT(0); // This means there is a bug in the initial debond level computation. 
                             // the outermost if() should not fail if we have laid out levels properly
            }
         }
         else
         {
            // We have too much debonding at the mid-girdermost section
            // nothing we can do
            return false;
         }
      }
      else
      {
         last_num_db = num_db_at_lvl;
         rit++;
      }
   }

   return true;
}

DebondLevelType pgsStrandDesignTool::GetMinAdjacentDebondLevel(DebondLevelType currLevel, StrandIndexType maxDbsTermAtSection)
{
   // We are at a certain debond level that exceeds section debonding limits. See what 
   // level toward mid-span can get us within limits
   ATLASSERT(0 < currLevel && currLevel < (DebondLevelType)m_DebondLevels.size());

   const DebondLevel& rcurr_lvl = m_DebondLevels[currLevel];
   StrandIndexType num_db_at_curr_level = rcurr_lvl.StrandsDebonded.size();
   
   ATLASSERT(maxDbsTermAtSection <= num_db_at_curr_level); // can't terminate more strands than we have at this level. calling routine on drugs.

   // Go after minimum level that has enough strands to alleviate our problem
   DebondLevelType min_lvl = -1;
   for (DebondLevelType levelIdx = 0; levelIdx < currLevel; levelIdx++)
   {
      const DebondLevel& rlvl = m_DebondLevels[levelIdx];
      StrandIndexType num_db = rlvl.StrandsDebonded.size();

      if (num_db_at_curr_level-num_db <= maxDbsTermAtSection)
      {
         min_lvl = levelIdx;
         break;
      }
   }

   if (min_lvl == -1)
   {
      ATLASSERT(0); // something messed up with initial determination of debond levels
      min_lvl = 0;
   }

   return min_lvl;
}

bool pgsStrandDesignTool::LayoutDebonding(const std::vector<DebondLevelType>& rDebondLevelAtSections)
{
   LOG("Entering LayoutDebonding. Debond Levels = "<<DumpIntVector(rDebondLevelAtSections));
   DebondInfoCollection db_info;
   SectionIndexType num_sections_where_strands_are_debonded = rDebondLevelAtSections.size();
   if (0 < num_sections_where_strands_are_debonded)
   {
      ATLASSERT(num_sections_where_strands_are_debonded <= m_NumDebondSections);

#ifdef _DEBUG
      // check that debond levels decrease inward
      std::vector<DebondLevelType>::const_iterator it=rDebondLevelAtSections.begin();
      DebondLevelType prev_debond_level = *it;
      while ( it != rDebondLevelAtSections.end() )
      {
         DebondLevelType next_debond_level = *it;
         ATLASSERT(next_debond_level <= prev_debond_level);

         it++;
         prev_debond_level = next_debond_level;
      }
#endif

      // work from inside outward
      SectionIndexType debondSectionIndex = num_sections_where_strands_are_debonded-1;
      StrandIndexType last_num_of_debonded_strands = 0;
      for(std::vector<DebondLevelType>::const_reverse_iterator rit=rDebondLevelAtSections.rbegin(); rit!=rDebondLevelAtSections.rend(); rit++)
      {
         DebondLevelType lvl = *rit;

         const DebondLevel& rlvl = m_DebondLevels[lvl];

         StrandIndexType num_debonded_strands_this_section = rlvl.StrandsDebonded.size();

         if (last_num_of_debonded_strands < num_debonded_strands_this_section)
         {
            // we have strands to debond at this section
            Float64 debond_location_from_left_end= GetDebondSectionLocation(debondSectionIndex, dbLeft);

            LOG("Debond required at section "<<debondSectionIndex<<"at "<<::ConvertFromSysUnits(debond_location_from_left_end,unitMeasure::Feet) << " ft");

            for (StrandIndexType debondedStrandIdx = last_num_of_debonded_strands; debondedStrandIdx < num_debonded_strands_this_section; debondedStrandIdx++)
            {
               StrandIndexType strandIndex = rlvl.StrandsDebonded[debondedStrandIdx];

               DEBONDINFO debondInfo;
               debondInfo.strandIdx = strandIndex;
               debondInfo.LeftDebondLength  = debond_location_from_left_end;
               debondInfo.RightDebondLength = debond_location_from_left_end;

               LOG("   Debond " << strandIndex);

               db_info.push_back(debondInfo);
            }
         }

         debondSectionIndex--;
         last_num_of_debonded_strands = num_debonded_strands_this_section;
      }
   }

   // set to our artifact
   m_pArtifact->SetStraightStrandDebondInfo(db_info);

   LOG("Exiting LayoutDebonding");

   return true;
}

// Losses differences can cause debond to fail - fudge by:
// The fudge factors will cause a slight amount of over-debonding
static double TensDebondFudge  = 1.002;
static double ComprDebondFudge = 1.03; // fudge compression more because it's easier to get more compression strength


void pgsStrandDesignTool::GetDebondLevelForTopTension(Float64 psForcePerStrand, StrandIndexType nss, Float64 tensDemand, Float64 outboardDistance,
                                                      Float64 Yb, Float64 Ag, Float64 St,
                                                      Int16* pOutboardLevel, Int16* pInboardLevel)
{
   ATLASSERT(outboardDistance<=m_DebondSectionLength);

   if(0 < tensDemand) 
   {
      // First determine minimum level required to alleviate demand
      Uint16 level=0;
      bool found = false;
      DebondLevelIterator it = m_DebondLevels.begin();
      while(true)
      {
         level++;
         it++;
         if ( it !=m_DebondLevels.end() )
         {
           const DebondLevel& lvl = *it;

           // can only attain level with min number of strands
           if (lvl.MinTotalStrandsRequired <= nss)
           {
               // stress relief for lvl
               Float64 stress =  lvl.ComputeReliefStress(psForcePerStrand, Yb, Ag, St);

              // if this makes it by fudge, call it good enough
              if ( stress > tensDemand*TensDebondFudge)
              {
                 found = true;
                 break;
              }
           }
         }
         else
         {
            // no level that alleviates the demand
            break;
         }
      }

      if (found)
      {
         // We have a level that works, and we know that it will alleviate stress if we put it inboard of our location.
         *pOutboardLevel = level;
         *pInboardLevel  = level;

         // But, let's try a little harder to see if the transfer effect allows less debonding
         if (outboardDistance<=1.0e-5)
         {
            // location is so close to outboard section - call it fully transferred so we don't need inboard debonding
            *pInboardLevel = 0;
         }
         else if (outboardDistance < GetTransferLength(pgsTypes::Permanent))
         {
            // Location is between outboard section and transfer length. See if we can use next lower
            // level at inboard section location.
            // Note that we might try to improve this later by trying even less debonding inboard, but things are complicated enough as is.
            Float64 outb_allev =  m_DebondLevels[level].ComputeReliefStress(psForcePerStrand, Yb, Ag, St); // no fudge

            if (tensDemand < outb_allev)
            {
               // We might be below transfer line
               // Stress relief (alleviation) provided by next-lower level
               Float64 inb_allev = m_DebondLevels[level-1].ComputeReliefStress(psForcePerStrand, Yb, Ag, St);

               Float64 transfer_provided = 1.0-(outboardDistance / GetTransferLength(pgsTypes::Permanent));
            
               Float64 allev_provided = inb_allev + transfer_provided*(outb_allev-inb_allev);

               if (allev_provided > tensDemand*TensDebondFudge)
               {
                  // we can use next-lower level at inboard section
                  *pInboardLevel = level-1;
               }
            }
         }
      }
      else
      {
         // no levels can alleviate demand - return negative of max
         *pOutboardLevel = -1 * (m_DebondLevels.size()-1);
         *pInboardLevel  = *pOutboardLevel;
      }
   }
   else
   {
      ATLASSERT(0);
      *pOutboardLevel = -1;
      *pInboardLevel  = -1;
   }
}

void pgsStrandDesignTool::GetDebondLevelForBottomCompression(Float64 psForcePerStrand, StrandIndexType nss, Float64 compDemand, Float64 outboardDistance,
                                                             Float64 Yb, Float64 Ag, Float64 Sb,
                                                             Int16* pOutboardLevel, Int16* pInboardLevel)
{
   ATLASSERT(outboardDistance<=m_DebondSectionLength);

   if(compDemand < 0)
   {
      // First determine minimum level required to alleviate demand
      Uint16 level=0;
      bool found = false;
      DebondLevelIterator it = m_DebondLevels.begin();
      while(true)
      {
         level++;
         it++;
         if ( it !=m_DebondLevels.end() )
         {
           const DebondLevel& lvl = *it;

           // can only attain level with min number of strands
           if (lvl.MinTotalStrandsRequired <= nss)
           {
               // stress relief for lvl
               Float64 stress =  lvl.ComputeReliefStress(psForcePerStrand, Yb, Ag, Sb);

              // if this makes it by fudge, call it good enough
              if (stress < compDemand * ComprDebondFudge)
              {
                 found = true;
                 break;
              }
           }
         }
         else
         {
            // no level that alleviates the demand
            break;
         }
      }

      if (found)
      {
         // We have a level that works, and we know that it will alleviate stress if we put it inboard of our location.
         *pOutboardLevel = level;
         *pInboardLevel  = level;

         // But, let's try a little harder to see if the transfer effect allows less debonding
         if (outboardDistance<=1.0e-5)
         {
            // location is so close to outboard section - call it fully transferred so we don't need inboard debonding
            *pInboardLevel = 0;
         }
         else if (outboardDistance < GetTransferLength(pgsTypes::Permanent))
         {
            // Location is between outboard section and transfer length. See if we can use next lower
            // level at inboard section location.
            // Note that we might try to improve this later by trying even less debonding inboard, but things are complicated enough as is.
            Float64 outb_allev =  m_DebondLevels[level].ComputeReliefStress(psForcePerStrand, Yb, Ag, Sb); // no fudge

            if (compDemand > outb_allev)
            {
               // We might be below transfer line
               // Stress relief (alleviation) provided by next-lower level
               Float64 inb_allev = m_DebondLevels[level-1].ComputeReliefStress(psForcePerStrand, Yb, Ag, Sb);

               Float64 transfer_provided = 1.0-(outboardDistance / GetTransferLength(pgsTypes::Permanent));
            
               Float64 allev_provided = inb_allev + transfer_provided*(outb_allev-inb_allev);

               if (allev_provided < compDemand*ComprDebondFudge)
               {
                  // we can use next-lower level at inboard section
                  *pInboardLevel = level-1;
               }
            }
         }
      }
      else
      {
         // no levels can alleviate demand - return negative of max
         *pOutboardLevel = -1 * (m_DebondLevels.size()-1);
         *pInboardLevel  = *pOutboardLevel;
      }
   }
   else
   {
      ATLASSERT(0);
      *pOutboardLevel = -1;
      *pInboardLevel  = -1;
   }
}

std::vector<DebondLevelType> pgsStrandDesignTool::ComputeDebondsForDemand(const std::vector<StressDemand>& demands, StrandIndexType nss, Float64 psForcePerStrand, 
                                                                Float64 allowTens, Float64 allowComp)
{
   GET_IFACE(ISectProp2,pSectProp);

   std::vector<DebondLevelType> debond_levels;
   SectionIndexType max_db_sections = GetMaxNumberOfDebondSections();

   // set up our vector to return debond levels at each section
   debond_levels.assign(max_db_sections,0);

   for (std::vector<StressDemand>::const_iterator sit=demands.begin(); sit!=demands.end(); sit++)
   {
      const StressDemand& demand = *sit;

      LOG("Debonding design for stresses at "<<::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),unitMeasure::Feet) << " ft");
      LOG("Applied Top stress = "<< ::ConvertFromSysUnits(demand.m_TopStress,unitMeasure::KSI) << " ksi, Bottom stress = " << ::ConvertFromSysUnits(demand.m_BottomStress,unitMeasure::KSI) << " ksi.");

      // Section properties of beam - non-prismatic
      // using casting yard stage because we are computing stress on the girder due to prestress which happens in this stage
      Float64 Yb = pSectProp->GetYb(pgsTypes::CastingYard,demand.m_Poi);
      Float64 Ag = pSectProp->GetAg(pgsTypes::CastingYard,demand.m_Poi);
      Float64 St = pSectProp->GetStGirder(pgsTypes::CastingYard, demand.m_Poi);
      Float64 Sb = pSectProp->GetSb(pgsTypes::CastingYard, demand.m_Poi);

      if (demand.m_TopStress>allowTens || demand.m_BottomStress<allowComp)
      {
         // get debond increment this poi is just outside of
         SectionIndexType outboard_inc, inboard_inc;
         Float64 out_to_in_distance;
         this->GetDebondSectionForLocation(demand.m_Poi.GetDistFromStart(), &outboard_inc, &inboard_inc, &out_to_in_distance);
         ATLASSERT(outboard_inc>=0 && outboard_inc<max_db_sections);
         ATLASSERT(inboard_inc>=0 && inboard_inc<max_db_sections);

         // compute debond level required to alleviate stresses at top
         DebondLevelType out_db_level(0), in_db_level(0);
         if (allowTens < demand.m_TopStress)
         {
            // debonding needs to reduce stress by this much
            Float64 tens_demand = demand.m_TopStress - allowTens;

            DebondLevelType out_top_db_level, in_top_db_level;
            this->GetDebondLevelForTopTension(psForcePerStrand, nss, tens_demand, out_to_in_distance, Yb, Ag, St,
                                              &out_top_db_level, &in_top_db_level);

            LOG("Debonding needed to control top tensile overstress of " << ::ConvertFromSysUnits(tens_demand,unitMeasure::KSI) << " KSI at "<<::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),unitMeasure::Feet) << " ft. Outboard level required was "<< out_top_db_level<<" Inboard level required was "<< in_top_db_level);

            if (out_top_db_level < 0)
            {
               ATLASSERT(0); // algorithm should avoid this
               LOG("Debond design failed at location - continue, but failure is likely");
               out_top_db_level *= -1;
            }

            if (in_top_db_level < 0)
            {
               ATLASSERT(0); // algorithm should avoid this
               LOG("Debond design failed at location - continue, but failure is likely");
               in_top_db_level *= -1;
            }

            out_db_level = max(out_db_level, out_top_db_level);
            in_db_level  = max(in_db_level,  in_top_db_level);
         }

         if (demand.m_BottomStress < allowComp)
         {
            // debonding needs to reduce stress by this much
            Float64 comp_demand = demand.m_BottomStress - allowComp;

            DebondLevelType out_bot_db_level, in_bot_db_level;
            this->GetDebondLevelForBottomCompression(psForcePerStrand, nss, comp_demand, out_to_in_distance, Yb, Ag, Sb,
                                                     &out_bot_db_level, &in_bot_db_level);

            LOG("Debonding needed to control bottom compressive overstress of " << ::ConvertFromSysUnits(comp_demand,unitMeasure::KSI) << " KSI at "<<::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),unitMeasure::Feet) << " ft. Outboard level required was "<< out_bot_db_level<<" Inboard level required was "<< in_bot_db_level);

            if (out_bot_db_level <0 )
            {
               ATLASSERT(0); // algorithm should avoid this
               LOG("Debond design failed at location - continue, but failure is likely");
               out_bot_db_level *= -1;
            }

            if (in_bot_db_level < 0)
            {
               ATLASSERT(0); // algorithm should avoid this
               LOG("Debond design failed at location - continue, but failure is likely");
               in_bot_db_level *= -1;
            }

            out_db_level = max(out_db_level, out_bot_db_level);
            in_db_level  = max(in_db_level,  in_bot_db_level);

         }

         // replace level at current location only if new is larger
         DebondLevelType curr_lvl = debond_levels[outboard_inc];
         if (curr_lvl < out_db_level)
         {
            debond_levels[outboard_inc] = out_db_level;
         }

         curr_lvl = debond_levels[inboard_inc];
         if (curr_lvl < in_db_level)
         {
            debond_levels[inboard_inc] = in_db_level;
         }
      }
   }

   LOG("Smooth out our vector before we return it");
   RefineDebondLevels(debond_levels);

   return debond_levels;
}