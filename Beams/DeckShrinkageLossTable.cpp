///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// DeckShrinkageLossTable.cpp : Implementation of CDeckShrinkageLossTable
#include "stdafx.h"
#include "DeckShrinkageLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToDeckShrinkageTable::CElasticGainDueToDeckShrinkageTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( section_modulus, pDisplayUnits->GetSectModulusUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
   DEFINE_UV_PROTOTYPE( time,        pDisplayUnits->GetWholeDaysUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   strain.SetFormat( sysNumericFormatTool::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CElasticGainDueToDeckShrinkageTable* CElasticGainDueToDeckShrinkageTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   ATLASSERT(pSpecEntry->IsDeckShrinkageApplicable()); // Should be vetted by caller

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   // Create and configure the table
   ColumnIndexType numColumns = 12;
   CElasticGainDueToDeckShrinkageTable* table = new CElasticGainDueToDeckShrinkageTable( numColumns, pDisplayUnits );
   pgsReportStyleHolder::ConfigureTable(table);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("[5.9.5.4.3d] Shrinkage of Deck Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSS")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to shrinkage of the deck concrete") << rptNewLine;
   *pParagraph << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( spMode == pgsTypes::spmGross )
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSS_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("Delta_FpSS_Transformed.png")) << rptNewLine;
   }

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-US.png")) << rptNewLine;
      }
   }
   else if ( pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims )
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2006-US.png")) << rptNewLine;
      }
   }
   else
   {
      if ( IS_SI_UNITS(pDisplayUnits) )
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-SI.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-US.png")) << rptNewLine;
      }
   }

   *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;

   if ( IS_SI_UNITS(pDisplayUnits) )
   {
      ATLASSERT( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims );
      *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_SI.png")) << rptNewLine;
   }
   else
   {
      if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims )
      {
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_US.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_US2015.png")) << rptNewLine;
      }
   }

   *pParagraph << _T("Girder stresses due to slab shrinkage") << rptNewLine;
   if ( spMode == pgsTypes::spmGross )
   {
      *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Ftop_Gross.png")) << rptNewLine;
      *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Fbot_Gross.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Ftop_Transformed.png")) << rptNewLine;
      *pParagraph << rptRcImage(strImagePath + _T("SlabShrinkageStress_Fbot_Transformed.png")) << rptNewLine;
   }

   rptRcTable* pParamTable = pgsReportStyleHolder::CreateDefaultTable(9,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   (*pParamTable)(0,0) << COLHDR(_T("V/S") << rptNewLine << _T("deck"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
     (*pParamTable)(0,1) << Sub2(_T("k"),_T("s")) << rptNewLine << _T("deck");
   }
   else
   {
     (*pParamTable)(0,1) << Sub2(_T("k"),_T("vs")) << rptNewLine << _T("deck");
   }


   (*pParamTable)(0,2) << Sub2(_T("k"),_T("hs"));
   (*pParamTable)(0,3) << COLHDR(RPT_FC << rptNewLine << _T("deck"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,4) << Sub2(_T("k"),_T("f"));
   (*pParamTable)(0,5) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("t = ") << Sub2(_T("t"),_T("f")) << _T(" - ") << Sub2(_T("t"),_T("d"));
   (*pParamTable)(0,6) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(0,7) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(0,8) << Sub2(symbol(epsilon),_T("ddf")) << _T("x 1000");

   // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdRefinedLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return table;
   }

   if ( IsZero(ptl->GetVolumeSlab()) || IsZero(ptl->GetSurfaceAreaSlab()) )
   {
      (*pParamTable)(1,0) << table->ecc.SetValue(0.0);
   }
   else
   {
      (*pParamTable)(1,0) << table->ecc.SetValue(ptl->GetVolumeSlab()/ptl->GetSurfaceAreaSlab());
   }

   (*pParamTable)(1,1) << table->scalar.SetValue(ptl->GetCreepDeck().GetKvs());
   (*pParamTable)(1,2) << table->scalar.SetValue(ptl->Getkhs());
   (*pParamTable)(1,3) << table->stress.SetValue(ptl->GetFcSlab() );
   (*pParamTable)(1,4) << table->scalar.SetValue(ptl->GetCreepDeck().GetKf());
   (*pParamTable)(1,5) << table->scalar.SetValue(ptl->GetCreepDeck().GetKtd());
   (*pParamTable)(1,6) << ptl->GetDeckK1Shrinkage();
   (*pParamTable)(1,7) << ptl->GetDeckK2Shrinkage();
   (*pParamTable)(1,8) << table->strain.SetValue(ptl->Get_eddf() * 1000);

   pParamTable = pgsReportStyleHolder::CreateDefaultTable(8,_T(""));
   *pParagraph << pParamTable << rptNewLine;
   (*pParamTable)(0,0) << COLHDR( Sub2(_T("E"),_T("p")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,1) << COLHDR( Sub2(_T("E"),_T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,2) << COLHDR( Sub2(_T("E"),_T("c deck")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(0,3) << Sub2(_T("K"),_T("sh"));
   (*pParamTable)(0,4) << Sub2(_T("K"),_T("1"));
   (*pParamTable)(0,5) << Sub2(_T("K"),_T("2"));
   (*pParamTable)(0,6) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");
   (*pParamTable)(0,7) << Sub2(symbol(psi),_T("d")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");

   (*pParamTable)(1,0) << table->mod_e.SetValue( ptl->GetEp() );
   (*pParamTable)(1,1) << table->mod_e.SetValue( ptl->GetEc() );
   (*pParamTable)(1,2) << table->mod_e.SetValue( ptl->GetEcd() );
   (*pParamTable)(1,3) << pSpecEntry->GetDeckShrinkageElasticGain();
   (*pParamTable)(1,4) << ptl->GetDeckK1Creep();
   (*pParamTable)(1,5) << ptl->GetDeckK2Creep();
   (*pParamTable)(1,6) << table->scalar.SetValue(ptl->GetCreepDeckToFinal().GetCreepCoefficient());
   (*pParamTable)(1,7) << table->scalar.SetValue(ptl->GetCreepDeck().GetCreepCoefficient());

   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(0,1) << COLHDR( Sub2(_T("A"),_T("d")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,2) << COLHDR( Sub2(_T("e"),_T("pc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,3) << COLHDR( Sub2(_T("e"),_T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,4) << COLHDR( Sub2(_T("A"),_T("c")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,5) << COLHDR( Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,6) << COLHDR( Sub2(_T("S"),_T("tc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*table)(0,7) << COLHDR( Sub2(_T("S"),_T("bc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   }
   else
   {
      (*table)(0,1) << COLHDR( Sub2(_T("A"),_T("dn")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,2) << COLHDR( Sub2(_T("e"),_T("pct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,3) << COLHDR( Sub2(_T("e"),_T("dt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,4) << COLHDR( Sub2(_T("A"),_T("ct")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,5) << COLHDR( Sub2(_T("I"),_T("ct")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(0,6) << COLHDR( Sub2(_T("S"),_T("tct")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*table)(0,7) << COLHDR( Sub2(_T("S"),_T("bct")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   }
   (*table)(0,8) << COLHDR( symbol(DELTA) << RPT_STRESS(_T("cdf")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,9) << COLHDR( symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,10) << COLHDR(RPT_FTOP << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(0,11) << COLHDR(RPT_FBOT << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   table->m_Sign =  ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 ) ? 1 : -1;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   table->compositeIntervalIdx = pIntervals->GetCompositeDeckInterval();

   return table;
}

void CElasticGainDueToDeckShrinkageTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
  // Typecast to our known type (eating own doggy food)
   boost::shared_ptr<const lrfdRefinedLosses2005> ptl = boost::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return;
   }

   GET_IFACE2(pBroker,IProductForces,pProductForces);
   Float64 fTop,fBot;
   pProductForces->GetDeckShrinkageStresses(poi,&fTop,&fBot);

   GET_IFACE2(pBroker,ISectionProperties,pProps);
   Float64 St = pProps->GetS(compositeIntervalIdx,poi,pgsTypes::TopGirder);
   Float64 Sb = pProps->GetS(compositeIntervalIdx,poi,pgsTypes::BottomGirder);

   (*this)(row,1) << area.SetValue( ptl->GetAd() );
   (*this)(row,2) << ecc.SetValue( ptl->GetEccpc() );
   (*this)(row,3) << ecc.SetValue( m_Sign*ptl->GetDeckEccentricity() );
   (*this)(row,4) << area.SetValue( pDetails->pLosses->GetAc() );
   (*this)(row,5) << mom_inertia.SetValue( pDetails->pLosses->GetIc() );
   (*this)(row,6) << section_modulus.SetValue(St);
   (*this)(row,7) << section_modulus.SetValue(Sb);
   (*this)(row,8) << stress.SetValue( ptl->GetDeltaFcdf() );
   (*this)(row,9) << stress.SetValue( ptl->ElasticGainDueToDeckShrinkage() );
   (*this)(row,10) << stress.SetValue( fTop );
   (*this)(row,11) << stress.SetValue( fBot );
}
