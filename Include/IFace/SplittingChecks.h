///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

class pgsSplittingCheckArtifact;
class pgsGirderArtifact;

/*****************************************************************************
INTERFACE
   ISplittingChecks

   Interface for spltting checks

DESCRIPTION
   Interface to allowable prestressing strand stresses.
*****************************************************************************/
// {C290BDA0-FD1F-4D2C-81C3-686A0A620405}
DEFINE_GUID(IID_ISplittingChecks,
   0xc290bda0, 0xfd1f, 0x4d2c, 0x81, 0xc3, 0x68, 0x6a, 0xa, 0x62, 0x4, 0x5);
interface ISplittingChecks : IUnknown
{
   //------------------------------------------------------------------------
   // Returns the distance from the ends of the girder within which the Splitting
   // stress requirements must be checked. 5.9.4.4 (pre2017: 5.10.10.1)
   virtual Float64 GetSplittingZoneLength(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const = 0;

   virtual std::shared_ptr<pgsSplittingCheckArtifact> CheckSplitting(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig = nullptr) const = 0;
   /// Returns the area of steel required to pass the spec check
   virtual Float64 GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const = 0;
   virtual void ReportSplittingChecks(IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const = 0;
   virtual void ReportSplittingCheckDetails(IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const = 0;
};

