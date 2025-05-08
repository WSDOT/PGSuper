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
#include <Reporter\Chapter.h>
#include <PgsExt\RatingArtifact.h>

class IEAFDisplayUnits;
namespace WBFL { namespace EAF { class Broker; }; };

class CLoadRatingSummaryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLoadRatingSummaryChapterBuilder(bool bSelect = true);
   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

private:
   void ReportRatingFactor(std::shared_ptr<WBFL::EAF::Broker> pBroker,rptRcTable* pTable,RowIndexType& row,const pgsRatingArtifact* pRatingArtifact,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptParagraph* pRemarks) const;
   void ReportRatingFactor2(std::shared_ptr<WBFL::EAF::Broker> pBroker,rptRcTable* pTable,RowIndexType row,LPCTSTR strTruck,const pgsRatingArtifact* pRatingArtifact,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptParagraph* pRemarks) const;
};
