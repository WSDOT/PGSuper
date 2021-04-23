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

// ElasticGainDueToDeckPlacementTable.cpp : Implementation of CElasticGainDueToDeckPlacementTable
#include "stdafx.h"
#include "ElasticGainDueToDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToDeckPlacementTable::CElasticGainDueToDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CElasticGainDueToDeckPlacementTable* CElasticGainDueToDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bHasUserLoads = pUDL->DoUserLoadsExist(segmentKey);
   bool bHasDeckPanel = (deckType == pgsTypes::sdtCompositeSIP ? true : false);
   bool bHasLongitudinalJoints = pProductLoads->HasLongitudinalJointLoad();
   bool bHasDeckLoads = pGirder->HasStructuralLongitudinalJoints() || deckType == pgsTypes::sdtNone ? false : true; // if longitudinal joints are structural the deck dead loads go on the composite section

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

   ColumnIndexType numColumns = 5; // Location, Diaphragm, Madl, delta_f'cd, dfpED

   if (bHasDeckLoads)
   {
      numColumns += 2;
   }

   if (bHasUserLoads)
   {
      numColumns += 2;
   }

   if (bHasDeckPanel)
   {
      numColumns++;
   }

   if (bHasLongitudinalJoints)
   {
      numColumns++;
   }

   if (bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         numColumns += 2; // epsx, epsy
      }
      else
      {
         numColumns++; // eps
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         numColumns += 5; // Ixx, Ixy, Ixy, epsx, epsy
      }
      else
      {
         numColumns += 2; // Ig, eps
      }
   }


   CElasticGainDueToDeckPlacementTable* table = new CElasticGainDueToDeckPlacementTable( numColumns, pDisplayUnits);
   rptStyleManager::ConfigureTable(table);

   table->m_bIsPrismatic = bIsPrismatic;
   table->m_bIsAsymmetric = bIsAsymmetric;
   table->m_bHasDeckLoads = bHasDeckLoads;
   table->m_bHasDeckPanel = bHasDeckPanel;
   table->m_bHasUserLoads = bHasUserLoads;
   table->m_bHasLongitudinalJoints = bHasLongitudinalJoints;
   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(segmentKey,castDeckIntervalIdx);
   Float64 Ep = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(); // Ok to use straight since we just want E

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Elastic Gain Due to Additional Dead Load"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to loads applied to the non-composite girder") << rptNewLine;
   *pParagraph << rptNewLine;
   if (bHasDeckLoads)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl1.png"));
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Madl2.png"));
   }

   if (bHasDeckPanel)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Mpanel.png"));
   }

   if (bHasDeckLoads)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Mhaunch.png"));
      *pParagraph << rptRcImage(strImagePath + _T("Mdiaphragm.png"));
   }

   if (bHasLongitudinalJoints)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Mlj.png"));
   }

   if (bHasUserLoads)
   {
      *pParagraph << rptRcImage(strImagePath + _T("Muser.png"));
   }

   *pParagraph << rptNewLine;


   *pParagraph << Sub2(_T("M"),_T("Diaphragm")) << _T(" includes shear key dead load if applicable") << rptNewLine;

   if (bIsAsymmetric)
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd_Gross_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd_Transformed_Asymmetric.png")) << rptNewLine;
      }
   }
   else
   {
      if (spMode == pgsTypes::spmGross)
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd_Gross.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeltaFcd_Transformed.png")) << rptNewLine;
      }
   }

   *pParagraph << rptRcImage(strImagePath + _T("ElasticGain.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   table->mom_inertia.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;

   if (bIsPrismatic)
   {
      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());
      Float64 Ixx = pSectProp->GetIxx(releaseIntervalIdx, poi);
      if (bIsAsymmetric)
      {
         Float64 Iyy = pSectProp->GetIyy(releaseIntervalIdx, poi);
         Float64 Ixy = pSectProp->GetIxy(releaseIntervalIdx, poi);

         *pParagraph << Sub2(_T("I"), _T("xx")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
         *pParagraph << Sub2(_T("I"), _T("yy")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
         *pParagraph << Sub2(_T("I"), _T("xy")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
      }
      else
      {
         *pParagraph << Sub2(_T("I"), _T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
      }
   }
   table->mod_e.ShowUnitTag(false);
   table->mom_inertia.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   table->m_BAT = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                   analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MaxSimpleContinuousEnvelope);


   *pParagraph << rptNewLine;
   if (bHasDeckLoads)
   {
      if (bHasDeckPanel)
      {
         *pParagraph << _T("Slab+Panel: ") << Sub2(_T("K"), _T("s")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabElasticGain()) << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Slab: ") << Sub2(_T("K"), _T("s")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabElasticGain()) << rptNewLine;
      }

      *pParagraph << _T("Haunch: ") << Sub2(_T("K"), _T("h")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetSlabPadElasticGain()) << rptNewLine;
   }
   *pParagraph << _T("Diaphragms: ") << Sub2(_T("K"),_T("d")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetDiaphragmElasticGain()) << rptNewLine;

   if ( bHasUserLoads )
   {
      *pParagraph << _T("User DC: ") << Sub2(_T("K"),_T("dc")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadBeforeDeckDCElasticGain()) << rptNewLine;
      *pParagraph << _T("User DW: ") << Sub2(_T("K"),_T("dw")) << _T(" = ") << table->scalar.SetValue(pSpecEntry->GetUserLoadBeforeDeckDWElasticGain()) << rptNewLine;
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   if (bHasDeckLoads)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSlab)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   
   if ( bHasDeckPanel )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }

   if (bHasDeckLoads)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }

   (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if (bHasLongitudinalJoints)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   }
   
   if ( bHasUserLoads )
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftUserDC)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("M"), pProductLoads->GetProductLoadName(pgsTypes::pftUserDW)), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }
   
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("adl")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   
   if (bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
      }
      else
      {
         if (spMode == pgsTypes::spmGross)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
      }
   }
   
   (*table)(0,col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pED")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IndexType deckCastingRegionIdx = INVALID_INDEX;
   if (m_bHasDeckLoads)
   {
      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   if (m_bHasDeckLoads)
   {
      (*this)(row+rowOffset, col++) << moment.SetValue(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, m_BAT, rtIncremental));
   
      if ( m_bHasDeckPanel )
      {
         (*this)(row+rowOffset,col++) << moment.SetValue( pProdForces->GetMoment( castDeckIntervalIdx, pgsTypes::pftSlabPanel,      poi, m_BAT, rtIncremental ) );
      }
   
      (*this)(row+rowOffset, col++) << moment.SetValue(pProdForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, m_BAT, rtIncremental));
   }

   Float64 M = pProdForces->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, m_BAT, rtIncremental);
   if (castShearKeyIntervalIdx != INVALID_INDEX)
   {
      M += pProdForces->GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, m_BAT, rtIncremental);
   }
   (*this)(row+rowOffset,col++) << moment.SetValue( M );

   if (m_bHasLongitudinalJoints)
   {
      (*this)(row+rowOffset, col++) << moment.SetValue(pProdForces->GetMoment(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, m_BAT, rtIncremental));
   }
   
   if ( m_bHasUserLoads )
   {
      (*this)(row+rowOffset,col++) << moment.SetValue( pProdForces->GetMoment(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, m_BAT, rtIncremental ) );
      (*this)(row+rowOffset,col++) << moment.SetValue( pProdForces->GetMoment(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, m_BAT, rtIncremental ) );
   }

   (*this)(row+rowOffset, col++) << moment.SetValue(pDetails->pLosses->GetAddlGdrMoment());

   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);

   if (m_bIsPrismatic)
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
   }
   else
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Iyy);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixy);
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
      }
   }

   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFcd1() );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->ElasticGainDueToDeckPlacement() );
}
