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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SplittingCheckEngineer.h>

class rptChapter;
class rptParagraph;

class PGSEXTCLASS pgsLRFDSplittingCheckEngineer : public pgsSplittingCheckEngineer
{
public:
   pgsLRFDSplittingCheckEngineer();
   pgsLRFDSplittingCheckEngineer(IBroker* pBroker);
   virtual ~pgsLRFDSplittingCheckEngineer();

   virtual std::shared_ptr<pgsSplittingCheckArtifact> Check(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;
   virtual Float64 GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const override;
   virtual void ReportSpecCheck(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const override;
   virtual void ReportDetails(rptChapter* pChapter, const pgsSplittingCheckArtifact* pArtifact) const override;
   virtual Float64 GetSplittingZoneLength(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType) const override;

protected:
   virtual std::_tstring GetSpecReference() const;
   virtual void ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const;
   virtual void ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const;
   virtual void ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const;

   virtual Float64 GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const;

private:
   Float64 GetMaxSplittingStress(Float64 fy) const;
   Float64 GetSplittingZoneLengthFactor() const;
   std::array<pgsPointOfInterest, 2> GetPointsOfInterest(const CSegmentKey& segmentKey) const;
//   Float64 GetSplittingZoneLength(Float64 girderHeight) const;
};

class PGSEXTCLASS pgsPCIUHPCSplittingCheckEngineer : public pgsLRFDSplittingCheckEngineer
{
public:
   pgsPCIUHPCSplittingCheckEngineer();
   pgsPCIUHPCSplittingCheckEngineer(IBroker* pBroker);
   virtual ~pgsPCIUHPCSplittingCheckEngineer();

   virtual std::shared_ptr<pgsSplittingCheckArtifact> Check(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;

protected:
   virtual std::_tstring GetSpecReference() const override;
   virtual void ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;
   virtual void ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;
   virtual void ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;

   virtual Float64 GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const override;
};

class PGSEXTCLASS pgsFHWAUHPCSplittingCheckEngineer : public pgsLRFDSplittingCheckEngineer
{
public:
   pgsFHWAUHPCSplittingCheckEngineer();
   pgsFHWAUHPCSplittingCheckEngineer(IBroker* pBroker);
   virtual ~pgsFHWAUHPCSplittingCheckEngineer();

   virtual std::shared_ptr<pgsSplittingCheckArtifact> Check(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const override;

protected:
   virtual std::_tstring GetSpecReference() const override;
   virtual void ReportDimensions(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;
   virtual void ReportDemand(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;
   virtual void ReportResistance(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& strSplittingType, const pgsSplittingCheckArtifact* pArtifact, pgsTypes::MemberEndType endType) const override;

   virtual Float64 GetConcreteCapacity(pgsTypes::MemberEndType endType, const pgsSplittingCheckArtifact* pArtifact) const override;
};
