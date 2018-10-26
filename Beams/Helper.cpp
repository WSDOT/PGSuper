///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "stdafx.h"
#include "helper.h"
#include <Reporting\ReportStyleHolder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void ReportLeverRule(rptParagraph* pPara,bool isMoment, Float64 specialFactor, lrfdILiveLoadDistributionFactor::LeverRuleMethod& lrd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   if (lrd.Nb>1)
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,    pDisplayUnits->GetSpanLengthUnit(),    true );

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Fixed );
      scalar.SetWidth(6);
      scalar.SetPrecision(3);
      scalar.SetTolerance(1.0e-6);

      std::vector<Float64>::iterator iter;
      std::string strImageName(lrd.bWasExterior ? "LeverRuleExterior.gif" : "LeverRuleInterior.gif");
      (*pPara) << rptRcImage(std::string(pgsReportStyleHolder::GetImagePath()) + strImageName) << rptNewLine;
      (*pPara) << "Multiple Presence Factor: m = " << lrd.m << rptNewLine;
      if (isMoment)
      {
         (*pPara) << "mg" << Super(lrd.bWasExterior ? "ME" : "MI") << Sub(lrd.nLanesUsed > 1 ? "2+" : "1") << " = (";
      }
      else
      {
         (*pPara) << "mg" << Super(lrd.bWasExterior ? "VE" : "VI") << Sub(lrd.nLanesUsed > 1 ? "2+" : "1") << " = (";
      }

      if (specialFactor != 1.0)
      {
         (*pPara) << lrd.m << ")("<<specialFactor<<")[";
      }
      else
      {
         (*pPara) << lrd.m << ")[";
      }

      Float64 Sleft = lrd.Sleft;
      Float64 Sright = lrd.Sright;

      if (IsEqual(Sleft,Sright))
      {
         // equal spacing, take absolute values
         for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
         {
            Float64 d = fabs(*iter);

            if ( iter != lrd.AxleLocations.begin() )
               (*pPara) << " + ";

            (*pPara) << "(" << xdim.SetValue(d) << ")" << "(P/2)";
         }
         (*pPara) << "]/[(" << xdim.SetValue(Sleft) << ")(P)]";
      }
      else
      {
         // Unequal spacing, see if we have left and/or right
         bool is_left=false;
         bool is_right=false;
         for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
         {
            Float64 d = (*iter);
            if (d < 0.0)
            {
               is_left=true;
            }
            else
            {
               is_right=true;
            }
         }

         if (is_left && is_right)
            (*pPara) << " [";

         // do left, then right
         if (is_left)
         {
            bool first=true;
            for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
            {
               Float64 d = (*iter);
               if (d < 0.0)
               {
                  if ( !first )
                     (*pPara) << " + ";

                  d*=-1;

                  (*pPara) << "(" << xdim.SetValue(d) << ")" << "(P/2)";
                  first = false;
               }
            }
            (*pPara) << "]/[(" << xdim.SetValue(Sleft) << ")(P)]";
         }

         // right
         if (is_left && is_right)
            (*pPara) << " + [";

         if (is_right)
         {
            bool first=true;
            for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
            {
               Float64 d = (*iter);
               if (d > 0.0)
               {
                  if ( !first )
                     (*pPara) << " + ";


                  (*pPara) << "(" << xdim.SetValue(d) << ")" << "(P/2)";
                  first = false;
               }
            }
            (*pPara) << "]/[(" << xdim.SetValue(Sright) << ")(P)]";
         }

         if (is_left && is_right)
            (*pPara) << " ]";
      }

      (*pPara) << " = " << scalar.SetValue(lrd.mg) << rptNewLine;
   }
   else
   {
      ATLASSERT(lrd.Nb==1);
      (*pPara) << "For a single-beam superstructure, the lever rule decomposes to the Lanes/Beams method" << rptNewLine;
      lrfdILiveLoadDistributionFactor::LanesBeamsMethod lbm;
      lbm.mg = lrd.mg;
      lbm.Nl = lrd.nLanesUsed;
      lbm.Nb = 1;
      lbm.m  = lrd.m;

      ReportLanesBeamsMethod(pPara,lbm,pBroker,pDisplayUnits);
   }
}

void ReportRigidMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::RigidMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,    pDisplayUnits->GetSpanLengthUnit(),    true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   (*pPara) << rptRcImage(std::string(pgsReportStyleHolder::GetImagePath()) + "RigidMethod.gif") << rptNewLine;
   (*pPara) << "Multiple Presence Factor: m = " << rd.m << rptNewLine;
   (*pPara) << "g = " << "(" << rd.Nl << "/" << rd.Nb << ") + (" << xdim.SetValue(rd.Xext) << ")[";
   std::vector<Float64>::iterator iter;
   for ( iter = rd.e.begin(); iter != rd.e.end(); iter++ )
   {
      Float64 e = *iter;
      if ( iter != rd.e.begin() )
         (*pPara) << " + ";

      (*pPara) << "(" << xdim.SetValue(e) << ")";

   }
   (*pPara) << "]/[";
   for ( iter = rd.x.begin(); iter != rd.x.end(); iter++ )
   {
      Float64 x = *iter;
      if ( iter != rd.x.begin() )
         (*pPara) << " + ";

      (*pPara) << "(" << xdim.SetValue(x) << ")" << Super("2");
   }
   (*pPara) << "] = " << scalar.SetValue(rd.mg/rd.m) << rptNewLine;
   (*pPara) << "mg" << Super("ME") << Sub(rd.e.size() > 1 ? "2+" : "1") << " = " << scalar.SetValue(rd.mg) << rptNewLine;
}

void ReportLanesBeamsMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::LanesBeamsMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   (*pPara) << "Multiple Presence Factor: m = " << rd.m << rptNewLine;
   (*pPara) << "g = " << "(" << rd.m <<")("<< rd.Nl << "/" << rd.Nb << ") = " << scalar.SetValue(rd.mg) << rptNewLine;
}