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

#pragma once
#include <PgsExt\DevelopmentLength.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\TransferLength.h>

class rptChapter;
interface IEAFDisplayUnits;

class pgsDevelopmentLengthEngineer
{
public:
   pgsDevelopmentLengthEngineer();
   ~pgsDevelopmentLengthEngineer();

   void SetBroker(IBroker* pBroker);

   void Invalidate();

   const std::shared_ptr<pgsDevelopmentLength> GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig = nullptr) const;
   const std::shared_ptr<pgsDevelopmentLength> GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, Float64 fps, Float64 fpe, const GDRCONFIG* pConfig = nullptr) const;
   
   Float64 GetDevelopmentLength(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig = nullptr) const;

   Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi, StrandIndexType strandIdx, pgsTypes::StrandType strandType, bool bDebonded, const GDRCONFIG* pConfig = nullptr) const;
   Float64 GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi, StrandIndexType strandIdx, pgsTypes::StrandType strandType, Float64 fps, Float64 fpe, const GDRCONFIG* pConfig = nullptr) const;

   void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const;

private:
   typedef std::array<std::map<const pgsPointOfInterest, std::shared_ptr<pgsDevelopmentLength>>, 3> Cache;
   mutable Cache m_BondedCache;
   mutable Cache m_DebondCache;
   IBroker* m_pBroker;
};

class pgsDevelopmentLengthBase : public pgsDevelopmentLength
{
public:
   pgsDevelopmentLengthBase();
   pgsDevelopmentLengthBase(Float64 db, Float64 fpe, Float64 fps);

   void SetStrandDiameter(Float64 db);
   Float64 GetStrandDiameter() const;

protected:
   Float64 m_db;
};

class pgsLRFDDevelopmentLength : public pgsDevelopmentLengthBase
{
public:
   pgsLRFDDevelopmentLength();
   pgsLRFDDevelopmentLength(Float64 db, Float64 fpe, Float64 fps, Float64 mbrDepth, bool bDebonded);

   Float64 GetDevelopmentLengthFactor() const;

   virtual Float64 GetDevelopmentLength() const override;

protected:
   Float64 m_MbrDepth;
   bool m_bDebonded;
};

class pgsPCIUHPCDevelopmentLength : public pgsDevelopmentLengthBase
{
public:
   pgsPCIUHPCDevelopmentLength();
   pgsPCIUHPCDevelopmentLength(Float64 db, Float64 fpe, Float64 fps);

   virtual Float64 GetDevelopmentLength() const override;
};