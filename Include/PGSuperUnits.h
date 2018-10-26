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

#ifndef INCLUDED_PGSUPERUNITS_H_
#define INCLUDED_PGSUPERUNITS_H_

#include <Units\SysUnitsMgr.h>
#include <UnitMgt\UnitMgt.h>
#include <WBFLUnitServer.h>
#include <WBFLCogo.h>

BOOL CreateAppUnitSystem(IAppUnitSystem** ppAppUnitSystem);
void CreateDocUnitSystem(IAppUnitSystem* pAppUnitSystem,IDocUnitSystem** ppDocUnitSystem);

// Helper function for formatting text
template <class T>
inline CString FormatDimension(double value,const T& indirectMeasure,bool bIncludeUnitTag = true)
{
   value = ::ConvertFromSysUnits( IsZero(value,indirectMeasure.Tol) ? 0.00 : value, indirectMeasure.UnitOfMeasure );
   sysNumericFormatTool format_tool(indirectMeasure.Format,indirectMeasure.Width,indirectMeasure.Precision);
   std::string str = format_tool.AsString( value );
   CString strDimension;
   if ( bIncludeUnitTag )
      strDimension.Format("%s %s",str.c_str(),indirectMeasure.UnitOfMeasure.UnitTag().c_str());
   else
      strDimension.Format("%s",str.c_str());

   strDimension.TrimLeft();
   return strDimension;
}

inline CString FormatOffset(double offset,const unitmgtLengthData& indirectMeasure,bool bIncludeUnitTag = true)
{
   offset = ::ConvertFromSysUnits( IsZero(offset,indirectMeasure.Tol) ? 0.00 : offset, indirectMeasure.UnitOfMeasure );
   sysNumericFormatTool format_tool(indirectMeasure.Format,indirectMeasure.Width,indirectMeasure.Precision);
   std::string str = format_tool.AsString( offset );
   CString strOffset;
   if ( offset < 0 )
      strOffset.Format("%*.*f L",indirectMeasure.Width,indirectMeasure.Precision, offset );
   else if ( 0 < offset )
      strOffset.Format("%*.*f R",indirectMeasure.Width,indirectMeasure.Precision, offset );
   else
      strOffset.Format("%*.*f",  indirectMeasure.Width,indirectMeasure.Precision, offset );

   strOffset.TrimLeft();

   if ( bIncludeUnitTag )
      strOffset += indirectMeasure.UnitOfMeasure.UnitTag().c_str();

   strOffset.TrimLeft();

   return strOffset;
}

inline CString FormatScalar(double value,const unitmgtScalar& indirectMeasure)
{
   sysNumericFormatTool format_tool(indirectMeasure.Format,indirectMeasure.Width,indirectMeasure.Precision);
   std::string str = format_tool.AsString( value );
   CString strScalar;
   strScalar.Format("%s",str.c_str());
   strScalar.TrimLeft();
   return strScalar;
}

inline CString FormatStation(const unitStationFormat& format,double value)
{
   return format.AsString(value).c_str();
}

inline CString FormatDirection(IDirection* pDirection)
{
   NSDirectionType nsDir;
   EWDirectionType ewDir;
   long deg;
   long min;
   double sec;

   pDirection->get_NSDirection(&nsDir);
   pDirection->get_EWDirection(&ewDir);
   pDirection->get_Degree(&deg);
   pDirection->get_Minute(&min);
   pDirection->get_Second(&sec);

   CString str;
   str.Format("%c %02d %02d %04.1f %c",nsDir == nsNorth ? 'N' : 'S', deg, min, sec, ewDir == ewEast ? 'E' : 'W');
   str.TrimLeft();
   return str;
}

inline CString FormatAngle(IAngle* pAngle)
{
   long deg;
   long min;
   double sec;

   pAngle->get_Degree(&deg);
   pAngle->get_Minute(&min);
   pAngle->get_Second(&sec);

   char cDir = (deg < 0 ? 'R' : 'L');

   CString str;
   str.Format("%02d %02d %04.1f %c",abs(deg), min, sec, cDir);
   str.TrimLeft();
   return str;
}

#endif // INCLUDED_PGSUPERUNITS_H_