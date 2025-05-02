///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// TemporaryStrandRemovalTable.cpp : Implementation of CTemporaryStrandRemovalTable
#include "stdafx.h"
#include "Beams.h"
#include "TemporaryStrandRemovalTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace/PointOfInterest.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\StrandData.h>


CTemporaryStrandRemovalTable::CTemporaryStrandRemovalTable(ColumnIndexType NumColumns, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CTemporaryStrandRemovalTable* CTemporaryStrandRemovalTable::PrepareTable(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 Ec  = pMaterials->GetSegmentEc(segmentKey,tsRemovalIntervalIdx);
   Float64 Ep  = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE();

   GET_IFACE2(pBroker,IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& poiMiddle(vPoi.front());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 Apt = pStrandGeom->GetStrandArea(poiMiddle,tsInstallIntervalIdx,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::_tstring strImagePath(rptStyleManager::GetImagePath());

   ///////////////////////////////////////////////////////////////////////////////////////
   // Change in stress due to removal of temporary strands
   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Effect of temporary strand removal on permanent strands"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

   // Create and configure the table
   ColumnIndexType numColumns = 6; // location, fpj, dfpH, Ptr, fptr, dfptr
   if (bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         numColumns += 4; // etx, ety, epx, epy
      }
      else
      {
         numColumns += 2; // et, ep
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         numColumns += 8; // Ag, Ixx, Iyy, Ixy, etx, ety, epx, epy
      }
      else
      {
         numColumns += 4; // Ag, Ig, et, ep
      }
   }

   CTemporaryStrandRemovalTable* table = new CTemporaryStrandRemovalTable( numColumns, pDisplayUnits );
   rptStyleManager::ConfigureTable(table);

   table->m_bIsPrismatic = bIsPrismatic;
   table->m_bIsAsymmetric = bIsAsymmetric;


   *pParagraph << Sub2(_T("P"),_T("tr")) << _T(" = ") << Sub2(_T("A"),_T("t")) << _T("(") << Sub2(_T("f"),_T("pj")) << _T(" - ");

   if ( pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPretensioned )
   {
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pF")) << _T(" - ");
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pA")) << _T(" - ");
      *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pt")) << _T(" - ");
   }

   *pParagraph << symbol(DELTA) << Sub2(_T("f"),_T("pH")) << _T(")") << rptNewLine;

   if (spMode == pgsTypes::spmGross)
   {
      if (bIsAsymmetric)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_Fptr_Gross_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_Fptr_Gross.png")) << rptNewLine;
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_Fptr_Transformed_Asymmetric.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(strImagePath + _T("Delta_Fptr_Transformed.png")) << rptNewLine;
      }
   }

   table->mod_e.ShowUnitTag(true);
   table->ecc.ShowUnitTag(true);
   table->area.ShowUnitTag(true);
   table->mom_inertia.ShowUnitTag(true);

   if (bIsPrismatic)
   {
      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());
      Float64 Ag = pSectProp->GetAg(releaseIntervalIdx, poi);
      Float64 Ixx = pSectProp->GetIxx(releaseIntervalIdx, poi);
      Float64 Iyy = pSectProp->GetIyy(releaseIntervalIdx, poi);
      Float64 Ixy = pSectProp->GetIxy(releaseIntervalIdx, poi);
      if (spMode == pgsTypes::spmGross)
      {
         if (bIsAsymmetric)
         {
            *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xx")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("yy")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xy")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
         }
      }
      else
      {
         if (bIsAsymmetric)
         {
            *pParagraph << Sub2(_T("A"), _T("gn")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xxn")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("yyn")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xyn")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("A"), _T("gn")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("gn")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
         }
      }
   }

   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue(Ep) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue(Ec) << rptNewLine;
   *pParagraph << Sub2(_T("A"),_T("t")) << _T(" = ") << table->area.SetValue(Apt) << rptNewLine;


   table->mod_e.ShowUnitTag(false);
   table->ecc.ShowUnitTag(false);
   table->area.ShowUnitTag(false);
   table->mom_inertia.ShowUnitTag(false);

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pj")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pH")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("P"),_T("tr")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   if (bIsPrismatic)
   {
      if (spMode == pgsTypes::spmGross)
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("txt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }
   else
   {
      if (spMode == pgsTypes::spmGross)
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("txt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }

   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ptr")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CTemporaryStrandRemovalTable::AddRow(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level)
{
//   (*this)(row,0) << spanloc.SetValue(poi,end_size);
   ColumnIndexType col = 1;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;

   (*this)(row+rowOffset,col++) << stress.SetValue(pDetails->pLosses->GetFpjTemporary());
   (*this)(row+rowOffset,col++) << stress.SetValue(pDetails->pLosses->TemporaryStrand_AtShipping());
   (*this)(row+rowOffset,col++) << force.SetValue(pDetails->pLosses->GetPtr());

   if (m_bIsPrismatic)
   {
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
   }
   else
   {
      Float64 Ag, Ybg, Ixx, Iyy, Ixy;
      pDetails->pLosses->GetNetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);
      if (m_bIsAsymmetric)
      {
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Iyy);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixy);
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
      else
      {
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccPermanentFinal().Y());
      }
   }

   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->GetFptr() );
   (*this)(row+rowOffset,col++) << stress.SetValue( pDetails->pLosses->GetDeltaFptr() );
}
