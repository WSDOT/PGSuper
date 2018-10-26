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

#ifndef INCLUDED_REACTIONINTERFACEADAPTERS_H_
#define INCLUDED_REACTIONINTERFACEADAPTERS_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PGSuperTypes.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
inline bool DoDoReportAtPier(PierIndexType pier,GirderIndexType gdr, SpanIndexType currSpan, IBearingDesign* pointer)
{
   if (pier < currSpan || pier > currSpan+1)
   {
      return false;
   }
   else
   {
      bool bleft, bright;
      pointer->AreBearingReactionsAvailable(currSpan, gdr, &bleft, &bright);
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

// The adapters in this file allow tables to serve double duty by reporting pier reactions or girder bearing reactions.
// The two are identical except for the title and the interfaces they use to get responses
enum TableType { PierReactionsTable, BearingReactionsTable};

/*****************************************************************************
CLASS 
   IProductReactionAdapter

   Utility adapter class for redirecting reaction interfaces so they can be used by the same 
   reporting clients

DESCRIPTION

COPYRIGHT
   Copyright © 1997-2011
   Washington State Department Of Transportation
   All Rights Reserved

LOG
*****************************************************************************/
/****************************************************************************
CLASS
   IProductReactionAdapter
****************************************************************************/
// Use a local adapter so we can use either the IProductForces or IBearingDesign interfaces
// to provide results for the table.
// First the pure virtual adapter class:
class IProductReactionAdapter
{
public:
   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)=0;
   virtual Float64 GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat) = 0;
   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig = NULL,VehicleIndexType* pMaxConfig = NULL) = 0;
};

// Next implementation classes:
class ProductForcesReactionAdapter: public IProductReactionAdapter
{
public:
   ProductForcesReactionAdapter(IProductForces* pForces):
      m_Pointer(pForces)
   {;}

   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
   {
      return true; // always report pier reactions
   }

   virtual Float64 GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat)
   {
      return m_Pointer->GetReaction(stage, type, pier, gdr, bat);
   }

   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,
                                    bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                    VehicleIndexType* pMinConfig=NULL, VehicleIndexType* pMaxConfig=NULL)
   {
      m_Pointer->GetLiveLoadReaction(llType, stage, pier, gdr, bat, bIncludeImpact, bIncludeLLDF, pRmin, pRmax, pMinConfig, pMaxConfig);
   }

private:
   IProductForces* m_Pointer;
};

// Adapter to get bearing reactions. Note we have to play a game here because IProductForces wants reactions by pier
// and IBearingDesign returns them by span
class BearingDesignProductReactionAdapter: public IProductReactionAdapter
{
public:
   BearingDesignProductReactionAdapter(IBearingDesign* pForces, SpanIndexType span):
      m_Pointer(pForces),
      m_Span(span)
   {
      ATLASSERT(span!=ALL_SPANS); // Tables are not set up to deal with bearing reactions across all spans
   }

   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
   {
      return DoDoReportAtPier(pier, gdr, m_Span, m_Pointer);
   }

   virtual Float64 GetReaction(pgsTypes::Stage stage,ProductForceType type,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat)
   {
      ATLASSERT(pier==m_Span || pier==m_Span+1); // bearing reactions are by span.
      Float64 Rleft, Rright;
      m_Pointer->GetBearingProductReaction(stage, type, m_Span, gdr, ctIncremental, bat, &Rleft, &Rright);
      return pier==m_Span ? Rleft : Rright;
   }

   virtual void GetLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,
                                    bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,
                                    VehicleIndexType* pMinConfig=NULL, VehicleIndexType* pMaxConfig=NULL)
   {
    // convert from span to pier locations
     Float64 LeftRmin, LeftRmax, LeftTmin, LeftTmax;
     Float64 RightRmin, RightRmax, RightTmin, RightTmax;
     VehicleIndexType LeftMinConfig , LeftMaxConfig;
     VehicleIndexType RightMinConfig, RightMaxConfig;

     m_Pointer->GetBearingLiveLoadReaction(llType, stage, m_Span, gdr, bat, bIncludeImpact, bIncludeLLDF, 
                                           &LeftRmin, &LeftRmax, &LeftTmin, &LeftTmax, &RightRmin, &RightRmax, &RightTmin, &RightTmax,
                                           &LeftMinConfig, &LeftMaxConfig, &RightMinConfig, &RightMaxConfig);

     if(pier==m_Span)
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

private:
   IBearingDesign* m_Pointer;
   SpanIndexType m_Span;
};



/*****************************************************************************
CLASS 
   ICmbLsReactionAdapter

   Utility adapter class for redirecting reaction interfaces so they can be used by the same 
   reporting clients

DESCRIPTION

COPYRIGHT
   Copyright © 1997-2011
   Washington State Department Of Transportation
   All Rights Reserved

LOG
*****************************************************************************/
/****************************************************************************
CLASS
   ICmbLsReactionAdapter
****************************************************************************/
// Use a local adapter so we can use either the ICombinedForces or ILimitStateForces interfaces
// to provide results for the table.
// First the pure virtual adapter class:
class ICmbLsReactionAdapter
{
public:
   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)=0;

   // From ICombinedForces
   virtual Float64 GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat) = 0;
   virtual void GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) = 0;

   // From ILimitStateForces
   virtual void GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) = 0;
};

class CombinedLsForcesReactionAdapter: public ICmbLsReactionAdapter
{
public:
   CombinedLsForcesReactionAdapter(ICombinedForces* pCmbForces, ILimitStateForces* pForces):
      m_CmbPointer(pCmbForces), m_LsPointer(pForces)
   {;}

   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
   {
      return true; // always report for pier reactions
   }


   virtual Float64 GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat)
   {
      return m_CmbPointer->GetReaction(combo, stage, pier, gdr, type, bat);
   }

   virtual void GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
   {
      return m_CmbPointer->GetCombinedLiveLoadReaction(llType, stage, pier, gdr, bat, bIncludeImpact, pRmin, pRmax);
   }

   // From ILimitStateForces
   virtual void GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
   {
      return m_LsPointer->GetReaction(ls, stage, pier, gdr, bat, bIncludeImpact, pMin, pMax);
   }


private:
   ICombinedForces*   m_CmbPointer;
   ILimitStateForces* m_LsPointer;
};


// Adapter to get bearing reactions. Note we have to play a game here because IProductForces wants reactions by pier
// and IBearingDesign returns them by span
class CmbLsBearingDesignReactionAdapter: public ICmbLsReactionAdapter
{
public:
   CmbLsBearingDesignReactionAdapter(IBearingDesign* pForces, SpanIndexType span):
      m_Pointer(pForces),
      m_Span(span)
   {
      ATLASSERT(span!=ALL_SPANS); // Tables are not set up to deal with bearing reactions across all spans
   }

   virtual bool DoReportAtPier(PierIndexType pier,GirderIndexType gdr)
   {
      return DoDoReportAtPier(pier, gdr, m_Span, m_Pointer);
   }

   virtual Float64 GetReaction(LoadingCombination combo,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,CombinationType type,BridgeAnalysisType bat)
   {
      ATLASSERT(pier==m_Span || pier==m_Span+1); // bearing reactions are by span.
      Float64 Rleft, Rright;
      m_Pointer->GetBearingCombinedReaction(combo, stage, m_Span, gdr, type, bat, &Rleft, &Rright);
      return pier==m_Span ? Rleft : Rright;
   }

   virtual void GetCombinedLiveLoadReaction(pgsTypes::LiveLoadType llType,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
   {
     Float64 LeftRmin, LeftRmax;
     Float64 RightRmin, RightRmax;
     m_Pointer->GetBearingCombinedLiveLoadReaction(llType, stage, m_Span, gdr, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);

     if(pier==m_Span)
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

   virtual void GetReaction(pgsTypes::LimitState ls,pgsTypes::Stage stage,PierIndexType pier,GirderIndexType gdr,BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
   {
      Float64 LeftRmin, LeftRmax;
      Float64 RightRmin, RightRmax;
      m_Pointer->GetBearingLimitStateReaction(ls, stage, m_Span, gdr, bat, bIncludeImpact, &LeftRmin, &LeftRmax, &RightRmin, &RightRmax);
      if(pier==m_Span)
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

private:
   IBearingDesign* m_Pointer;
   SpanIndexType m_Span;
};




#endif // INCLUDED_REACTIONINTERFACEADAPTERS_H_
