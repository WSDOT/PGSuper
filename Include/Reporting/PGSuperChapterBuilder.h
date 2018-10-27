///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#pragma once

#include <Reporting\ReportingExp.h>

#include <ReportManager\ChapterBuilder.h>

/*****************************************************************************
CLASS 
   CPGSuperChapterBuilder

   Abstract base class for chapter builders.


DESCRIPTION
   Abstract base class for chapter builders.  Chapter builders are
   responsible for implementing the Build method by creating a
   rptChapter object and filling it with content.

LOG
   rab : 08.25.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CPGSuperChapterBuilder : public CChapterBuilder
{
public:
   CPGSuperChapterBuilder(bool bSelect=true);

   // returns 1
   virtual Uint16 GetMaxLevel() const;
   
   // creates a new chapter object and configures it with the correct style for PGSuper reports
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   // returns true if this chapter builder is selected by default
   // in the report definition dialog
   virtual bool Select() const;

   virtual bool NeedsUpdate(CReportHint* pHint,CReportSpecification* pRptSpec,Uint16 level) const;

protected:
   bool m_bSelect;
};
