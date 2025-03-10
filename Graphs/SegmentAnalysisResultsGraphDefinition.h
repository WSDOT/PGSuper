///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#pragma once

#include <Graphs\SegmentAnalysisResultsGraphBuilder.h>
#include <Graphs\GraphTypes.h>

#include <IFace\AnalysisResults.h>
#include <set>

// this load case stuff is a pinta. here's a little class to help things out
#define ACTIONS_AXIAL              0x0001
#define ACTIONS_SHEAR              0x0002
#define ACTIONS_MOMENT             0x0004
#define ACTIONS_STRESS             0x0008
#define ACTIONS_DEFLECTION         0x0010
#define ACTIONS_X_DEFLECTION       0x0020
#define ACTIONS_LOAD_RATING        0x0040
#define ACTIONS_REACTION           0x0080
#define ACTIONS_WEB_STRESS         0x0100 // prinicipal tension stress in webs

#define ACTIONS_MOMENT_SHEAR        ACTIONS_MOMENT  | ACTIONS_SHEAR
#define ACTIONS_FORCE_DEFLECTION    ACTIONS_AXIAL | ACTIONS_MOMENT_SHEAR | ACTIONS_DEFLECTION
#define ACTIONS_FORCE_STRESS        ACTIONS_AXIAL | ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS
#define ACTIONS_ALL                 ACTIONS_AXIAL | ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS | ACTIONS_DEFLECTION | ACTIONS_REACTION
#define ACTIONS_ALL_NO_REACTION     ACTIONS_AXIAL | ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS | ACTIONS_DEFLECTION

class CSegmentAnalysisResultsGraphDefinition
{
public:

   IDType m_ID;
   std::_tstring m_Name;
   GraphType m_GraphType;

   union LoadType 
   {
      pgsTypes::LimitState         LimitStateType;
      pgsTypes::ProductForceType   ProductLoadType;
      LoadingCombinationType       CombinedLoadType;
      pgsTypes::LiveLoadType       LiveLoadType;
   } m_LoadType;

   std::set<IntervalIndexType> m_IntervalApplicability; // intervals that this graph is applicable to
   int m_ApplicableActions; // 0 = all, 1 = Forces Only, 2 = Stress Only
   VehicleIndexType m_VehicleIndex;

   ActionType m_RatingAction; // only used with rating factor graphs

   CSegmentAnalysisResultsGraphDefinition();
   
   // constructor for limit states
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                pgsTypes::LimitState ls,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for combinations
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                LoadingCombinationType comb,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for product loads
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                pgsTypes::ProductForceType type,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for live loads
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for deck shrinkage stress
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,GraphType type,const std::vector<IntervalIndexType>& intervals);
   
   // constructor for demands
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals);

   // constructor for vehicular live loads
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<IntervalIndexType>& intervals,int apaction);

   // constructor for ultimate forces
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals,int apaction);
   CSegmentAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LiveLoadType llType,const std::vector<IntervalIndexType>& intervals,int apaction);

   // constructor for load rating 
   CSegmentAnalysisResultsGraphDefinition(IDType id, const std::_tstring& name, pgsTypes::LimitState lstype, GraphType lctype, ActionType ratingAction, VehicleIndexType vehicleIdx, const std::vector<IntervalIndexType>& intervals);

   void AddIntervals(const std::vector<IntervalIndexType>& intervals);

   bool operator< (const CSegmentAnalysisResultsGraphDefinition& rOther) const
   {
      return m_ID < rOther.m_ID;
   }
};

class CSegmentAnalysisResultsGraphDefinitions
{
public:
   CSegmentAnalysisResultsGraphDefinitions();

   void AddGraphDefinition(const CSegmentAnalysisResultsGraphDefinition& def);
   CSegmentAnalysisResultsGraphDefinition& GetGraphDefinition(IDType graphID);
   const CSegmentAnalysisResultsGraphDefinition& GetGraphDefinition(IDType graphID) const;
   IndexType GetGraphIndex(IDType graphID) const;
   void RemoveGraphDefinition(IDType graphID);

   std::_tstring GetDefaultLoadCase(IntervalIndexType intervalIdx) const;
   
   std::vector< std::pair<std::_tstring,IDType> > GetLoadings(IntervalIndexType intervalIdx, ActionType action) const;

   void Clear();

private:
   typedef std::map<IDType,CSegmentAnalysisResultsGraphDefinition> GraphDefinitionContainer;
   typedef GraphDefinitionContainer::iterator GraphDefinitionIterator;
   typedef GraphDefinitionContainer::const_iterator ConstGraphDefinitionIterator;
   GraphDefinitionContainer m_Definitions;
};
