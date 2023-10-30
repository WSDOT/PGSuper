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

#ifndef INCLUDED_PRODUCTSHEARTABLE_H_
#define INCLUDED_PRODUCTSHEARTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CProductShearTable

   Encapsulates the construction of the product shear table.


DESCRIPTION
   Encapsulates the construction of the product shear table.

LOG
   rab : 11.04.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CProductShearTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CProductShearTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CProductShearTable(const CProductShearTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CProductShearTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CProductShearTable& operator = (const CProductShearTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                             bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CProductShearTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CProductShearTable& rOther);

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

#endif // INCLUDED_PRODUCTSHEARTABLE_H_
