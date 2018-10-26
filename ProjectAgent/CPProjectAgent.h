///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#define EVT_PROJECTPROPERTIES 0x0001
//#define EVT_UNITS             0x0002
#define EVT_EXPOSURECONDITION 0x0004
#define EVT_RELHUMIDITY       0x0008
#define EVT_BRIDGE            0x0010
#define EVT_SPECIFICATION     0x0020
#define EVT_LIBRARYCONFLICT   0x0040
#define EVT_LOADMODIFIER      0x0080
#define EVT_GIRDERFAMILY      0x0100
#define EVT_LIVELOAD          0x0200
#define EVT_LIVELOADNAME      0x0400
#define EVT_ANALYSISTYPE      0x0800
#define EVT_RATING_SPECIFICATION 0x1000
#define EVT_CONSTRUCTIONLOAD     0x2000




//////////////////////////////////////////////////////////////////////////////
// CProxyIProjectPropertiesEventSink
template <class T>
class CProxyIProjectPropertiesEventSink : public IConnectionPointImpl<T, &IID_IProjectPropertiesEventSink, CComDynamicUnkArray>
{
public:

//IProjectPropertiesEventSink : IUnknown
public:
	HRESULT Fire_ProjectPropertiesChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_PROJECTPROPERTIES);
         return S_OK;
      }

		pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IProjectPropertiesEventSink* pIProjectPropertiesEventSink = reinterpret_cast<IProjectPropertiesEventSink*>(*pp);
				ret = pIProjectPropertiesEventSink->OnProjectPropertiesChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyIEnvironmentEventSink
template <class T>
class CProxyIEnvironmentEventSink : public IConnectionPointImpl<T, &IID_IEnvironmentEventSink, CComDynamicUnkArray>
{
public:

//IEnvironmentEventSink : IUnknown
public:
	HRESULT Fire_ExposureConditionChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_EXPOSURECONDITION);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IEnvironmentEventSink* pIEnvironmentEventSink = reinterpret_cast<IEnvironmentEventSink*>(*pp);
				ret = pIEnvironmentEventSink->OnExposureConditionChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
	HRESULT Fire_RelHumidityChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_RELHUMIDITY);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IEnvironmentEventSink* pIEnvironmentEventSink = reinterpret_cast<IEnvironmentEventSink*>(*pp);
				ret = pIEnvironmentEventSink->OnRelHumidityChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyIBridgeDescriptionEventSink
template <class T>
class CProxyIBridgeDescriptionEventSink : public IConnectionPointImpl<T, &IID_IBridgeDescriptionEventSink, CComDynamicUnkArray>
{
public:

public:
	HRESULT Fire_BridgeChanged(CBridgeChangedHint* pHint = NULL)
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_BRIDGE);
         pT->m_PendingBridgeChangedHints.push_back(pHint);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnBridgeChanged(pHint);
			}
			pp++;
		}
      if ( pHint )
      {
         delete pHint;
         pHint = NULL;
      }

		pT->Unlock();
		return ret;
	}

	HRESULT Fire_GirderFamilyChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_GIRDERFAMILY);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnGirderFamilyChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}

   HRESULT Fire_GirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         SpanGirderHashType hash = HashSpanGirder(span,gdr);
         std::map<SpanGirderHashType,Uint32>::iterator found = pT->m_PendingEventsHash.find(hash);
         if ( found != pT->m_PendingEventsHash.end() )
         {
            // there is already an event pending for this girder... add the new hints to it
            std::pair<SpanGirderHashType,Uint32> value = *found;
            value.second |= lHint;
            pT->m_PendingEventsHash.erase(found);
            pT->m_PendingEventsHash.insert( std::make_pair( value.first, value.second ) );
         }
         else
         {
            pT->m_PendingEventsHash.insert( std::make_pair(hash,lHint) );
         }
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnGirderChanged(span,gdr,lHint);
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}

   HRESULT Fire_ConstructionLoadChanged()
   {
		T* pT = (T*)this;
      
      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_CONSTRUCTIONLOAD);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnConstructionLoadChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
   }

	HRESULT Fire_LiveLoadChanged()
	{
		T* pT = (T*)this;
      
      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIVELOAD);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnLiveLoadChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}


	HRESULT Fire_LiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
	{
		T* pT = (T*)this;
      
      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIVELOADNAME);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IBridgeDescriptionEventSink* pEventSink = reinterpret_cast<IBridgeDescriptionEventSink*>(*pp);
				ret = pEventSink->OnLiveLoadNameChanged(strOldName,strNewName);
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyISpecificationEventSink
template <class T>
class CProxyISpecificationEventSink : public IConnectionPointImpl<T, &IID_ISpecificationEventSink, CComDynamicUnkArray>
{
public:

//ISpecificationEventSink : IUnknown
public:
	HRESULT Fire_SpecificationChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_SPECIFICATION);
         return S_OK;
      }

      if ( pT->m_bHoldingEvents )
         return S_OK;

		pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				ISpecificationEventSink* pISpecificationEventSink = reinterpret_cast<ISpecificationEventSink*>(*pp);
				ret = pISpecificationEventSink->OnSpecificationChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}

   HRESULT Fire_AnalysisTypeChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_ANALYSISTYPE);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				ISpecificationEventSink* pISpecificationEventSink = reinterpret_cast<ISpecificationEventSink*>(*pp);
				ret = pISpecificationEventSink->OnAnalysisTypeChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CProxyIRatingSpecificationEventSink
template <class T>
class CProxyIRatingSpecificationEventSink : public IConnectionPointImpl<T, &IID_IRatingSpecificationEventSink, CComDynamicUnkArray>
{
public:

//IRatingSpecificationEventSink : IUnknown
public:

	HRESULT Fire_RatingSpecificationChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_RATING_SPECIFICATION);
         return S_OK;
      }

      if ( pT->m_bHoldingEvents )
         return S_OK;

		pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IRatingSpecificationEventSink* pEventSink = reinterpret_cast<IRatingSpecificationEventSink*>(*pp);
				ret = pEventSink->OnRatingSpecificationChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyILibraryConflictEventSink
template <class T>
class CProxyILibraryConflictEventSink : public IConnectionPointImpl<T, &IID_ILibraryConflictEventSink, CComDynamicUnkArray>
{
public:

//ILibraryConflictGuiEventSink : IUnknown
public:
   HRESULT Fire_OnLibraryConflictResolved()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_LIBRARYCONFLICT);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				ILibraryConflictEventSink* pILibraryConflictEventSink = reinterpret_cast<ILibraryConflictEventSink*>(*pp);
				ret = pILibraryConflictEventSink->OnLibraryConflictResolved();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyILoadModifiersEventSink
template <class T>
class CProxyILoadModifiersEventSink : public IConnectionPointImpl<T, &IID_ILoadModifiersEventSink, CComDynamicUnkArray>
{
public:

//IEnvironmentEventSink : IUnknown
public:
	HRESULT Fire_LoadModifiersChanged()
	{
		T* pT = (T*)this;

      if ( pT->m_bHoldingEvents )
      {
         sysFlags<Uint32>::Set(&pT->m_PendingEvents,EVT_LOADMODIFIER);
         return S_OK;
      }

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				ILoadModifiersEventSink* pEventSink = reinterpret_cast<ILoadModifiersEventSink*>(*pp);
				ret = pEventSink->OnLoadModifiersChanged();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CProxyIEventsEventSink
template <class T>
class CProxyIEventsEventSink : public IConnectionPointImpl<T, &IID_IEventsSink, CComDynamicUnkArray>
{
public:

//IEventsSink : IUnknown
public:
	HRESULT Fire_OnHoldEvents()
	{
		T* pT = (T*)this;

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IEventsSink* pEventSink = reinterpret_cast<IEventsSink*>(*pp);
				ret = pEventSink->OnHoldEvents();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}

	HRESULT Fire_OnFirePendingEvents()
	{
		T* pT = (T*)this;

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IEventsSink* pEventSink = reinterpret_cast<IEventsSink*>(*pp);
				ret = pEventSink->OnFirePendingEvents();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}

	HRESULT Fire_OnCancelPendingEvents()
	{
		T* pT = (T*)this;

      pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IEventsSink* pEventSink = reinterpret_cast<IEventsSink*>(*pp);
				ret = pEventSink->OnCancelPendingEvents();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};
