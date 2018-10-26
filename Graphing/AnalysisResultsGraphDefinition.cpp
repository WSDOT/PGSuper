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

#include "stdafx.h"
#include "AnalysisResultsGraphDefinition.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition()
{
}

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
pgsTypes::LimitState ls,
const std::vector<IntervalIndexType>& intervals,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLimitState;
   m_LoadType.LimitStateType = ls;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for combinations
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
LoadingCombination comb,
const std::vector<IntervalIndexType>& intervals,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphCombined;
   m_LoadType.CombinedLoadType = comb;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for product loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
ProductForceType type,
const std::vector<IntervalIndexType>& intervals,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphProduct;
   m_LoadType.ProductLoadType = type;

   m_IntervalApplicability.insert(intervals.begin(),intervals.end());
   
   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
const std::vector<IntervalIndexType>& intervals,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for prestress
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
GraphType type,
const std::vector<IntervalIndexType>& intervals,
COLORREF c
): m_ID(id),m_Name(name),m_GraphType(type),m_Color(c)
{
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());
}

// constructor for demands
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
pgsTypes::LimitState lstype,
GraphType grtype,
const std::vector<IntervalIndexType>& intervals,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = ACTIONS_STRESS_ONLY;
   m_Color = c;
}

// constructor for vehicular live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
pgsTypes::LiveLoadType llType,
VehicleIndexType vehicleIndex,
const std::vector<IntervalIndexType>& intervals,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphVehicularLiveLoad;
   m_LoadType.LiveLoadType = llType;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
   m_Color = c;
   m_VehicleIndex = vehicleIndex;
}

// constructor for ultimate forces
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
pgsTypes::LimitState lstype,
GraphType grtype,
const std::vector<IntervalIndexType>& intervals,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
   m_Color = c;
}

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const CString name,
pgsTypes::LiveLoadType llType,
const std::vector<IntervalIndexType>& intervals,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;
   m_LoadType.LiveLoadType = llType;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
   m_Color = c;
   m_VehicleIndex = INVALID_INDEX; // not a specific vehicle, but rather an envelope
}

void CAnalysisResultsGraphDefinition::AddIntervals(const std::vector<IntervalIndexType>& intervals)
{
   // add new intervals to vector
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());
}

//////////////////////////////////////////////////////////////////////
CAnalysisResultsGraphDefinitions::CAnalysisResultsGraphDefinitions()
{
}

void CAnalysisResultsGraphDefinitions::AddGraphDefinition(const CAnalysisResultsGraphDefinition& def)
{
   m_Definitions.insert(def);
}

CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(IDType graphID)
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   GraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());
   return *found;
}

const CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(IDType graphID) const
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   ConstGraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());
   return *found;
}

void CAnalysisResultsGraphDefinitions::RemoveGraphDefinition(IDType graphID)
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   GraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());

   m_Definitions.erase(found);
}

CString CAnalysisResultsGraphDefinitions::GetDefaultLoadCase(IntervalIndexType intervalIdx) const
{
   // return the of the first graph definition that is applicable to the given stage
   ConstGraphDefinitionIterator iter;
   for ( iter = m_Definitions.begin(); iter != m_Definitions.end(); iter++ )
   {
      const CAnalysisResultsGraphDefinition& def = *iter;
      std::set<IntervalIndexType>::const_iterator found = def.m_IntervalApplicability.find(intervalIdx);
      if (found != def.m_IntervalApplicability.end())
         return def.m_Name;
   }

   return "DC";
}
   
std::vector< std::pair<CString,IDType> > CAnalysisResultsGraphDefinitions::GetLoadCaseNames(IntervalIndexType intervalIdx, ActionType action) const
{
   std::vector< std::pair<CString,IDType> > lcNames;

   ConstGraphDefinitionIterator iter;
   for ( iter = m_Definitions.begin(); iter != m_Definitions.end(); iter++ )
   {
      const CAnalysisResultsGraphDefinition& def = *iter;

      bool bApplicableAction = true;
      switch(action)
      {
         case actionMoment:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_MOMENT_ONLY ? true : false;
            break;

         case actionShear:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_SHEAR_ONLY ? true : false;
            break;

         case actionDisplacement:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_DISPLACEMENT_ONLY ? true : false;
            break;

         case actionStress:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_STRESS_ONLY ? true : false;
            break;

         default:
            ATLASSERT(false);
            break;
      }

      std::set<IntervalIndexType>::const_iterator found = def.m_IntervalApplicability.find(intervalIdx);
      if (found != def.m_IntervalApplicability.end() && bApplicableAction)
      {
         lcNames.push_back( std::make_pair( def.m_Name, def.m_ID ) );
      }
   }
   return lcNames;
}

void CAnalysisResultsGraphDefinitions::Clear()
{
   m_Definitions.clear();
}
