///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "convert.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// _PgsFileConvert1 properties

/////////////////////////////////////////////////////////////////////////////
// _PgsFileConvert1 operations

long _PgsFileConvert1::Convert(BSTR* inFname, BSTR* outFname)
{
	long result;
	static BYTE parms[] =
		VTS_PBSTR VTS_PBSTR;
	InvokeHelper(0x60030000, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		inFname, outFname);
	return result;
}
