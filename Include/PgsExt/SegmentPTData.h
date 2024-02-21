///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>
#include <psgLib\DuctLibraryEntry.h>

// Segment PT Data is plant installed post-tensioning

class CSegmentPTData;
class CPrecastSegmentData;

class PGSEXTCLASS CSegmentDuctData
{
public:
   enum GeometryType {Linear, Parabolic};
   enum DuctPointType {Left, Middle, Right};
   
   CSegmentDuctData();
   CSegmentDuctData(const CPrecastSegmentData* pSegment);

   bool operator==(const CSegmentDuctData& other) const;
   
   void Init(const CPrecastSegmentData* pSegment);
   void SetSegment(const CPrecastSegmentData* pSegment);
   const CPrecastSegmentData* GetSegment() const;

   HRESULT Load(IStructuredLoad* pStrLoad, IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave, IProgress* pProgress);
   
   CSegmentPTData* m_pPTData; // weak reference to parent

   std::_tstring Name; // name of library entry
   const DuctLibraryEntry* pDuctLibEntry;

   StrandIndexType nStrands;
   bool bPjCalc;
   Float64 Pj;
   Float64 LastUserPj;

   GeometryType DuctGeometryType; // if Linear, the Middle DuctPoint is undefined
   std::array<std::pair<Float64,pgsTypes::FaceType>, 3> DuctPoint; // use DuctPointType to access array

   pgsTypes::JackingEndType JackingEnd;

protected:
   void MakeCopy(const CSegmentDuctData& rOther);
   void MakeAssignment(const CSegmentDuctData& rOther);

   const CPrecastSegmentData* m_pSegment; // weak reference
};

class PGSEXTCLASS CSegmentPTData
{
public:
   CSegmentPTData();
   CSegmentPTData(const CSegmentPTData& other);
   ~CSegmentPTData();

   CSegmentPTData& operator = (const CSegmentPTData& rOther);
   bool operator==(const CSegmentPTData& rOther) const;
   bool operator!=(const CSegmentPTData& rOther) const;

   void SetSegment(CPrecastSegmentData* pSegment);
   CPrecastSegmentData* GetSegment();
   const CPrecastSegmentData* GetSegment() const;

   void AddDuct(CSegmentDuctData& duct);
   DuctIndexType GetDuctCount() const;
   CSegmentDuctData* GetDuct(DuctIndexType ductIdx);
   const CSegmentDuctData* GetDuct(DuctIndexType ductIdx) const;
   void SetDucts(const std::vector<CSegmentDuctData>& ducts);
   const std::vector<CSegmentDuctData>& GetDucts() const;
   void RemoveDuct(DuctIndexType ductIdx);
   void RemoveDucts();
   StrandIndexType GetStrandCount(DuctIndexType ductIdx) const;
   Float64 GetPjack(DuctIndexType ductIdx) const;

   HRESULT Load(IStructuredLoad* pStrLoad, IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave, IProgress* pProgress);

   pgsTypes::DuctType DuctType;
   pgsTypes::StrandInstallationType InstallationType;
   pgsTypes::SegmentPTEventType InstallationEvent;

   const WBFL::Materials::PsStrand* m_pStrand; // tendon strand type

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   std::vector<CSegmentDuctData> m_Ducts;

   void MakeCopy(const CSegmentPTData& rOther);
   void MakeAssignment(const CSegmentPTData& rOther);

   CPrecastSegmentData* m_pSegment; // weak reference
};