///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// BulbTeeDistFactorEngineer.cpp : Implementation of CBulbTeeDistFactorEngineer
#include "stdafx.h"
#include "BulbTeeDistFactorEngineer.h"
#include <IFace\Project.h>

#include "IBeamDistFactorEngineer.h"
#include "MultiWebDistFactorEngineer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// 
void CBulbTeeDistFactorEngineer::Init()
{
   ATLASSERT(!m_pImpl);
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btDeckBulbTee);

   m_pImpl = pEngineer;

   ATLASSERT(m_pImpl);
}

void CBulbTeeDistFactorEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;

   m_pImpl->SetBroker(pBroker, statusGroupID);
}


Float64 CBulbTeeDistFactorEngineer::GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetMomentDF(spanKey,ls);
}

Float64 CBulbTeeDistFactorEngineer::GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr)
{
   return m_pImpl->GetMomentDF(spanKey, ls, fcgdr);
}

Float64 CBulbTeeDistFactorEngineer::GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   return m_pImpl->GetNegMomentDF(pierIdx,gdrIdx,ls,pierFace);
}

Float64 CBulbTeeDistFactorEngineer::GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   return m_pImpl->GetNegMomentDF(pierIdx,gdrIdx,ls,pierFace,fcgdr);
}

Float64 CBulbTeeDistFactorEngineer::GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetShearDF(spanKey,ls);
}

Float64 CBulbTeeDistFactorEngineer::GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr)
{
   return m_pImpl->GetShearDF(spanKey,ls,fcgdr);
}

void CBulbTeeDistFactorEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_pImpl->BuildReport(girderKey,pChapter,pDisplayUnits);
}

bool CBulbTeeDistFactorEngineer::Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   return m_pImpl->Run1250Tests(spanKey,ls,pid,bridgeId,resultsFile,poiFile);
}

bool CBulbTeeDistFactorEngineer::GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2 )
{
   return m_pImpl->GetDFResultsEx(spanKey, ls,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2); 
}

Float64 CBulbTeeDistFactorEngineer::GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetSkewCorrectionFactorForMoment(spanKey,ls);
}

Float64 CBulbTeeDistFactorEngineer::GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   return m_pImpl->GetSkewCorrectionFactorForShear(spanKey,ls);
}

std::_tstring CBulbTeeDistFactorEngineer::GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   return m_pImpl->GetComputationDescription(girderKey, libraryEntryName, decktype, connect);
}
