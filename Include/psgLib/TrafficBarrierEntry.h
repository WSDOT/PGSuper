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

#pragma once

#include "PsgLibLib.h"

#include <PsgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <WBFLTools.h>
#include <WBFLGeometry.h>

#include <System\SubjectT.h>

class TrafficBarrierEntry;
class TrafficBarrierEntryObserver;
namespace PGS {namespace Library{class DifferenceItem;};};

#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<TrafficBarrierEntryObserver, TrafficBarrierEntry>;

interface ISidewalkBarrier;

/*****************************************************************************
CLASS 
   TrafficBarrierEntryObserver

   A pure virtual entry class for observing concrete material entries.


DESCRIPTION
   This class may be used to describe observe concrete  materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS TrafficBarrierEntryObserver
{
public:

   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(TrafficBarrierEntry& subject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   TrafficBarrierEntry

   A library entry class for traffic barriers


DESCRIPTION
   This class may be used to describe traffic barriers in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS TrafficBarrierEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<TrafficBarrierEntryObserver, TrafficBarrierEntry>
{
public:
   enum WeightMethod
   {
      Compute,
      Input
   };
   static CString GetWeightMethodType(TrafficBarrierEntry::WeightMethod weightMethod);

   TrafficBarrierEntry();

   TrafficBarrierEntry(const TrafficBarrierEntry& rOther);

   virtual ~TrafficBarrierEntry() = default;

   TrafficBarrierEntry& operator = (const TrafficBarrierEntry& rOther);


   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   // Get the icon for this entry
   virtual HICON GetIcon() const;

   void SetBarrierPoints(IPoint2dCollection* points);
   void GetBarrierPoints(IPoint2dCollection** points) const;

   void CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPolyShape** polyShape) const;

   void SetWeightMethod(WeightMethod method);
   WeightMethod GetWeightMethod() const;
   void SetWeight(Float64 w);
   Float64 GetWeight() const;
   void SetEc(Float64 ec);
   Float64 GetEc() const;

   bool IsBarrierStructurallyContinuous() const;
   void IsBarrierStructurallyContinuous(bool bContinuous);

   void SetCurbOffset(Float64 curbOffset);
   Float64 GetCurbOffset() const;

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const TrafficBarrierEntry& rOther, std::vector<std::unique_ptr<PGS::Library::DifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const TrafficBarrierEntry& rOther,bool bConsiderName=false) const;

protected:
   void CopyValuesAndAttributes(const TrafficBarrierEntry& rOther);

private:
   CComPtr<IPoint2dCollection> m_BarrierPoints;
   WeightMethod m_WeightMethod;
   Float64 m_Weight;
   Float64 m_Ec;
   Float64 m_CurbOffset; // offset from face of barrier to "notional" curb line
   bool m_bStructurallyContinuous;

   bool ComparePoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const;
   void CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2);
   void CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const;
   void ConvertDimensionsToPoints(Float64 x1,Float64 x2,Float64 x3,Float64 x4,Float64 x5,Float64 y1,Float64 y2,Float64 y3);
   void CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPoint2dCollection* points,IPolyShape** polyShape) const;
};

