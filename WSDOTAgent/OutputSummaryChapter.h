///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_OUTPUTSUMMARYCHAPTER_H_
#define INCLUDED_OUTPUTSUMMARYCHAPTER_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_REPORTER_CHAPTER_H_
#include <Reporter\Chapter.h>
#endif

// PROJECT INCLUDES
//


// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   COutputSummaryChapter

   Output summary chapter


DESCRIPTION
   Output summary chapter.  Condensed summary of output.

LOG
   rab : 08.25.1998 : Created file
*****************************************************************************/

class COutputSummaryChapter : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   COutputSummaryChapter(bool bSelect = true);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const override;

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
   COutputSummaryChapter(const COutputSummaryChapter&) = delete;
   COutputSummaryChapter& operator=(const COutputSummaryChapter&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_OUTPUTSUMMARYCHAPTER_H_
