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

// ElasticGainDueToSIDLTable.cpp : Implementation of CElasticGainDueToSIDLTable
#include "stdafx.h"
#include "ElasticGainDueToSIDLTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToSIDLTable::CElasticGainDueToSIDLTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticGainDueToSIDLTable* CElasticGainDueToSIDLTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool bHasUserLoads = pUDL->DoUserLoadsExist(segmentKey);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay = pBridge->HasOverlay();

   GET_IFACE2(pBroker,IProductLoads,pLoad);
   bool bHasSidewalk = pLoad->HasSidewalkLoad(segmentKey);

   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bHasDeckLoads = pGirder->HasStructuralLongitudinalJoints() && pBridge->GetDeckType() != pgsTypes::sdtNone ? true : false; // if longitudinal joints are structural and there is a deck, the deck dead loads go on the composite section
   bool bIs2StageComposite = pGirder->HasStructuralLongitudinalJoints() && ::IsStructuralDeck(pBridge->GetDeckType()) ? true : false; 

   ColumnIndexType numColumns = 9;

   if (bHasDeckLoads)
   {
      numColumns += 2;
   }

   if (bIs2StageComposite)
   {
      numColumns += 3;
   }

   if ( bHasUserLoads )
   {
      numColumns += 2;
   }

   if ( bHasSidewalk )
   {
      numColumns++;
   }

   if ( bHasOverlay )
   {
      numColumns++;
   }

   CElasticGainDueToSIDLTable* table = new CElasticGainDueToSIDLTable( numColumns, pDisplayUnits);
   rptStyleManager::ConfigureTable(table);

   table->m_bHasDeckLoads = bHasDeckLoads;
   table->m_bIs2StageComposite = bIs2StageComposite;
   table->m_bHasUserLoads = bHasUserLoads;
   table->m_bHasSidewalk  = bHasSidewalk;
   table->m_bHasOverlay   = bHasOverlay;

   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(segmentKey,railingSystemIntervalIdx);
   Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE(); // Ok to use straight since we just want E

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Elastic Gain Due to Superimposed Dead Load"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to loads applied to the composite girder") << rptNewLine;
   *pParagraph << rptNewLine;

   if (bIs2StageComposite)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Msidl1.png")) << rptNewLine;
      *pParagraph << rptRcImage(strImagePath + _T("Msidl2.png"));
   }
   else
   {
      if (bHasDeckLoads)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Msidl_with_Deck.png"));
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Msidl.png"));
      }
   }
   if (bHasSidewalk)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Msw.png"));
   }
   if (bHasOverlay)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Mo.png"));
   }
   if (bHasUserLoads)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Muser.png"));
   }
   *pParagraph << rptNewLine;

   if (bIs2StageComposite)
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_with_Deck_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_with_Deck_Transformed.png")) << rptNewLine;
      }
   }
   else
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd2_Transformed.png")) << rptNewLine;
      }
   }

   *pParagraph << rptRcImage(strImagePath + _T("ElasticGain2.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   table->m_BAT = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                   analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MaxSimpleContinuousEnvelope);


   *pParagraph << rptNewLine;
   if (bHasDeckLoads)
   {
      *pParagraph << _T("Slab: ") << Sub2(_T("K"), _T("s")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabElasticGain()) << rptNewLine;
      *pParagraph << _T("Haunch: ") << Sub2(_T("K"), _T("h")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabPadElasticGain()) << rptNewLine;
   }

   *pParagraph << _T("Railing System: ") << Sub2(_T("K"),_T("r")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetRailingSystemElasticGain())      << rptNewLine;

   if ( bHasOverlay )
   {
      *pParagraph << _T("Overlay: " )    << Sub2(_T("K"),_T("o")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetOverlayElasticGain())   << rptNewLine;
   }

   if ( bHasUserLoads )
   {
      *pParagraph << _T("User DC: ") << Sub2(_T("K"),_T("dc")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadAfterDeckDCElasticGain()) << rptNewLine;
      *pParagraph << _T("User DW: ") << Sub2(_T("K"),_T("dw")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadAfterDeckDWElasticGain()) << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if (bHasDeckLoads)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSlab)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   if (bIs2StageComposite)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl1")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bHasSidewalk )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   if ( bHasOverlay )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftOverlay)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   if ( bHasUserLoads )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftUserDC)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftUserDW)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }

   if (bIs2StageComposite)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl2")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   else
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("sidl")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (bIs2StageComposite)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c1")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc1")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c2")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc2")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      if (bIs2StageComposite)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("ct1")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bct1")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("ct2")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bct2")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("ct")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*table)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bgt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   (*table)(0,col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f''"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSIDL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToSIDLTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IndexType deckCastingRegionIdx = INVALID_INDEX;
   if (m_bHasDeckLoads)
   {
      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   if (m_bHasDeckLoads)
   {
      (*this)(row+rowOffset, col++) << moment.SetValue(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, m_BAT, rtIncremental));
      (*this)(row+rowOffset, col++) << moment.SetValue(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, m_BAT, rtIncremental));
   }

   if (m_bIs2StageComposite)
   {
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment1());
   }

   if ( m_bHasSidewalk )
   {
      (*this)(row+rowOffset,col++) << moment.SetValue(pProdForces->GetMoment( railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, m_BAT, rtIncremental ));
   }

   (*this)(row+rowOffset,col++) << moment.SetValue(pProdForces->GetMoment( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, m_BAT, rtIncremental ));

   if ( m_bHasOverlay )
   {
      (*this)(row+rowOffset,col++) << moment.SetValue(pProdForces->GetMoment( overlayIntervalIdx, pgsTypes::pftOverlay, poi, m_BAT, rtIncremental ));
   }

   if ( m_bHasUserLoads )
   {
      (*this)(row+rowOffset,col++) << moment.SetValue( pProdForces->GetMoment(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC,      poi, m_BAT, rtIncremental ) );
      (*this)(row+rowOffset,col++) << moment.SetValue( pProdForces->GetMoment(compositeUserLoadIntervalIdx, pgsTypes::pftUserDW,      poi, m_BAT, rtIncremental ) );
   }

   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
   Float64 Ac2, Ybc2, Ic2;
   pDetails->pLosses->GetCompositeProperties2(&Ac2, &Ybc2, &Ic2);

   if (m_bIs2StageComposite)
   {
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment2());
   }
   else
   {
      (*this)(row + rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetSidlMoment1() + pDetails->pLosses->GetSidlMoment2());
   }
   
   (*this)(row+rowOffset,col++) << ecc.SetValue( pDetails->pLosses->GetEccPermanentFinal().Y() );

   if (m_bIs2StageComposite)
   {
      Float64 Ac1, Ybc1, Ic1;
      pDetails->pLosses->GetCompositeProperties1(&Ac1, &Ybc1, &Ic1);

      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic1);
      (*this)(row + rowOffset, col++) << cg.SetValue(Ybc1);
      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic2);
      (*this)(row + rowOffset, col++) << cg.SetValue(Ybc2);
   }
   else
   {
      (*this)(row + rowOffset, col++) << mom_inertia.SetValue(Ic2);
      (*this)(row + rowOffset, col++) << cg.SetValue(Ybc2);
   }

   (*this)(row+rowOffset,col++) << cg.SetValue( Ybg );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFcd2(true/*apply elastic gains reduction*/) );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->ElasticGainDueToSIDL(true/*apply elastic gains reduction*/) );
}
