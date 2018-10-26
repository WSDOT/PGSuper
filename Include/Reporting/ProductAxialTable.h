///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <IFace\Intervals.h>
#include "ReportNotes.h"

/*****************************************************************************
CLASS 
   CProductAxialTable

   Encapsulates the construction of the product forces table.


DESCRIPTION
   Encapsulates the construction of the product forces table.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CProductAxialTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CProductAxialTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CProductAxialTable(const CProductAxialTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CProductAxialTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CProductAxialTable& operator = (const CProductAxialTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                             bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const;

protected:

   //------------------------------------------------------------------------
   void MakeCopy(const CProductAxialTable& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const CProductAxialTable& rOther);

};
