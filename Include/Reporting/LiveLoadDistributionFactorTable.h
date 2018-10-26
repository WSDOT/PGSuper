///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_LIVELOADDISTRIBUTIONFACTORTABLE_H_
#define INCLUDED_LIVELOADDISTRIBUTIONFACTORTABLE_H_

#include <Reporting\ReportingExp.h>

interface IDisplayUnits;

/*****************************************************************************
CLASS 
   CLiveLoadDistributionFactorTable

   Encapsulates the construction of the live load distribution factor table.


DESCRIPTION
   Encapsulates the construction of the live load distribution factor table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.13.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CLiveLoadDistributionFactorTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CLiveLoadDistributionFactorTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CLiveLoadDistributionFactorTable(const CLiveLoadDistributionFactorTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CLiveLoadDistributionFactorTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CLiveLoadDistributionFactorTable& operator = (const CLiveLoadDistributionFactorTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(rptChapter* pChapter,
                      IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                      IDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CLiveLoadDistributionFactorTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CLiveLoadDistributionFactorTable& rOther);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_LIVELOADDISTRIBUTIONFACTORTABLE_H_
