///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\PrincipalWebStressDetailsChapterBuilder.h>
#include <Reporting\PrincipalWebStressDetailsReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\PrincipalWebStress.h>

#include <WBFLGenericBridgeTools.h>

#include <psgLib/PrincipalTensionStressCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IFNFIRSTNEWLINE(bFirst, pTable, RowIdx, ColIdx) if (!bFirst) { (*pTable)(RowIdx, ColIdx) << rptNewLine; }

/****************************************************************************
CLASS
   CPrincipalWebStressDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrincipalWebStressDetailsChapterBuilder::CPrincipalWebStressDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPrincipalWebStressDetailsChapterBuilder::GetName() const
{
   return TEXT("Principal Web Stress Details");
}

rptChapter* CPrincipalWebStressDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   auto pTSDRptSpec = std::dynamic_pointer_cast<const CPrincipalWebStressDetailsReportSpecification>(pRptSpec);

   m_bReportShear = pTSDRptSpec->ReportShear();
   m_bReportAxial = pTSDRptSpec->ReportAxial();

   CComPtr<IBroker> pBroker;
   pTSDRptSpec->GetBroker(&pBroker);

   // check if method is correct. Either bail out here should have been checked before this, but...
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step Principal Web Stress analysis results not available when time step losses not used.") << color(Black) << rptNewLine;
      return pChapter;
   }
   else
   {
      GET_IFACE2(pBroker,ILibrary, pLib);
      GET_IFACE2(pBroker,ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();
      if (principal_tension_stress_criteria.Method != pgsTypes::ptsmNCHRP)
      {
         *pPara << color(Red) << _T("Time Step Principal Web Stress analysis results not available when NCHRP method not used.") << color(Black) << rptNewLine;
         return pChapter;
      }
   }

   IntervalIndexType rptIntervalIdx = pTSDRptSpec->GetInterval();

   const pgsPointOfInterest& rptPoi(pTSDRptSpec->GetPointOfInterest());
   CSegmentKey segmentKey(rptPoi.GetSegmentKey());
   PoiList vPoi;
   if ( pTSDRptSpec->ReportAtAllLocations() )
   {
      segmentKey.segmentIndex = ALL_SEGMENTS;

      GET_IFACE2(pBroker, IPrincipalWebStress, pPrincipalWebStress);
      pPrincipalWebStress->GetPrincipalWebStressPointsOfInterest(segmentKey, rptIntervalIdx, &vPoi);
   }
   else
   {
      vPoi.push_back(rptPoi);
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      true);
   location.IncludeSpanAndGirder(true);

   // reporting for a specific poi... list poi at top of report
   if ( !pTSDRptSpec->ReportAtAllLocations() )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      CString strLocation;
      rptReportContent& rcLocation = location.SetValue(POI_SPAN,rptPoi);
#if defined _DEBUG || defined _BETA_VERSION
      strLocation.Format(_T("Point Of Interest: ID = %d %s"),rptPoi.GetID(),location.AsString().c_str());
#else
      strLocation.Format(_T("%s"),location.AsString().c_str());
#endif
      strLocation.Replace(_T("<sub>"),_T(""));
      strLocation.Replace(_T("</sub>"),_T(""));

      *pPara << rcLocation << rptNewLine;
      //pPara->SetName(strLocation);
      *pPara << rptNewLine;
   }

   // reporting for a specific interval... list interval name at top of report
   if ( rptIntervalIdx != INVALID_INDEX )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      CString str;
      str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(rptIntervalIdx),pIntervals->GetDescription(rptIntervalIdx).c_str());
      *pPara << str << rptNewLine;
      //pPara->SetName(str);
   }

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   // Incremental stresses
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType firstIntervalIdx = (rptIntervalIdx == INVALID_INDEX ? 0 : rptIntervalIdx);
   IntervalIndexType lastIntervalIdx  = (rptIntervalIdx == INVALID_INDEX ? nIntervals-1 : rptIntervalIdx);
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      // Heading
      if ( rptIntervalIdx == INVALID_INDEX )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;

         CString str;
         str.Format(_T("Incremental Web Stresses - Interval %d : %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
         *pPara << str << rptNewLine;
         pPara->SetName(str);
      }

      std::vector<pgsTypes::ProductForceType> vLoads = pProductLoads->GetProductForcesForGirder(segmentKey);

       BuildIncrementalStressTables(pChapter, pBroker, intervalIdx, vPoi, vLoads, pDisplayUnits);

      if (liveLoadIntervalIdx <= intervalIdx)
      {
          BuildLiveLoadStressTable(pChapter, pBroker, intervalIdx, vPoi, pDisplayUnits);
      }
   }

   // Combined stresses. From live load to last interval
   for ( IntervalIndexType intervalIdx = liveLoadIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      // Heading
      if ( rptIntervalIdx == INVALID_INDEX )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;

         CString str;
         str.Format(_T("Combined Web Stresses - Interval %d : %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx).c_str());
         *pPara << str << rptNewLine;
         pPara->SetName(str);
      }


       BuildCombinedStressTables(pChapter, pBroker, intervalIdx, vPoi, pDisplayUnits);
      (*pPara) << rptNewLine;
   }


   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CPrincipalWebStressDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CPrincipalWebStressDetailsChapterBuilder>();
}

void CPrincipalWebStressDetailsChapterBuilder::BuildIncrementalStressTables(rptChapter* pChapter, IBroker* pBroker, IntervalIndexType intervalIdx, PoiList vPoi, const std::vector<pgsTypes::ProductForceType>& vLoads, IEAFDisplayUnits* pDisplayUnits) const
{
   if (!m_bReportShear && !m_bReportAxial)
   {
      return;
   }

   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   inertia,    pDisplayUnits->GetMomentOfInertiaUnit(),  false );
   INIT_UV_PROTOTYPE(rptLength3UnitValue,   l3,         pDisplayUnits->GetSectModulusUnit(),      false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    false );

   location.IncludeSpanAndGirder(true);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   // Fit up to five product loads per table
   const IndexType nLoadsPerTable = 5;
   IndexType nLoads = vLoads.size();
   IndexType nTables = nLoads / nLoadsPerTable;
   IndexType modTableLoads = nLoads % nLoadsPerTable;
   if (0 < modTableLoads)
   {
      nTables++;
   }

   std::vector<pgsTypes::ProductForceType>::const_iterator itLoad = vLoads.begin();

   if (m_bReportShear)
   {
      bool bWasDuctReduced = false;
      rptParagraph* pShearPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pShearPara;
      (*pShearPara) << _T("Incremental Shear Stresses") << rptNewLine;
      pShearPara = new rptParagraph;
      (*pChapter) << pShearPara;
      (*pShearPara) << rptRcImage(strImagePath + _T("PrincipalShearStress.png")) << rptNewLine;

      for (IndexType iTable = 0; iTable < nTables; iTable++)
      {
         IndexType nTableLoads = (iTable == nTables-1 &&  modTableLoads!=0) ? modTableLoads : nLoadsPerTable;

         // Shear Stress table header
         rptRcTable* pShearStressTable = rptStyleManager::CreateDefaultTable(6 + nTableLoads * 3);
         *pShearPara << pShearStressTable << rptNewLine;
         RowIndexType shearStressRowIdx = 0;
         ColumnIndexType shearStressColIdx = 0;

         pShearStressTable->SetNumberOfHeaderRows(2);
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Support"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(Sub2(_T("I"), _T("tr")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << _T("Web Location");
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(_T("Q"), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit());
         pShearStressTable->SetRowSpan(shearStressRowIdx, shearStressColIdx, 2);
         (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(Sub2(_T("b"), _T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

         std::vector<pgsTypes::ProductForceType>::const_iterator itLoadHdr = itLoad;
         for (IndexType itableLoad = 0; itableLoad < nTableLoads; itableLoad++)
         {
            pgsTypes::ProductForceType pfType = *itLoadHdr++;

            pShearStressTable->SetColumnSpan(shearStressRowIdx, shearStressColIdx, 3);
            (*pShearStressTable)(shearStressRowIdx, shearStressColIdx) << pProductLoads->GetProductLoadName(pfType);

            shearStressRowIdx++;
            (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(symbol(DELTA) << _T("V"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
            (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(symbol(DELTA) << symbol(tau), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << COLHDR(symbol(SIGMA) << symbol(tau), rptStressUnitTag, pDisplayUnits->GetStressUnit());

            shearStressRowIdx = 0;
         }

         shearStressRowIdx = pShearStressTable->GetNumberOfHeaderRows();

         // Fill tables
         for (const auto& poi : vPoi)
         {
            const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
            const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

            (*pShearStressTable)(shearStressRowIdx, 0) << location.SetValue(POI_SPAN, poi);
            (*pShearStressTable)(shearStressRowIdx, 1) << inertia.SetValue(tsDetails.Itr);

            // All load cases have same number of web section elevations
            bool bFirstLoad = true;
            std::vector<pgsTypes::ProductForceType>::const_iterator itLoadTbl = itLoad;
            for (IndexType itableLoad = 0; itableLoad < nTableLoads; itableLoad++)
            {
               pgsTypes::ProductForceType pfType = *itLoadTbl++;
               const TIME_STEP_PRINCIPALSTRESSINWEBDETAILS& prDetails = tsDetails.PrincipalStressDetails[pfType];

               // Shear stress table
               ColumnIndexType startshearStressColIdx;
               if (bFirstLoad)
               {
                  startshearStressColIdx = 2;
               }
               else
               {
                  startshearStressColIdx = shearStressColIdx;
               }

               bool bFirstWebSection = true;
               for (const auto& webSection : prDetails.WebSections)
               {
                  shearStressColIdx = startshearStressColIdx;

                  if (bFirstLoad)
                  {
                     IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                     (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << webSection.strLocation;
                     IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                     (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << ecc.SetValue(-webSection.YwebSection);
                     IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                     (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << l3.SetValue(webSection.Qc);
                     IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                     if (webSection.bIsShearWidthAdjustedForTendon)
                     {
                        bWasDuctReduced = true;
                        (*pShearStressTable)(shearStressRowIdx, shearStressColIdx) << _T("* ");
                     }
                     (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << ecc.SetValue(webSection.bw);
                  }

                  if (bFirstWebSection)
                  {
                     (*pShearStressTable)(shearStressRowIdx, shearStressColIdx) << force.SetValue(prDetails.Vu);
                  }
                  shearStressColIdx++;

                  IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                  (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << stress.SetValue(webSection.tau);
                  IFNFIRSTNEWLINE(bFirstWebSection, pShearStressTable, shearStressRowIdx, shearStressColIdx);
                  (*pShearStressTable)(shearStressRowIdx, shearStressColIdx++) << stress.SetValue(webSection.tau_s);

                  bFirstWebSection = false;
               }

               bFirstLoad = false;
            } // load

            shearStressRowIdx++;
         }

         itLoad += nTableLoads;
      }

      if (bWasDuctReduced)
      {
         (*pShearPara) << _T("* Web width reduced due to proximity to duct") << rptNewLine;
      }

      (*pShearPara) << _T("Y = elevation in web where principal stress is computed, measured downwards from top centerline of non-composite girder.") << rptNewLine;
   }

   if (m_bReportAxial)
   {
      // Axial Tables
      rptParagraph* pAxialPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      (*pChapter) << pAxialPara;
      (*pAxialPara) << _T("Incremental Axial Stresses") << rptNewLine;
      pAxialPara = new rptParagraph;
      (*pChapter) << pAxialPara;
      (*pAxialPara) << rptRcImage(strImagePath + _T("PrincipalAxialStress.png")) << rptNewLine;

      itLoad = vLoads.begin();
      for (IndexType iTable = 0; iTable < nTables; iTable++)
      {
         IndexType nTableLoads = (iTable == nTables-1 &&  modTableLoads!=0) ? modTableLoads : nLoadsPerTable;

         // Axial Stress table header
         rptRcTable* pAxialStressTable = rptStyleManager::CreateDefaultTable(4 + nTableLoads * 4);
         *pAxialPara << pAxialStressTable << rptNewLine;
         RowIndexType axialStressRowIdx = 0;
         ColumnIndexType axialStressColIdx = 0;

         pAxialStressTable->SetNumberOfHeaderRows(2);
         pAxialStressTable->SetRowSpan(axialStressRowIdx, axialStressColIdx, 2);
         (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(_T("Location from") << rptNewLine << _T("Left Support"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         pAxialStressTable->SetRowSpan(axialStressRowIdx, axialStressColIdx, 2);
         (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(Sub2(_T("H"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         pAxialStressTable->SetRowSpan(axialStressRowIdx, axialStressColIdx, 2);
         (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << _T("Web Location");
         pAxialStressTable->SetRowSpan(axialStressRowIdx, axialStressColIdx, 2);
         (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

         std::vector<pgsTypes::ProductForceType>::const_iterator itLoadHdr = itLoad;
         for (IndexType itableLoad = 0; itableLoad < nTableLoads; itableLoad++)
         {
            pgsTypes::ProductForceType pfType = *itLoadHdr++;

            pAxialStressTable->SetColumnSpan(axialStressRowIdx, axialStressColIdx, 4);
            if (pfType == (pgsTypes::ProductForceType)pftTimeStepSize)
            {
               (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx) << _T("Vp"); // special case
            }
            else
            {
               (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx) << pProductLoads->GetProductLoadName(pfType);
            }

            axialStressRowIdx++;
            (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("top")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("bot")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(symbol(DELTA) << Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << COLHDR(symbol(SIGMA) << Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

            axialStressRowIdx = 0;
         }

         axialStressRowIdx = pAxialStressTable->GetNumberOfHeaderRows();

         // Fill tables
         for (const auto& poi : vPoi)
         {
            const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
            const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

            (*pAxialStressTable)(axialStressRowIdx, 0) << location.SetValue(POI_SPAN, poi);
            (*pAxialStressTable)(axialStressRowIdx, 1) << ecc.SetValue(tsDetails.PrincipalStressDetails[pgsTypes::pftGirder].Hg);

            // All load cases have same number of web section elevations
            bool bFirstLoad = true;
            std::vector<pgsTypes::ProductForceType>::const_iterator itLoadTbl = itLoad;
            for (IndexType itableLoad = 0; itableLoad < nTableLoads; itableLoad++)
            {
               pgsTypes::ProductForceType pfType = *itLoadTbl++;
               const TIME_STEP_PRINCIPALSTRESSINWEBDETAILS& prDetails = tsDetails.PrincipalStressDetails[pfType];

               // Axial stress table
               ColumnIndexType startaxialStressColIdx;
               if (bFirstLoad)
               {
                  startaxialStressColIdx = 2;
               }
               else
               {
                  startaxialStressColIdx = axialStressColIdx;
               }

               bool bFirstWebSection = true;
               for (const auto& webSection : prDetails.WebSections)
               {
                  axialStressColIdx = startaxialStressColIdx;

                  if (bFirstLoad)
                  {
                     IFNFIRSTNEWLINE(bFirstWebSection, pAxialStressTable, axialStressRowIdx, axialStressColIdx);
                     (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << webSection.strLocation;
                     IFNFIRSTNEWLINE(bFirstWebSection, pAxialStressTable, axialStressRowIdx, axialStressColIdx);
                     (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << ecc.SetValue(-webSection.YwebSection);
                  }

                  if (bFirstWebSection)
                  {
                     (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << stress.SetValue(prDetails.fTop);
                     (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << stress.SetValue(prDetails.fBot);
                  }
                  else
                  {
                     axialStressColIdx += 2;
                  }

                  IFNFIRSTNEWLINE(bFirstWebSection, pAxialStressTable, axialStressRowIdx, axialStressColIdx);
                  (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << stress.SetValue(webSection.fpcx);
                  IFNFIRSTNEWLINE(bFirstWebSection, pAxialStressTable, axialStressRowIdx, axialStressColIdx);
                  (*pAxialStressTable)(axialStressRowIdx, axialStressColIdx++) << stress.SetValue(webSection.fpcx_s);

                  bFirstWebSection = false;
               }

               bFirstLoad = false;
            } // load

            axialStressRowIdx++;
         }

         itLoad += nTableLoads;
      }
   }
}

void CPrincipalWebStressDetailsChapterBuilder::BuildLiveLoadStressTable(rptChapter * pChapter, IBroker * pBroker, IntervalIndexType intervalIdx, PoiList vPoi, IEAFDisplayUnits * pDisplayUnits) const
{
   
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);
   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      false);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   location.IncludeSpanAndGirder(true);

   // Create different paragraphs for shear stress and axial stress tables
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Live Load Stresses") << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7);
   *pPara << pTable << rptNewLine;
   ColumnIndexType ColIdx = 0;

   (*pTable)(0, ColIdx++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0, ColIdx++) << _T("Web Location");
   (*pTable)(0, ColIdx++) << COLHDR(Sub2(_T("f"), _T("top")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, ColIdx++) << COLHDR(Sub2(_T("f"), _T("bot")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, ColIdx++) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, ColIdx++) << COLHDR(_T("V"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(0, ColIdx++) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());

   RowIndexType RowIdx = 1;
   IndexType startColIdx = 1;

   GET_IFACE2(pBroker,IPrincipalWebStress,pPrincipalWebStress);

   // Fill tables
   for (const auto& poi : vPoi)
   {
      const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* pWebStressDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(poi, intervalIdx);

      (*pTable)(RowIdx, 0)  << location.SetValue( POI_SPAN, poi );

      bool bFirstWebSection = true;
      for (const auto& webDetail : *pWebStressDetails)
      {
         ColIdx = startColIdx;

         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << webDetail.strLocation;
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Ftop);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Fbot);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Fpcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << force.SetValue(webDetail.LL_Vu);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Tau);

         bFirstWebSection = false;
      }

      RowIdx++;
   }
}


void CPrincipalWebStressDetailsChapterBuilder::BuildCombinedStressTables(rptChapter * pChapter, IBroker * pBroker, IntervalIndexType intervalIdx, PoiList vPoi, IEAFDisplayUnits * pDisplayUnits) const
{
   
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          false);
   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      false);
   location.IncludeSpanAndGirder(true);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   CString strInterval;
   strInterval.Format(_T("Interval %d"), LABEL_INTERVAL(intervalIdx));

   // Create different paragraphs for shear stress and axial stress tables
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Cummulative Combined Principal Web Stresses ") << strInterval << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;
   (*pPara) << rptRcImage(strImagePath + _T("PrincipalTensionStress.png")) << rptNewLine;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable( 23 );
   *pPara << pTable << rptNewLine;
   RowIndexType RowIdx = 0;
   ColumnIndexType ColIdx = 0;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(RowIdx, ColIdx, 2);
   (*pTable)(RowIdx, ColIdx++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   pTable->SetRowSpan(RowIdx, ColIdx, 2);
   (*pTable)(RowIdx, ColIdx++) << _T("Web Location");

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcDC);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcDW);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcCR);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcSH);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcRE);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetLoadCombinationName(lcPS);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetProductLoadName(pgsTypes::pftPretension);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning);
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << symbol(SIGMA) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 2);
   (*pTable)(0, ColIdx) << _T("LL+IM Design");
   (*pTable)(1, ColIdx++) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   pTable->SetColumnSpan(0, ColIdx, 3);
   (*pTable)(0, ColIdx) << GetLimitStateString(pgsTypes::ServiceIII);
   (*pTable)(1, ColIdx++) << COLHDR(Sub2(_T("f"), _T("pcx")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << COLHDR(symbol(tau) , rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(1, ColIdx++) << COLHDR(Sub2(_T("f"), _T("max")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

   RowIdx = 2;
   IndexType startColIdx = 1;

   GET_IFACE2(pBroker,IPrincipalWebStress,pPrincipalWebStress);

   // Fill tables
   for (const auto& poi : vPoi)
   {
      const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* pWebStressDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(poi, intervalIdx);

      (*pTable)(RowIdx, 0)  << location.SetValue( POI_SPAN, poi );

      bool bFirstWebSection = true;
      for (const auto& webDetail : *pWebStressDetails)
      {
         ColIdx = startColIdx;

         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << webDetail.strLocation;
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcDC].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcDC].tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcDW].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcDW].tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcCR].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcCR].tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcSH].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcSH].tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcRE].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcRE].tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcPS].f_pcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LoadComboResults[lcPS].tau);

         // Prestress
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.PrePrestress_Fpcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.PrePrestress_Tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.PostPrestress_Fpcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.PostPrestress_Tau);

         // LL
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Fpcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.LL_Tau);


         // Service III  - Principal stress
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.Service3Fpcx);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.Service3Tau);
         IFNFIRSTNEWLINE(bFirstWebSection,pTable,RowIdx, ColIdx);
         (*pTable)(RowIdx, ColIdx++) << stress.SetValue(webDetail.Service3PrincipalStress);

         bFirstWebSection = false;
      }

      RowIdx++;
   }
}

