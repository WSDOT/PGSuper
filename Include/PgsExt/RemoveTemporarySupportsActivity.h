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
#pragma once

#include <PgsExt\PgsExtExp.h>

/*****************************************************************************
CLASS 
   CRemoveTemporarySupportsActivity

DESCRIPTION
   Encapsulates the data for the remove temporary support activity
*****************************************************************************/

class PGSEXTCLASS CTemporarySupportActivityBase
{
public:
   CTemporarySupportActivityBase();
   CTemporarySupportActivityBase(const CTemporarySupportActivityBase& rOther);
   ~CTemporarySupportActivityBase();

   CTemporarySupportActivityBase& operator= (const CTemporarySupportActivityBase& rOther);
   bool operator==(const CTemporarySupportActivityBase& rOther) const;
   bool operator!=(const CTemporarySupportActivityBase& rOther) const;

   void Enable(bool bEnable=true);
   bool IsEnabled() const;

   void Clear();

   void AddTempSupport(SupportIDType tsID);
   void AddTempSupports(const std::vector<SupportIDType>& tempSupports);
   const std::vector<SupportIDType>& GetTempSupports() const;
   bool HasTempSupport(SupportIDType tsID) const;
   void RemoveTempSupport(SupportIDType tsID);
   IndexType GetTempSupportCount() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CTemporarySupportActivityBase& rOther);
   virtual void MakeAssignment(const CTemporarySupportActivityBase& rOther);

   virtual LPCTSTR GetUnitName() = 0;

   bool m_bEnabled;
   std::vector<SupportIDType> m_TempSupports;
};

class PGSEXTCLASS CRemoveTemporarySupportsActivity : public CTemporarySupportActivityBase
{
protected:
   virtual LPCTSTR GetUnitName() { return _T("RemoveTemporarySupports"); }
};
