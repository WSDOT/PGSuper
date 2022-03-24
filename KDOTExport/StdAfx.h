// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#include <PGSuperAll.h>

#include <PgsExt\GirderLabel.h>
#include <AgentTools.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <afxdlgs.h>

#if defined _NOGRID
#include <NoGrid.h>
#else
#include <grid\gxall.h>
#endif

#include "Documentation\KDOT.hh"

#include <EAF\EAFHelp.h>
