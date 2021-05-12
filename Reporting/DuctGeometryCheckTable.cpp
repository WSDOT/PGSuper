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

#include "StdAfx.h"
#include <Reporting\DuctGeometryCheckTable.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\DuctSizeArtifact.h>

#include <IFace\Bridge.h>
#include <IFace\Allowables.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CDuctGeometryCheckTable
****************************************************************************/

CDuctGeometryCheckTable::CDuctGeometryCheckTable()
{
}

CDuctGeometryCheckTable::CDuctGeometryCheckTable(const CDuctGeometryCheckTable& rOther)
{
   MakeCopy(rOther);
}

CDuctGeometryCheckTable::~CDuctGeometryCheckTable()
{
}

CDuctGeometryCheckTable& CDuctGeometryCheckTable::operator= (const CDuctGeometryCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CDuctGeometryCheckTable::Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey = pGirderArtifact->GetGirderKey();

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   if (nMaxSegmentDucts + nGirderDucts == 0)
   {
      // no ducts
      return;
   }

   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   INIT_UV_PROTOTYPE(rptLengthUnitValue, size, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, radius, pDisplayUnits->GetSpanLengthUnit(), false);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Duct Geometry [5.4.6.1]");
   pPara->SetName(_T("Duct Geometry"));
   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, IDuctLimits, pDuctLimits);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      Float64 Rmin = pDuctLimits->GetSegmentTendonRadiusOfCurvatureLimit(segmentKey);
      (*pPara) << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(": The radius of curvature of segment tendon ducts shall not be less that ") << size.SetValue(Rmin) << rptNewLine;
   }

   Float64 Rmin = pDuctLimits->GetGirderTendonRadiusOfCurvatureLimit(girderKey);
   (*pPara) << _T("Girder: The radius of curvature of girder tendon ducts shall not be less that ") << size.SetValue(Rmin) << rptNewLine;
   (*pPara) << rptNewLine;


   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3);
   *pPara << pTable << rptNewLine;

   (*pTable)(0,0) << _T("Duct");
   (*pTable)(0,1) << COLHDR(_T("R"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,2) << _T("Status");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nMaxSegmentDucts; ductIdx++, row++)
      {
         ColumnIndexType col = 0;

         (*pTable)(row, col++) << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(", Duct ") << LABEL_DUCT(ductIdx);

         if (ductIdx < nDucts)
         {
            const pgsDuctSizeArtifact* pDuctSizeArtifact = pSegmentArtifact->GetDuctSizeArtifact(ductIdx);

            Float64 r, rmin;
            pDuctSizeArtifact->GetRadiusOfCurvature(&r, &rmin);

            (*pTable)(row, col++) << radius.SetValue(r);

            if (pDuctSizeArtifact->PassedRadiusOfCurvature())
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }
         }
         else
         {
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
         }
      } // next duct
   } // next segment

   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++, row++ )
   {
      ColumnIndexType col = 0;
      const pgsDuctSizeArtifact* pDuctSizeArtifact = pGirderArtifact->GetDuctSizeArtifact(ductIdx);
      (*pTable)(row,col++) << _T("Girder Duct ") << LABEL_DUCT(ductIdx);

      Float64 r, rmin;
      pDuctSizeArtifact->GetRadiusOfCurvature(&r,&rmin);

      (*pTable)(row,col++) << radius.SetValue(r);

      if ( pDuctSizeArtifact->PassedRadiusOfCurvature() )
      {
         (*pTable)(row,col++) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,col++) << RPT_FAIL;
      }
   } // next duct
}

void CDuctGeometryCheckTable::MakeCopy(const CDuctGeometryCheckTable& rOther)
{
   // Add copy code here...
}

void CDuctGeometryCheckTable::MakeAssignment(const CDuctGeometryCheckTable& rOther)
{
   MakeCopy( rOther );
}
