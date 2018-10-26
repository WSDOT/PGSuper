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

#ifndef INCLUDED_BRIDGEDESCCHAPTERBUILDER_H_
#define INCLUDED_BRIDGEDESCCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CBridgeDescChapterBuilder

   Bridge Description Chapter Builder.


DESCRIPTION
   Bridge Description Chapter Builder.  Generates a chapter that echos the bridge
   description input.  Level 1 echos the basic input.  Level 2 provides the details
   of library items referenced in the basic input.

COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.03.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CBridgeDescChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CBridgeDescChapterBuilder();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

   static void WriteAlignmentData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level);
   static void WriteProfileData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level);
   static void WriteCrownData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level);


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

   // Prevent accidental copying and assignment
   CBridgeDescChapterBuilder(const CBridgeDescChapterBuilder&);
   CBridgeDescChapterBuilder& operator=(const CBridgeDescChapterBuilder&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_BRIDGEDESCCHAPTERBUILDER_H_
