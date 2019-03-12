///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_TEST1250_H_
#define INCLUDED_IFACE_TEST1250_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#include <PGSuperTypes.h>
#include <string>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
inline bool create_test_file_names(const CString& strExt,const CString& input, CString* pResultsFileName, CString* pPoiFileName, CString* pErrFileName)
{
   // files must be of type .pgs
   CString tmp(input);
   tmp.MakeLower();
   int loc = tmp.Find(strExt,0);
   if (loc>0)
   {
      CString basename = input.Left(input.GetLength()-4);
      *pResultsFileName = basename + _T(".dbr");
      *pPoiFileName = basename + _T(".dbp");
      *pErrFileName = basename + _T(".err");
      return true;
   }
   else
   {
      CString msg;
      msg.Format(_T("Error - Test input files must have %s extension - not: %s"),strExt,input);
      ::AfxMessageBox(msg);

      return false;
   }
}


// special long for regression tests
#define RUN_REGRESSION 666666
#define RUN_CADTEST    777777
#define RUN_GEOMTEST   777666

/*****************************************************************************
INTERFACE
   ITest1250
   Interface for 12-50 testing

DESCRIPTION
   Interface for 12-50 testing
*****************************************************************************/
// {9A8EA2AC-7209-11d3-ADC5-00105A9AF985}
DEFINE_GUID(IID_ITest1250, 
0x9a8ea2ac, 0x7209, 0x11d3, 0xad, 0xc5, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ITest1250 : IUnknown
{
   virtual bool RunTest(long  type,
                        const std::_tstring& outputFileName,
                        const std::_tstring& poiFileName) = 0;

   virtual bool RunTestEx(long  type, const std::vector<SpanGirderHashType>& girderList,
                          const std::_tstring& outputFileName,
                          const std::_tstring& poiFileName) = 0;

   virtual bool IsTesting() const = 0;
};

#endif // INCLUDED_IFACE_TEST1250_H_

