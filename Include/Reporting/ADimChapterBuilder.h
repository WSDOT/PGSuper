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

#pragma once

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


/*****************************************************************************
CLASS 
   CADimChapterBuilder

   "A" Dimension Chapter Builder.


DESCRIPTION
   "A" Dimension Chapter builder.  Reports the details of the "A" Dimension
   calculation.

LOG
   rab : 11.03.1998 : Created file
*****************************************************************************/

class SpecLibraryEntry;

class REPORTINGCLASS CADimChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CADimChapterBuilder(const CADimChapterBuilder&) = delete;
   CADimChapterBuilder(bool bSelect = true);

   CADimChapterBuilder& operator=(const CADimChapterBuilder&) = delete;
   
   LPCTSTR GetName() const override;
   rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;
   std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

protected:
   void BuildAdimContent(rptChapter* pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level, std::shared_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& girderKey,const SpecLibraryEntry* pSpecEntry) const;
   void BuildDirectHaunchElevationContent(rptChapter* pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec, Uint16 level) const;
};
