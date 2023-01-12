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

// PostTensionInteractionTable.cpp : Implementation of CPostTensionInteractionTable
#include "stdafx.h"
#include "PostTensionInteractionTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPostTensionInteractionTable::CPostTensionInteractionTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
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

CPostTensionInteractionTable* CPostTensionInteractionTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   std::_tstring strImagePath(rptStyleManager::GetImagePath());
   
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   pgsTypes::TTSUsage tempStrandUsage = pStrands->GetTemporaryStrandUsage();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing() ? true : false;

   GET_IFACE2(pBroker, IGirder, pGirder);
   bool bIsPrismatic = pGirder->IsPrismatic(releaseIntervalIdx, segmentKey);

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   // Create and configure table
   ColumnIndexType numColumns = 7; // location, location, x, fptmax, P, fcgpt, dfpt

   if (!bIsPrismatic)
   {
      if (bIsAsymmetric)
      {
         numColumns += 6; // eptx, pty, Ag, Ixx, Iyy, Ixy
      }
      else
      {
         numColumns += 3; // ept, Ag, Ig
      }
   }

   CPostTensionInteractionTable* table = new CPostTensionInteractionTable(numColumns, pDisplayUnits);
   rptStyleManager::ConfigureTable(table);

   table->m_bIsPrismatic = bIsPrismatic;
   table->m_bIsAsymmetric = bIsAsymmetric;

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName(_T("Effect of temporary strand jacking on previously stressed temporary strands"));
   *pParagraph << pParagraph->GetName() << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (spMode == pgsTypes::spmGross)
   {
      if (bIsAsymmetric)
      {
         if (tempStrandUsage == pgsTypes::ttsPTBeforeShipping)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Before_Shipping_Gross_Asymmetric.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Gross_Asymmetric.png")) << rptNewLine;
         }
      }
      else
      {
         if (tempStrandUsage == pgsTypes::ttsPTBeforeShipping)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Before_Shipping_Gross.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Gross.png")) << rptNewLine;
         }
      }
   }
   else
   {
      if (bIsAsymmetric)
      {
         if (tempStrandUsage == pgsTypes::ttsPTBeforeShipping)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Before_Shipping_Transformed_Asymmetric.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Transformed_Asymmetric.png")) << rptNewLine;
         }
      }
      else
      {
         if (tempStrandUsage == pgsTypes::ttsPTBeforeShipping)
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Before_Shipping_Transformed.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(strImagePath + _T("Delta_Fpt_Transformed.png")) << rptNewLine;
         }
      }
   }

   table->mod_e.ShowUnitTag(true);
   table->area.ShowUnitTag(true);
   table->mom_inertia.ShowUnitTag(true);
   table->ecc.ShowUnitTag(true);
   
   GET_IFACE2(pBroker, IMaterials, pMaterials);
   Float64 Eci = pMaterials->GetSegmentEc(segmentKey, tsInstallIntervalIdx);
   Float64 Ep = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Temporary)->GetE();

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 1);
   const pgsPointOfInterest& poi(vPoi.front());

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   Float64 Apt = pStrandGeom->GetStrandArea(poi, tsInstallIntervalIdx, pgsTypes::Temporary);

   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue(Ep) << rptNewLine;

   if (tempStrandUsage == pgsTypes::ttsPTBeforeShipping)
   {
      *pParagraph << Sub2(_T("E"), _T("c")) << _T(" = ") << table->mod_e.SetValue(Eci) << rptNewLine;
   }
   else
   {
      *pParagraph << Sub2(_T("E"), _T("ci")) << _T(" = ") << table->mod_e.SetValue(Eci) << rptNewLine;
   }

   *pParagraph << Sub2(_T("A"),_T("pt")) << _T(" = ") << table->area.SetValue(Apt) << rptNewLine;

   if (bIsPrismatic)
   {
      Float64 Ag = pSectProp->GetAg(releaseIntervalIdx, poi);
      Float64 Ixx = pSectProp->GetIxx(releaseIntervalIdx, poi);
      Float64 Iyy = pSectProp->GetIyy(releaseIntervalIdx, poi);
      Float64 Ixy = pSectProp->GetIxy(releaseIntervalIdx, poi);

      gpPoint2d ecc = pStrandGeom->GetEccentricity(tsInstallIntervalIdx, pgsPointOfInterest(segmentKey, 0), pgsTypes::Temporary);

      if (spMode == pgsTypes::spmGross)
      {
         if (bIsAsymmetric)
         {
            *pParagraph << Sub2(_T("e"), _T("ptx")) << _T(" = ") << table->ecc.SetValue(ecc.X()) << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("pty")) << _T(" = ") << table->ecc.SetValue(ecc.Y()) << rptNewLine;
            *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xx")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("yy")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xy")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("e"), _T("pt")) << _T(" = ") << table->ecc.SetValue(ecc.Y()) << rptNewLine;
            *pParagraph << Sub2(_T("A"), _T("g")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("g")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
         }
      }
      else
      {
         if (bIsAsymmetric)
         {
            *pParagraph << Sub2(_T("e"), _T("ptxt")) << _T(" = ") << table->ecc.SetValue(ecc.X()) << rptNewLine;
            *pParagraph << Sub2(_T("e"), _T("ptyt")) << _T(" = ") << table->ecc.SetValue(ecc.Y()) << rptNewLine;
            *pParagraph << Sub2(_T("A"), _T("gt")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xxt")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("yyt")) << _T(" = ") << table->mom_inertia.SetValue(Iyy) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("xyt")) << _T(" = ") << table->mom_inertia.SetValue(Ixy) << rptNewLine;
         }
         else
         {
            *pParagraph << Sub2(_T("e"), _T("ptt")) << _T(" = ") << table->ecc.SetValue(ecc.Y()) << rptNewLine;
            *pParagraph << Sub2(_T("A"), _T("gt")) << _T(" = ") << table->area.SetValue(Ag) << rptNewLine;
            *pParagraph << Sub2(_T("I"), _T("gt")) << _T(" = ") << table->mom_inertia.SetValue(Ixx) << rptNewLine;
         }
      }
   }

   table->mod_e.ShowUnitTag(false);
   table->area.ShowUnitTag(false);
   table->mom_inertia.ShowUnitTag(false);
   table->ecc.ShowUnitTag(false);


   ColumnIndexType col = 0;
   *pParagraph << table << rptNewLine;
   (*table)(0, col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("End of Girder"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0, col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0, col++) << COLHDR(_T("x"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR(RPT_STRESS(_T("pt max")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0, col++) << COLHDR(Sub2(_T("P"),_T("pt")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

   if (!bIsPrismatic)
   {
      if (spMode == pgsTypes::spmGross)
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ptx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xx")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xy")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
      }
      else
      {
         if (bIsAsymmetric)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ptxt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ptyt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xxt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("yyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("xyt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
         else
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ptt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("gt")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("I"), _T("gt")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         }
      }
   }

   (*table)(0, col++) << COLHDR(RPT_STRESS(_T("cgpt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   return table;
}

void CPostTensionInteractionTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   Float64 Ag, Ybg, Ixx, Iyy, Ixy;
   pDetails->pLosses->GetNoncompositeProperties(&Ag, &Ybg, &Ixx, &Iyy, &Ixy);

   ColumnIndexType col = 2;
   RowIndexType rowOffset = GetNumberOfHeaderRows() - 1;
   (*this)(row+rowOffset, col++) << offset.SetValue( pDetails->pLosses->GetLocation() );
   (*this)(row+rowOffset, col++) << stress.SetValue( pDetails->pLosses->GetFptMax() );
   (*this)(row+rowOffset, col++) << force.SetValue( pDetails->pLosses->GetPptMax() );

   if (!m_bIsPrismatic)
   {
      if(m_bIsAsymmetric)
      { 
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().X());
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Iyy);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixy);
      }
      else
      {
         (*this)(row+rowOffset, col++) << ecc.SetValue(pDetails->pLosses->GetEccTemporary().Y());
         (*this)(row+rowOffset, col++) << area.SetValue(Ag);
         (*this)(row+rowOffset, col++) << mom_inertia.SetValue(Ixx);
      }
   }

   (*this)(row+rowOffset, col++) << stress.SetValue( pDetails->pLosses->GetFcgpt() );
   (*this)(row+rowOffset, col++) << stress.SetValue( pDetails->pLosses->GetDeltaFpt() );
}
