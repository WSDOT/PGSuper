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
#include "Reporting\TemporarySupportElevationsChapterBuilder.h"

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PsgLib\TemporarySupportData.h>
#include <PsgLib\GirderGroupData.h>
#include <PsgLib\BridgeDescription2.h>


CTemporarySupportElevationsChapterBuilder::CTemporarySupportElevationsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTemporarySupportElevationsChapterBuilder::GetName() const
{
   return _T("Temporary Support Elevations");
}

rptChapter* CTemporarySupportElevationsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   auto pBroker = pGirderRptSpec->GetBroker();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   GirderIndexType girderIndex;
   if (pGdrRptSpec)
   {
      girderIndex = pGdrRptSpec->GetGirderKey().girderIndex;
   }
   else if (pGdrLineRptSpec)
   {
      girderIndex = pGdrLineRptSpec->GetGirderKey().girderIndex;
   }
   else
   {
      //ATLASSERT(false); // not expecting a different kind of report spec
      //return pChapter;
      girderIndex = ALL_GIRDERS;
   }

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false);

   GET_IFACE2(pBroker, IBridge, pBridge);
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   if (nTS == 0)
   {
      *pPara << _T("No temporary supports modeled") << rptNewLine;
   }

   GET_IFACE2_NOCHECK(pBroker, ITempSupport, pTempSupport); // not used if no temp supports

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::array<std::_tstring, 2> strMemberEnd{ _T("Start"),_T("End") };

   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      CString strTitle;
      strTitle.Format(_T("Temporary Support %d - %s"), LABEL_TEMPORARY_SUPPORT(tsIdx), CTemporarySupportData::AsString(pBridge->GetTemporarySupportType(tsIdx)));
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4, strTitle);

      *pPara << pTable << rptNewLine;

      ColumnIndexType col = 0;
      (*pTable)(0, col++) << _T("Girder");
      (*pTable)(0, col++) << _T("Segment");
      (*pTable)(0, col++) << _T("CL Brg") << rptNewLine << _T("Location");
      (*pTable)(0, col++) << COLHDR(_T("Bottom") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      
      const auto* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      auto group = pBridgeDesc->GetGirderGroup(pTS->GetSpan());
      auto grpIdx = group->GetIndex();
      auto nGirders = group->GetGirderCount();
      auto first_girder_idx = (girderIndex == ALL_GIRDERS ? 0 : girderIndex);
      auto last_girder_idx = (girderIndex == ALL_GIRDERS ? nGirders - 1 : girderIndex);

      for (auto gdrIdx = first_girder_idx; gdrIdx <= last_girder_idx; gdrIdx++)
      {
         std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> vElevDetails = pTempSupport->GetElevationDetails(tsIdx, gdrIdx);

         for (const auto& details : vElevDetails)
         {
            col = 0;
            (*pTable)(row, col++) << LABEL_GIRDER(details.girderIdx);
            (*pTable)(row, col++) << LABEL_SEGMENT(details.segmentIdx);
            if (details.bContinuous)
            {
               (*pTable)(row, col++) << _T("CL TS");
            }
            else
            {
               (*pTable)(row, col++) << strMemberEnd[details.endType];
            }
            (*pTable)(row, col++) << dim.SetValue(details.Elevation);

            row++;
         }
      }
      *pPara << rptNewLine;
   }

   return pChapter;
}
