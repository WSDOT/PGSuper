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


//////////////////////////////////////////////////////////////////////////////
// CProxyIProjectPropertiesEventSink
template <class T>
class CProxyIExtendUIEventSink : public WBFL::EAF::EventSinkManager<T>
{
public:

//IExtendUIEventSink : IUnknown
public:
	HRESULT Fire_OnUIHintsReset()
	{
		//T* pT = (T*)this;
		//pT->Lock();
		HRESULT ret = S_OK;
		for(auto& [id,sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   if (callback != nullptr)
			{
				ret = callback->OnHintsReset();
			}
		}
		//pT->Unlock();
		return ret;
	}
};
