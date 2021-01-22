///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\DuctSizeCheckTable.h>

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
   CDuctSizeCheckTable
****************************************************************************/

CDuctSizeCheckTable::CDuctSizeCheckTable()
{
}

CDuctSizeCheckTable::CDuctSizeCheckTable(const CDuctSizeCheckTable& rOther)
{
   MakeCopy(rOther);
}

CDuctSizeCheckTable::~CDuctSizeCheckTable()
{
}

CDuctSizeCheckTable& CDuctSizeCheckTable::operator= (const CDuctSizeCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CDuctSizeCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const
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

   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, size, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_SCALAR_PROTOTYPE( rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Size of Ducts [5.4.6.2]");
   pPara->SetName(_T("Size of Ducts"));
   *pChapter << pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   // The permissible ratios, Kmax and Tmax
   GET_IFACE2(pBroker, IDuctLimits, pDuctLimits);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      Float64 Kmax = pDuctLimits->GetSegmentTendonAreaLimit(segmentKey);
      Float64 Tmax = pDuctLimits->GetSegmentTendonDuctSizeLimit(segmentKey);
      (*pPara) << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      (*pPara) << _T("The inside cross-sectional area of the segment tendon duct shall be at least ") << Kmax << _T(" times the net area of the prestress steel.") << rptNewLine;
      (*pPara) << _T("The size of segment tendon ducts shall not exceed ") << Tmax << _T(" times the least gross concrete thickness at the duct.") << rptNewLine;
      (*pPara) << rptNewLine;
   }

   Float64 Kmax = pDuctLimits->GetGirderTendonAreaLimit(girderKey);
   Float64 Tmax = pDuctLimits->GetGirderTendonDuctSizeLimit(girderKey);
   (*pPara) << _T("Girder Ducts") << rptNewLine;
   (*pPara) << _T("The inside cross-sectional area of the girder tendon duct shall be at least ") << Kmax << _T(" times the net area of the prestress steel.") << rptNewLine;
   (*pPara) << _T("The size of girder tendon ducts shall not exceed ") << Tmax << _T(" times the least gross concrete thickness at the duct.") << rptNewLine;
   (*pPara) << rptNewLine;


   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9);
   *pPara << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << _T("Duct");
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("duct")),rptAreaUnitTag,pDisplayUnits->GetAreaUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("pt")),rptAreaUnitTag,pDisplayUnits->GetAreaUnit());
   (*pTable)(0,col++) << Sub2(_T("A"),_T("duct")) << _T("/") << Sub2(_T("A"),_T("pt"));
   (*pTable)(0,col++) << _T("Status");
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::NinthEdition2020)
   {
      (*pTable)(0, col++) << COLHDR(_T("OD"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*pTable)(0, col++) << COLHDR(_T("Nominal Diameter (ND)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   (*pTable)(0,col++) << COLHDR(_T("Least") << rptNewLine << _T("Thickness (Tmin)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::NinthEdition2020)
   {
      (*pTable)(0, col++) << _T("OD/Tmin");
   }
   else
   {
      (*pTable)(0, col++) << _T("ND/Tmin");
   }
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nMaxSegmentDucts; ductIdx++, row++)
      {
         col = 0;
         (*pTable)(row, col++) << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(", Duct ") << LABEL_DUCT(ductIdx);

         if (ductIdx < nDucts)
         {
            const pgsDuctSizeArtifact* pDuctSizeArtifact = pSegmentArtifact->GetDuctSizeArtifact(ductIdx);

            Float64 Apt, Aduct, Kmax;
            pDuctSizeArtifact->GetDuctArea(&Apt, &Aduct, &Kmax);

            Float64 duct_diameter, MinGrossThickness, Tmax;
            pDuctSizeArtifact->GetDuctSize(&duct_diameter, &MinGrossThickness, &Tmax);

            (*pTable)(row, col++) << area.SetValue(Aduct);
            (*pTable)(row, col++) << area.SetValue(Apt);
            if (IsZero(Apt))
            {
               (*pTable)(row, col++) << _T("-");
            }
            else
            {
               (*pTable)(row, col++) << scalar.SetValue(Aduct / Apt);
            }

            if (pDuctSizeArtifact->PassedDuctArea())
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }

            (*pTable)(row, col++) << size.SetValue(duct_diameter);
            (*pTable)(row, col++) << size.SetValue(MinGrossThickness);

            if (IsZero(MinGrossThickness))
            {
               (*pTable)(row, col++) << _T("-");
            }
            else
            {
               (*pTable)(row, col++) << scalar.SetValue(duct_diameter / MinGrossThickness);
            }

            if (pDuctSizeArtifact->PassedDuctSize())
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
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
            (*pTable)(row, col++) << _T("-");
         }
      }
   }

   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++, row++ )
   {
      col = 0;
      const pgsDuctSizeArtifact* pDuctSizeArtifact = pGirderArtifact->GetDuctSizeArtifact(ductIdx);
      (*pTable)(row,col++) << _T("Girder Duct ") << LABEL_DUCT(ductIdx);

      Float64 Apt, Aduct, Kmax;
      pDuctSizeArtifact->GetDuctArea(&Apt,&Aduct,&Kmax);

      Float64 OD, MinGrossThickness, Tmax;
      pDuctSizeArtifact->GetDuctSize(&OD,&MinGrossThickness,&Tmax);

      (*pTable)(row,col++) << area.SetValue(Aduct);
      (*pTable)(row,col++) << area.SetValue(Apt);
      if ( IsZero(Apt) )
      {
         (*pTable)(row,col++) << _T("-");
      }
      else
      {
         (*pTable)(row,col++) << scalar.SetValue(Aduct/Apt);
      }

      if ( pDuctSizeArtifact->PassedDuctArea() )
      {
         (*pTable)(row,col++) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,col++) << RPT_FAIL;
      }

      (*pTable)(row,col++) << size.SetValue(OD);
      (*pTable)(row,col++) << size.SetValue(MinGrossThickness);
      
      if ( IsZero(MinGrossThickness) )
      {
         (*pTable)(row,col++) << _T("-");
      }
      else
      {
         (*pTable)(row,col++) << scalar.SetValue(OD/MinGrossThickness);
      }
      
      if ( pDuctSizeArtifact->PassedDuctSize() )
      {
         (*pTable)(row,col++) << RPT_PASS;
      }
      else
      {
         (*pTable)(row,col++) << RPT_FAIL;
      }
   } // next duct
}

void CDuctSizeCheckTable::MakeCopy(const CDuctSizeCheckTable& rOther)
{
   // Add copy code here...
}

void CDuctSizeCheckTable::MakeAssignment(const CDuctSizeCheckTable& rOther)
{
   MakeCopy( rOther );
}
