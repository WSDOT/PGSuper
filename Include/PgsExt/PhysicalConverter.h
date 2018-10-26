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

class arvPhysicalConverter: public sysNumericFormatTool
{
public:
   arvPhysicalConverter(Format format = Automatic, Uint16 width = 0, Uint16 precision = 0) :
      sysNumericFormatTool(format,width,precision)
      {
      }

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
      arvPhysicalConverter(umd.Format, umd.Width, umd.Precision),
      m_rT(umd)
      {
      }

   std::_tstring AsString(Float64 val) const
   {
      if (fabs(val) > m_rT.Tol/10.)
         return arvPhysicalConverter::AsString(val);
      else
         return arvPhysicalConverter::AsString(0.0);
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
   const T&             m_rT;
};

// a template class for printing out scalar values
template <class T>
class ScalarFormatTool : public arvPhysicalConverter
{
public:
   // built to take a unitmgtIndirectMeasureDataT
   ScalarFormatTool(const T& umd) :
      arvPhysicalConverter(umd.Format,umd.Width,umd.Precision)
      {
      }

   Float64 Convert(Float64 value) const
   {
      return value;
   }

   std::_tstring UnitTag() const
   {
      return _T("");
   }
};

typedef ScalarFormatTool<unitmgtScalar>          ScalarTool;
typedef PhysicalFormatTool<unitmgtLengthData>    LengthTool;
typedef PhysicalFormatTool<unitmgtMomentData>    MomentTool;
typedef PhysicalFormatTool<unitmgtLengthData>    DeflectionTool;
typedef PhysicalFormatTool<unitmgtAngleData>     RotationTool;
typedef PhysicalFormatTool<unitmgtStressData>    StressTool;
typedef PhysicalFormatTool<unitmgtForceData>     ShearTool;
typedef PhysicalFormatTool<unitmgtForceData>     ForceTool;
typedef PhysicalFormatTool<unitmgtTimeData>      TimeTool;
typedef PhysicalFormatTool<unitmgtLength2Data>   AreaTool;
typedef PhysicalFormatTool<unitmgtLength3Data>   SectionModulusTool;
typedef PhysicalFormatTool<unitmgtLength4Data>   MomentOfInertiaTool;
typedef PhysicalFormatTool<unitmgtPerLengthData> CurvatureTool;

#endif // INCLUDED_PHYSICALCONVERTER_H_