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

#ifndef INCLUDED_PGSEXT_PGSUPERCALCULATIONSHEET_H_
#define INCLUDED_PGSEXT_PGSUPERCALCULATIONSHEET_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
struct IBroker;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   PGSuperCalculationSheet

   Calculation sheet border specialized for PGSuper


DESCRIPTION

LOG
   rdp : 02.24.2000 : Created file
*****************************************************************************/

class PGSuperCalculationSheet : public WsdotCalculationSheet
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   PGSuperCalculationSheet(IBroker* pBroker);

   //------------------------------------------------------------------------
   // Copy constructor
   PGSuperCalculationSheet(const PGSuperCalculationSheet& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~PGSuperCalculationSheet();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   PGSuperCalculationSheet& operator = (const PGSuperCalculationSheet& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const PGSuperCalculationSheet& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const PGSuperCalculationSheet& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   PGSuperCalculationSheet();

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_PGSUPERCALCULATIONSHEET_H_
