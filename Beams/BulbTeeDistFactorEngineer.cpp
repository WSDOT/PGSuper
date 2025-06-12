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

// BulbTeeDistFactorEngineer.cpp : Implementation of BulbTeeDistFactorEngineer
#include "stdafx.h"
#include "Beams.h"
#include <Beams\BulbTeeDistFactorEngineer.h>
#include <IFace\Project.h>

#include <Beams/IBeamDistFactorEngineer.h>
#include "MultiWebDistFactorEngineer.h"

using namespace PGS::Beams;

BulbTeeDistFactorEngineer::BulbTeeDistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID) :
   DistFactorEngineer(pBroker, statusGroupID)
{
   m_pImpl = std::make_unique<MultiWebDistFactorEngineer>(MultiWebDistFactorEngineer::BeamType::DeckBulbTee, pBroker, statusGroupID);
}

Float64 BulbTeeDistFactorEngineer::GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig)
{
   return m_pImpl->GetMomentDF(spanKey,ls, pConfig);
}

Float64 BulbTeeDistFactorEngineer::GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig)
{
   return m_pImpl->GetNegMomentDF(pierIdx,gdrIdx,ls,pierFace,pConfig);
}

Float64 BulbTeeDistFactorEngineer::GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig)
{
   return m_pImpl->GetShearDF(spanKey,ls, pConfig);
}

void BulbTeeDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   m_pImpl->BuildReport(girderKey,pChapter,pDisplayUnits);
}

bool BulbTeeDistFactorEngineer::Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   return m_pImpl->Run1250Tests(spanKey,ls,pid,bridgeId,resultsFile,poiFile);
}

bool BulbTeeDistFactorEngineer::GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2 )
{
   return m_pImpl->GetDFResultsEx(spanKey, ls,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2); 
}

Float64 BulbTeeDistFactorEngineer::GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetSkewCorrectionFactorForMoment(spanKey,ls);
}

Float64 BulbTeeDistFactorEngineer::GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetSkewCorrectionFactorForShear(spanKey,ls);
}

std::_tstring BulbTeeDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   return m_pImpl->GetComputationDescription(girderKey, libraryEntryName, decktype, connect);
}
