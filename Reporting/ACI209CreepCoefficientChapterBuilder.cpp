///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\ACI209CreepCoefficientChapterBuilder.h>

#include <IFace\Bridge.h>
#include <Material\ConcreteBase.h>
#include <Material\ACI209Concrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CACI209CreepCoefficientChapterBuilder
****************************************************************************/

CACI209CreepCoefficientChapterBuilder::CACI209CreepCoefficientChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CACI209CreepCoefficientChapterBuilder::GetName() const
{
   return TEXT("Creep Coefficient Details");
}

rptChapter* CACI209CreepCoefficientChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, vsRatio, pDisplayUnits->GetComponentDimUnit(), false );

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Taken from \"Prediction of Creep, Shrinkage, and Temperature Effects in Concrete Structures\" by ACI Committee 209") << rptNewLine;
   *pPara << _T("ACI 209R-92, Reapproved 2008") << rptNewLine;
   *pPara << rptNewLine;

   *pPara << rptRcImage(strImagePath + _T("ACI209_CreepCoefficient.png")) << _T("  (Eq. 2-8)") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("ACI209_LoadingAgeCorrection_Moist_Cured.png")) << _T("  Moist Cured Concrete (Eq. 2-11)") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("ACI209_LoadingAgeCorrection_Steam_Cured.png")) << _T("  Steam Cured Concrete (Eq. 2-12)") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("ACI209_HumidityCorrection.png")) << _T("  (Eq. 2-14)") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("ACI209_VolumeSurfaceRatioCorrection.png")) << _T("  (Eq. 2-21)") << rptNewLine;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(6);
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


   RowIndexType rowIdx = 0;
   ColumnIndexType colIdx = 0;
   (*pTable)(rowIdx,colIdx++) << _T("Element");
   (*pTable)(rowIdx,colIdx++) << _T("Curing") << rptNewLine << _T("Method");
   (*pTable)(rowIdx,colIdx++) << COLHDR(_T("V/S"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("vs"));
   (*pTable)(rowIdx,colIdx++) << _T("H (%)");
   (*pTable)(rowIdx,colIdx++) << Sub2(symbol(gamma),_T("hc"));


   std::_tstring strCuring[] = { _T("Moist"), _T("Steam") };

   rowIdx = pTable->GetNumberOfHeaderRows();
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      ColumnIndexType colIdx = 0;

      CSegmentKey segmentKey(girderKey,segIdx);
      const matConcreteBase* pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
      const matACI209Concrete* pACIConcrete = dynamic_cast<const matACI209Concrete*>(pConcrete);

      (*pTable)(rowIdx,colIdx++) << _T("Segment ") << LABEL_SEGMENT(segIdx);
      (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());
      (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetSizeFactorCreep();
      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetRelativeHumidityFactorCreep();

      rowIdx++;

      if ( segIdx != nSegments-1 )
      {
         CClosureKey closureKey(segmentKey);
         const matConcreteBase* pConcrete = pMaterials->GetClosureJointConcrete(closureKey);
         const matACI209Concrete* pACIConcrete = dynamic_cast<const matACI209Concrete*>(pConcrete);
   
         colIdx = 0;

         (*pTable)(rowIdx,colIdx++) << _T("Closure Joint");
         (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
         (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());
         (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetSizeFactorCreep();
         (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
         (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetRelativeHumidityFactorCreep();

         rowIdx++;
      }
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      const matConcreteBase* pConcrete = pMaterials->GetDeckConcrete();
      const matACI209Concrete* pACIConcrete = dynamic_cast<const matACI209Concrete*>(pConcrete);

      ColumnIndexType colIdx = 0;

      (*pTable)(rowIdx,colIdx++) << _T("Deck");
      (*pTable)(rowIdx,colIdx++) << strCuring[pConcrete->GetCureMethod()];
      (*pTable)(rowIdx,colIdx++) << vsRatio.SetValue(pConcrete->GetVSRatio());
      (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetSizeFactorCreep();
      (*pTable)(rowIdx,colIdx++) << pConcrete->GetRelativeHumidity();
      (*pTable)(rowIdx,colIdx++) << pACIConcrete->GetRelativeHumidityFactorCreep();

      rowIdx++;
   }

   return pChapter;
}

CChapterBuilder* CACI209CreepCoefficientChapterBuilder::Clone() const
{
   return new CACI209CreepCoefficientChapterBuilder;
}
