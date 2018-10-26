///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
void CBulbTeeDistFactorEngineer::Init(bool treatAsWsDotI)
{
   ATLASSERT(!m_pImpl);

   if ( treatAsWsDotI)
   {
      // WSDOT treats connected bulb tees as I beams
      CComObject<CIBeamDistFactorEngineer>* pEngineer;
      CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);

      m_pImpl = pEngineer;
   }
   else
   {
      // Otherwise bulb tees are same as other multi-web sections
      CComObject<CMultiWebDistFactorEngineer>* pEngineer;
      CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);

      pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btDeckBulbTee);

      m_pImpl = pEngineer;
   }

   ATLASSERT(m_pImpl);
}

void CBulbTeeDistFactorEngineer::SetBroker(IBroker* pBroker,long agentID)
{
   m_pBroker = pBroker;

   m_pImpl->SetBroker(pBroker, agentID);
}


double CBulbTeeDistFactorEngineer::GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   return m_pImpl->GetMomentDF(span,gdr,ls);
}

double CBulbTeeDistFactorEngineer::GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   return m_pImpl->GetMomentDF(span, gdr, ls, fcgdr);
}

double CBulbTeeDistFactorEngineer::GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   return m_pImpl->GetNegMomentDF(pier,gdr,ls,pierFace);
}

double CBulbTeeDistFactorEngineer::GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   return m_pImpl->GetNegMomentDF(pier,gdr,ls,pierFace,fcgdr);
}

double CBulbTeeDistFactorEngineer::GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   return m_pImpl->GetShearDF(span,gdr,ls);
}

double CBulbTeeDistFactorEngineer::GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   return m_pImpl->GetShearDF(span,gdr,ls,fcgdr);
}

double CBulbTeeDistFactorEngineer::GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   return m_pImpl->GetReactionDF(pier,gdr,ls);
}

double CBulbTeeDistFactorEngineer::GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   return m_pImpl->GetReactionDF(pier,gdr,ls,fcgdr);
}

void CBulbTeeDistFactorEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDisplayUnits)
{
   m_pImpl->BuildReport(span,gdr,pChapter,pDisplayUnits);
}

bool CBulbTeeDistFactorEngineer::Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,const char* pid,const char* bridgeId,std::ofstream& resultsFile, std::ofstream& poiFile)
{
   return m_pImpl->Run1250Tests(span,gdr,ls,pid,bridgeId,resultsFile,poiFile);
}

bool CBulbTeeDistFactorEngineer::GetDFResultsEx(SpanIndexType span, GirderIndexType gdr,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2,
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) 
{
   return m_pImpl->GetDFResultsEx(span, gdr, ls,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2, gR, gR1, gR2 ); 
}

std::string CBulbTeeDistFactorEngineer::GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::string& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(ISpecification,    pSpec);
   GET_IFACE(ILibrary,pLib);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();

   // Things are only weird for WSDOT method
   std::string descr;
   if (method==LLDF_WSDOT)
   {
      if (decktype == pgsTypes::sdtCompositeOverlay && connect == pgsTypes::atcConnectedAsUnit)
      {
         descr = "WSDOT Method per Design Memorandum 2-1999 Dated February 22, 1999 using type (k) cross section.";
      }
      else if (connect == pgsTypes::atcConnectedAsUnit)
      {
         descr = "AASHTO LRFD Method per Article 4.6.2.2. Using type (i,j) cross section connected transversely sufficiently to act as a unit.";
      }
      else
      {
         descr = "AASHTO LRFD Method per Article 4.6.2.2. Using type (i,j) cross section connected transversely only enough to prevent relative vertical displacement along interface.";
      }

      // special string if roa is ignored
      std::string straction = pLiveLoads->GetLLDFSpecialActionText();
      if ( !straction.empty() )
      {
         descr += straction;
      }
   }
   else
   {
      return m_pImpl->GetComputationDescription(span,gdr,libraryEntryName,decktype,connect);
   }

   return descr;
}
