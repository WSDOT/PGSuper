///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2006  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_COMBINEDSTRESSTABLE_H_
#define INCLUDED_COMBINEDSTRESSTABLE_H_

#include <Reporting\ReportingExp.h>

interface IDisplayUnits;

/*****************************************************************************
CLASS 
   CCombinedStressTable

   Encapsulates the construction of the combined stress table.


DESCRIPTION
   Encapsulates the construction of the combined stress table.


COPYRIGHT
   Copyright © 1997-2006
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 04.05.2006 : Created file
*****************************************************************************/

class REPORTINGCLASS CCombinedStressTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCombinedStressTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCombinedStressTable(const CCombinedStressTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCombinedStressTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCombinedStressTable& operator = (const CCombinedStressTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      SpanIndexType span,GirderIndexType girder,
                      IDisplayUnits* pDispUnits,
                      pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CCombinedStressTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCombinedStressTable& rOther);

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

#endif // INCLUDED_COMBINEDSTRESSTABLE_H_
