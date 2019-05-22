///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "Reporting\TemporarySupportElevationDetailsChapterBuilder.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <PgsExt\TemporarySupportData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTemporarySupportElevationDetailsChapterBuilder::CTemporarySupportElevationDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTemporarySupportElevationDetailsChapterBuilder::GetName() const
{
   return _T("Temporary Support Elevation Details");
}

CChapterBuilder* CTemporarySupportElevationDetailsChapterBuilder::Clone() const
{
   return new CTemporarySupportElevationDetailsChapterBuilder();
}

rptChapter* CTemporarySupportElevationDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pGirderRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("TemporarySupportElevation.png")) << rptNewLine;

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, elev, pDisplayUnits->GetSpanLengthUnit(), false);
   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   GET_IFACE2(pBroker, IBridge, pBridge);

   bool bHasOverlay = false;
   if (pBridge->HasOverlay() && !pBridge->IsFutureOverlay())
   {
      bHasOverlay = true;
   }

   SupportIndexType nTS = pBridge->GetTemporarySupportCount();

   GET_IFACE2(pBroker, ITempSupport, pTempSupport);

   std::array<std::_tstring, 2> strMemberEnd{ _T("Start"),_T("End") };

   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> vElevDetails = pTempSupport->GetElevationDetails(tsIdx);

      CString strTitle;
      strTitle.Format(_T("Temporary Support %d - %s"), LABEL_TEMPORARY_SUPPORT(tsIdx), CTemporarySupportData::AsString(pBridge->GetTemporarySupportType(tsIdx)));
      ColumnIndexType nColumns = (bHasOverlay ? 13 : 12);
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns, strTitle);

      *pPara << pTable << rptNewLine;

      ColumnIndexType col = 0;
      (*pTable)(0, col++) << _T("Girder");
      (*pTable)(0, col++) << _T("Segment");
      (*pTable)(0, col++) << _T("CL Brg") << rptNewLine << _T("Location");
      (*pTable)(0, col++) << _T("Station");
      (*pTable)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable)(0, col++) << COLHDR(_T("Finish") << rptNewLine << _T("Grade") << rptNewLine << _T("Elev"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable)(0, col++) << _T("Profile") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << _T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      if (bHasOverlay)
      {
         (*pTable)(0, col++) << COLHDR(_T("Overlay Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      (*pTable)(0, col++) << COLHDR(_T("Slab") << rptNewLine << _T("Offset"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(0, col++) << COLHDR(_T("Adjusted") << rptNewLine << _T("Girder") << rptNewLine << _T("Height"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(0, col++) << COLHDR(_T("Bottom") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      RowIndexType row = pTable->GetNumberOfHeaderRows();
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
         (*pTable)(row, col++) << rptRcStation(details.Station, &pDisplayUnits->GetStationFormat());
         (*pTable)(row, col++) << RPT_OFFSET(details.Offset,elev);
         (*pTable)(row, col++) << elev.SetValue(details.FinishedGradeElevation);
         (*pTable)(row, col++) << details.ProfileGrade;
         (*pTable)(row, col++) << details.GirderGrade;
         (*pTable)(row, col++) << details.GirderOrientation;
         if (bHasOverlay)
         {
            (*pTable)(row, col++) << dim.SetValue(details.OverlayDepth);
         }
         (*pTable)(row, col++) << dim.SetValue(details.SlabOffset);
         (*pTable)(row, col++) << dim.SetValue(details.Hg);
         (*pTable)(row, col++) << elev.SetValue(details.Elevation);

         row++;
      }
      *pPara << rptNewLine;
   }

   return pChapter;
}
