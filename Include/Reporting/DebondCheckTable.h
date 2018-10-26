///////////////////////////////////////////////////////////////////////
// PGSplice - Precast Post-tensioned Spliced Girder Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#ifndef INCLUDED_DEBONDCHECKTABLE_H_
#define INCLUDED_DEBONDCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsDebondArtifact;


/*****************************************************************************
CLASS 
   CDebondCheckTable

   Encapsulates the construction of the debond check table.


DESCRIPTION
   Encapsulates the construction of the debond check table.



COPYRIGHT
   Copyright © 2003
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 06.11.2003 : Created file
*****************************************************************************/

class REPORTINGCLASS CDebondCheckTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CDebondCheckTable();

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CDebondCheckTable();

   // GROUP: OPERATORS

   // GROUP: OPERATIONS

   void Build(rptChapter* pChapter, IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const;

private:
   //------------------------------------------------------------------------
   // Builds the table for debonded strands in a row
   rptRcTable* Build1(const pgsDebondArtifact* pDebondArtifact,SpanIndexType span,GirderIndexType girder,pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const;

   //------------------------------------------------------------------------
   // Builds the table for debonded strands at a section
   rptRcTable* Build2(const pgsDebondArtifact* pDebondArtifact,SpanIndexType span,GirderIndexType girder, Float64 girderLength, pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Copy constructor
   CDebondCheckTable(const CDebondCheckTable& rOther);
};

#endif // INCLUDED_DEBONDCHECKTABLE_H_
