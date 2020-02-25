///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
const std::_tstring& name,
pgsTypes::LimitState ls,
const std::vector<IntervalIndexType>& intervals,
int actions
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLimitState;
   m_LoadType.LimitStateType = ls;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
}

// constructor for combinations
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
LoadingCombinationType comb,
const std::vector<IntervalIndexType>& intervals,
int actions
): m_ID(id),m_Name(name)
{
   m_GraphType = graphCombined;
   m_LoadType.CombinedLoadType = comb;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
}

// constructor for product loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
pgsTypes::ProductForceType type,
const std::vector<IntervalIndexType>& intervals,
int actions
): m_ID(id),m_Name(name)
{
   m_GraphType = graphProduct;
   m_LoadType.ProductLoadType = type;

   m_IntervalApplicability.insert(intervals.begin(),intervals.end());
   
   m_ApplicableActions = actions;
}

// constructor for live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
const std::vector<IntervalIndexType>& intervals,
int actions
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = actions;
}

// constructor for deck shrinkage stress
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
GraphType type,
const std::vector<IntervalIndexType>& intervals
): m_ID(id),m_Name(name),m_GraphType(type)
{
   ATLASSERT(m_GraphType == graphDeckShrinkageStress); // only type supported now
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());
   m_ApplicableActions = ACTIONS_STRESS_ONLY;
}

// constructor for demands
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
   IDType id,
   const std::_tstring& name,
   pgsTypes::LimitState lstype,
   GraphType grtype,
   const std::vector<IntervalIndexType>& intervals
) : m_ID(id), m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;
   m_IntervalApplicability.insert(intervals.begin(), intervals.end());
   m_ApplicableActions = ACTIONS_STRESS_ONLY;
}

// constructor for vehicular live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
pgsTypes::LiveLoadType llType,
VehicleIndexType vehicleIdx,
const std::vector<IntervalIndexType>& intervals,
int apaction
): m_ID(id),m_Name(name)
{
   m_GraphType = graphVehicularLiveLoad;
   m_LoadType.LiveLoadType = llType;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
   m_VehicleIndex = vehicleIdx;
}

// constructor for ultimate forces
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
pgsTypes::LimitState lstype,
GraphType grtype,
const std::vector<IntervalIndexType>& intervals,
int apaction
): m_ID(id),m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
}

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
IDType id,
const std::_tstring& name,
pgsTypes::LiveLoadType llType,
const std::vector<IntervalIndexType>& intervals,
int apaction
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;
   m_LoadType.LiveLoadType = llType;
   m_IntervalApplicability.insert(intervals.begin(),intervals.end());

   m_ApplicableActions = apaction;
   m_VehicleIndex = INVALID_INDEX; // not a specific vehicle, but rather an envelope
}

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(IDType id, const std::_tstring& name, pgsTypes::LimitState lstype, GraphType lctype, ActionType ratingAction, VehicleIndexType vehicleIdx, const std::vector<IntervalIndexType>& intervals) :
   m_ID(id), m_Name(name), m_GraphType(lctype), m_RatingAction(ratingAction), m_VehicleIndex(vehicleIdx)
{
   ATLASSERT(m_RatingAction == actionMoment || m_RatingAction == actionShear || m_RatingAction == actionStress); // this are the only actions that produce rating factors
   m_LoadType.LimitStateType = lstype;
   m_IntervalApplicability.insert(intervals.begin(), intervals.end());
   m_ApplicableActions = ACTIONS_LOAD_RATING;
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
   m_Definitions.insert(std::make_pair(def.m_ID,def));
}

CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(IDType graphID)
{
   GraphDefinitionIterator found = m_Definitions.find(graphID);
   ASSERT(found != m_Definitions.end());
   return found->second;
}

const CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(IDType graphID) const
{
   ConstGraphDefinitionIterator found = m_Definitions.find(graphID);
   ASSERT(found != m_Definitions.end());
   return found->second;
}

IndexType CAnalysisResultsGraphDefinitions::GetGraphIndex(IDType graphID) const
{
   IndexType index = 0;
   ConstGraphDefinitionIterator iter(m_Definitions.begin());
   ConstGraphDefinitionIterator end(m_Definitions.end());
   for ( ; iter != end; iter++, index++ )
   {
      const CAnalysisResultsGraphDefinition& def = iter->second;
      if ( def.m_ID == graphID )
      {
         return index;
      }
   }

   ATLASSERT(false); // graphID not found
   return INVALID_INDEX;
}

void CAnalysisResultsGraphDefinitions::RemoveGraphDefinition(IDType graphID)
{
   GraphDefinitionIterator found = m_Definitions.find(graphID);
   ASSERT(found != m_Definitions.end());
   m_Definitions.erase(found);
}

std::_tstring CAnalysisResultsGraphDefinitions::GetDefaultLoadCase(IntervalIndexType intervalIdx) const
{
   // return the of the first graph definition that is applicable to the given stage
   ConstGraphDefinitionIterator iter;
   for ( iter = m_Definitions.begin(); iter != m_Definitions.end(); iter++ )
   {
      const CAnalysisResultsGraphDefinition& def = iter->second;
      std::set<IntervalIndexType>::const_iterator found = def.m_IntervalApplicability.find(intervalIdx);
      if (found != def.m_IntervalApplicability.end())
      {
         return def.m_Name;
      }
   }

   return _T("DC");
}
   
std::vector< std::pair<std::_tstring,IDType> > CAnalysisResultsGraphDefinitions::GetLoadings(IntervalIndexType intervalIdx, ActionType action) const
{
   std::vector< std::pair<std::_tstring,IDType> > lcNames;

   ConstGraphDefinitionIterator iter(m_Definitions.begin());
   ConstGraphDefinitionIterator iterEnd(m_Definitions.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CAnalysisResultsGraphDefinition& def = iter->second;

      bool bApplicableAction = true;
      switch(action)
      {
         case actionAxial:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_AXIAL_ONLY ? true : false;
            break;

         case actionShear:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_SHEAR_ONLY ? true : false;
            break;

         case actionReaction:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_REACTION_ONLY ? true : false;
            break;

         case actionMoment:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_MOMENT_ONLY ? true : false;
            break;

         case actionDeflection:
         case actionRotation:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_DEFLECTION_ONLY ? true : false;
            break;

         case actionXDeflection:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_X_DEFLECTION_ONLY ? true : false;
            break;

         case actionStress:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_STRESS_ONLY ? true : false;
            break;

         case actionLoadRating:
            bApplicableAction = def.m_ApplicableActions & ACTIONS_LOAD_RATING ? true : false;
            break;

         default:
            ATLASSERT(false);
            break;
      }

      if ( intervalIdx == INVALID_INDEX && bApplicableAction )
      {
         lcNames.push_back( std::make_pair( def.m_Name, def.m_ID ) );
      }
      else
      {
         std::set<IntervalIndexType>::const_iterator found = def.m_IntervalApplicability.find(intervalIdx);
         if (found != def.m_IntervalApplicability.end() && bApplicableAction)
         {
            lcNames.push_back( std::make_pair( def.m_Name, def.m_ID ) );
         }
      }
   }
   return lcNames;
}

void CAnalysisResultsGraphDefinitions::Clear()
{
   m_Definitions.clear();
}
