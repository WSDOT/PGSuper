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

#include "StdAfx.h"
#include <Reporting\StrandEccTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace/PointOfInterest.h>


rptRcTable* CStrandEccTable::Build(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   bool bAsymmetric = (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing());

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);

   // Gross Properties, report gross only
   // Transformed Properties
   //    Non-timestep, report Transformed and Gross
   //    Timestep, report transformed only
   bool bReportOnlyGrossOrTransformed = (spMode == pgsTypes::spmGross ? true : false);
   pgsTypes::SectionPropertyType spType2 = pgsTypes::sptGross;

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP)
   {
      // need eccentricity based on complete transformed section for time-step analysis
      spType = pgsTypes::sptTransformed;
      bReportOnlyGrossOrTransformed = true;
   }

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   auto releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary) ? true : false);

   // Setup table
   std::_tostringstream os;
   os << _T("Strand Eccentricity: Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx);

   ColumnIndexType nCols = 0;
   nCols += (bAsymmetric ? 4 : 2); // straight and harped
   nCols += (bTempStrands ? (bAsymmetric ? 6 : 3) : (bAsymmetric ? 2 : 1)); // temporary (ecc,ecc all, ecc perm only) or (ecc, ecc perm only)
   if (!bReportOnlyGrossOrTransformed)
      nCols *= 2;
   
   nCols += 2; // location at start of table (put it here so we can just double nCols if there is a double table for gross and transformed)

   nCols += (releaseIntervalIdx == intervalIdx ? 2 : 0); // strand slopes

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, os.str().c_str());

   p_table->SetNumberOfHeaderRows(bAsymmetric ? 3 : 2);

   ColumnIndexType col = 0;

   // build first heading row
   p_table->SetRowSpan(0, col, bAsymmetric ? 3 : 2);
   (*p_table)(0, col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   p_table->SetRowSpan(0, col, bAsymmetric ? 3 : 2);
   (*p_table)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   // straight/harped/temporary
   p_table->SetColumnSpan(0, col, (bTempStrands ? (bAsymmetric ? 10 : 5) : (bAsymmetric ? 4 : 3)));
   if(bReportOnlyGrossOrTransformed)
      (*p_table)(0, col) << _T("Eccentricity");
   else
      (*p_table)(0, col) << _T("Transformed Section");

   if (spMode == pgsTypes::spmGross)
   {
      if (bAsymmetric)
      {
         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << _T("Straight");
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey));
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(1, col++) << COLHDR(_T("Straight") << rptNewLine << Sub2(_T("e"), _T("s")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)) << rptNewLine << Sub2(_T("e"), _T("h")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      if (bTempStrands)
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Temporary");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All") << rptNewLine << _T("w/ Temp)");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Permanent") << rptNewLine << _T("w/o Temp)");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("Temporary") << rptNewLine << Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp), ") << Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp), ") << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All Strands");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("All Strands") << rptNewLine << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }
   else
   {
      if (bAsymmetric)
      {
         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << _T("Straight");
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("stx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey));
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("htx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(1, col++) << COLHDR(_T("Straight") << rptNewLine << Sub2(_T("e"), _T("st")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)) << rptNewLine << Sub2(_T("e"), _T("ht")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      if (bTempStrands)
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Temporary");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ttx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("tty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All") << rptNewLine << _T("(w/ Temp)");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pstx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Permanent") << rptNewLine << _T("(w/o Temp)");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ptx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("Temporary") << rptNewLine << Sub2(_T("e"), _T("tt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp), ") << Sub2(_T("e"), _T("pst")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp), ") << Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All Strands");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ptx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("pty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("All Strands") << rptNewLine << Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }

   if (!bReportOnlyGrossOrTransformed)
   {
      p_table->SetColumnSpan(0, col, (bTempStrands ? (bAsymmetric ? 10 : 5) : (bAsymmetric ? 4 : 3)));
      (*p_table)(0, col) << _T("Gross Section");

      if (bAsymmetric)
      {
         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << _T("Straight");
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("sy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

         p_table->SetColumnSpan(1, col, 2);
         (*p_table)(1, col) << LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey));
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("hy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }
      else
      {
         (*p_table)(1, col++) << COLHDR(_T("Straight") << rptNewLine << Sub2(_T("e"), _T("s")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*p_table)(1, col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)) << rptNewLine << Sub2(_T("e"), _T("h")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      if (bTempStrands)
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Temporary");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("tx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("ty")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All") << rptNewLine << _T("(w/ Temp)");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psx")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("psy")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Permanent") << rptNewLine << _T("(w/o Temp");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("Temporary") << rptNewLine << Sub2(_T("e"), _T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp), ") << Sub2(_T("e"), _T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(1, col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp), ") << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
      else
      {
         if (bAsymmetric)
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("All Strands");
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("px")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*p_table)(2, col++) << COLHDR(Sub2(_T("e"), _T("py")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
         else
         {
            (*p_table)(1, col++) << COLHDR(_T("All Strands") << rptNewLine << Sub2(_T("e"), _T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         }
      }
   }

   if (releaseIntervalIdx == intervalIdx)
   {
      // strand slope
      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("Strand Slope");

      if (bAsymmetric) p_table->SetRowSpan(1, col, 2);
      (*p_table)(1, col++) << _T("Average") << rptNewLine << _T("(1:n)");

      if (bAsymmetric) p_table->SetRowSpan(1, col, 2);
      (*p_table)(1, col++) << _T("Maximum") << rptNewLine << _T("(1:n)");
   }

   INIT_UV_PROTOTYPE(rptPointOfInterest, rptReleasePoi, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptPointOfInterest, rptErectedPoi, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue, ecc, pDisplayUnits->GetComponentDimUnit(), false);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi);
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   pPoi->GetPointsOfInterest(segmentKey, POI_SPECIAL | POI_START_FACE | POI_END_FACE, &vPoi);
   pPoi->SortPoiList(&vPoi);
   pPoi->RemovePointsOfInterest(vPoi, POI_CLOSURE);
   pPoi->RemovePointsOfInterest(vPoi, POI_BOUNDARY_PIER);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      (*p_table)(row, col++) << rptReleasePoi.SetValue(POI_RELEASED_SEGMENT, poi);
      (*p_table)(row, col++) << rptErectedPoi.SetValue(POI_ERECTED_SEGMENT, poi);

      if (0 < Ns)
      {
         auto e = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Straight);
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << ecc.SetValue(e.X());
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
         else
         {
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
      }
      else
      {
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << _T("-");
            (*p_table)(row, col++) << _T("-");
         }
         else
         {
            (*p_table)(row, col++) << _T("-");
         }
      }

      if (0 < Nh)
      {
         auto e = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Harped);
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << ecc.SetValue(e.X());
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
         else
         {
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
      }
      else
      {
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << _T("-");
            (*p_table)(row, col++) << _T("-");
         }
         else
         {
            (*p_table)(row, col++) << _T("-");
         }
      }

      if (bTempStrands)
      {
         if (0 < Nt)
         {
            auto e = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, pgsTypes::Temporary);
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << ecc.SetValue(e.X());
               (*p_table)(row, col++) << ecc.SetValue(e.Y());
            }
            else
            {
               if (bAsymmetric)
               {
                  (*p_table)(row, col++) << ecc.SetValue(e.X());
                  (*p_table)(row, col++) << ecc.SetValue(e.Y());
               }
               else
               {
                  (*p_table)(row, col++) << ecc.SetValue(e.Y());
               }
            }
         }
         else
         {
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << _T("-");
               (*p_table)(row, col++) << _T("-");
            }
            else
            {
               (*p_table)(row, col++) << _T("-");
            }
         }

         auto e = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, true /*include temporary strands*/);
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << ecc.SetValue(e.X());
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
         else
         {
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
      }

      auto e = pStrandGeom->GetEccentricity(spType, intervalIdx, poi, false/*exclude temporary strands*/);
      if (bAsymmetric)
      {
         (*p_table)(row, col++) << ecc.SetValue(e.X());
         (*p_table)(row, col++) << ecc.SetValue(e.Y());
      }
      else
      {
         (*p_table)(row, col++) << ecc.SetValue(e.Y());
      }

      if (!bReportOnlyGrossOrTransformed)
      {
         if (0 < Ns)
         {
            auto e = pStrandGeom->GetEccentricity(spType2, intervalIdx, poi, pgsTypes::Straight);
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << ecc.SetValue(e.X());
               (*p_table)(row, col++) << ecc.SetValue(e.Y());
            }
            else
            {
               (*p_table)(row, col++) << ecc.SetValue(e.Y());
            }
         }
         else
         {
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << _T("-");
               (*p_table)(row, col++) << _T("-");
            }
            else
            {
               (*p_table)(row, col++) << _T("-");
            }
         }

         if (0 < Nh)
         {
            auto e = pStrandGeom->GetEccentricity(spType2, intervalIdx, poi, pgsTypes::Harped);
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << ecc.SetValue(e.X());
               (*p_table)(row, col++) << ecc.SetValue(e.Y());
            }
            else
            {
               (*p_table)(row, col++) << ecc.SetValue(e.Y());
            }
         }
         else
         {
            if (bAsymmetric)
            {
               (*p_table)(row, col++) << _T("-");
               (*p_table)(row, col++) << _T("-");
            }
            else
            {
               (*p_table)(row, col++) << _T("-");
            }
         }

         if (bTempStrands)
         {
             if (0 < Nt)
             {
                auto e = pStrandGeom->GetEccentricity(spType2, intervalIdx, poi, pgsTypes::Temporary);
                if (bAsymmetric)
                {
                   (*p_table)(row, col++) << ecc.SetValue(e.X());
                   (*p_table)(row, col++) << ecc.SetValue(e.Y());
                }
                else
                {
                   (*p_table)(row, col++) << ecc.SetValue(e.Y());
                }
             }
             else
             {
                if (bAsymmetric)
                {
                   (*p_table)(row, col++) << _T("-");
                   (*p_table)(row, col++) << _T("-");
                }
                else
                {
                   (*p_table)(row, col++) << _T("-");
                }
             }

             auto e = pStrandGeom->GetEccentricity(spType2, intervalIdx, poi, true /*include temporary strands*/);
             if (bAsymmetric)
             {
                (*p_table)(row, col++) << ecc.SetValue(e.X());
                (*p_table)(row, col++) << ecc.SetValue(e.Y());
             }
             else
             {
                (*p_table)(row, col++) << ecc.SetValue(e.Y());
             }
         }

         auto e = pStrandGeom->GetEccentricity(spType2, intervalIdx, poi, false/*exclude temporary strands*/);
         if (bAsymmetric)
         {
            (*p_table)(row, col++) << ecc.SetValue(e.X());
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
         else
         {
            (*p_table)(row, col++) << ecc.SetValue(e.Y());
         }
      }

      if (releaseIntervalIdx == intervalIdx)
      {
         if (0 < Nh)
         {
            Float64 avg_slope = pStrandGeom->GetAvgStrandSlope(poi);
            avg_slope = fabs(avg_slope);

            if (IsZero(1. / avg_slope))
            {
               (*p_table)(row, col++) << symbol(infinity);
            }
            else
            {
               (*p_table)(row, col++) << avg_slope;
            }

            Float64 max_slope = pStrandGeom->GetMaxStrandSlope(poi);
            max_slope = fabs(max_slope);
            if (IsZero(1. / max_slope))
            {
               (*p_table)(row, col++) << symbol(infinity);
            }
            else
            {
               (*p_table)(row, col++) << max_slope;
            }
         }
         else
         {
            (*p_table)(row, col++) << _T("-");
            (*p_table)(row, col++) << _T("-");
         }
      }

      row++;
   }

   return p_table;
}
