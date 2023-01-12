///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

/// Base class for computing development length. This class manages common parameters
class pgsDevelopmentLengthBase : public pgsDevelopmentLength
{
public:
   pgsDevelopmentLengthBase() = default;
   pgsDevelopmentLengthBase(Float64 db, Float64 fpe, Float64 fps);

   void SetStrandDiameter(Float64 db);
   Float64 GetStrandDiameter() const;

protected:
   Float64 m_db{0.0};
};

/// This class implements development length calculation based on the AASHTO LRFD BDS
class pgsLRFDDevelopmentLength : public pgsDevelopmentLengthBase
{
public:
   pgsLRFDDevelopmentLength() = default;
   pgsLRFDDevelopmentLength(Float64 db, Float64 fpe, Float64 fps, Float64 mbrDepth, bool bDebonded);

   Float64 GetDevelopmentLengthFactor() const;

   virtual Float64 GetDevelopmentLength() const override;

protected:
   Float64 m_MbrDepth{0.0};
   bool m_bDebonded{false};
};

/// This class implements development length calculation based on the PCI UHPC Structural Design Guidance
class pgsPCIUHPCDevelopmentLength : public pgsDevelopmentLengthBase
{
public:
   pgsPCIUHPCDevelopmentLength() = default;
   pgsPCIUHPCDevelopmentLength(Float64 db, Float64 fpe, Float64 fps);

   virtual Float64 GetDevelopmentLength() const override;
};

/// This class implements development length calculation based on the FHWA UHPC Draft Guide Specifications
class pgsFHWAUHPCDevelopmentLength : public pgsDevelopmentLengthBase
{
public:
   pgsFHWAUHPCDevelopmentLength() = default;
   pgsFHWAUHPCDevelopmentLength(Float64 lt,Float64 db, Float64 fpe, Float64 fps);

   virtual Float64 GetDevelopmentLength() const override;

private:
   Float64 m_lt{ 0.0 };
};

/// This class provides common introductory reporting content for subclasses that report development length calculation details
class pgsDevelopmentLengthReporterBase : public pgsDevelopmentLengthReporter
{
public:
   pgsDevelopmentLengthReporterBase() = delete;
   pgsDevelopmentLengthReporterBase(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer);
   virtual void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;

protected:
   const pgsDevelopmentLengthEngineer* m_pEngineer;
   IBroker* m_pBroker;

   const std::_tstring& GetAdjustableStrandName() const;
   PoiAttributeType GetLocationPoiAttribute() const;
   const PoiList& GetPoiList() const;

private:
   mutable std::_tstring m_AdjustableStrandName;
   mutable PoiAttributeType m_PoiType;
   mutable PoiList m_vPoi;
};

/// This class reports the details of development length calculations based on the AASHTO LRFD BDS
class pgsLRFDDevelopmentLengthReporter : public pgsDevelopmentLengthReporterBase
{
public:
   pgsLRFDDevelopmentLengthReporter() = delete;
   pgsLRFDDevelopmentLengthReporter(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer);
   virtual void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;
};

/// This class reports the details of development length calculations based on the PCI UHPC Structural Design Guidance
class pgsPCIUHPCDevelopmentLengthReporter : public pgsDevelopmentLengthReporterBase
{
public:
   pgsPCIUHPCDevelopmentLengthReporter() = delete;
   pgsPCIUHPCDevelopmentLengthReporter(IBroker* pBroker,const pgsDevelopmentLengthEngineer* pEngineer);
   virtual void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;
};

/// This class reports the details of development length calculations based on the FHWA UHPC Draft Guide Specifications
class pgsFHWAUHPCDevelopmentLengthReporter : public pgsDevelopmentLengthReporterBase
{
public:
   pgsFHWAUHPCDevelopmentLengthReporter() = delete;
   pgsFHWAUHPCDevelopmentLengthReporter(IBroker* pBroker, const pgsDevelopmentLengthEngineer* pEngineer);
   virtual void ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const override;
};
