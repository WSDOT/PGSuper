///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_SHEARCHECKTABLE_H_
#define INCLUDED_SHEARCHECKTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CShearCheckTable

   Encapsulates the construction of the vertical shear capacity check table.


DESCRIPTION
   Encapsulates the construction of the vertical shear capacity check table.

LOG
   rdp : 12.26.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CShearCheckTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CShearCheckTable();

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CShearCheckTable();

   // GROUP: OPERATORS

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the  table.
   virtual rptRcTable* Build(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                             IEAFDisplayUnits* pDisplayUnits,
                             IntervalIndexType intervalIdx,
                             pgsTypes::LimitState ls,
                             bool& bStrutAndTieRequired) const;
   //------------------------------------------------------------------------
   // Builds the notes after the  table.
   void BuildNotes(rptChapter* pChapter, 
                   IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                   IEAFDisplayUnits* pDisplayUnits,
                   IntervalIndexType intervalIdx,
                   pgsTypes::LimitState ls, bool bStrutAndTieRequired) const;

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
   CShearCheckTable(const CShearCheckTable& rOther);
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_SHEARCHECKTABLE_H_
