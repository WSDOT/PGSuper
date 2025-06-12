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

#include <Beams/BeamsExp.h>
#include <IFace\DistFactorEngineer.h>
#include <Plugins\Beams.h>

namespace PGS
{
   namespace Beams
   {
      class BEAMSCLASS BulbTeeDistFactorEngineer : public DistFactorEngineer
      {
      public:
         BulbTeeDistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);

      public:
         // DistFactorEngineerBase
         Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) override;
         Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig = nullptr) override;
         Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) override;
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;
         bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) override;
         bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                             Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                             Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                             Float64* gV,  Float64* gV1,  Float64* gV2 ) override;   // shear
         Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
         Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;

      private:
         // Farm most of the hard work out to other classes
         std::unique_ptr<DistFactorEngineer> m_pImpl;
      };
   };
};