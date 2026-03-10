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
#include "TxDataExporter.h"


CTxDataExporter::CTxDataExporter(void)
{
}

CTxDataExporter::~CTxDataExporter(void)
{
}

void CTxDataExporter::WriteIntToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, Int64 intVal)
{
   CString intstring;
   intstring.Format(_T("%d"), intVal);

   WriteStringToCell(hTable, strRangeName, rowIdx, intstring);
}

void CTxDataExporter::WriteFloatToCell(IndexType hTable, LPCTSTR strRangeName, IndexType rowIdx, Float64 fltVal)
{
   // Write full precision
   CString fltstring;
   fltstring.Format(_T("%f"), fltVal);

   WriteStringToCell(hTable, strRangeName, rowIdx, fltstring);
}

CString CTxDataExporter::CreateXYZString(Float64 sysVal,Float64 toler)
{
   Float64 val = ::CeilOffTol(sysVal,toler);
   val = ConvertFromSysUnits(val,WBFL::Units::Measure::Inch);

   CString str;
   str.Format(_T("%.3f"), val);
   return str;
}


std::_tstring CTxDataExporter::CreateFeetInchFracString(Float64 feetValue, Float64 zeroTolerance, Uint16 denominator, CTxDataExporter::Rounding rounding)
{
   // English formatted output  ft'-inn/d"
   Int16 sign = ::BinarySign(feetValue);
   Float64 value = fabs(feetValue); // make a positive number
   Int16 ft = Int16(value); // integer part
   value -= ft; // remove the feet part
   value *= 12; // value now in inches
   Uint16 in = Uint16(value);

   value -= in; // value is now fractions of an inch

   value = ::RoundOff(value, 1.0 / denominator); // round off to the accuracy of the denominator

   Uint16 numerator;
   switch (rounding)
   {
   case CTxDataExporter::RoundOff: numerator = Uint16(::RoundOff(value * denominator, 1)); break;
   case CTxDataExporter::RoundUp: numerator = Uint16(::CeilOff(value * denominator, 1)); break;
   case CTxDataExporter::RoundDown: numerator = Uint16(::FloorOff(value * denominator, 1)); break;
   default: ATLASSERT(false); numerator = Uint16(::RoundOff(value * denominator, 1)); break;
   }

   // reduce the fraction
   while (numerator % 2 == 0 && denominator % 2 == 0)
   {
      numerator /= 2;
      denominator /= 2;
   }

   if (numerator == denominator)
   {
      in++;
      numerator = 0;
   }

   if (in == 12)
   {
      ft++;
      in = 0;
   }

   std::_tostringstream os;
   if (numerator == 0) // whole number of inches
   {
      if (ft == 0) // 1"
         os << sign * in << _T("\"");
      else // 1'-3"
         os << sign * ft << _T("'-") << in << _T("\"");
   }
   else // has fractional inches
   {
      if (ft == 0 && in == 0)
      {
         if (sign < 0)
            os << __T("-");

         os << numerator << _T("/") << denominator << _T("\"");
      }
      else if (ft == 0) // 1 1/2"
      {
         os << sign * in << _T(" ") << numerator << _T("/") << denominator << _T("\"");
      }
      else // 3'-1 1/2"
      {
         os << sign * ft << _T("'-") << in << _T(" ") << numerator << _T("/") << denominator << _T("\"");
      }
   }

   return os.str();
}