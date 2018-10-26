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
int id,
const CString name,
pgsTypes::LimitState ls,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLimitState;
   m_LoadType.LimitStateType = ls;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);


   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for combinations
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
LoadingCombination comb,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphCombined;
   m_LoadType.CombinedLoadType = comb;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = actions;
   m_Color = c;
}

   // constructor for product loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
ProductForceType type,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphProduct;
   m_LoadType.ProductLoadType = type;
   
   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);
   
   m_ApplicableActions = actions;
   m_Color = c;
}

   // constructor for live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int actions,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = actions;
   m_Color = c;
}

// constructor for prestress
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
GraphType type,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = type;
   m_StageApplicability.insert(pgsTypes::CastingYard);
   m_StageApplicability.insert(pgsTypes::GirderPlacement);
   m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);
   m_StageApplicability.insert(pgsTypes::BridgeSite1);
   m_StageApplicability.insert(pgsTypes::BridgeSite2);
   m_StageApplicability.insert(pgsTypes::BridgeSite3);
   m_ApplicableActions = ACTIONS_STRESS_ONLY | ACTIONS_DISPLACEMENT_ONLY;
   m_Color = c;
}

// constructor for demands
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
pgsTypes::LimitState lstype,
GraphType grtype,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = ACTIONS_STRESS_ONLY;
   m_Color = c;
}

// constructor for vehicular live loads
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
pgsTypes::LiveLoadType llType,
VehicleIndexType vehicleIndex,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphVehicularLiveLoad;
   m_LoadType.LiveLoadType = llType;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = apaction;
   m_Color = c;
   m_VehicleIndex = vehicleIndex;
}

// constructor for ultimate forces
CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
pgsTypes::LimitState lstype,
GraphType grtype,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = grtype;
   m_LoadType.LimitStateType = lstype;

   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = apaction;
   m_Color = c;
}

CAnalysisResultsGraphDefinition::CAnalysisResultsGraphDefinition(
int id,
const CString name,
pgsTypes::LiveLoadType llType,
bool apcy, 
bool apgdrplacement,
bool aptsr, 
bool apbs1, 
bool apbs2, 
bool apbs3,
int apaction,
COLORREF c
): m_ID(id),m_Name(name)
{
   m_GraphType = graphLiveLoad;
   m_LoadType.LiveLoadType = llType;
   
   if ( apcy )
      m_StageApplicability.insert(pgsTypes::CastingYard);

   if ( apgdrplacement )
      m_StageApplicability.insert(pgsTypes::GirderPlacement);

   if ( aptsr )
      m_StageApplicability.insert(pgsTypes::TemporaryStrandRemoval);

   if ( apbs1 )
      m_StageApplicability.insert(pgsTypes::BridgeSite1);

   if ( apbs2 )
      m_StageApplicability.insert(pgsTypes::BridgeSite2);

   if ( apbs3 )
      m_StageApplicability.insert(pgsTypes::BridgeSite3);

   m_ApplicableActions = apaction;
   m_Color = c;
   m_VehicleIndex = INVALID_INDEX; // not a specific vehicle, but rather an envelope
}

//////////////////////////////////////////////////////////////////////
CAnalysisResultsGraphDefinitions::CAnalysisResultsGraphDefinitions()
{
}

void CAnalysisResultsGraphDefinitions::AddGraphDefinition(const CAnalysisResultsGraphDefinition& def)
{
   m_Definitions.insert(def);
}

CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(int graphID)
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   GraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());
   return *found;
}

const CAnalysisResultsGraphDefinition& CAnalysisResultsGraphDefinitions::GetGraphDefinition(int graphID) const
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   ConstGraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());
   return *found;
}

void CAnalysisResultsGraphDefinitions::RemoveGraphDefinition(int graphID)
{
   CAnalysisResultsGraphDefinition key;
   key.m_ID = graphID;
   GraphDefinitionIterator found = m_Definitions.find(key);
   ASSERT(found != m_Definitions.end());

   m_Definitions.erase(found);
}

CString CAnalysisResultsGraphDefinitions::GetDefaultLoadCase(pgsTypes::Stage stage) const
{
   // return the of the first graph definition that is applicable to the given stage
   ConstGraphDefinitionIterator iter;
   for ( iter = m_Definitions.begin(); iter != m_Definitions.end(); iter++ )
   {
      const CAnalysisResultsGraphDefinition& def = *iter;
      std::set<pgsTypes::Stage>::const_iterator found = def.m_StageApplicability.find(stage);
      if (found != def.m_StageApplicability.end())
         return def.m_Name;
   }

   return "DC";
}
   
std::vector< std::pair<CString,int> > CAnalysisResultsGraphDefinitions::GetLoadCaseNames(pgsTypes::Stage stage, ActionType action) const
{
   std::vector< std::pair<CString,int> > lcNames;

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

      std::set<pgsTypes::Stage>::const_iterator found = def.m_StageApplicability.find(stage);
      if (found != def.m_StageApplicability.end() && bApplicableAction)
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
