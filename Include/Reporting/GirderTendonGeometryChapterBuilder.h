///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
   CGirderTendonGeometryChapterBuilder

   Chapter builder for reporting PT tendon geometry


DESCRIPTION
   Chapter builder for reporting PT tendon geometry
*****************************************************************************/

class REPORTINGCLASS CGirderTendonGeometryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CGirderTendonGeometryChapterBuilder(bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

protected:

private:
   // Prevent accidental copying and assignment
   CGirderTendonGeometryChapterBuilder(const CGirderTendonGeometryChapterBuilder&) = delete;
   CGirderTendonGeometryChapterBuilder& operator=(const CGirderTendonGeometryChapterBuilder&) = delete;
};
