// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include <PGSuperAll.h>

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <afxwin.h>
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include <IFace\Tools.h>

#if defined NOGRID
#include "nogrid.h"
#else
#include <grid\gxall.h>
#endif // NOGRID

#include <WBFLAtlExt.h>

#include <HtmlHelp.h>
#include <WBFLCore.h>
#include <WBFLTools.h>
#include <WBFLGeometry.h>
#include <WBFLSections.h>
#include <WBFLCogo.h>
#include <WBFLGenericBridge.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderLabel.h>
#include <afxwin.h>

#include <EAF\EAFUtilities.h> // so all files have EAFGetBroker
#include <EAF\EAFResources.h> // so all files have EAF resource identifiers
#include <EAF\EAFHints.h>     // so all files have EAF Doc/View hints

#include <afxdlgs.h>

#include "MakePgz\zip.h"
#include "MakePgz\unzip.h"

#include <MFCTools\MFCTools.h>

using namespace ATL;
