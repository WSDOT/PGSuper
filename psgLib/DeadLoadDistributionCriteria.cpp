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
#include "StdAfx.h"
#include <psgLib/DeadLoadDistributionCriteria.h>
#include <psgLib/LibraryEntryDifferenceItem.h>
//#include <EAF/EAFDisplayUnits.h>

bool DeadLoadDistributionCriteria::Compare(const DeadLoadDistributionCriteria& other, const SpecLibraryEntryImpl& impl, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool bReturnOnFirstDifference) const
{
   bool bSame = true;
   if (TrafficBarrierDistribution != other.TrafficBarrierDistribution || MaxGirdersTrafficBarrier != other.MaxGirdersTrafficBarrier)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Distribution of Railing System Loads are different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   if (OverlayDistribution != other.OverlayDistribution)
   {
      bSame = false;
      vDifferences.emplace_back(std::make_unique<pgsLibraryEntryDifferenceStringItem>(_T("Distribution of Overlay Dead Load is different"), _T(""), _T("")));
      if (bReturnOnFirstDifference) return false;
   }

   return bSame;
}

void DeadLoadDistributionCriteria::Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Dead Load Distribution Criteria") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   switch (TrafficBarrierDistribution)
   {
   case pgsTypes::tbdGirder:
      *pPara << _T("Railing system weight is distributed to ") << MaxGirdersTrafficBarrier << _T(" nearest girders") << rptNewLine;
      break;

  case pgsTypes::tbdMatingSurface:
      *pPara << _T("Railing system weight is distributed to ") << MaxGirdersTrafficBarrier << _T(" nearest mating surfaces") << rptNewLine;
      break;

  case pgsTypes::tbdWebs:
      *pPara << _T("Railing system weight is distributed to ") << MaxGirdersTrafficBarrier << _T(" nearest webs") << rptNewLine;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   *pPara << rptNewLine;
   if (OverlayDistribution == pgsTypes::olDistributeTributaryWidth)
   {
      *pPara << _T("Overlay load is distributed using tributary width.") << rptNewLine;
   }
   else
   {
      *pPara << _T("Overlay load is distributed uniformly among all girders per LRFD 4.6.2.2.1") << rptNewLine;
   }
}

void DeadLoadDistributionCriteria::Save(WBFL::System::IStructuredSave* pSave) const
{
   pSave->BeginUnit(_T("DeadLoadDistributionCriteria"), 1.0);
   pSave->Property(_T("TrafficBarrierDistribution"), TrafficBarrierDistribution);
   pSave->Property(_T("MaxGirdersTrafficBarrier"), MaxGirdersTrafficBarrier);
   pSave->Property(_T("OverlayDistribution"), OverlayDistribution);
   pSave->EndUnit();
}

void DeadLoadDistributionCriteria::Load(WBFL::System::IStructuredLoad* pLoad)
{
   PRECONDITION(83 <= pLoad->GetVersion());

   if (!pLoad->BeginUnit(_T("DeadLoadDistributionCriteria")))  THROW_LOAD(InvalidFileFormat, pLoad);

   int value;
   if (!pLoad->Property(_T("TrafficBarrierDistribution"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   TrafficBarrierDistribution = (pgsTypes::TrafficBarrierDistribution)value;

   if (!pLoad->Property(_T("MaxGirdersTrafficBarrier"), &MaxGirdersTrafficBarrier)) THROW_LOAD(InvalidFileFormat, pLoad);

   if (!pLoad->Property(_T("OverlayDistribution"), &value)) THROW_LOAD(InvalidFileFormat, pLoad);
   OverlayDistribution = (pgsTypes::OverlayLoadDistributionType)value;

   if (!pLoad->EndUnit()) THROW_LOAD(InvalidFileFormat, pLoad);
}
