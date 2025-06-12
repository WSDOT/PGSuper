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
#include <PgsExt\RatingArtifact.h>

class REPORTINGCLASS CLoadRatingDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLoadRatingDetailsChapterBuilder(bool bSelect = true);

   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

private:
   mutable bool m_bReportAtAllPoi;
   mutable bool m_bIsSingleGirderLine;

   void ReportRatingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,bool bSplicedGirder) const;

   void MomentRatingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,bool bPositiveMoment,const std::vector<const pgsRatingArtifact*>& pRatingArtifacts,bool bSplicedGirder) const;
   void ShearRatingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<const pgsRatingArtifact*>& pRatingArtifacts,bool bSplicedGirder) const;
   void StressRatingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<const pgsRatingArtifact*>& pRatingArtifacts,bool bSplicedGirder) const;
   void ReinforcementYieldingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,bool bPositiveMoment,const std::vector<const pgsRatingArtifact*>& pRatingArtifacts,bool bSplicedGirder) const;
   void LoadPostingDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<const pgsRatingArtifact*>& pRatingArtifacts,const std::vector<CGirderKey>& girderKeys) const;

   bool ReportAtThisPoi(const pgsPointOfInterest& poi,const pgsPointOfInterest& controllingPoi) const;
};
