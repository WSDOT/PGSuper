///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_USERSHEARTABLE_H_
#define INCLUDED_USERSHEARTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CUserShearTable

   Encapsulates the construction of the User forces table.


DESCRIPTION
   Encapsulates the construction of the User forces table.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CUserShearTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CUserShearTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CUserShearTable(const CUserShearTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CUserShearTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CUserShearTable& operator = (const CUserShearTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                             IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CUserShearTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CUserShearTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_USERSHEARTABLE_H_
