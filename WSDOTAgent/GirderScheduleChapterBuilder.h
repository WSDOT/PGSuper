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

#include <PsgLib\Keys.h>

namespace WBFL { namespace EAF { class Broker; }; };

class CGirderScheduleChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CGirderScheduleChapterBuilder(bool bSelect = true);

   virtual LPCTSTR GetName() const override;

   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

private:
   int GetReinforcementDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,CLSID& familyCLSID,Float64* pz1Spacing,Float64 *pz1Length,Float64 *pz2Spacing,Float64* pz2Length,Float64 *pz3Spacing,Float64* pz3Length) const;

   struct DebondInformation
   {
      std::vector<StrandIndexType> Strands;
      Float64 Length;
   };
   int GetDebondDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,std::vector<DebondInformation>& debondInfo) const;
};
