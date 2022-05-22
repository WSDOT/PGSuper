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

// DeckShrinkageLossTable.cpp : Implementation of CDeckShrinkageLossTable
#include "stdafx.h"
#include "DeckShrinkageLossTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <PsgLib\SpecLibraryEntry.h>

#include <PgsExt\GirderMaterial.h>

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

   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   strain.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   strain.SetWidth(6);
   strain.SetPrecision(3);
}

CElasticGainDueToDeckShrinkageTable* CElasticGainDueToDeckShrinkageTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   if (IsNonstructuralDeck(pBridge->GetDeckType()))
   {
      // no deck, no deck shrinkage
      return nullptr;
   }

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   ATLASSERT(pSpecEntry->IsDeckShrinkageApplicable()); // Should be vetted by caller

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);
   bool bUHPC = pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::PCI_UHPC ? true : false;
   bool bPCTT = (bUHPC ? pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.bPCTT : false);

   // Create and configure the table
   ColumnIndexType numColumns = 13;
   CElasticGainDueToDeckShrinkageTable* table = new CElasticGainDueToDeckShrinkageTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Shrinkage of deck concrete"));
   *pParagraph << _T("[") << LrfdCw8th(_T("5.9.5.4.3d"), _T("5.9.3.4.3d")) << _T("] Shrinkage of Deck Concrete : ") << symbol(DELTA) << RPT_STRESS(_T("pSS")) << rptNewLine;

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

   if (bUHPC)
   {
      if (bPCTT)
         *pParagraph << rptRcImage(strImagePath + _T("DeckCreepShrinkage_UHPC_PCTT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(strImagePath + _T("DeckCreepShrinkage_UHPC.png")) << rptNewLine;

      *pParagraph << rptRcImage(strImagePath + _T("UHPC_Factors.png")) << rptNewLine;
   }
   else
   {
      if (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2006Interims)
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeckCreepShrinkage.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("DeckCreepShrinkage_2007.png")) << rptNewLine;
      }

      if (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims)
      {
         if (IS_SI_UNITS(pDisplayUnits))
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-SI.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn-US.png")) << rptNewLine;
         }
      }
      else if (pSpecEntry->GetSpecificationType() == lrfdVersionMgr::ThirdEditionWith2006Interims)
      {
         if (IS_SI_UNITS(pDisplayUnits))
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
         if (IS_SI_UNITS(pDisplayUnits))
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-SI.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("KvsEqn2007-US.png")) << rptNewLine;
         }
      }

      *pParagraph << rptRcImage(strImagePath + _T("HumidityFactor.png")) << rptNewLine;

      if (IS_SI_UNITS(pDisplayUnits))
      {
         ATLASSERT(pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims);
         *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_SI.png")) << rptNewLine;
      }
      else
      {
         if (pSpecEntry->GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2015Interims)
         {
            *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_US.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("ConcreteFactors_Deck_US2015.png")) << rptNewLine;
         }
      }
   }

   *pParagraph << _T("Girder stresses due to deck shrinkage") << rptNewLine;
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

   rptRcTable* pParamTable = rptStyleManager::CreateDefaultTable(19,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   RowIndexType row = 0;
   ColumnIndexType col = 0;

   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("E"), _T("c deck")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(row,col++) << COLHDR(_T("V/S") << rptNewLine << _T("deck"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
     (*pParamTable)(row, col++) << Sub2(_T("k"),_T("s")) << rptNewLine << _T("deck");
   }
   else
   {
     (*pParamTable)(row, col++) << Sub2(_T("k"),_T("vs")) << rptNewLine << _T("deck");
   }


   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("hs"));
   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("hc"));
   (*pParamTable)(row, col++) << COLHDR(RPT_FCI << _T(" = 0.8") << RPT_FC << rptNewLine << _T("deck"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(row, col++) << COLHDR(RPT_FC << rptNewLine << _T("deck"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(row, col++) << Sub2(_T("k"),_T("f"));
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("i")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("d")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("f")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit() );
   (*pParamTable)(row, col++) << Sub2(_T("k"),_T("td")) << rptNewLine << _T("t = ") << Sub2(_T("t"),_T("f")) << _T(" - ") << Sub2(_T("t"),_T("d"));
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("sh"));
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("1")) << rptNewLine << _T("Shrinkage");
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("2")) << rptNewLine << _T("Shrinkage");
   (*pParamTable)(row, col++) << Sub2(symbol(epsilon),_T("ddf")) << _T("x 1000");
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("1")) << rptNewLine << _T("Creep");
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("2")) << rptNewLine << _T("Creep");
   (*pParamTable)(row, col++) << Sub2(symbol(psi), _T("d")) << _T("(") << Sub2(_T("t"), _T("f")) << _T(",") << Sub2(_T("t"), _T("d")) << _T(")");

   // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
   if (!ptl)
   {
      ATLASSERT(false); // made a bad cast? Bail...
      return table;
   }

   row = 1;
   col = 0;

   (*pParamTable)(row, col++) << table->mod_e.SetValue(ptl->GetEcd());
   if ( IsZero(ptl->GetDeckCreep()->GetVolume()) || IsZero(ptl->GetDeckCreep()->GetSurfaceArea()) )
   {
      (*pParamTable)(row,col++) << table->ecc.SetValue(0.0);
   }
   else
   {
      (*pParamTable)(row, col++) << table->ecc.SetValue(ptl->GetDeckCreep()->GetVolume()/ptl->GetDeckCreep()->GetSurfaceArea());
   }

   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetDeckCreep()->GetKvs());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->Getkhs());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetDeckCreep()->GetKhc());
   (*pParamTable)(row, col++) << table->stress.SetValue(0.8*ptl->GetFcSlab()); // See NCHRP 496 (page 27 and 30)
   (*pParamTable)(row, col++) << table->stress.SetValue(ptl->GetFcSlab());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetDeckCreep()->GetKf());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetDeckInitialAge());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetFinalAge());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetDeckCreep()->GetKtd(ptl->GetDeckMaturityAtFinal()));
   (*pParamTable)(row, col++) << (spMode == pgsTypes::spmGross ? pSpecEntry->GetDeckShrinkageElasticGain() : 1.0);
   (*pParamTable)(row, col++) << ptl->GetDeckK1Shrinkage();
   (*pParamTable)(row, col++) << ptl->GetDeckK2Shrinkage();
   (*pParamTable)(row, col++) << table->strain.SetValue(ptl->Get_eddf() * 1000);
   (*pParamTable)(row, col++) << ptl->GetDeckCreep()->GetK1();
   (*pParamTable)(row, col++) << ptl->GetDeckCreep()->GetK2();
   (*pParamTable)(row, col++) << table->creep.SetValue(ptl->GetDeckCreep()->GetCreepCoefficient(ptl->GetDeckMaturityAtFinal(),ptl->GetDeckInitialAge()));

   pParamTable = rptStyleManager::CreateDefaultTable(15,_T(""));
   *pParagraph << pParamTable << rptNewLine;

   row = 0;
   col = 0;
   (*pParamTable)(row, col++) << COLHDR( Sub2(_T("E"),_T("c")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pParamTable)(row, col++) << COLHDR(_T("V/S"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if (lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType())
   {
      (*pParamTable)(row, col++) << Sub2(_T("k"), _T("s"));
   }
   else
   {
      (*pParamTable)(row, col++) << Sub2(_T("k"), _T("vs"));
   }
   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("hs"));
   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("hc"));
   (*pParamTable)(row, col++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(row, col++) << COLHDR(RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("f"));
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("i")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("d")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(row, col++) << COLHDR(Sub2(_T("t"), _T("f")), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit());
   (*pParamTable)(row, col++) << Sub2(_T("k"), _T("td")) << rptNewLine << _T("t = ") << Sub2(_T("t"), _T("f")) << _T(" - ") << Sub2(_T("t"), _T("d"));
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("1")) << rptNewLine << _T("Creep");
   (*pParamTable)(row, col++) << Sub2(_T("K"), _T("2")) << rptNewLine << _T("Creep");
   (*pParamTable)(row, col++) << Sub2(symbol(psi),_T("b")) << _T("(") << Sub2(_T("t"),_T("f")) << _T(",") << Sub2(_T("t"),_T("d")) << _T(")");

   row = 1;
   col = 0;
   (*pParamTable)(row, col++) << table->mod_e.SetValue( ptl->GetEc() );
   if (IsZero(ptl->GetDeckCreep()->GetVolume()) || IsZero(ptl->GetDeckCreep()->GetSurfaceArea()))
   {
      (*pParamTable)(row, col++) << table->ecc.SetValue(0.0);
   }
   else
   {
      (*pParamTable)(row, col++) << table->ecc.SetValue(ptl->GetDeckCreep()->GetVolume() / ptl->GetDeckCreep()->GetSurfaceArea());
   }

   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKvs());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->Getkhs());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKhc());
   (*pParamTable)(row, col++) << table->stress.SetValue(ptl->GetFci());
   (*pParamTable)(row, col++) << table->stress.SetValue(ptl->GetFc());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKf());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetAgeAtDeckPlacement());
   (*pParamTable)(row, col++) << table->time.SetValue(ptl->GetFinalAge());
   (*pParamTable)(row, col++) << table->scalar.SetValue(ptl->GetGirderCreep()->GetKtd(ptl->GetMaturityDeckPlacementToFinal()));
   (*pParamTable)(row, col++) << ptl->GetGirderCreep()->GetK1();
   (*pParamTable)(row, col++) << ptl->GetGirderCreep()->GetK2();
   (*pParamTable)(row, col++) << table->creep.SetValue(ptl->GetGirderCreep()->GetCreepCoefficient(ptl->GetMaturityDeckPlacementToFinal(),ptl->GetAgeAtDeckPlacement()));

   *pParagraph << table << rptNewLine;
   row = 0;
   col = 0;
   (*table)(row,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   if ( spMode == pgsTypes::spmGross )
   {
      (*table)(row, col++) << COLHDR( Sub2(_T("A"),_T("d")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("e"),_T("pc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("e"),_T("d")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("A"),_T("c")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("S"),_T("tc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("S"),_T("bc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   }
   else
   {
      (*table)(row, col++) << COLHDR( Sub2(_T("A"),_T("dn")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("e"),_T("pct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("e"),_T("dt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("A"),_T("ct")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("I"),_T("ct")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("S"),_T("tct")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*table)(row, col++) << COLHDR( Sub2(_T("S"),_T("bct")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   }
   (*table)(row, col++) << COLHDR( symbol(DELTA) << RPT_STRESS(_T("cdf")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(row, col++) << Sub2(_T("K"), _T("df"));
   (*table)(row, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pSS")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*table)(row, col++) << COLHDR(RPT_FTOP << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(row, col++) << COLHDR(RPT_FBOT << rptNewLine << _T("Girder"),rptStressUnitTag,pDisplayUnits->GetStressUnit());

   table->m_Sign =  ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::FourthEdition2007 ) ? 1 : -1;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   table->compositeIntervalIdx = pIntervals->GetFirstCompositeDeckInterval();

   return table;
}

void CElasticGainDueToDeckShrinkageTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
  // Typecast to our known type (eating own doggy food)
   std::shared_ptr<const lrfdRefinedLosses2005> ptl = std::dynamic_pointer_cast<const lrfdRefinedLosses2005>(pDetails->pLosses);
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

   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   Float64 Ac, Ybc, Ic;
   pDetails->pLosses->GetCompositeProperties2(&Ac, &Ybc, &Ic);
   (*this)(row+rowOffset, col++) << area.SetValue( ptl->GetAd() );
   (*this)(row+rowOffset, col++) << ecc.SetValue( ptl->GetEccpc() );
   (*this)(row+rowOffset, col++) << ecc.SetValue( m_Sign*ptl->GetDeckEccentricity() );
   (*this)(row+rowOffset, col++) << area.SetValue( Ac );
   (*this)(row+rowOffset, col++) << mom_inertia.SetValue( Ic );
   (*this)(row+rowOffset, col++) << section_modulus.SetValue(St);
   (*this)(row+rowOffset, col++) << section_modulus.SetValue(Sb);
   (*this)(row+rowOffset, col++) << stress.SetValue( ptl->GetDeltaFcdf() );
   (*this)(row+rowOffset, col++) << scalar.SetValue(ptl->GetKdf());
   (*this)(row+rowOffset, col++) << stress.SetValue( ptl->ElasticGainDueToDeckShrinkage() );
   (*this)(row+rowOffset, col++) << stress.SetValue( fTop );
   (*this)(row+rowOffset, col++) << stress.SetValue( fBot );
}
