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

#include "StdAfx.h"
#include <Reporting\ReactionInterfaceAdapters.h>
#include <sstream>
#include <PgsExt\StageCompare.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// MISCELLANEOUS
//
inline bool DoDoReportAtPier(pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr, SpanIndexType currSpan, IBearingDesign* pointer)
{
   if (pier < currSpan || pier > currSpan+1)
   {
      ATLASSERT(0); 
      return false;
   }
   else
   {
      bool bleft, bright;
      pointer->AreBearingReactionsAvailable(stage,currSpan, gdr, &bleft, &bright);
      if (pier==currSpan)
      {
         return bleft;
      }
      else
      {
         return bright;
      }
   }
}

inline void LabelPier(std::_tostringstream& os, PierIndexType pier, PierIndexType nPiers, PierReactionFaceType face)
{
   if (pier == 0 || pier == nPiers-1 )
   {
      os << _T("Abutment ") << LABEL_PIER(pier);
   }
   else
   {
      os << _T("Pier ") << LABEL_PIER(pier);
   }

   if (face == rftBack)
   {
      os << _T(" - Back");
   }
   else if (face == rftAhead)
   {
      os << _T(" - Ahead");
   }
}         

inline ReactionLocation MakeReactionLocation(PierIndexType pier, PierIndexType nPiers, PierReactionFaceType face, GirderIndexType girder)
{
   ReactionLocation location;
   location.Pier = pier;
   location.Face = face;
   location.Girder = girder; // analysis agent should sort out if pier has different # girders framing into it

   std::_tostringstream os;
   LabelPier(os, pier, nPiers, location.Face);

   location.PierLabel = os.str();

   return location;
}

inline ReactionLocationContainer GetBearingReactionLocations(pgsTypes::Stage stage,SpanIndexType span, GirderIndexType girder, 
                                                          IBridge* pBridge, IBearingDesign* pBearing)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();
   SpanIndexType nSpans = nPiers-1;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpan   = (span == ALL_SPANS ? nSpans : startSpan+1);

   for ( SpanIndexType spanIdx = startSpan; spanIdx < endSpan; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (girder < nGirders ? girder : nGirders-1);

      bool bleft, bright;
      pBearing->AreBearingReactionsAvailable(stage, spanIdx, gdrIdx, &bleft, &bright);
      if( bleft )
      {
         ReactionLocation location = MakeReactionLocation(spanIdx, nPiers, rftAhead, gdrIdx);
         container.push_back( location );
      }

      if( bright)
      {
         ReactionLocation location = MakeReactionLocation(spanIdx+1, nPiers, rftBack, gdrIdx);
         container.push_back( location );
      }
   }

   return container;
}

inline ReactionLocationContainer GetPierReactionLocations(SpanIndexType span, GirderIndexType girder, IBridge* pBridge)
{
   ReactionLocationContainer container;

   PierIndexType nPiers = pBridge->GetPierCount();
   if (span == ALL_SPANS)
   {
      for (PierIndexType pier=0; pier<nPiers; pier++)
      {
         ReactionLocation location = MakeReactionLocation(pier, nPiers, rftMid, girder);
         container.push_back( location );
      }
   }
   else
   {
      // For one span we have two piers
      PierIndexType pier = span;
      ReactionLocation location = MakeReactionLocation(pier, nPiers, rftMid, girder);
      container.push_back( location );

      pier = span+1;
      location = MakeReactionLocation(pier, nPiers, rftMid, girder);
      container.push_back( location );
   }

   return container;
}

/****************************************************************************
CLASS
   ProductForcesReactionAdapter
   A simple iterator class for walking reaction locations
****************************************************************************/
ReactionLocationIter::ReactionLocationIter(const ReactionLocationContainer& container):
   m_rContainer(container)
{
   m_Iter = m_rContainer.begin();
}

void ReactionLocationIter::First()
{
   m_Iter = m_rContainer.begin();
}

void ReactionLocationIter::Next()
{
   m_Iter++;
}

bool ReactionLocationIter::IsDone()
{
   return m_Iter==m_rContainer.end();
}

const ReactionLocation& ReactionLocationIter::CurrentItem()
{
   ATLASSERT(!IsDone());
   return *m_Iter;
}

/****************************************************************************
CLASS
   ProductForcesReactionAdapter
****************************************************************************/

ProductForcesReactionAdapter::ProductForcesReactionAdapter(IProductForces* pForces, SpanIndexType span, GirderIndexType girder):
   m_Pointer(pForces), m_Span(span), m_Girder(girder)
{
}

ReactionLocationIter ProductForcesReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   if (m_Locations.empty())
   {
      m_Locations = GetPierReactionLocations(m_Span, m_Girder, pBridge);
   }

   return ReactionLocationIter(m_Locations);
}

bool ProductForcesReactionAdapter::DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
{
   return true; // always report pier reactions for all piers
}

Float64 ProductForcesReactionAdapter::GetReaction(pgsTypes::Stage stage,const ReactionLocation& rLocation,ProductForceType type,BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_Pointer->GetReaction(stage, type, rLocation.Pier, rLocation.Girder, bat);
}

void ProductForcesReactionAdapter::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage, const ReactionLocation& rLocation,BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   ATLASSERT(rLocation.Face == rftMid);

   m_Pointer->GetLiveLoadReaction(llType, stage, rLocation.Pier, rLocation.Girder, bat, bIncludeImpact, bIncludeLLDF, pRmin, pRmax, pMinConfig, pMaxConfig);
}

/****************************************************************************
CLASS
   BearingDesignProductReactionAdapter
****************************************************************************/
BearingDesignProductReactionAdapter::BearingDesignProductReactionAdapter(IBearingDesign* pForces, pgsTypes::Stage stage, SpanIndexType span, GirderIndexType girder):
   m_pBearingDesign(pForces),
   m_Span(span),
   m_Girder(girder),
   m_Stage(stage)
{
}

ReactionLocationIter BearingDesignProductReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = GetBearingReactionLocations(m_Stage, m_Span, m_Girder, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool BearingDesignProductReactionAdapter::DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
{
   return DoDoReportAtPier(m_Stage, pier, gdr, m_Span, m_pBearingDesign);
}

Float64 BearingDesignProductReactionAdapter::GetReaction(pgsTypes::Stage stage,const ReactionLocation& rLocation,ProductForceType type,BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

   SpanIndexType span = (rLocation.Face==rftAhead) ? rLocation.Pier : rLocation.Pier-1;

   Float64 Rleft, Rright;
   m_pBearingDesign->GetBearingProductReaction(stage, type, span, rLocation.Girder, ctIncremental, bat, &Rleft, &Rright);

   return rLocation.Face==rftAhead ? Rleft : Rright;
}

void BearingDesignProductReactionAdapter::GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage, const ReactionLocation& rLocation,BridgeAnalysisType bat,
                                                       bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                                       VehicleIndexType* pMinConfig, VehicleIndexType* pMaxConfig)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

 // convert from span to pier locations
  Float64 LeftRmin, LeftRmax, LeftTmin, LeftTmax;
  Float64 RightRmin, RightRmax, RightTmin, RightTmax;
  VehicleIndexType LeftMinConfig , LeftMaxConfig;
  VehicleIndexType RightMinConfig, RightMaxConfig;

  SpanIndexType span = (rLocation.Face==rftAhead) ? rLocation.Pier : rLocation.Pier-1;

  m_pBearingDesign->GetBearingLiveLoadReaction(llType, stage, span, rLocation.Girder, bat, bIncludeImpact, bIncludeLLDF, 
                                        &LeftRmin, &LeftRmax, &LeftTmin, &LeftTmax, &RightRmin, &RightRmax, &RightTmin, &RightTmax,
                                        &LeftMinConfig, &LeftMaxConfig, &RightMinConfig, &RightMaxConfig);

  if(rLocation.Face==rftAhead)
  {
      *pRmin = LeftRmin;
      *pRmax = LeftRmax;
      if (pMinConfig!=NULL)
      {
         *pMinConfig = LeftMinConfig;
         *pMaxConfig = LeftMaxConfig;
      }
  }
  else
  {
      *pRmin = RightRmin;
      *pRmax = RightRmax;
      if (pMinConfig!=NULL)
      {
         *pMinConfig = RightMinConfig;
         *pMaxConfig = RightMaxConfig;
      }
  }
}

/////////////////////////////////////////////
// class CmbLsBearingDesignReactionAdapter
/////////////////////////////////////////////

CombinedLsForcesReactionAdapter::CombinedLsForcesReactionAdapter(ICombinedForces* pCmbForces, ILimitStateForces* pForces, SpanIndexType span, GirderIndexType girder):
   m_CmbPointer(pCmbForces), m_LsPointer(pForces), m_Span(span), m_Girder(girder)
{;}

ReactionLocationIter CombinedLsForcesReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   if (m_Locations.empty())
   {
      m_Locations = GetPierReactionLocations(m_Span, m_Girder, pBridge);
   }

   return ReactionLocationIter(m_Locations);
}

bool CombinedLsForcesReactionAdapter::DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
{
   return true; // always report for pier reactions
}

Float64 CombinedLsForcesReactionAdapter::GetReaction(LoadingCombination combo,pgsTypes::Stage stage,const ReactionLocation& rLocation,CombinationType type,BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_CmbPointer->GetReaction(combo, stage, rLocation.Pier, rLocation.Girder, type, bat);
}

void CombinedLsForcesReactionAdapter::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const ReactionLocation& rLocation,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_CmbPointer->GetCombinedLiveLoadReaction(llType, stage, rLocation.Pier, rLocation.Girder, bat, bIncludeImpact, pRmin, pRmax);
}

// From ILimitStateForces
void CombinedLsForcesReactionAdapter::GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,const ReactionLocation& rLocation,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   ATLASSERT(rLocation.Face == rftMid);

   return m_LsPointer->GetReaction(ls, stage, rLocation.Pier, rLocation.Girder, bat, bIncludeImpact, pMin, pMax);
}

/////////////////////////////////////////////
// class CmbLsBearingDesignReactionAdapter
/////////////////////////////////////////////
CmbLsBearingDesignReactionAdapter::CmbLsBearingDesignReactionAdapter(IBearingDesign* pForces, pgsTypes::Stage stage, SpanIndexType span, GirderIndexType girder):
   m_pBearingDesign(pForces),
   m_Span(span),
   m_Girder(girder),
   m_Stage(stage)
{
}

ReactionLocationIter CmbLsBearingDesignReactionAdapter::GetReactionLocations(IBridge* pBridge)
{
   m_Locations = GetBearingReactionLocations(m_Stage, m_Span, m_Girder, pBridge, m_pBearingDesign);

   return ReactionLocationIter(m_Locations);
}

bool CmbLsBearingDesignReactionAdapter::DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
{
   return DoDoReportAtPier(m_Stage, pier, gdr, m_Span, m_pBearingDesign);
}

Float64 CmbLsBearingDesignReactionAdapter::GetReaction(LoadingCombination combo,pgsTypes::Stage stage,const ReactionLocation& rLocation,CombinationType type,BridgeAnalysisType bat)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

   SpanIndexType span = (rLocation.Face==rftAhead) ? rLocation.Pier : rLocation.Pier-1;

   Float64 Rleft, Rright;
   m_pBearingDesign->GetBearingCombinedReaction(combo, stage, span, rLocation.Girder, type, bat, &Rleft, &Rright);

   return rLocation.Face==rftAhead ? Rleft : Rright;
}

void CmbLsBearingDesignReactionAdapter::GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,const ReactionLocation& rLocation,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

  SpanIndexType span = (rLocation.Face==rftAhead) ? rLocation.Pier : rLocation.Pier-1;

  Float64 LeftRmin, LeftRmax;
  Float64 RightRmin, RightRmax;
  m_pBearingDesign->GetBearingCombinedLiveLoadReaction(llType, stage, span, rLocation.Girder, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);

  if(rLocation.Face==rftAhead)
  {
     *pRmin = LeftRmin;
     *pRmax = LeftRmax;
  }
  else
  {
     *pRmin = RightRmin;
     *pRmax = RightRmax;
  }
}

void CmbLsBearingDesignReactionAdapter::GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,const ReactionLocation& rLocation,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   ATLASSERT(rLocation.Face != rftMid); // bearings are on sides

  SpanIndexType span = (rLocation.Face==rftAhead) ? rLocation.Pier : rLocation.Pier-1;

   Float64 LeftRmin, LeftRmax;
   Float64 RightRmin, RightRmax;
   m_pBearingDesign->GetBearingLimitStateReaction(ls, stage, span, rLocation.Girder, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);

   if(rLocation.Face==rftAhead)
   {
      *pRmin = LeftRmin;
      *pRmax = LeftRmax;
   }
   else
   {
      *pRmin = RightRmin;
      *pRmax = RightRmax;
   }
}


/////////////////////////////////////////////
// class ReactionDecider
/////////////////////////////////////////////
ReactionDecider::ReactionDecider(ReactionTableType tableType, const ReactionLocation& location,
                                 IBridge* pBridge)
{
   // full pier reactions are always reported
   if (tableType==PierReactionsTable)
   {
      m_AlwaysReport = true;
   }
   else
   {
      // Always report bearing data if simple supports always
      bool bIntegralOnLeft, bIntegralOnRight;
      pBridge->IsIntegralAtPier(location.Pier, &bIntegralOnLeft, &bIntegralOnRight);
      bool bContinuousOnLeft, bContinuousOnRight;
      pBridge->IsContinuousAtPier(location.Pier,&bContinuousOnLeft,&bContinuousOnRight);

      bool isSimple(true);
      if(location.Face==rftBack)
      {
         isSimple = !(bIntegralOnLeft || bContinuousOnLeft);
      }
      else if(location.Face==rftAhead)
      {
         isSimple = !(bIntegralOnRight || bContinuousOnRight);
      }
      else
      {
         ATLASSERT(0); // should not happen for bearing location
      }

      if (isSimple)
      {
         m_AlwaysReport = true;
      }
      else
      {
         m_AlwaysReport = false; // when we report is based on stage when BC becomes continuous

         pgsTypes::Stage back_continunity_stage, ahead_continuity_stage;
         pBridge->GetContinuityStage(location.Pier, &back_continunity_stage, &ahead_continuity_stage);

         m_ThresholdStage = location.Face==rftBack ? back_continunity_stage : ahead_continuity_stage;
      }
   }
}

// If true, report results
bool ReactionDecider::DoReport(pgsTypes::Stage stage)
{
   if (m_AlwaysReport)
   {
      return true;
   }
   else
   {
      return -1 == StageCompare(stage, m_ThresholdStage); 
   }
}
