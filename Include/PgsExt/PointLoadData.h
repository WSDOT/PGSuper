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
// PointLoadData.h: interface for the CPointLoadData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_)
#define AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct IStructuredSave;
struct IStructuredLoad;


#include <PgsExt\PgsExtExp.h>
#include <PgsExt\Keys.h>

// namespace to contain user load enums
struct PGSEXTCLASS UserLoads
{
   enum LoadCase{DC, DW, LL_IM};
   enum DistributedLoadType {Uniform, Trapezoidal};
   enum UserLoadType { Distributed, Point, Moment };

   static IDType ms_NextDistributedLoadID;
   static IDType ms_NextPointLoadID;
   static IDType ms_NextMomentLoadID;

   static UserLoadType GetUserLoadTypeFromID(IDType id)
   {
      if ( id < 9999 )
      {
         return Distributed;
      }
      else if ( id < 19999 )
      {
         return Point;
      }
      else
      {
         return Moment;
      }
   }

   static Int32 GetNumLoadCases()
   {
      return 3;
   }

   static LoadCase GetLoadCase(Int32 load_casenum)
   {
      switch(load_casenum)
      {
      case DC:
         return DC;
         break;
      case DW:
         return DW;
         break;
      case LL_IM:
         return LL_IM;
         break;
      default:
         ATLASSERT(false);
         return DC;
      }
   }

   static std::_tstring GetLoadCaseName(Int32 load_casenum)
   {
      switch(load_casenum)
      {
      case DC:
         return std::_tstring(_T("DC"));
         break;
      case DW:
         return std::_tstring(_T("DW"));
         break;
      case LL_IM:
         return std::_tstring(_T("LL + IM"));
         break;
      default:
         ATLASSERT(false);
         return std::_tstring(_T("Error"));
      }
   }

   static int GetNumDistributedLoadTypes()
   {
      return 2;
   }

   static DistributedLoadType GetDistributedLoadType(Int32 num)
   {
      switch(num)
      {
         case Uniform:
            return Uniform;
            break;
         case Trapezoidal:
            return Trapezoidal;
            break;
         default:
            ATLASSERT(false);
            return Trapezoidal;
      }
   }

   static std::_tstring GetDistributedLoadTypeName(Int32 num)
   {
      switch(num)
      {
         case Uniform:
            return std::_tstring(_T("Uniform over entire span"));
            break;
         case Trapezoidal:
            return std::_tstring(_T("Trapezoidal"));
            break;
         default:
            ATLASSERT(false);
            return std::_tstring(_T("Error"));
      }
   }

};

class PGSEXTCLASS CPointLoadData  
{
public:
	CPointLoadData();
   CPointLoadData(const CPointLoadData& other);
	virtual ~CPointLoadData();

   CPointLoadData& operator=(const CPointLoadData& other);

   HRESULT Save(IStructuredSave* pSave);
   HRESULT Load(IStructuredLoad* pSave);

   bool operator == (const CPointLoadData& rOther) const;
   bool operator != (const CPointLoadData& rOther) const;

   // properties
   IDType                m_ID; // this is the load's ID
   UserLoads::LoadCase   m_LoadCase;

   CSpanKey m_SpanKey;
   bool     m_bLoadOnCantilever[2]; // if true, the load is on the cantilever and not the span itself, use pgsTypes::MemberEndType to access array
   Float64  m_Location;   // measured from CL bearing at start of span
   bool     m_Fractional;
   Float64  m_Magnitude;
   std::_tstring m_Description;

   EventIndexType m_StageIndex; // this is the event index corrosponding to the old stage model (BrideSite1,2,3)

protected:
   void MakeCopy(const CPointLoadData& rOther);
   void MakeAssignment(const CPointLoadData& rOther);
};

#endif // !defined(AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_)
