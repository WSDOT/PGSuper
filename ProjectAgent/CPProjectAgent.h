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

#include <PsgLib\Keys.h>

#define EVT_PROJECTPROPERTIES    0x0001
//#define EVT_UNITS                0x0002
#define EVT_EXPOSURECONDITION    0x0004
#define EVT_CLIMATECONDITION     0x0004
#define EVT_RELHUMIDITY          0x0008
#define EVT_BRIDGE               0x0010
#define EVT_SPECIFICATION        0x0020
#define EVT_LIBRARYCONFLICT      0x0040
#define EVT_LOADMODIFIER         0x0080
#define EVT_GIRDERFAMILY         0x0100
#define EVT_LIVELOAD             0x0200
#define EVT_LIVELOADNAME         0x0400
#define EVT_ANALYSISTYPE         0x0800
#define EVT_RATING_SPECIFICATION 0x1000
#define EVT_CONSTRUCTIONLOAD     0x2000
#define EVT_LOSSPARAMETERS       0x4000




//////////////////////////////////////////////////////////////////////////////
// CProxyIProjectPropertiesEventSink
template <class T>
class CProxyIProjectPropertiesEventSink : public WBFL::EAF::EventSinkManager<IProjectPropertiesEventSink>
{
public:

//IProjectPropertiesEventSink : IUnknown
public:
	HRESULT Fire_ProjectPropertiesChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_PROJECTPROPERTIES);
         return S_OK;
      }

		//pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
           callback->OnProjectPropertiesChanged();
		}
		//pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyIEnvironmentEventSink
template <class T>
class CProxyIEnvironmentEventSink : public WBFL::EAF::EventSinkManager<IEnvironmentEventSink>
{
public:

//IEnvironmentEventSink : IUnknown
public:
	HRESULT Fire_ExposureConditionChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_EXPOSURECONDITION);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for(auto& [id,sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   ret = callback->OnExposureConditionChanged();
        }
		//pT->Unlock();
		return ret;
	}

	HRESULT Fire_ClimateConditionChanged()
	{
		T* pT = (T*)this;

		if (0 < pT->m_EventHoldCount)
		{
			WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents, EVT_CLIMATECONDITION);
			return S_OK;
		}

		//pT->Lock();
		HRESULT ret = S_OK;
		for(auto& [id,sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnClimateConditionChanged();
		}
		//pT->Unlock();
		return ret;
	}

	HRESULT Fire_RelHumidityChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_RELHUMIDITY);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for(auto& [id,sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnRelHumidityChanged();
		}
		//pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyIBridgeDescriptionEventSink
template <class T>
class CProxyIBridgeDescriptionEventSink : public WBFL::EAF::EventSinkManager<IBridgeDescriptionEventSink>
{
public:

public:
	HRESULT Fire_BridgeChanged(CBridgeChangedHint* pHint = nullptr)
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_BRIDGE);
         pT->m_PendingBridgeChangedHints.push_back(pHint);
         return S_OK;
      }

      if (pT->m_pSpecEntry->GetPrestressLossCriteria().LossMethod != PrestressLossCriteria::LossMethodType::TIME_STEP)
      {
         pT->CreatePrecastGirderBridgeTimelineEvents();
      }

      pT->ValidateBridgeModel();

      //pT->Lock();
		HRESULT ret = S_OK;
		for(auto& [id,sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnBridgeChanged(pHint);
		}
      if ( pHint )
      {
         delete pHint;
         pHint = nullptr;
      }

		//pT->Unlock();
		return ret;
	}

	HRESULT Fire_GirderFamilyChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_GIRDERFAMILY);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnGirderFamilyChanged();
		}
		//pT->Unlock();
		return ret;
	}

   HRESULT Fire_GirderChanged(const CGirderKey& girderKey,Uint32 lHint)
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         std::map<CGirderKey,Uint32>::iterator found( pT->m_PendingEventsHash.find(girderKey) );
         if ( found != pT->m_PendingEventsHash.end() )
         {
            // there is already an event pending for this girder... add the new hints to it
            std::pair<CGirderKey,Uint32> value = *found;
            value.second |= lHint;
            pT->m_PendingEventsHash.erase(found);
            pT->m_PendingEventsHash.insert( std::make_pair( value.first, value.second ) );
         }
         else
         {
            pT->m_PendingEventsHash.insert( std::make_pair(girderKey,lHint) );
         }
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnGirderChanged(girderKey,lHint);
		}
		//pT->Unlock();
		return ret;
	}

   HRESULT Fire_ConstructionLoadChanged()
   {
		T* pT = (T*)this;
      
      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_CONSTRUCTIONLOAD);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnConstructionLoadChanged();
		}
		//pT->Unlock();
		return ret;
   }

	HRESULT Fire_LiveLoadChanged()
	{
		T* pT = (T*)this;
      
      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIVELOAD);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnLiveLoadChanged();
		}
		//pT->Unlock();
		return ret;
	}


	HRESULT Fire_LiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
	{
		T* pT = (T*)this;
      
      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIVELOADNAME);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnLiveLoadNameChanged(strOldName,strNewName);
		}
		//pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyISpecificationEventSink
template <class T>
class CProxyISpecificationEventSink : public WBFL::EAF::EventSinkManager<ISpecificationEventSink>
{
public:

//ISpecificationEventSink : IUnknown
public:
	HRESULT Fire_SpecificationChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_SPECIFICATION);
         return S_OK;
      }

		//pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnSpecificationChanged();
		}
		//pT->Unlock();
		return ret;
	}

   HRESULT Fire_AnalysisTypeChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_ANALYSISTYPE);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnAnalysisTypeChanged();
		}
		//pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyIRatingSpecificationEventSink
template <class T>
class CProxyIRatingSpecificationEventSink : public WBFL::EAF::EventSinkManager<IRatingSpecificationEventSink>
{
public:

//IRatingSpecificationEventSink : IUnknown
public:

	HRESULT Fire_RatingSpecificationChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_RATING_SPECIFICATION);
         return S_OK;
      }

		//pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnRatingSpecificationChanged();
		}
		//pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyILibraryConflictEventSink
template <class T>
class CProxyILibraryConflictEventSink : public WBFL::EAF::EventSinkManager<ILibraryConflictEventSink>
{
public:

//ILibraryConflictGuiEventSink : IUnknown
public:
   HRESULT Fire_OnLibraryConflictResolved()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIBRARYCONFLICT);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnLibraryConflictResolved();
		}
		//pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyILoadModifiersEventSink
template <class T>
class CProxyILoadModifiersEventSink : public WBFL::EAF::EventSinkManager<ILoadModifiersEventSink>
{
public:

//IEnvironmentEventSink : IUnknown
public:
	HRESULT Fire_LoadModifiersChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_LOADMODIFIER);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnLoadModifiersChanged();
		}
		//pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyIEventsEventSink
template <class T>
class CProxyIEventsEventSink : public WBFL::EAF::EventSinkManager<IEventsSink>
{
public:

//IEventsSink : IUnknown
public:
	HRESULT Fire_OnHoldEvents()
	{
		T* pT = (T*)this;

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnHoldEvents();
		}
		//pT->Unlock();
		return ret;
	}

	HRESULT Fire_OnFirePendingEvents()
	{
		T* pT = (T*)this;

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnFirePendingEvents();
		}
		//pT->Unlock();
		return ret;
	}

	HRESULT Fire_OnCancelPendingEvents()
	{
		T* pT = (T*)this;

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnCancelPendingEvents();
		}
		//pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyILossParametersEventSink
template <class T>
class CProxyILossParametersEventSink : public WBFL::EAF::EventSinkManager<ILossParametersEventSink>
{
public:

//ILossParametersEventSink : IUnknown
public:
	HRESULT Fire_OnLossParametersChanged()
	{
		T* pT = (T*)this;

      if ( 0 < pT->m_EventHoldCount )
      {
         WBFL::System::Flags<Uint32>::Set(&pT->m_PendingEvents,EVT_LOSSPARAMETERS);
         return S_OK;
      }

      //pT->Lock();
		HRESULT ret = S_OK;
		for (auto& [id, sink] : this->m_EventSinks)
		{
		   auto callback = sink.lock();
		   callback->OnLossParametersChanged();
		}
		//pT->Unlock();
		return ret;
	}
};
