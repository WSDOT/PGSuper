///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <Details.h>
#include <IFace\BearingDesignParameters.h>
#include <Reporting\ReactionInterfaceAdapters.h>

class pgsBearingDesignEngineer
{
public:
   pgsBearingDesignEngineer() = default;
   pgsBearingDesignEngineer(IBroker* pBroker);
   pgsBearingDesignEngineer(const pgsBearingDesignEngineer& other) = default;

   void SetBroker(IBroker* pBroker);

   Float64 BearingSkewFactor(const ReactionLocation& reactionLocation, bool isFlexural) const;

   Float64 GetBearingCyclicRotation(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
	   const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF) const;

   void GetBearingRotationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
	   const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF, bool isFlexural, ROTATIONDETAILS* pDetails) const;

   void GetBearingDesignProperties(DESIGNPROPERTIES* pDetails) const;

   void GetBearingReactionDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi, 
	   const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF, REACTIONDETAILS* pDetails) const;

   void GetBearingShearDeformationDetails(pgsTypes::AnalysisType analysisType, const pgsPointOfInterest& poi,
	   const ReactionLocation& reactionLocation, bool bIncludeImpact, bool bIncludeLLDF, SHEARDEFORMATIONDETAILS* pDetails) const;

private:
   IBroker* m_pBroker;


};
