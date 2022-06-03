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
#include <PgsExt\TransferLength.h>

class pgsPointOfInterest;
class rptChapter;
interface IEAFDisplayUnits;

class pgsTransferLengthEngineer
{
public:
   pgsTransferLengthEngineer();
   ~pgsTransferLengthEngineer();

   void SetBroker(IBroker* pBroker);

   void Invalidate();

   std::shared_ptr<pgsTransferLength> GetTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const;
   Float64 GetTransferLength(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const;

   //------------------------------------------------------------------------
   // Returns the transfer length adjustment factor. The factor is 0 at the
   // point where bond begins and 1.0 at the end of the transfer length
   Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const;

   //------------------------------------------------------------------------
   // Returns the transfer length adjustment factor. The factor is 0 at the
   // point where bond begins and 1.0 at the end of the transfer length
   Float64 GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, StrandIndexType strandIdx, const GDRCONFIG* pConfig = nullptr) const;

   void ReportTransferLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const;

private:
   mutable std::array<std::map<const CSegmentKey, std::shared_ptr<pgsTransferLength>>, 3> m_Cache;
   IBroker* m_pBroker;
};

class pgsTransferLengthBase : public pgsTransferLength
{
public:
   virtual void ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const = 0;
};

class pgsMinuteTransferLength : public pgsTransferLengthBase
{
public:
   virtual Float64 GetTransferLength() const override;
   virtual void ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const override;
};

class pgsLRFDTransferLength : public pgsTransferLengthBase
{
public:
   pgsLRFDTransferLength();
   pgsLRFDTransferLength(Float64 db, matPsStrand::Coating coating);

   void SetStrandDiameter(Float64 db);
   Float64 GetStrandDiameter() const;

   void SetCoating(matPsStrand::Coating coating);
   matPsStrand::Coating GetCoating() const;

   virtual Float64 GetTransferLength() const override;
   virtual void ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const override;

protected:
   matPsStrand::Coating m_Coating;
   Float64 m_db;
};

class pgsPCIUHPCTransferLength : public pgsTransferLengthBase
{
public:
   pgsPCIUHPCTransferLength();
   pgsPCIUHPCTransferLength(Float64 db);

   void SetStrandDiameter(Float64 db);
   Float64 GetStrandDiameter() const;

   virtual Float64 GetTransferLength() const override;
   virtual void ReportDetails(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const override;

protected:
   Float64 m_db;
};