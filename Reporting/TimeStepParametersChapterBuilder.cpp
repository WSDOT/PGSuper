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
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#include <WBFLGenericBridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// When defined, the computation details for initial strains are reported
// This is the summation part of Tadros 1977 Eqns 3 and 4.
//#define REPORT_INITIAL_STRAIN_DETAILS

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
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
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
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        length,   pDisplayUnits->GetSpanLengthUnit(), false);

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
         (*pPara) << _T("Closure Joint at TS ") << LABEL_SEGMENT(segIdx) << rptNewLine;

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
            (*pTable)(row,col++) << pMaterials->GetClosureJointConcreteAge(closureKey,intervalIdx);
            (*pTable)(row,col++) << stress.SetValue(pMaterials->GetClosureJointFc(closureKey,intervalIdx));
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosureJointEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            (*pTable)(row,col++) << pMaterials->GetClosureJointAgingCoefficient(closureKey,intervalIdx);
            (*pTable)(row,col++) << modE.SetValue(pMaterials->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx));
            (*pTable)(row,col++) << 1E6*pMaterials->GetClosureJointFreeShrinkageStrain(closureKey,intervalIdx);
            (*pTable)(row,col++) << 1E6*pMaterials->GetClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End);
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
               (*pCreepTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
               (*pCreepTable)(row,col++) << pMaterials->GetClosureJointCreepCoefficient(closureKey,loadIntIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
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

   std::vector<pgsPointOfInterest> vPOI(pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

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

      ColumnIndexType nColumns = 109 + 4*nRebar;
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
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("gcr")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("gcr")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("gcr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("gcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("gsh")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("gsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

      // Net slab properties, unrestrained deformations, and component restraining forces
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("A"),_T("nd")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("tnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("Y"),_T("bnd")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("I"),_T("nd")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("dcr")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(Overline(symbol(phi)),_T("dcr")) << _T("(i)"), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("dcr")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("M")),_T("dcr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*pTable2)(0,col2++) << symbol(DELTA) << Sub2(Overline(symbol(epsilon)),_T("dsh")) << _T("(i)");
      (*pTable2)(0,col2++) << COLHDR(Sub2(Overline(_T("N")),_T("dsh")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());

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

      // Change in strand stress during this interval
      
      // strands (straight)
      (*pTable2)(0,col2++) << COLHDR(symbol(DELTA) << Sub2(_T("f"),_T("ps s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("pse s")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      
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
      (*pTable2)(0,col2++) << COLHDR(Sub2(_T("f"),_T("top deck")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

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

         Float64 P = 0;
         Float64 M = 0;
         for ( int i = 0; i < 15; i++ )
         {
            P += tsDetails.dP[i];
            M += tsDetails.dM[i];
         }

         (*pTable2)(row2,col2++) << force.SetValue(P);
         (*pTable2)(row2,col2++) << moment.SetValue(M);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Girder.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.Ytn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Girder.Ybn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Girder.In);

#if defined REPORT_INITIAL_STRAIN_DETAILS
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator creepStrainIter(tsDetails.Girder.ec.begin());
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator creepStrainIterEnd(tsDetails.Girder.ec.end());
         std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::size_type i = tsDetails.Girder.ec.size()-1;
         for ( ; creepStrainIter != creepStrainIterEnd; creepStrainIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_STRAIN& creepStrain(*creepStrainIter);
            (*pTable2)(row2,col2) << _T("[") << force.SetValue(creepStrain.P) << _T("/((") << modE.SetValue(creepStrain.E) << _T(")(") << area.SetValue(creepStrain.A) << _T("))](") << creepStrain.Ce << _T(" - ") << creepStrain.Cs << _T(")");
            if ( i != 0 )
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            else
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << tsDetails.Girder.e;

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator creepCurvatureIter(tsDetails.Girder.rc.begin());
         std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator creepCurvatureIterEnd(tsDetails.Girder.rc.end());
         i = tsDetails.Girder.rc.size()-1;
         for ( ; creepCurvatureIter != creepCurvatureIterEnd; creepCurvatureIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_CURVATURE& creepCurvature(*creepCurvatureIter);
            (*pTable2)(row2,col2) << _T("[(") << moment.SetValue(creepCurvature.M) << _T(")(12)/((") << modE.SetValue(creepCurvature.E) << _T(")(") << momI.SetValue(creepCurvature.I) << _T("))](") << creepCurvature.Ce << _T(" - ") << creepCurvature.Cs << _T(")");
            if ( i != 0 )
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            else
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Girder.r);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.PrCreep);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Girder.MrCreep);
         (*pTable2)(row2,col2++) << tsDetails.Girder.esi;
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Girder.PrShrinkage);

         (*pTable2)(row2,col2++) << area.SetValue(tsDetails.Deck.An);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Deck.Ytn);
         (*pTable2)(row2,col2++) << ecc.SetValue(tsDetails.Deck.Ybn);
         (*pTable2)(row2,col2++) << momI.SetValue(tsDetails.Deck.In);

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         creepStrainIter    = tsDetails.Deck.ec.begin();
         creepStrainIterEnd = tsDetails.Deck.ec.end();
         i = tsDetails.Deck.ec.size()-1;
         for ( ; creepStrainIter != creepStrainIterEnd; creepStrainIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_STRAIN& creepStrain(*creepStrainIter);
            (*pTable2)(row2,col2) << _T("[") << force.SetValue(creepStrain.P) << _T("/((") << modE.SetValue(creepStrain.E) << _T(")(") << area.SetValue(creepStrain.A) << _T("))](") << creepStrain.Ce << _T(" - ") << creepStrain.Cs << _T(")");
            if ( i != 0 )
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            else
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << tsDetails.Deck.e;

         
#if defined REPORT_INITIAL_STRAIN_DETAILS
         creepCurvatureIter    = tsDetails.Deck.rc.begin();
         creepCurvatureIterEnd = tsDetails.Deck.rc.end();
         i = tsDetails.Deck.rc.size()-1;
         for ( ; creepCurvatureIter != creepCurvatureIterEnd; creepCurvatureIter++, i-- )
         {
            const TIME_STEP_CONCRETE::CREEP_CURVATURE& creepCurvature(*creepCurvatureIter);
            (*pTable2)(row2,col2) << _T("[(") << moment.SetValue(creepCurvature.M) << _T(")(12)/((") << modE.SetValue(creepCurvature.E) << _T(")(") << momI.SetValue(creepCurvature.I) << _T("))](") << creepCurvature.Ce << _T(" - ") << creepCurvature.Cs << _T(")");
            if ( i != 0 )
               (*pTable2)(row2,col2) << _T(" + ") << rptNewLine;
            else
               (*pTable2)(row2,col2) << _T(" = ") << rptNewLine;
         }
#endif // REPORT_INITIAL_STRAIN_DETAILS
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.Deck.r);

         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.PrCreep);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Deck.MrCreep);
         (*pTable2)(row2,col2++) << tsDetails.Deck.esi;
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.PrShrinkage);

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
            (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Strands[strandType].PrRelaxation);
         }

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            (*pTable2)(row2,col2+0) << area.SetValue(tsDetails.Tendons[ductIdx].As) << rptNewLine;
            (*pTable2)(row2,col2+1) << ecc.SetValue(tsDetails.Tendons[ductIdx].Ys) << rptNewLine;
            (*pTable2)(row2,col2+2) << stress.SetValue(tsDetails.Tendons[ductIdx].fr) << rptNewLine;
            (*pTable2)(row2,col2+3) << tsDetails.Tendons[ductIdx].er << rptNewLine;
            (*pTable2)(row2,col2+4) << force.SetValue(tsDetails.Tendons[ductIdx].PrRelaxation) << rptNewLine;
         }
         col2 += 5;

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CR] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SH] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_PS] );

         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SH] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_PS] );

         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CR][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SH][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_PS][pgsTypes::Ahead];
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR][pgsTypes::Ahead]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH][pgsTypes::Ahead]);
         (*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_PS][pgsTypes::Ahead]);

         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_CR] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_SH] );
         (*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_PS] );
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CR]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_SH]);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_PS]);

         Float64 der = 0;
         Float64 drr = 0;
         Float64 dPgirder = 0;
         Float64 dMgirder = 0;
         Float64 dPdeck = 0;
         Float64 dMdeck = 0;
         for ( int i = 0; i < 18; i++ )
         {
            der = tsDetails.der[i];
            drr = tsDetails.drr[i];

            dPgirder = tsDetails.Girder.dP[i];
            dMgirder = tsDetails.Girder.dM[i];

            dPdeck = tsDetails.Deck.dP[i];
            dMdeck = tsDetails.Deck.dM[i];
         }

         (*pTable2)(row2,col2++) << der;
         (*pTable2)(row2,col2++) << curvature.SetValue(drr);

         (*pTable2)(row2,col2++) << force.SetValue( dPgirder );
         (*pTable2)(row2,col2++) << moment.SetValue( dMgirder );
         (*pTable2)(row2,col2++) << force.SetValue( dPdeck );
         (*pTable2)(row2,col2++) << moment.SetValue( dMdeck );
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
         (*pTable2)(row2,col2++) << force.SetValue(tsDetails.Deck.P);
         (*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Deck.M);
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

         (*pTable2)(row2,col2++) << stress.SetValue(/*tsDetails.Girder.fBot[TIMESTEP_DC][ctCummulative] + tsDetails.Girder.fBot[TIMESTEP_DW][ctCummulative] + */tsDetails.Girder.fBot[TIMESTEP_CR][ctCummulative] + tsDetails.Girder.fBot[TIMESTEP_SH][ctCummulative] + tsDetails.Girder.fBot[TIMESTEP_PS][ctCummulative]);
         (*pTable2)(row2,col2++) << stress.SetValue(/*tsDetails.Girder.fTop[TIMESTEP_DC][ctCummulative] + tsDetails.Girder.fTop[TIMESTEP_DW][ctCummulative] + */tsDetails.Girder.fTop[TIMESTEP_CR][ctCummulative] + tsDetails.Girder.fTop[TIMESTEP_SH][ctCummulative] + tsDetails.Girder.fTop[TIMESTEP_PS][ctCummulative]);
         (*pTable2)(row2,col2++) << stress.SetValue(/*tsDetails.Deck.fTop[TIMESTEP_DC][ctCummulative] + tsDetails.Deck.fTop[TIMESTEP_DW][ctCummulative] + */tsDetails.Deck.fTop[TIMESTEP_CR][ctCummulative] + tsDetails.Deck.fTop[TIMESTEP_SH][ctCummulative] + tsDetails.Deck.fTop[TIMESTEP_PS][ctCummulative]);

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
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.PrCreep);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.MrCreep);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Girder.esi;
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.PrShrinkage);

      (*pTable2)(row2,col2++) << _T(""); //area.SetValue(tsDetails.Deck.An);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Deck.Ynt);
      (*pTable2)(row2,col2++) << _T(""); //ecc.SetValue(tsDetails.Deck.Ynb);
      (*pTable2)(row2,col2++) << _T(""); //momI.SetValue(tsDetails.Deck.In);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Deck.e;
      (*pTable2)(row2,col2++) << _T(""); //curvature.SetValue(tsDetails.Deck.r);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.PrCreep);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.MrCreep);
      (*pTable2)(row2,col2++) << _T(""); //tsDetails.Deck.esi;
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.PrShrinkage);

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

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pr[TIMESTEP_PS] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mr[TIMESTEP_PS] );

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_CR][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_SH][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << tsDetails.e[TIMESTEP_PS][pgsTypes::Ahead];
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_CR][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_SH][pgsTypes::Ahead]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << curvature.SetValue(tsDetails.r[TIMESTEP_PS][pgsTypes::Ahead]);

      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_CR] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_SH] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << force.SetValue( tsDetails.Pre[TIMESTEP_PS] );
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CR]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_SH]);
      (*pTable2)(row2,col2++) << _T(""); //(*pTable2)(row2,col2++) << moment.SetValue(tsDetails.Mre[TIMESTEP_PS]);

      (*pTable2)(row2,col2++) << _T(""); //tsDetails.er;
      (*pTable2)(row2,col2++) << _T(""); //::ConvertFromSysUnits(tsDetails.rr,pDisplayUnits->GetCurvatureUnit().UnitOfMeasure);

      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Girder.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Girder.dM);
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.dP);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.dM);
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
      (*pTable2)(row2,col2++) << _T(""); //force.SetValue(tsDetails.Deck.P);
      (*pTable2)(row2,col2++) << _T(""); //moment.SetValue(tsDetails.Deck.M);
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
      (*pTable2)(row2,col2++) << _T(""); //stress.SetValue(tsDetails.Stress[pgsTypes::TopDeck]);

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
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.Pr[TIMESTEP_CR]+tsDetails.Pr[TIMESTEP_SH]+tsDetails.Pr[TIMESTEP_PS]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]+tsDetails.Mr[TIMESTEP_SH]+tsDetails.Mr[TIMESTEP_PS]);
         (*pTable3)(row3,col3++) << tsDetails.e[TIMESTEP_CR][pgsTypes::Ahead] + tsDetails.e[TIMESTEP_SH][pgsTypes::Ahead] + tsDetails.e[TIMESTEP_PS][pgsTypes::Ahead];
         (*pTable3)(row3,col3++) << ::ConvertFromSysUnits(tsDetails.r[TIMESTEP_CR][pgsTypes::Ahead]+tsDetails.r[TIMESTEP_SH][pgsTypes::Ahead]+tsDetails.r[TIMESTEP_PS][pgsTypes::Ahead],pDisplayUnits->GetCurvatureUnit().UnitOfMeasure);
         (*pTable3)(row3,col3++) << force.SetValue(tsDetails.Pre[TIMESTEP_CR]+tsDetails.Pre[TIMESTEP_SH]+tsDetails.Pre[TIMESTEP_PS]);
         (*pTable3)(row3,col3++) << moment.SetValue(tsDetails.Mre[TIMESTEP_CR]+tsDetails.Mre[TIMESTEP_SH]+tsDetails.Mre[TIMESTEP_PS]);
      }

      row3++;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////
   //INIT_UV_PROTOTYPE( rptMomentUnitValue,        moment2,   pDisplayUnits->GetSmallMomentUnit(), false);

   //rptRcTable* pTable4 = pgsReportStyleHolder::CreateDefaultTable(5,_T("Deflection"));
   //*pPara << pTable4 << rptNewLine;
   //(*pTable4)(0,0) << _T("POI");
   //(*pTable4)(0,1) << _T("Xg");
   //(*pTable4)(0,2) << COLHDR(_T("m"), rptMomentUnitTag, pDisplayUnits->GetSmallMomentUnit());
   //(*pTable4)(0,3) << COLHDR(symbol(DELTA) << symbol(phi), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
   //(*pTable4)(0,4) << _T("dmc");

   //IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(CSegmentKey(girderKey,nSegments/2));

   //GET_IFACE2(pBroker,IInfluenceResults,pInflResults);
   //std::vector<pgsPointOfInterest> v(pPOI->GetPointsOfInterest(CSegmentKey(girderKey,nSegments/2),POI_MIDSPAN | POI_ERECTED_SEGMENT));
   //ATLASSERT(v.size() == 1);
   //const pgsPointOfInterest& midPoi(v.front());
   //std::vector<Float64> moments(pInflResults->GetUnitLoadMoment(vPOI,midPoi,pgsTypes::ContinuousSpan,erectSegmentIntervalIdx));
   //std::vector<Float64>::iterator mIter(moments.begin());
   //iter = vPOI.begin();
   //row = 1;
   //Float64 delta = 0;
   //for ( ; iter != end; iter++, mIter++, row++ )
   //{
   //   ColumnIndexType col = 0;

   //   const pgsPointOfInterest& poi(*iter);
   //   Float64 m = *mIter;
   //   
   //   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);
   //   Float64 c = pDetails->TimeStepDetails[erectSegmentIntervalIdx].rr;

   //   Float64 Xg = pPOI->ConvertPoiToGirderCoordinate(poi);

   //   (*pTable4)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, 0.0 );
   //   (*pTable4)(row,col++) << length.SetValue( Xg );
   //   (*pTable4)(row,col++) << ::ConvertFromSysUnits(m*::ConvertToSysUnits(1.0,unitMeasure::Kip),unitMeasure::KipInch);//moment2.SetValue( m );
   //   (*pTable4)(row,col++) << curvature.SetValue( c );

   //}

   return pChapter;
}

CChapterBuilder* CTimeStepParametersChapterBuilder::Clone() const
{
   return new CTimeStepParametersChapterBuilder;
}
