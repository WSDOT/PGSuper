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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SplittingCheckArtifact.h>

class rptChapter;

class PGSEXTCLASS pgsSplittingCheckEngineer
{
public:
   pgsSplittingCheckEngineer();
   pgsSplittingCheckEngineer(IBroker* pBroker);
   virtual ~pgsSplittingCheckEngineer();

   void SetBroker(IBroker* pBroker);

   static LPCTSTR GetCheckName();

   virtual std::shared_ptr<pgsSplittingCheckArtifact> Check(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const = 0;
   virtual void ReportSpecCheck(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const = 0;
   virtual void ReportDetails(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const = 0;
   virtual Float64 GetSplittingZoneLength(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const = 0;

protected:
   CComPtr<IBroker> m_pBroker;
};
