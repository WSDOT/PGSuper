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

#ifndef INCLUDED_PHYSICALCONVERTER_H_
#define INCLUDED_PHYSICALCONVERTER_H_

class arvPhysicalConverter: public sysINumericFormatToolBase
{
public:
   virtual Float64 Convert(Float64 value) const=0;
   virtual std::_tstring UnitTag() const =0;
};

// a template class for printing out physical values
template <class T>
class PhysicalFormatTool : public arvPhysicalConverter
{
public:
   // built to take a unitmgtIndirectMeasureDataT
   PhysicalFormatTool(const T& umd) :
      // these formats are for reports, let's graphs a bit less precision
      m_FormatTool(umd.Format, umd.Width-1, umd.Precision-1),
      m_rT(umd)
      {
         CHECK(umd.Width>0);     // Make sure these are positive. Otherwise subtraction
         CHECK(umd.Precision>0); // above will cause UINT's to roll over
      }

   std::_tstring AsString(Float64 val) const
   {
      if (fabs(val) > m_rT.Tol/10.)
         return m_FormatTool.AsString(val);
      else
         return m_FormatTool.AsString(0.0);
   }

   Float64 Convert(Float64 value) const
   {
      return ::ConvertFromSysUnits(value, m_rT.UnitOfMeasure);
   }

   std::_tstring UnitTag() const
   {
      return m_rT.UnitOfMeasure.UnitTag();
   }
private:
   sysNumericFormatTool m_FormatTool;
   const T&             m_rT;
};

// a template class for printing out scalar values
template <class T>
class ScalarFormatTool : public arvPhysicalConverter
{
public:
   // built to take a unitmgtIndirectMeasureDataT
   ScalarFormatTool(const T& umd) :
      // these formats are for reports, let's graphs a bit less precision
      m_FormatTool(umd.Format, umd.Width-1, umd.Precision-1),
      m_rT(umd)
      {
         CHECK(umd.Width>0);     // Make sure these are positive. Otherwise subtraction
         CHECK(umd.Precision>0); // above will cause UINT's to roll over
      }

   std::_tstring AsString(Float64 val) const
   {
      return m_FormatTool.AsString(val);
   }

   Float64 Convert(Float64 value) const
   {
      return value;
   }

   std::_tstring UnitTag() const
   {
      return _T("");
   }
private:
   sysNumericFormatTool m_FormatTool;
   const T&             m_rT;
};

typedef ScalarFormatTool<unitmgtScalar>        ScalarTool;
typedef PhysicalFormatTool<unitmgtLengthData>  LengthTool;
typedef PhysicalFormatTool<unitmgtMomentData>  MomentTool;
typedef PhysicalFormatTool<unitmgtLengthData>  DisplacementTool;
typedef PhysicalFormatTool<unitmgtStressData>  StressTool;
typedef PhysicalFormatTool<unitmgtForceData>   ShearTool;
typedef PhysicalFormatTool<unitmgtLength2Data> AreaTool;

#endif // INCLUDED_PHYSICALCONVERTER_H_