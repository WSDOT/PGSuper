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
#include <Reporter\Reporter.h>
#include <PGSuperTypes.h>
#include <PsgLib\Keys.h>

class IEAFDisplayUnits;

namespace PGS
{
   namespace Beams
   {
      /// @brief Base class for live load distribution factor engineer objects
      class BEAMSCLASS DistFactorEngineer
      {
      public:
         DistFactorEngineer() = delete;
         DistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID) :
            m_pBroker(pBroker), m_StatusGroupID(statusGroupID)
         {
         }
         DistFactorEngineer(const DistFactorEngineer&) = delete;
         virtual ~DistFactorEngineer() = default;
   
         DistFactorEngineer& operator=(const DistFactorEngineer&) = delete;

         // Returns the distribution factor for moment
         virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) = 0;

         // Returns the distribution factor for negative moment over a pier
         virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig = nullptr) = 0;

         // Returns the distribution factor for shear
         virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) = 0;

         // Creates a detailed report of the distribution factor computation
         virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) = 0;

         // Creates a string that defines how the distribution factors are to be computed
         virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype,pgsTypes::AdjacentTransverseConnectivity connect) = 0;

         // Runs NCHRP 12-50 Tests for live load distribution factors
         virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) = 0;

         // Get all types of factors
         virtual bool GetDFResultsEx(const CSpanKey& spanKey, pgsTypes::LimitState ls,
            Float64* gpM, Float64* gpM1, Float64* gpM2,     // pos moment
            Float64* gnM, Float64* gnM1, Float64* gnM2,     // neg moment
            Float64* gV, Float64* gV1, Float64* gV2) = 0;      // shear

         virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) = 0;
         virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) = 0;

      protected:
         StatusGroupIDType m_StatusGroupID;
         inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() { return m_pBroker.lock(); }

      private:
         std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
      };
   };
};