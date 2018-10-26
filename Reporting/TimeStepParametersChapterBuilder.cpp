///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\TimeStepParametersChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>

#include <WBFLGenericBridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTimeStepParametersChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTimeStepParametersChapterBuilder::CTimeStepParametersChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTimeStepParametersChapterBuilder::GetName() const
{
   return TEXT("Time Step Parameters");
}

rptChapter* CTimeStepParametersChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   GET_IFACE2(pBroker,IPointOfInterest,pSegmentPOI);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterials);
   GET_IFACE2(pBroker,ILongRebarGeometry,pRebarGeom);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        ecc,      pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,        stress,   pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE( rptLength2UnitValue,       area,     pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE( rptLength4UnitValue,       momI,     pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,        modE,     pDisplayUnits->GetModEUnit(), false);
   INIT_UV_PROTOTYPE( rptForceUnitValue,         force,    pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue,        moment,   pDisplayUnits->GetMomentUnit(), false);
   INIT_UV_PROTOTYPE( rptPerLengthUnitValue,     curvature, pDisplayUnits->GetCurvatureUnit(), false);

   location.IncludeSpanAndGirder(true);

   ///////////////////////////////////////////////////////////////////////////////////
   // Time Step Parameters that are independent of POI
   ///////////////////////////////////////////////////////////////////////////////////

   // Report Concrete Parameters
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey thisSegmentKey(girderKey.groupIndex,girderKey.girderIndex,segIdx);

      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      (*pPara) << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      ColumnIndexType col = 0;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(9,_T("Concrete Parameters"));
      (*pPara) << pTable << rptNewLine;

      (*pTable)(0,col++) << _T("Interval");
      (*pTable)(0,col++) << COLHDR(_T("Age"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
      (*pTable)(0,col++) << COLHDR(_T("f'c"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << COLHDR(_T("Ec"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << symbol(psi);
      (*pTable)(0,col++) << symbol(chi);
      (*pTable)(0,col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
      (*pTable)(0,col++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));


      RowIndexType row = pTable->GetNumberOfHeaderRows();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
      {
         col = 0;
         (*pTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
         (*pTable)(row,col++) << pMaterials->GetSegmentConcreteAge(thisSegmentKey,intervalIdx);
         (*pTable)(row,col++) << stress.SetValue(pMaterials->GetSegmentFc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << modE.SetValue(pMaterials->GetSegmentEc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         (*pTable)(row,col++) << pMaterials->GetSegmentAgingCoefficient(thisSegmentKey,intervalIdx);
         (*pTable)(row,col++) << modE.SetValue(pMaterials->GetSegmentAgeAdjustedEc(thisSegmentKey,intervalIdx));
         (*pTable)(row,col++) << 1E6*pMaterials->GetSegmentFreeShrinkageStrain(thisSegmentKey,intervalIdx);
         (*pTable)(row,col++) << 1E6*pMaterials->GetSegmentFreeShrinkageStrain(thisSegmentKey,intervalIdx,pgsTypes::End);
      }

      rptRcTable* pCreepTable = pgsReportStyleHolder::CreateDefaultTable(1 + 2*(nIntervals-1),_T("Creep Coefficients"));
      *pPara << pCreepTable << rptNewLine;
      pCreepTable->SetNumberOfHeaderRows(2);

      col = 0;
      pCreepTable->SetRowSpan(0,col,2);
      pCreepTable->SetRowSpan(1,col,SKIP_CELL);
      (*pCreepTable)(0,col++) << _T("Interval") << rptNewLine << _T("i");

      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals-1; intervalIdx++ )
      {
         (*pCreepTable)(0,col)   << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ie")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
         (*pCreepTable)(0,col+1) << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ib")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");

         pCreepTable->SetColumnSpan(1,col,2);
         (*pCreepTable)(1,col++) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
         pCreepTable->SetColumnSpan(1,col++,SKIP_CELL);
      }

      row = pCreepTable->GetNumberOfHeaderRows();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
      {
         col = 0;
         (*pCreepTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
         for ( IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals-1; loadIntIdx++ )
         {
            (*pCreepTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            (*pCreepTable)(row,col++) << pMaterials->GetSegmentCreepCoefficient(thisSegmentKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
         }
         for ( IntervalIndexType i = intervalIdx; i < nIntervals-1; i++ )
         {
            (*pCreepTable)(row,col++) << _T("");
            (*pCreepTable)(row,col++) << _T("");
         }
      }

      if ( segIdx != nSegments-1 )
      {
         CSegmentKey closureKey(thisSegmentKey);

         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         (*pPara) << _T("Closure Pour at TS ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         ColumnIndexType col = 0;
         rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(9,_T("Concrete Parameters"));
         (*pPara) << pTable << rptNewLine;

         (*pTable)(0,col++) << _T("Interval");
         (*pTable)(0,col++) << COLHDR(_T("Age"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
         (*pTable)(0,col++) << COLHDR(_T("f'c"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << COLHDR(_T("Ec"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << symbol(psi);
         (*pTable)(0,col++) << symbol(chi);
         (*pTable)(0,col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
         (*pTable)(0,col++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
         (*pTable)(0,col++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));

         RowIndexType row = pTable->GetNumberOfHeaderRows();
         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
         {
            col = 0;
            (*pTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
            (*pTable)(row,col++) << pMaterials->GetClosurePourConcreteAge(closureKey,intervalIdx);
            (*pTable)(row,col++) << stress.SetValue(pMaterials->GetClosurePourFc(closureKey,intervalIdx));
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosurePourEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << pMaterials->GetClosurePourCreepCoefficient(closureKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            (*pTable)(row,col++) << pMaterials->GetClosurePourAgingCoefficient(closureKey,intervalIdx);
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosurePourAgeAdjustedEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << 1E6*pMaterials->GetClosurePourFreeShrinkageStrain(closureKey,intervalIdx);
            (*pTable)(row,col++) << 1E6*pMaterials->GetClosurePourFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End);
         }


         rptRcTable* pCreepTable = pgsReportStyleHolder::CreateDefaultTable(1 + 2*(nIntervals-1),_T("Creep Coefficients"));
         *pPara << pCreepTable << rptNewLine;
         pCreepTable->SetNumberOfHeaderRows(2);

         col = 0;
         pCreepTable->SetRowSpan(0,col,2);
         pCreepTable->SetRowSpan(1,col,SKIP_CELL);
         (*pCreepTable)(0,col++) << _T("Interval") << rptNewLine << _T("i");

         for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals-1; intervalIdx++ )
         {
            (*pCreepTable)(0,col)   << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ie")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
            (*pCreepTable)(0,col+1) << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ib")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");

            pCreepTable->SetColumnSpan(1,col,2);
            (*pCreepTable)(1,col++) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
            pCreepTable->SetColumnSpan(1,col++,SKIP_CELL);
         }

         row = pCreepTable->GetNumberOfHeaderRows();
         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
         {
            col = 0;
            (*pCreepTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
            for ( IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals-1; loadIntIdx++ )
            {
               (*pCreepTable)(row,col++) << pMaterials->GetClosurePourCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
               (*pCreepTable)(row,col++) << pMaterials->GetClosurePourCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
            }
            for ( IntervalIndexType i = intervalIdx; i < nIntervals-1; i++ )
            {
               (*pCreepTable)(row,col++) << _T("");
               (*pCreepTable)(row,col++) << _T("");
            }
         }
      } // if segIdx != 0
   } // next segment

   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("Deck") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType col = 0;
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(9,_T("Concrete Parameters"));
   (*pPara) << pTable << rptNewLine;

   (*pTable)(0,col++) << _T("Interval");
   (*pTable)(0,col++) << COLHDR(_T("Age"),rptTimeUnitTag,pDisplayUnits->GetLongTimeUnit());
   (*pTable)(0,col++) << COLHDR(_T("f'c"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(0,col++) << COLHDR(_T("Ec"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(0,col++) << symbol(psi);
   (*pTable)(0,col++) << symbol(chi);
   (*pTable)(0,col++) << COLHDR(_T("E'c = Ec/(") << _T("1+") << symbol(chi) << symbol(psi) << _T(")"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*pTable)(0,col++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));
   (*pTable)(0,col++) << Sub2(symbol(epsilon),_T("sh")) << _T("x10") << Super(_T("6"));


   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
   {
      col = 0;
      (*pTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
      (*pTable)(row,col++) << pMaterials->GetDeckConcreteAge(intervalIdx);
      (*pTable)(row,col++) << stress.SetValue(pMaterials->GetDeckFc(intervalIdx));
      (*pTable)(row,col++) << modE.SetValue(pMaterials->GetDeckEc(intervalIdx));
      (*pTable)(row,col++) << pMaterials->GetDeckCreepCoefficient(intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
      (*pTable)(row,col++) << pMaterials->GetDeckAgingCoefficient(intervalIdx);
      (*pTable)(row,col++) << modE.SetValue(pMaterials->GetDeckAgeAdjustedEc(intervalIdx));
      (*pTable)(row,col++) << 1E6*pMaterials->GetDeckFreeShrinkageStrain(intervalIdx);
      (*pTable)(row,col++) << 1E6*pMaterials->GetDeckFreeShrinkageStrain(intervalIdx,pgsTypes::End);
   }

   rptRcTable* pCreepTable = pgsReportStyleHolder::CreateDefaultTable(1 + 2*(nIntervals-1),_T("Creep Coefficients"));
   *pPara << pCreepTable << rptNewLine;
   pCreepTable->SetNumberOfHeaderRows(2);

   col = 0;
   pCreepTable->SetRowSpan(0,col,2);
   pCreepTable->SetRowSpan(1,col,SKIP_CELL);
   (*pCreepTable)(0,col++) << _T("Interval") << rptNewLine << _T("i");

   for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals-1; intervalIdx++ )
   {
      (*pCreepTable)(0,col)   << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ie")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");
      (*pCreepTable)(0,col+1) << symbol(psi) << _T("(") << Sub2(_T("t"),_T("ib")) << _T(",") << Sub2(_T("t"),_T("jm")) << _T(")");

      pCreepTable->SetColumnSpan(1,col,2);
      (*pCreepTable)(1,col++) << _T("j = ") << LABEL_INTERVAL(intervalIdx);
      pCreepTable->SetColumnSpan(1,col++,SKIP_CELL);
   }

   row = pCreepTable->GetNumberOfHeaderRows();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row++ )
   {
      col = 0;
      (*pCreepTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
      for ( IntervalIndexType loadIntIdx = 0; loadIntIdx < intervalIdx && loadIntIdx < nIntervals-1; loadIntIdx++ )
      {
         (*pCreepTable)(row,col++) << pMaterials->GetDeckCreepCoefficient(loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         (*pCreepTable)(row,col++) << pMaterials->GetDeckCreepCoefficient(loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
      }
      for ( IntervalIndexType i = intervalIdx; i < nIntervals-1; i++ )
      {
         (*pCreepTable)(row,col++) << _T("");
         (*pCreepTable)(row,col++) << _T("");
      }
   }


   ///////////////////////////////////////////////////////////////////////////////////
   // Time Step Parameters that are dependent on POI
   ///////////////////////////////////////////////////////////////////////////////////

   std::vector<pgsPointOfInterest> vPOI(pSegmentPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      *pPara << location.SetValue(POI_ERECTED_SEGMENT,poi,0.0) << _T(" (ID = " ) << poi.GetID() << _T(")") << rptNewLine;

      CComPtr<IRebarSection> rebar_section;
      pRebarGeom->GetRebars(poi,&rebar_section);

      IndexType nRebar = 0;
      rebar_section->get_Count(&nRebar);

      ColumnIndexType nColumns = 119 + 5*nRebar;
      rptRcTable* pTable2 = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Time Step Parameters"));
      *pPara << pTable2 << rptNewLine << rptNewLine;

      ColumnIndexType col2 = 0;
      (*pTable2)(0,col2++) << _T("Interval");

      // Transformed properties
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("tr")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tr")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("tr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("e")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("e")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Net girder properties, unrestrained deformations, and component restraining forces
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ng")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tng")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bng")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("ng")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("g")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("g")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("g")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Net slab properties, unrestrained deformations, and component restraining forces
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("nd")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("nd")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("d")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("d")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("d")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("d")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Slab top mat rebar properties
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("tm")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tm")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      // Slab bottom mat rebar properties
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("bm")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bm")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      // Girder Rebar
      for (IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("gb")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("gb")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      // unrestrained deformations and component restraining forces for strand (straight)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for strand (harped)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for strand (temporary)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r ps")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("ps")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // unrestrained deformations and component restraining forces for tendon
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("pt")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("r pt")) << _T("(i)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("pt")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("pt")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // total cross section restraining force
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Ncr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Nsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Nps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Mcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Msh")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(_T("Mps")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // initial strains
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("cr"));
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("sh"));
      (*pTable2)(0,col2++) << Overline(symbol(epsilon) << _T("ps"));
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("cr"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("sh"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Overline(symbol(phi)) << _T("ps"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // cross section forces in this interval due to initial strains
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i cr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i sh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("i ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i sh")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("i ps")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // Deformations due to externally applied loads and restraining forces (symbols might not be exactly correct)
      (*pTable2)(0,col2++) << symbol(DELTA) << symbol(epsilon) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << symbol(phi) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // Deformations in each part during this interval

      // girder
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("g"));
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(symbol(phi),_T("g")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // deck
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("d"));
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(symbol(phi),_T("d")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // deck top mat rebar
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("tm"));

      // deck bottom mat rebar
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("bm"));

      // girder rebar
      for ( IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("gb"));
      }

      // strand (straight)
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("ps s"));

      // strand (harped)
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("ps h"));

      // strand (temporary)
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("ps t"));

      // tendon
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("pt"));

      // Component forces due to deformations during this interval

      // girder
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("g")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("d")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("d")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck top mat rebar
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("tm")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // deck bottom mat rebar
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("bm")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // girder rebar
      for ( IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("gb")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      }

      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps s")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps h")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ps t")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // tendons
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("pt")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // Component forces at the end of this interval

      // girder
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("g")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("g")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("d")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("d")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      // deck top mat rebar
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("tm")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // deck bottom mat rebar
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("bm")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // girder rebar
      for ( IndexType rebarIdx = 0; rebarIdx < nRebar; rebarIdx++ )
      {
         (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("gb")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());
      }

      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps s")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps h")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ps t")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // tendons
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("pt")), rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit());

      // Elastic strains during this interval

      // girder
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("ge"));
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(symbol(phi),_T("ge")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // deck
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(symbol(epsilon),_T("de"));
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(symbol(phi),_T("de")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());

      // Change in strand stress during this interval
      
      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps ")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse ")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
      // strands (harped)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps h")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse h")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
      // strands (temporary)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps t")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse t")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
      // tendons
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pte")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // girder stress
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("bot girder")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("top girder")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("top slab")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

      // check
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("ext")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("ext")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("N"),_T("int")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("N"),_T("int")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // check
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("ext")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("ext")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("M"),_T("int")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("M"),_T("int")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);

      RowIndexType row2 = pTable2->GetNumberOfHeaderRows();

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++, row2++ )
      {
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);
         col2 = 0;
         (*pTable2)(row2,col2++) << LABEL_INTERVAL(intervalIdx);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Atr);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Ytr);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Itr);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.M);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Girder.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.Ytn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.Ybn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Girder.In);
         (*pTable2)(row2,col2++) << tsDetails.Girder.e;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Girder.r);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.PrCreep+tsDetails.Girder.PrShrinkage);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.MrCreep);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Slab.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Slab.Ytn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Slab.Ybn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Slab.In);
         (*pTable2)(row2,col2++) << tsDetails.Slab.e;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Slab.r);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Slab.PrCreep+tsDetails.Slab.PrShrinkage);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Slab.MrCreep);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].As);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].Ys);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].As);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].Ys);

         std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
         std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);

            (*pTable2)(row2,col2++) << area.SetValue(tsRebar.As);
            (*pTable2)(row2,col2++) << ecc.SetValue(tsRebar.Ys);
         }

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
            (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Strands[strandType].As);
            (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Strands[strandType].Ys);
            (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[strandType].fr);
            (*pTable2)(row2,col2++) << tsDetails.Strands[strandType].er;
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[strandType].Pr);
         }

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2+0) << area.SetValue(tsDetails.Tendons[ductIdx].As) << rptNewLine;
            (*pTable2)(row2,col2+1) << ecc.SetValue(tsDetails.Tendons[ductIdx].Ys) << rptNewLine;
            (*pTable2)(row2,col2+2) << stress.SetValue(tsDetails.Tendons[ductIdx].fr) << rptNewLine;
            (*pTable2)(row2,col2+3) << tsDetails.Tendons[ductIdx].er << rptNewLine;
            (*pTable2)(row2,col2+4) << force.SetValue(tsDetails.Tendons[ductIdx].Pr) << rptNewLine;
         }
         col2 += 5;

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CREEP] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SHRINKAGE] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_RELAXATION] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CREEP] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SHRINKAGE] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_RELAXATION] );

         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CREEP][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SHRINKAGE][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_RELAXATION][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CREEP][pgsTypes::Ahead]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SHRINKAGE][pgsTypes::Ahead]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_RELAXATION][pgsTypes::Ahead]);

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_CREEP] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_SHRINKAGE] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_RELAXATION] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CREEP]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_SHRINKAGE]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_RELAXATION]);

         (*pTable2)(row2,col2++) << tsDetails.er;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.rr);

         (*pTable2)(row2,col2++) << tsDetails.Girder.de;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Girder.dr);
         (*pTable2)(row2,col2++) << tsDetails.Slab.de;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Slab.dr);
         (*pTable2)(row2,col2++) << tsDetails.DeckRebar[pgsTypes::drmTop].de;
         (*pTable2)(row2,col2++) << tsDetails.DeckRebar[pgsTypes::drmBottom].de;

         iter = tsDetails.GirderRebar.begin();
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);

            (*pTable2)(row2,col2++) << tsRebar.de;
         }

         for ( int i = 0; i < 3; i++ )
         {
            (*pTable2)(row2,col2++) << tsDetails.Strands[i].de;
         }
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2) << tsDetails.Tendons[ductIdx].de << rptNewLine;
         }
         col2 += 1;

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.dP);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.dM);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Slab.dP);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Slab.dM);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].dP);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].dP);

         iter = tsDetails.GirderRebar.begin();
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);
            (*pTable2)(row2,col2++) << force.SetValue(tsRebar.dP);
         }

         for ( int i = 0; i < 3; i++ )
         {
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[i].dP);
         }
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2) << force.SetValue(tsDetails.Tendons[ductIdx].dP) << rptNewLine;
         }
         col2 += 1;

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.M);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Slab.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Slab.M);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].P);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].P);

         iter = tsDetails.GirderRebar.begin();
         for ( ; iter != end; iter++ )
         {
            const TIME_STEP_REBAR& tsRebar(*iter);
            (*pTable2)(row2,col2++) << force.SetValue(tsRebar.P);
         }

         for ( int i = 0; i < 3; i++ )
         {
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[i].P);
         }

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2) << force.SetValue(tsDetails.Tendons[ductIdx].P) << rptNewLine;
         }
         col2 += 1;

         (*pTable2)(row2,col2++) << tsDetails.Girder.dee;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Girder.der);
         (*pTable2)(row2,col2++) << tsDetails.Slab.dee;
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Slab.der);

         for ( int i = 0; i < 3; i++ )
         {
            (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[i].dFps);
            (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[i].fpe);
         }

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2+0) << stress.SetValue(tsDetails.Tendons[ductIdx].dFps) << rptNewLine;
            (*pTable2)(row2,col2+1) << stress.SetValue(tsDetails.Tendons[ductIdx].fpe)  << rptNewLine;
         }
         col2 += 2;

         (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Girder.fBot);
         (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Girder.fTop);
         (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Slab.fTop);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.dPext);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Pext);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.dPint);
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Pint);

         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMext);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mext);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.dMint);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mint);
      } // next interval


      // After last interval, report total loss
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[nIntervals-1]);
      col2 = 0;
      (*pTable2)(row2,col2++) << _T("Total");

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Atr);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Ytr);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Itr);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.M);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Girder.An);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Girder.Ynt);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Girder.Ynb);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Girder.In);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.e;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Girder.r);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.Pr);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.Mr);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Slab.An);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Slab.Ynt);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Slab.Ynb);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Slab.In);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Slab.e;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Slab.r);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Slab.Pr);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Slab.Mr);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].As);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].Ys);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].As);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].Ys);

      std::vector<TIME_STEP_REBAR>::const_iterator iter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::const_iterator end(tsDetails.GirderRebar.end());
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);

         (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsRebar.As);
         (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsRebar.Ys);
      }

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
         (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Strands[strandType].As);
         (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Strands[strandType].Ys);
         (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Strands[strandType].fr);
         (*pTable2)(row2,col2++) << _T(""); //tsDetails.Strands[strandType].er;
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[strandType].Pr);
      }

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         (*pTable2)(row2,col2+0) << _T(""); //area.SetValue(tsDetails.Tendons[ductIdx].As) << rptNewLine;
         (*pTable2)(row2,col2+1) << _T(""); //ecc.SetValue(tsDetails.Tendons[ductIdx].Ys) << rptNewLine;
         (*pTable2)(row2,col2+2) << _T(""); //stress.SetValue(tsDetails.Tendons[ductIdx].fr) << rptNewLine;
         (*pTable2)(row2,col2+3) << _T(""); //tsDetails.Tendons[ductIdx].er << rptNewLine;
         (*pTable2)(row2,col2+4) << _T(""); //force.SetValue(tsDetails.Tendons[ductIdx].Pr) << rptNewLine;
      }
      col2 += 5;

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CREEP] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SHRINKAGE] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_RELAXATION] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CREEP] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SHRINKAGE] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_RELAXATION] );

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CREEP][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SHRINKAGE][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_RELAXATION][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CREEP][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SHRINKAGE][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_RELAXATION][pgsTypes::Ahead]);

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_CREEP] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_SHRINKAGE] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_RELAXATION] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CREEP]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_SHRINKAGE]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_RELAXATION]);

      (*pTable2)(row2,col2++) << _T(""); //tsDetails.er;
      (*pTable2)(row2,col2++) << _T(""); //::ConvertFromSysUnits(tsDetails.rr,pDisplayUnits->GetCurvatureUnit().UnitOfMeasure);

      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.de;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Girder.dr);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Slab.de;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Slab.dr);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.DeckRebar[pgsTypes::drmTop].de;
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.DeckRebar[pgsTypes::drmBottom].de;

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);

         (*pTable2)(row2,col2++) << _T(""); //tsRebar.de;
      }

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << _T(""); //tsDetails.Strands[i].de;
      }
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         (*pTable2)(row2,col2) << _T(""); //tsDetails.Tendons[ductIdx].de << rptNewLine;
      }
      col2 += 1;

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.dM);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Slab.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Slab.dM);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].dP);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].dP);

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsRebar.dP);
      }

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[i].dP);
      }
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         (*pTable2)(row2,col2) << _T(""); //force.SetValue(tsDetails.Tendons[ductIdx].dP) << rptNewLine;
      }
      col2 += 1;

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.M);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Slab.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Slab.M);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmTop].P);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.DeckRebar[pgsTypes::drmBottom].P);

      iter = tsDetails.GirderRebar.begin();
      for ( ; iter != end; iter++ )
      {
         const TIME_STEP_REBAR& tsRebar(*iter);
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsRebar.P);
      }

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Strands[i].P);
      }

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         (*pTable2)(row2,col2) << _T(""); //force.SetValue(tsDetails.Tendons[ductIdx].P) << rptNewLine;
      }
      col2 += 1;

      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.dee;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Girder.der);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Slab.dee;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Slab.der);

      for ( int i = 0; i < 3; i++ )
      {
         (*pTable2)(row2,col2++) << stress.SetValue(tsDetails.Strands[i].loss);
         (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Strands[i].fpe);
      }

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         (*pTable2)(row2,col2+0) << stress.SetValue(tsDetails.Tendons[ductIdx].loss) << rptNewLine;
         (*pTable2)(row2,col2+1) << _T(""); //stress.SetValue(tsDetails.Tendons[ductIdx].fpe)  << rptNewLine;
      }
      col2 += 2;

      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::BottomGirder]);
      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::TopGirder]);
      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::TopSlab]);

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.dPext);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Pext);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.dPint);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Pint);

      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.dMext);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Mext);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.dMint);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Mint);
   } // next poi


   rptRcTable* pTable3 = pgsReportStyleHolder::CreateDefaultTable(1+6*nIntervals,_T("Initial Strain Analysis"));
   *pPara << pTable3 << rptNewLine;
   pTable3->SetNumberOfHeaderRows(2);
   ColumnIndexType col3 = 0;
   pTable3->SetRowSpan(0,col3,2);
   pTable3->SetRowSpan(1,col3,SKIP_CELL);
   (*pTable3)(0,col3++) << _T("POI");
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      pTable3->SetColumnSpan(0,col3,6);
      (*pTable3)(0,col3) << _T("Interval ") << LABEL_INTERVAL(intervalIdx);
      pTable3->SetColumnSpan(0,col3+1,SKIP_CELL);
      pTable3->SetColumnSpan(0,col3+2,SKIP_CELL);
      pTable3->SetColumnSpan(0,col3+3,SKIP_CELL);
      pTable3->SetColumnSpan(0,col3+4,SKIP_CELL);
      pTable3->SetColumnSpan(0,col3+5,SKIP_CELL);
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("P"),_T("r")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("M"),_T("r")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable3)(1,col3++) << Sub2(symbol(epsilon),_T("i"));
      (*pTable3)(1,col3++) << COLHDR(Sub2(symbol(phi),_T("i")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("P"),_T("re")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable3)(1,col3++) << COLHDR(Sub2(_T("M"),_T("re")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   iter = vPOI.begin();
   RowIndexType row3 = pTable3->GetNumberOfHeaderRows();
   for ( ; iter != end; iter++ )
   {
      col3 = 0;

      pgsPointOfInterest& poi = *iter;
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);

      (*pTable3)(row3,col3++) << location.SetValue(POI_GIRDER,poi,0.0);

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.Pr[TIMESTEP_CREEP]+tsDetails.Pr[TIMESTEP_SHRINKAGE]+tsDetails.Pr[TIMESTEP_RELAXATION]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CREEP]+tsDetails.Mr[TIMESTEP_SHRINKAGE]+tsDetails.Mr[TIMESTEP_RELAXATION]);
         (*pTable3)(row3,col3++) << tsDetails.e[TIMESTEP_CREEP][pgsTypes::Ahead] + tsDetails.e[TIMESTEP_SHRINKAGE][pgsTypes::Ahead] + tsDetails.e[TIMESTEP_RELAXATION][pgsTypes::Ahead];
         (*pTable3)(row3,col3++) << ::ConvertFromSysUnits(tsDetails.r[TIMESTEP_CREEP][pgsTypes::Ahead]+tsDetails.r[TIMESTEP_SHRINKAGE][pgsTypes::Ahead]+tsDetails.r[TIMESTEP_RELAXATION][pgsTypes::Ahead],pDisplayUnits->GetCurvatureUnit().UnitOfMeasure);
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.Pre[TIMESTEP_CREEP]+tsDetails.Pre[TIMESTEP_SHRINKAGE]+tsDetails.Pre[TIMESTEP_RELAXATION]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CREEP]+tsDetails.Mre[TIMESTEP_SHRINKAGE]+tsDetails.Mre[TIMESTEP_RELAXATION]);
      }

      row3++;
   }

   return pChapter;
}

CChapterBuilder* CTimeStepParametersChapterBuilder::Clone() const
{
   return new CTimeStepParametersChapterBuilder;
}
