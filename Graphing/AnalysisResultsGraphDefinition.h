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
#pragma once

#include <Graphing\AnalysisResultsGraphBuilder.h>

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

class CAnalysisResultsGraphDefinition
{
public:

   IDType m_ID;
   CString m_Name;
   GraphType m_GraphType;

   union LoadType 
   {
      pgsTypes::LimitState         LimitStateType;
      ProductForceType             ProductLoadType;
      LoadingCombination           CombinedLoadType;
      pgsTypes::LiveLoadType       LiveLoadType;
   } m_LoadType;

   std::set<IntervalIndexType> m_IntervalApplicability; // intervals that this graph is applicable to
   int m_ApplicableActions; // 0 = all, 1 = Forces Only, 2 = Stress Only
   VehicleIndexType m_VehicleIndex;
   COLORREF m_Color1;
   COLORREF m_Color2;

   CAnalysisResultsGraphDefinition();
   
   // constructor for limit states
   CAnalysisResultsGraphDefinition(IDType id,const CString name,
                pgsTypes::LimitState ls,
                const std::vector<IntervalIndexType>& intervals,int actions,COLORREF c);
   
   // constructor for combinations
   CAnalysisResultsGraphDefinition(IDType id,const CString name,
                LoadingCombination comb,
                const std::vector<IntervalIndexType>& intervals,int actions,COLORREF c);
   
   // constructor for product loads
   CAnalysisResultsGraphDefinition(IDType id,const CString name,
                ProductForceType type,
                const std::vector<IntervalIndexType>& intervals,int actions,COLORREF c);
   
   // constructor for live loads
   CAnalysisResultsGraphDefinition(IDType id,const CString name,
                const std::vector<IntervalIndexType>& intervals,int actions,COLORREF c);
   
   // constructor for prestress
   CAnalysisResultsGraphDefinition(IDType id,const CString name,GraphType type,const std::vector<IntervalIndexType>& intervals,COLORREF c1,COLORREF c2);
   
   // constructor for demands
   CAnalysisResultsGraphDefinition(IDType id,const CString name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals,COLORREF c);

   // constructor for vehicular live loads
   CAnalysisResultsGraphDefinition(IDType id,const CString name,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<IntervalIndexType>& intervals,int apaction,COLORREF c);

   // constructor for ultimate forces
   CAnalysisResultsGraphDefinition(IDType id,const CString name,pgsTypes::LimitState lstype,GraphType lctype,const std::vector<IntervalIndexType>& intervals,int apaction,COLORREF c);
   CAnalysisResultsGraphDefinition(IDType id,const CString name,pgsTypes::LiveLoadType llType,const std::vector<IntervalIndexType>& intervals,int apaction,COLORREF c);

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
   void RemoveGraphDefinition(IDType graphID);

   CString GetDefaultLoadCase(IntervalIndexType intervalIdx) const;
   
   std::vector< std::pair<CString,IDType> > GetLoadCaseNames(IntervalIndexType intervalIdx, ActionType action) const;

   void Clear();

private:
   typedef std::set<CAnalysisResultsGraphDefinition> GraphDefinitionContainer;
   typedef GraphDefinitionContainer::iterator GraphDefinitionIterator;
   typedef GraphDefinitionContainer::const_iterator ConstGraphDefinitionIterator;
   GraphDefinitionContainer m_Definitions;
};

#endif //  INCLUDED_ANALYSISRESULTSGRAPHDEFINITION_H_