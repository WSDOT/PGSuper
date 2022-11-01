///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporter\Chapter.h>
#include <PgsExt\RatingArtifact.h>

interface IEAFDisplayUnits;

class CLoadRatingSummaryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLoadRatingSummaryChapterBuilder(bool bSelect = true);
   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

private:
   // Prevent accidental copying and assignment
   CLoadRatingSummaryChapterBuilder(const CLoadRatingSummaryChapterBuilder&) = delete;
   CLoadRatingSummaryChapterBuilder& operator=(const CLoadRatingSummaryChapterBuilder&) = delete;

   void ReportRatingFactor(IBroker* pBroker,rptRcTable* pTable,RowIndexType& row,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const;
   void ReportRatingFactor2(IBroker* pBroker,rptRcTable* pTable,RowIndexType row,LPCTSTR strTruck,const pgsRatingArtifact* pRatingArtifact,IEAFDisplayUnits* pDisplayUnits,rptParagraph* pRemarks) const;
};
