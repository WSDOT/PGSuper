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

#ifndef INCLUDED_TOGASPECCHECKSUMMARYCHAPTERBUILDER_H_
#define INCLUDED_TOGASPECCHECKSUMMARYCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

class pgsSegmentArtifact;
class CSpanGirderReportSpecification;

/*****************************************************************************
CLASS 
   CTogaSpecCheckSummaryChapterBuilder

   Writes out a summary of the spec chec.


DESCRIPTION
   Writes out a summary of the spec chec. The summary is either PASS or FAILED
   with a listing of everything that failed.

LOG
   rab : 03.17.1999 : Created file
*****************************************************************************/

class CTogaSpecCheckSummaryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CTogaSpecCheckSummaryChapterBuilder(bool referToDetailsReport);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;


   //------------------------------------------------------------------------
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

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
   bool m_ReferToDetailsReport;
   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CTogaSpecCheckSummaryChapterBuilder(const CTogaSpecCheckSummaryChapterBuilder&) = delete;
   CTogaSpecCheckSummaryChapterBuilder& operator=(const CTogaSpecCheckSummaryChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_TOGASPECCHECKSUMMARYCHAPTERBUILDER_H_
