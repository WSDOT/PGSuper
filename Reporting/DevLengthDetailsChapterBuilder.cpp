///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

   ColumnIndexType nColumns = 7;
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
   (*pTable)(0,col++) << COLHDR(Sub2(_T("l"),_T("d")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

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
   (*pTable)(row,col++) << length.SetValue(devDetails.ldb);
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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area,   pDisplayUnits->GetAreaUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,  pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE2(pBroker,IPretensionForce,pPSForce);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);

   GET_IFACE2_NOCHECK(pBroker,IMaterials,pMaterials); // only used if there are rebar in the girder

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = girderKey.girderIndex == ALL_GIRDERS ? 0 : Min(girderKey.girderIndex,nGirders-1);
      GirderIndexType lastGirderIdx  = girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx;
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CGirderKey thisGirderKey(grpIdx,gdrIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);
         PoiList vOtherPoi;
         pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), POI_STIRRUP_ZONE | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2 | POI_HARPINGPOINT | POI_CONCLOAD | POI_PSXFER | POI_PSDEV | POI_DEBOND | POI_DECKBARCUTOFF | POI_BARCUTOFF | POI_BARDEVELOP | POI_H | POI_15H | POI_FACEOFSUPPORT, &vOtherPoi, POIFIND_OR);
         pPoi->MergePoiLists(vPoi, vOtherPoi, &vPoi);
         ATLASSERT( vPoi.size() != 0 );
         const pgsPointOfInterest& dummy_poi( vPoi.front() );

         STRANDDEVLENGTHDETAILS bonded_details   = pPSForce->GetDevLengthDetails(dummy_poi,false); // not debonded
         STRANDDEVLENGTHDETAILS debonded_details = pPSForce->GetDevLengthDetails(dummy_poi,true);  // debonded

         // Transfer Length
         rptParagraph* pParagraph_h = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pParagraph_h;
         rptParagraph* pParagraph = new rptParagraph;
         *pChapter << pParagraph;

         if ( bonded_details.ltDetails.bMinuteValue )
         {
            (*pParagraph_h) << _T("Zero Transfer Length Selected in Project Criteria") << rptNewLine;
            (*pParagraph) << _T("Actual length used ")<< Sub2(_T("l"),_T("t")) << _T(" = ") << length.SetValue(bonded_details.ltDetails.lt) << rptNewLine;
         }
         else
         {
            if ( bonded_details.ltDetails.bEpoxy )
            {
               (*pParagraph_h) << _T("Transfer Length") << rptNewLine;
               (*pParagraph) << _T("See \"Guidelines for the use of Epoxy-Coated Strand\", Section 5.5.2, PCI Journal, July-August 1993") << rptNewLine;
            }
            else
            {
               (*pParagraph_h) << _T("Transfer Length [") << LrfdCw8th(_T("5.11.4.1"),_T("5.9.4.3.1")) << _T("]") << rptNewLine;
            }
            (*pParagraph) << Sub2(_T("l"),_T("t")) << _T(" = ") << bonded_details.ltDetails.ndb << Sub2(_T("d"),_T("b")) << rptNewLine;
            (*pParagraph) << Sub2(_T("d"),_T("b")) << _T(" = ") << length.SetValue(bonded_details.ltDetails.db) << rptNewLine;
            (*pParagraph) << Sub2(_T("l"),_T("t")) << _T(" = ") << length.SetValue(bonded_details.ltDetails.lt) << rptNewLine;
         }

         // Development Length
         pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pParagraph;
         (*pParagraph) << _T("Development Length [") << LrfdCw8th(_T("5.11.4.2"),_T("5.9.4.3.2")) << _T("]") << rptNewLine;

         pParagraph = new rptParagraph;
         *pChapter << pParagraph;

         if ( IS_US_UNITS(pDisplayUnits) )
         {
            (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("DevLength_US.png")) << rptNewLine;
         }
         else
         {
            (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("DevLength_SI.png")) << rptNewLine;
         }

         (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("DevLengthReduction.png")) << rptNewLine;
         (*pParagraph) << RPT_STRESS(_T("px")) << _T("/") << RPT_STRESS(_T("ps")) << _T(" = Development Length Reduction Factor (See LRFD Eqn. (") << LrfdCw8th(_T("5.11.4.2-2 and -3"),_T("5.9.4.3.2-2 and -3")) << _T(")") << rptNewLine;

         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(13);
         (*pParagraph) << pTable << rptNewLine;


         if ( girderKey.groupIndex == ALL_GROUPS )
         {
            pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         }

         ColumnIndexType col = 0;
         pTable->SetNumberOfHeaderRows(2);
         pTable->SetRowSpan(0,col,2);
         (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

         pTable->SetColumnSpan(0, col, 6);
         (*pTable)(0, col) << _T("Bonded Strands ")   << symbol(kappa) << _T(" = ") << bonded_details.k;
         (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(1, col++) << COLHDR(Sub2(_T("d"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(1, col++) << COLHDR(Sub2(_T("l"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(1, col++) << RPT_STRESS(_T("px")) << _T("/") << RPT_STRESS(_T("ps"));

         pTable->SetColumnSpan(0, col, 6);
         (*pTable)(0, col) << _T("Debonded Strands ") << symbol(kappa) << _T(" = ") << debonded_details.k;
         (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pTable)(1, col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pTable)(1, col++) << COLHDR(Sub2(_T("d"),_T("b")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*pTable)(1, col++) << COLHDR(Sub2(_T("l"),_T("d")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*pTable)(1, col++) << COLHDR(Sub2(_T("l"),_T("px")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*pTable)(1, col++) << RPT_STRESS(_T("px")) << _T("/") << RPT_STRESS(_T("ps"));

         stress.ShowUnitTag(false);
         length.ShowUnitTag(false);

         RowIndexType row = pTable->GetNumberOfHeaderRows();
         for (const pgsPointOfInterest& poi : vPoi)
         {
            Float64 segment_length = pBridge->GetSegmentLength(poi.GetSegmentKey());
            Float64 half_segment_length = segment_length/2;

            STRANDDEVLENGTHDETAILS bonded_details   = pPSForce->GetDevLengthDetails(poi,false); // not debonded
            STRANDDEVLENGTHDETAILS debonded_details = pPSForce->GetDevLengthDetails(poi,true); // debonded
#pragma Reminder("UPDATE: bond factor should not be computed in the report") // use the code in the engineer agent

            Float64 lpx;
            if ( poi.GetDistFromStart() < half_segment_length )
            {
               lpx = poi.GetDistFromStart();
            }
            else
            {
               lpx = segment_length - poi.GetDistFromStart();
            }

            Float64 bond_factor;
            if ( lpx < bonded_details.ltDetails.lt )
            {
               bond_factor = bonded_details.fpe*lpx/(bonded_details.fps*bonded_details.ltDetails.lt);
            }
            else if ( lpx < bonded_details.ld )
            {
               bond_factor = (bonded_details.fpe + ((lpx - bonded_details.ltDetails.lt)/(bonded_details.ld - bonded_details.ltDetails.lt))*(bonded_details.fps - bonded_details.fpe))/bonded_details.fps;
            }
            else
            {
               bond_factor = 1.0;
            }

            bond_factor = ForceIntoRange(0.0,bond_factor,1.0);

            (*pTable)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi );
            (*pTable)(row,1) << stress.SetValue(bonded_details.fps);
            (*pTable)(row,2) << stress.SetValue(bonded_details.fpe);
            (*pTable)(row,3) << length.SetValue(bonded_details.db);
            (*pTable)(row,4) << length.SetValue(bonded_details.ld);
            (*pTable)(row,5) << length.SetValue(lpx);
            (*pTable)(row,6) << scalar.SetValue(bond_factor);

            if ( lpx < debonded_details.ltDetails.lt )
            {
               bond_factor = debonded_details.fpe*lpx/(debonded_details.fps*debonded_details.ltDetails.lt);
            }
            else if ( lpx < debonded_details.ld )
            {
               bond_factor = (debonded_details.fpe + ((lpx - debonded_details.ltDetails.lt)/(debonded_details.ld - debonded_details.ltDetails.lt))*(debonded_details.fps - debonded_details.fpe))/debonded_details.fps;
            }
            else
            {
               bond_factor = 1.0;
            }

            bond_factor = ForceIntoRange(0.0,bond_factor,1.0);

            (*pTable)(row,7) << stress.SetValue(debonded_details.fps);
            (*pTable)(row,8) << stress.SetValue(debonded_details.fpe);
            (*pTable)(row,9) << length.SetValue(debonded_details.db);
            (*pTable)(row,10) << length.SetValue(debonded_details.ld);
            (*pTable)(row,11) << length.SetValue(lpx);
            (*pTable)(row,12) << scalar.SetValue(bond_factor);

            row++;
         } // next poi

         ////////////////////////////////////////////////////////////
         // Development of longitudinal rebar in segments
         SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(thisGirderKey,segIdx);
            pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pParagraph;

#pragma Reminder("UPDATE: update heading so that it works for both PGSuper and PGSplice")
            (*pParagraph) << _T("Development Length of Longitudinal Rebar [") << LrfdCw8th(_T("5.11.2.1"),_T("5.10.8.2.1")) << _T("]") << rptNewLine;
            (*pParagraph) << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

            pParagraph = new rptParagraph;
            *pChapter << pParagraph;

#pragma Reminder("UPDATE: need to report this for closure joint rebar also")

            CComPtr<IRebarLayout> rebarLayout;
            pLongRebarGeometry->GetRebarLayout(thisSegmentKey, &rebarLayout);
            CollectionIndexType nRebars;
            rebarLayout->get_Count(&nRebars);
            if (nRebars == 0)
            {
               (*pParagraph) << _T("No longitudinal rebar exists in girder") << rptNewLine;
            }
            else
            {
	            if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
	            {
	               (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_2016.png")) << rptNewLine;
	            }
	            else if ( lrfdVersionMgr::SeventhEditionWith2015Interims == lrfdVersionMgr::GetVersion() )
	            {
	               (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_2015.png")) << rptNewLine;
	            }
	            else
	            {
	               if ( IS_US_UNITS(pDisplayUnits) )
	               {
	                  (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_US.png")) << rptNewLine;
	               }
	               else
	               {
	                  (*pParagraph) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LongitudinalRebarDevelopment_SI.png")) << rptNewLine;
	               }
	            }

               rptRcTable* pTable = CreateDevelopmentTable(pDisplayUnits);
               (*pParagraph) << pTable << rptNewLine;
   
               Float64 fc = pMaterials->GetSegmentFc28(thisSegmentKey);
               pgsTypes::ConcreteType concType = pMaterials->GetSegmentConcreteType(thisSegmentKey);
               bool hasFct = pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(thisSegmentKey);
               Float64 Fct = hasFct ? pMaterials->GetSegmentConcreteAggSplittingStrength(thisSegmentKey) : 0.0;

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
                        REBARDEVLENGTHDETAILS devDetails = pLongRebarGeometry->GetSegmentRebarDevelopmentLengthDetails(thisSegmentKey, rebar, concType, fc, hasFct, Fct);

                        CComBSTR barname;
                        rebar->get_Name(&barname);

                        WriteRowToDevelopmentTable(pTable, row, barname, devDetails, area, length, stress, scalar);

                        row++;
                     } // end if
                  } // next patternIdx
               } // next rebarIdx
            }// end if
         } // next segment
      } // next girder
   } // next group

   ////////////////////////////////////////////////////////////
   // Development of deck longitudinal rebar
   // Only report if explicit bars are defined (not if just As)
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   (*pParagraph) << _T("Development Length of Longitudinal Rebar [") << LrfdCw8th(_T("5.11.2.1"),_T("5.10.8.2.1")) << _T("] in Deck Slab") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   (*pParagraph) << _T("Full-length deck bars are considered to be developed along entire length of girder.") << rptNewLine;

   const std::vector<CDeckRebarData::NegMomentRebarData> negMomRebarColl = pDeck->DeckRebarData.NegMomentRebar;
   if ( IsNonstructuralDeck(pDeck->GetDeckType()) || negMomRebarColl.empty() )
   {
      (*pParagraph) << _T("There is no deck, the deck is a nonstructural overlay, or there is no partial-length longitudinal rebar in the deck.") << rptNewLine;
   }
   else
   {
      (*pParagraph) << _T("Partial-length longitudinal rebar specified using ")<<Sub2(_T("A"),_T("s")) << _T(" are considered to be developed along entire specified bar length.") << rptNewLine;

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

