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

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CRatingSummaryTable

   Encapsulates the construction of the rating summary table
*****************************************************************************/

/// @brief Encapsulates the construction of the rating summary table
class REPORTINGCLASS CRatingSummaryTable
{
public:
   enum RatingTableType
   { Design, Legal, Permit };

   CRatingSummaryTable() = delete;
   ~CRatingSummaryTable() = delete;

   /// @brief Builds the rating summary table with results listed by limit state and structural action
   static rptRcTable* BuildByLimitState(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,RatingTableType ratingTableType);

   /// @brief Builds the rating summary table with results listed by rating vehicle
   static rptRcTable* BuildByVehicle(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType);

   /// @brief Builds the load posting table for a a legal load rating (but not emergency vehicles)
   static rptRcTable* BuildLoadPosting(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,bool* pbMustCloseBridge);

   /// @brief Builds the load posting table for emergency vehicles
   static rptRcTable* BuildEmergencyVehicleLoadPosting(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys);

   /// @brief Builds the yield stress ratio table for permit load rating types
   static rptRcTable* BuildYieldStressRatio(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys, pgsTypes::LoadRatingType ratingType);
};

