///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Beams\Helper.h>
#include <PgsExt\ReportStyleHolder.h>

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
      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

      std::vector<Float64>::iterator iter;
      std::_tstring strImageName(lrd.bWasExterior ? _T("LeverRuleExterior.gif") : _T("LeverRuleInterior.gif"));
      (*pPara) << rptRcImage(std::_tstring(pgsReportStyleHolder::GetImagePath()) + strImageName) << rptNewLine;
      (*pPara) << _T("Multiple Presence Factor: m = ") << lrd.m << rptNewLine;
      if (isMoment)
      {
         (*pPara) << _T("mg") << Super(lrd.bWasExterior ? _T("ME") : _T("MI")) << Sub(lrd.nLanesUsed > 1 ? _T("2+") : _T("1")) << _T(" = (");
      }
      else
      {
         (*pPara) << _T("mg") << Super(lrd.bWasExterior ? _T("VE") : _T("VI")) << Sub(lrd.nLanesUsed > 1 ? _T("2+") : _T("1")) << _T(" = (");
      }

      if (specialFactor != 1.0)
      {
         (*pPara) << lrd.m << _T(")(")<<specialFactor<<_T(")[");
      }
      else
      {
         (*pPara) << lrd.m << _T(")[");
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
               (*pPara) << _T(" + ");

            (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
         }
         (*pPara) << _T("]/[(") << xdim.SetValue(Sleft) << _T(")(P)]");
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
            (*pPara) << _T(" [");

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
                     (*pPara) << _T(" + ");

                  d*=-1;

                  (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
                  first = false;
               }
            }
            (*pPara) << _T("]/[(") << xdim.SetValue(Sleft) << _T(")(P)]");
         }

         // right
         if (is_left && is_right)
            (*pPara) << _T(" + [");

         if (is_right)
         {
            bool first=true;
            for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
            {
               Float64 d = (*iter);
               if (d > 0.0)
               {
                  if ( !first )
                     (*pPara) << _T(" + ");


                  (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
                  first = false;
               }
            }
            (*pPara) << _T("]/[(") << xdim.SetValue(Sright) << _T(")(P)]");
         }

         if (is_left && is_right)
            (*pPara) << _T(" ]");
      }

      (*pPara) << _T(" = ") << scalar.SetValue(lrd.mg) << rptNewLine;
   }
   else
   {
      ATLASSERT(lrd.Nb==1);
      (*pPara) << _T("For a single-beam superstructure, the lever rule decomposes to the Lanes/Beams method") << rptNewLine;
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

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << rptRcImage(std::_tstring(pgsReportStyleHolder::GetImagePath()) + _T("RigidMethod.gif")) << rptNewLine;
   (*pPara) << _T("Multiple Presence Factor: m = ") << rd.m << rptNewLine;
   (*pPara) << _T("g = ") << _T("(") << rd.Nl << _T("/") << rd.Nb << _T(") + (") << xdim.SetValue(rd.Xext) << _T(")[");
   std::vector<Float64>::iterator iter;
   for ( iter = rd.e.begin(); iter != rd.e.end(); iter++ )
   {
      Float64 e = *iter;
      if ( iter != rd.e.begin() )
         (*pPara) << _T(" + ");

      (*pPara) << _T("(") << xdim.SetValue(e) << _T(")");

   }
   (*pPara) << _T("]/[");
   for ( iter = rd.x.begin(); iter != rd.x.end(); iter++ )
   {
      Float64 x = *iter;
      if ( iter != rd.x.begin() )
         (*pPara) << _T(" + ");

      (*pPara) << _T("(") << xdim.SetValue(x) << _T(")") << Super(_T("2"));
   }
   (*pPara) << _T("] = ") << scalar.SetValue(rd.mg/rd.m) << rptNewLine;
   (*pPara) << _T("mg") << Super(_T("ME")) << Sub(rd.e.size() > 1 ? _T("2+") : _T("1")) << _T(" = ") << scalar.SetValue(rd.mg) << rptNewLine;
}

void ReportLanesBeamsMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::LanesBeamsMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << _T("Multiple Presence Factor: m = ") << rd.m << rptNewLine;
   (*pPara) << _T("g = ") << _T("(") << rd.m <<_T(")(")<< rd.Nl << _T("/") << rd.Nb << _T(") = ") << scalar.SetValue(rd.mg) << rptNewLine;
}