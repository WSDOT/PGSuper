///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <IFace\AnalysisResults.h>

class IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CProductRotationTable

   Encapsulates the construction of the product Rotation table.


DESCRIPTION
   Encapsulates the construction of the product Rotation table.

LOG
   rab : 11.05.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CBearingDesignPropertiesTable
{
public:
   CBearingDesignPropertiesTable() = default;

   struct TABLEPARAMETERS
   {
       bool bSegments;
       bool bConstruction;
       bool bDeck;
       bool bDeckPanels;
       bool bPedLoading;
       bool bSidewalk;
       bool bShearKey;
       bool bLongitudinalJoint;
       bool bContinuousBeforeDeckCasting;
       GroupIndexType startGroup;
       GroupIndexType endGroup;
   };




   // 
   // 
   // 
   // Builds ......
   virtual rptRcTable* BuildBearingDesignPropertiesTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY
};
