///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\DevLengthDetailsChapterBuilder.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <IFace\Intervals.h>
#include <WBFLGenericBridgeTools.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Some utility functions
rptRcTable* CreateDevelopmentTable(IEAFDisplayUnits* pDisplayUnits)
{
   bool is_2015 = lrfdVersionMgr::SeventhEditionWith2015Interims == lrfdVersionMgr::GetVersion();
   bool is_2016 = lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion();

   ColumnIndexType nColumns = 8;
   if ( is_2015 || is_2016 )
   {
      nColumns += 2; // lamda rl and lamda lw
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable( nColumns, _T(""));

   ColumnIndexType col=0;
   (*pTable)(0,col++) << _T("Bar Size");
   (*pTable)(0,col++) << COLHDR(Sub2(_T("A"),_T("b")),  rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(_T("d"),_T("b")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(RPT_FY,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) << COLHDR(RPT_FC,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (is_2015)
   {
      (*pTable)(0,col++) << symbol(lambda) << Sub(_T("rl"));
      (*pTable)(0,col++) << symbol(lambda) << Sub(_T("lw"));
   }
   else if ( is_2016 )
   {
      (*pTable)(0,col++) << symbol(lambda) << Sub(_T("rl"));
      (*pTable)(0,col++) << symbol(lambda);
   }
   (*pTable)(0,col++) << _T("Modification")<<rptNewLine<<_T("Factor");
   (*pTable)(0, col++) << COLHDR(Sub2(_T("l"), _T("db")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   return pTable;
}

void WriteRowToDevelopmentTable(rptRcTable* pTable, RowIndexType row, CComBSTR barname, const REBARDEVLENGTHDETAILS& devDetails,
                                       rptAreaUnitValue& area, rptLengthUnitValue& length, rptStressUnitValue& stress, rptRcScalar& scalar)
{
   bool is_2015 = lrfdVersionMgr::SeventhEditionWith2015Interims == lrfdVersionMgr::GetVersion();
   bool is_2016 = lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion();

   ColumnIndexType col=0;
   (*pTable)(row,col++) << barname;
   (*pTable)(row,col++) << area.SetValue(devDetails.Ab);
   (*pTable)(row,col++) << length.SetValue(devDetails.db);
   (*pTable)(row,col++) << stress.SetValue(devDetails.fy);
   (*pTable)(row,col++) << stress.SetValue(devDetails.fc);
   if (is_2015 || is_2016 )
   {
      (*pTable)(row,col++) << scalar.SetValue(devDetails.lambdaRl);
      (*pTable)(row,col++) << scalar.SetValue(devDetails.lambdaLw);
   }
   (*pTable)(row,col++) << scalar.SetValue(devDetails.factor);
   (*pTable)(row, col++) << length.SetValue(devDetails.ldb);
   (*pTable)(row, col++) << length.SetValue(devDetails.ld);
}


/****************************************************************************
CLASS
   CDevLengthDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDevLengthDetailsChapterBuilder::CDevLengthDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDevLengthDetailsChapterBuilder::GetName() const
{
   return TEXT("Transfer and Development Length Details");
}

rptChapter* CDevLengthDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);

   rptRcScalar scalar;
   scalar.SetFormat(WBFL::System::NumericFormatTool::Format::Fixed);
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE2(pBroker, ILongRebarGeometry, pLongRebarGeometry);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   GET_IFACE2_NOCHECK(pBroker, IIntervals, pIntervals); // only used if there are rebar in the girder
   GET_IFACE2(pBroker, IMaterials, pMaterials);

   GET_IFACE2(pBroker, IPretensionForce, pPSForce);

   GET_IFACE2(pBroker, IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         if (1 < nSegments)
         {
            rptParagraph* pPara = new rptParagraph;
            (*pChapter) << pPara;
            (*pPara) << bold(ON) << _T("Segment ") << LABEL_SEGMENT(segIdx) << bold(OFF) << rptNewLine;
         }

         CSegmentKey segmentKey(thisGirderKey, segIdx);
         pPSForce->ReportTransferLengthDetails(segmentKey, pChapter);
         pPSForce->ReportDevelopmentLengthDetails(segmentKey, pChapter);


         ////////////////////////////////////////////////////////////
         // Development of longitudinal rebar in segments
         rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pParagraph;

         (*pParagraph) << _T("Development length of longitudinal reinforcement") << rptNewLine;

         pParagraph = new rptParagraph;
         *pChapter << pParagraph;

         (*pParagraph) << _T("AASHTO LRFD BDS ") << LrfdCw8th(_T("5.11.2.1"), _T("5.10.8.2.1"));
         if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
         {
            (*pParagraph) << _T(" and PCI UHPC SDG E.10.3.1");
         }
         (*pParagraph) << rptNewLine;

#pragma Reminder("UPDATE: need to report this for closure joint rebar also")

         CComPtr<IRebarLayout> rebarLayout;
         pLongRebarGeometry->GetRebarLayout(segmentKey, &rebarLayout);
         CollectionIndexType nRebars;
         rebarLayout->get_Count(&nRebars);
         if (nRebars == 0)
         {
            (*pParagraph) << _T("No longitudinal reinforcement defined") << rptNewLine;
         }
         else
         {
            if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
            {
               (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_PCIUHPC.png")) << rptNewLine;
            }
            else
            {
               if (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion())
               {
                  (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_2016.png")) << rptNewLine;
               }
               else if (lrfdVersionMgr::SeventhEditionWith2015Interims == lrfdVersionMgr::GetVersion())
               {
                  (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_2015.png")) << rptNewLine;
               }
               else
               {
                  if (IS_US_UNITS(pDisplayUnits))
                  {
                     (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_US.png")) << rptNewLine;
                  }
                  else
                  {
                     (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_SI.png")) << rptNewLine;
                  }
               }
            }

            rptRcTable* pTable = CreateDevelopmentTable(pDisplayUnits);
            (*pParagraph) << pTable << rptNewLine;
   
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            Float64 fci = pMaterials->GetSegmentFc(segmentKey,releaseIntervalIdx);
            Float64 fc = pMaterials->GetSegmentFc28(segmentKey);
            pgsTypes::ConcreteType concType = pMaterials->GetSegmentConcreteType(segmentKey);
            bool hasFct = pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
            Float64 Fct = hasFct ? pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;

            // Cycle over all rebar in section and output development details for each unique size
            RowIndexType row(1);
            std::set<Float64> diamSet;
            for (CollectionIndexType rebarIdx = 0; rebarIdx < nRebars; rebarIdx++)
            {
               CComPtr<IRebarLayoutItem> layoutItem;
               rebarLayout->get_Item(rebarIdx, &layoutItem);
               CollectionIndexType nPatterns;
               layoutItem->get_Count(&nPatterns);
               for ( CollectionIndexType patternIdx = 0; patternIdx < nPatterns; patternIdx++ )
               {
                  CComPtr<IRebarPattern> rebarPattern;
                  layoutItem->get_Item(patternIdx, &rebarPattern);
                  CComPtr<IRebar> rebar;
                  rebarPattern->get_Rebar(&rebar);
                  Float64 diam;
                  rebar->get_NominalDiameter(&diam);
                  if (diamSet.end()==diamSet.find(diam))
                  {
                     // We have a unique bar
                     diamSet.insert(diam);
                     
                     CComBSTR barname;
                     rebar->get_Name(&barname);

                     // development length in fresh concrete
                     REBARDEVLENGTHDETAILS devDetails = pLongRebarGeometry->GetSegmentRebarDevelopmentLengthDetails(segmentKey, rebar, concType, fci, hasFct, Fct);
                     WriteRowToDevelopmentTable(pTable, row, barname, devDetails, area, length, stress, scalar);
                     row++;

                     // development length in mature concrete
                     devDetails = pLongRebarGeometry->GetSegmentRebarDevelopmentLengthDetails(segmentKey, rebar, concType, fc, hasFct, Fct);
                     WriteRowToDevelopmentTable(pTable, row, barname, devDetails, area, length, stress, scalar);
                     row++;
                  } // end if
               } // next patternIdx
            } // next rebarIdx
         }// end if
      } // next segment
   } // next girder


   ////////////////////////////////////////////////////////////
   // Development of deck longitudinal rebar
   // Only report if explicit bars are defined (not if just As)
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   (*pParagraph) << _T("Development length of longitudinal reinforcement in deck slab") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   (*pParagraph) << _T("AASHTO LRFD BDS ") << LrfdCw8th(_T("5.11.2.1"), _T("5.10.8.2.1"));
   if (pDeck->Concrete.Type == pgsTypes::PCI_UHPC)
   {
      (*pParagraph) << _T(" and PCI UHPC SDG E.10.3.1");
   }
   (*pParagraph) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   (*pParagraph) << _T("Primary reinforcement is assumed to be fully developed along their entire length.") << rptNewLine;

   const std::vector<CDeckRebarData::NegMomentRebarData> negMomRebarColl = pDeck->DeckRebarData.NegMomentRebar;
   if ( IsNonstructuralDeck(pDeck->GetDeckType()) || negMomRebarColl.empty() )
   {
      (*pParagraph) << _T("There is no deck, the deck is a nonstructural overlay, or there is no partial-length longitudinal rebar in the deck.") << rptNewLine;
   }
   else
   {
      (*pParagraph) << _T("Supplemental reinforcement specified using ")<<Sub2(_T("A"),_T("s")) << _T(" is assumed to be developed along their entire length.") << rptNewLine;

      rptRcTable* pTable = CreateDevelopmentTable(pDisplayUnits);
      (*pParagraph) << pTable << rptNewLine;

      // Need deck concrete properties for all
      Float64 fc = pMaterials->GetDeckFc28();
      pgsTypes::ConcreteType concType = pMaterials->GetDeckConcreteType();
      bool hasFct = pMaterials->DoesDeckConcreteHaveAggSplittingStrength();
      Float64 Fct = hasFct ? pMaterials->GetDeckConcreteAggSplittingStrength() : 0.0;

      CComPtr<IRebar> rebar; // need one of these to feed our dev length function
      rebar.CoCreateInstance(CLSID_Rebar);

      std::set<Float64> diamSet; // only report unique bars

      RowIndexType row(1);
      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter(negMomRebarColl.begin());
      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator end(negMomRebarColl.end());
      for ( ; iter != end; iter++ )
      {
         const CDeckRebarData::NegMomentRebarData& rdata = *iter;

         matRebar::Size size = rdata.RebarSize;
         if (size != matRebar::bsNone)
         {
            const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(rdata.RebarType, rdata.RebarGrade, size);

            Float64 db = pRebar->GetNominalDimension();

            if (diamSet.end()==diamSet.find(db))
            {
               diamSet.insert(db);

               // remove metric from bar name
               std::_tstring barst(pRebar->GetName());
               std::size_t sit = barst.find(_T(" "));
               if (sit != std::_tstring::npos)
               {
                  barst.erase(sit, barst.size()-1);
               }

               CComBSTR barname(barst.c_str());
               Float64 Es = pRebar->GetE();
               Float64 density = 0.0;
               Float64 fpu = pRebar->GetUltimateStrength();
               Float64 fpy = pRebar->GetYieldStrength();
               Float64 Ab  = pRebar->GetNominalArea();

               rebar->Init(barname, Es, density, fpu, fpy, db, Ab, INVALID_INDEX);
               // NOTE: using INVALID_INDEX above because it doesn't matter what Generic Bridge Model
               // stage this rebar is introduced for purposes of computing development length. The
               // next line of code computes the development length on the lfy

               REBARDEVLENGTHDETAILS devDetails = pLongRebarGeometry->GetDeckRebarDevelopmentLengthDetails(rebar, concType, fc, hasFct, Fct);

               WriteRowToDevelopmentTable(pTable, row, barname, devDetails, area, length, stress, scalar);
               row++;
            }
         }
      }
   }


   return pChapter;
}

CChapterBuilder* CDevLengthDetailsChapterBuilder::Clone() const
{
   return new CDevLengthDetailsChapterBuilder;
}

