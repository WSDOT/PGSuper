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

#ifndef INCLUDED_ANALYSISRESULTSGRAPHDEFINITION_H_
#define INCLUDED_ANALYSISRESULTSGRAPHDEFINITION_H_
#pragma once

#include <Graphing\AnalysisResultsGraphBuilder.h>
#include <Graphing\GraphingTypes.h>

#include <IFace\AnalysisResults.h>
#include <set>

// this load case stuff is a pinta. here's a little class to help things out
#define ACTIONS_AXIAL_ONLY        0x0001
#define ACTIONS_SHEAR_ONLY        0x0002
#define ACTIONS_MOMENT_ONLY       0x0004
#define ACTIONS_STRESS_ONLY       0x0008
#define ACTIONS_DEFLECTION_ONLY   0x0010

#define ACTIONS_MOMENT_SHEAR        ACTIONS_MOMENT_ONLY  | ACTIONS_SHEAR_ONLY
#define ACTIONS_FORCE_DEFLECTION    ACTIONS_AXIAL_ONLY | ACTIONS_MOMENT_SHEAR | ACTIONS_DEFLECTION_ONLY
#define ACTIONS_FORCE_STRESS        ACTIONS_AXIAL_ONLY | ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS_ONLY
#define ACTIONS_ALL                 ACTIONS_AXIAL_ONLY | ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS_ONLY | ACTIONS_DEFLECTION_ONLY 

class CAnalysisResultsGraphDefinition
{
public:

   IDType m_ID;
   std::_tstring m_Name;
   GraphType m_GraphType;

   union LoadType 
   {
      pgsTypes::LimitState         LimitStateType;
      pgsTypes::ProductForceType             ProductLoadType;
      LoadingCombinationType       CombinedLoadType;
      pgsTypes::LiveLoadType       LiveLoadType;
   } m_LoadType;

   std::set<IntervalIndexType> m_IntervalApplicability; // intervals that this graph is applicable to
   int m_ApplicableActions; // 0 = all, 1 = Forces Only, 2 = Stress Only
   VehicleIndexType m_VehicleIndex;

   CAnalysisResultsGraphDefinition();
   
   // constructor for limit states
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                pgsTypes::LimitState ls,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for combinations
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                LoadingCombinationType comb,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for product loads
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                pgsTypes::ProductForceType type,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for live loads
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,
                const std::vector<IntervalIndexType>& intervals,int actions);
   
   // constructor for deck shrinkage stress
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,GraphType type,const std::vector<IntervalIndexType>& intervals);
   
   // constructor for demands
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals);

   // constructor for vehicular live loads
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<IntervalIndexType>& intervals,int apaction);

   // constructor for ultimate forces
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals,int apaction);
   CAnalysisResultsGraphDefinition(IDType id,const std::_tstring& name,pgsTypes::LiveLoadType llType,const std::vector<IntervalIndexType>& intervals,int apaction);

   void AddIntervals(const std::vector<IntervalIndexType>& intervals);

   bool operator< (const CAnalysisResultsGraphDefinition& rOther) const
   {
      return m_ID < rOther.m_ID;
   }
};

class CAnalysisResultsGraphDefinitions
{
public:
   CAnalysisResultsGraphDefinitions();

   void AddGraphDefinition(const CAnalysisResultsGraphDefinition& def);
   CAnalysisResultsGraphDefinition& GetGraphDefinition(IDType graphID);
   const CAnalysisResultsGraphDefinition& GetGraphDefinition(IDType graphID) const;
   IndexType GetGraphIndex(IDType graphID) const;
   void RemoveGraphDefinition(IDType graphID);

   std::_tstring GetDefaultLoadCase(IntervalIndexType intervalIdx) const;
   
   std::vector< std::pair<std::_tstring,IDType> > GetLoadings(IntervalIndexType intervalIdx, ActionType action) const;

   void Clear();

private:
   typedef std::set<CAnalysisResultsGraphDefinition> GraphDefinitionContainer;
   typedef GraphDefinitionContainer::iterator GraphDefinitionIterator;
   typedef GraphDefinitionContainer::const_iterator ConstGraphDefinitionIterator;
   GraphDefinitionContainer m_Definitions;
};

#endif //  INCLUDED_ANALYSISRESULTSGRAPHDEFINITION_H_