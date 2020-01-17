///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

/*****************************************************************************
CLASS 
   CErectPiersActivity

DESCRIPTION
   Encapsulates the data for the pier erection activity for permanent
   and temporary supports
*****************************************************************************/

class PGSEXTCLASS CSupportActivityBase
{
public:
   CSupportActivityBase();
   CSupportActivityBase(const CSupportActivityBase& rOther);
   ~CSupportActivityBase();

   CSupportActivityBase& operator= (const CSupportActivityBase& rOther);
   bool operator==(const CSupportActivityBase& rOther) const;
   bool operator!=(const CSupportActivityBase& rOther) const;

   void Enable(bool bEnable=true);
   bool IsEnabled() const;

   void Clear();

   void AddPier(PierIDType pierID);
   void AddPiers(const std::vector<PierIDType>& piers);
   const std::set<PierIDType>& GetPiers() const;
   bool HasPier(PierIDType pierID) const;
   void RemovePier(PierIDType pierID);
   IndexType GetPierCount() const;

   void AddTempSupport(SupportIDType tsID);
   void AddTempSupports(const std::vector<SupportIDType>& tempSupports);
   const std::set<SupportIDType>& GetTempSupports() const;
   bool HasTempSupport(SupportIDType tsID) const;
   void RemoveTempSupport(SupportIDType tsID);
   IndexType GetTemporarySupportCount() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CSupportActivityBase& rOther);
   void MakeAssignment(const CSupportActivityBase& rOther);

   // called by load/save to give subclasses an opportunity to load/save data
   virtual HRESULT LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress);
   virtual HRESULT SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress);

   virtual LPCTSTR GetUnitName() = 0;
   virtual Float64 GetUnitVersion() = 0;

   bool m_bEnabled;
   std::set<PierIDType> m_Piers;
   std::set<SupportIDType> m_TempSupports;
};

class PGSEXTCLASS CErectPiersActivity : public CSupportActivityBase
{
protected:
   virtual LPCTSTR GetUnitName() { return _T("ErectPiers"); }
   virtual Float64 GetUnitVersion() { return 1.0; }
};

class PGSEXTCLASS CCastClosureJointActivity : public CSupportActivityBase
{
public:
   CCastClosureJointActivity();
   CCastClosureJointActivity(const CCastClosureJointActivity& rOther);

   CCastClosureJointActivity& operator= (const CCastClosureJointActivity& rOther);
   bool operator==(const CCastClosureJointActivity& rOther) const;
   bool operator!=(const CCastClosureJointActivity& rOther) const;

   Float64 GetConcreteAgeAtContinuity() const;
   void SetConcreteAgeAtContinuity(Float64 age);

   void SetCuringDuration(Float64 duration);
   Float64 GetCuringDuration() const;

protected:
   virtual LPCTSTR GetUnitName() override { return _T("CastClosurePours"); }
   virtual Float64 GetUnitVersion() override { return 2.0; }

   void MakeCopy(const CCastClosureJointActivity& rOther);
   void MakeAssignment(const CCastClosureJointActivity& rOther);

   virtual HRESULT LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress) override;
   virtual HRESULT SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress) override;

   Float64 m_Age; // age of concrete when continuity between segments is acheived
   Float64 m_CuringDuration; // duration of time closure is cured (used in shrinkage computations)
};
