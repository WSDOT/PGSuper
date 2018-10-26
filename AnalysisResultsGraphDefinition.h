///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "PGSuperTypes.h"
#include <IFace\AnalysisResults.h>
#include <set>

// this load case stuff is a pinta. here's a little class to help things out
#define ACTIONS_MOMENT_ONLY       0x0001
#define ACTIONS_SHEAR_ONLY        0x0002
#define ACTIONS_STRESS_ONLY       0x0004
#define ACTIONS_DISPLACEMENT_ONLY 0x0008

#define ACTIONS_MOMENT_SHEAR        ACTIONS_MOMENT_ONLY  | ACTIONS_SHEAR_ONLY
#define ACTIONS_FORCE_DISPLACEMENT  ACTIONS_MOMENT_SHEAR | ACTIONS_DISPLACEMENT_ONLY
#define ACTIONS_FORCE_STRESS        ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS_ONLY
#define ACTIONS_ALL                 ACTIONS_MOMENT_SHEAR | ACTIONS_STRESS_ONLY | ACTIONS_DISPLACEMENT_ONLY 

enum GraphType 
{ 
   graphCombined, 
   graphLiveLoad, 
   graphVehicularLiveLoad,
   graphLimitState, 
   graphProduct,
   graphPrestress, 
   graphDemand, 
   graphAllowable, 
   graphCapacity,
   graphMinCapacity
};

enum ActionType 
{
   actionMoment, 
   actionShear, 
   actionDisplacement, 
   actionStress
};

class CAnalysisResultsGraphDefinition
{
public:

   int m_ID;
   CString m_Name;
   GraphType m_GraphType;

   union LoadType 
   {
      pgsTypes::LimitState         LimitStateType;
      ProductForceType             ProductLoadType;
      LoadingCombination           CombinedLoadType;
      pgsTypes::LiveLoadType       LiveLoadType;
   } m_LoadType;

   std::set<pgsTypes::Stage> m_StageApplicability; // stages that this graph is applicable to
  int m_ApplicableActions; // 0 = all, 1 = Forces Only, 2 = Stress Only
  VehicleIndexType m_VehicleIndex;
  COLORREF m_Color;

   CAnalysisResultsGraphDefinition();
   
   // constructor for limit states
   CAnalysisResultsGraphDefinition(int id,const CString name,
                pgsTypes::LimitState ls,
                bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int actions,COLORREF c);
   
   // constructor for combinations
   CAnalysisResultsGraphDefinition(int id,const CString name,
                LoadingCombination comb,
                bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int actions,COLORREF c);
   
   // constructor for product loads
   CAnalysisResultsGraphDefinition(int id,const CString name,
                ProductForceType type,
                bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int actions,COLORREF c);
   
   // constructor for live loads
   CAnalysisResultsGraphDefinition(int id,const CString name,
                bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int actions,COLORREF c);
   
   // constructor for prestress
   CAnalysisResultsGraphDefinition(int id,const CString name,GraphType type,COLORREF c);
   
   // constructor for demands
   CAnalysisResultsGraphDefinition(int id,const CString name,pgsTypes::LimitState lstype,GraphType lctype,bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,COLORREF c);

   // constructor for vehicular live loads
   CAnalysisResultsGraphDefinition(int id,const CString name,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int apaction,COLORREF c);

   // constructor for ultimate forces
   CAnalysisResultsGraphDefinition(int id,const CString name,pgsTypes::LimitState lstype,GraphType lctype,bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int apaction,COLORREF c);
   CAnalysisResultsGraphDefinition(int id,const CString name,pgsTypes::LiveLoadType llType,bool apcy, bool apgdrplacement, bool aptsr, bool apbs1, bool apbs2, bool apbs3,int apaction,COLORREF c);

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
   CAnalysisResultsGraphDefinition& GetGraphDefinition(int graphID);
   const CAnalysisResultsGraphDefinition& GetGraphDefinition(int graphID) const;
   void RemoveGraphDefinition(int graphID);

   CString GetDefaultLoadCase(pgsTypes::Stage stage) const;
   
   std::vector< std::pair<CString,int> > GetLoadCaseNames(pgsTypes::Stage stg, ActionType action) const;

   void Clear();

private:
   typedef std::set<CAnalysisResultsGraphDefinition> GraphDefinitionContainer;
   typedef GraphDefinitionContainer::iterator GraphDefinitionIterator;
   typedef GraphDefinitionContainer::const_iterator ConstGraphDefinitionIterator;
   GraphDefinitionContainer m_Definitions;
};

#endif //  INCLUDED_ANALYSISRESULTSGRAPHDEFINITION_H_