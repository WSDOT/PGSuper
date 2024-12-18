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


//////////////////////////////////////////////////////////////////////////////
// CProxyIReporterEventSink
template <class T>
class CProxyIReporterEventSink : public IConnectionPointImpl<T, &IID_IReporterEventSink, CComDynamicUnkArray>
{
//IReporterEventSink : IUnknown
public:
	HRESULT Fire_ReportsChanged()
	{
		T* pT = (T*)this;

		pT->Lock();
		HRESULT ret = S_OK;
		IUnknown** pp = this->m_vec.begin();
		while (pp < this->m_vec.end())
		{
			if (*pp != nullptr)
			{
				IReporterEventSink* pIReporterEventSink = reinterpret_cast<IReporterEventSink*>(*pp);
				ret = pIReporterEventSink->OnReportsChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};
