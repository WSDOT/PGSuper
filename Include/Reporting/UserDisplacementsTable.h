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

#ifndef INCLUDED_USERDeflectionSTABLE_H_
#define INCLUDED_USERDeflectionSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CUserDeflectionsTable

   Encapsulates the construction of the User Deflections table.


DESCRIPTION
   Encapsulates the construction of the User Deflections table.

LOG
   rab : 11.05.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CUserDeflectionsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CUserDeflectionsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CUserDeflectionsTable(const CUserDeflectionsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CUserDeflectionsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CUserDeflectionsTable& operator = (const CUserDeflectionsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the typical Deflections  table.
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
   void MakeCopy(const CUserDeflectionsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CUserDeflectionsTable& rOther);

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

#endif // INCLUDED_PRODUCTDeflectionSTABLE_H_
