#pragma once

#include "ShearSteelPage2.h"

// CClosurePourStirrupsPage dialog

class CClosurePourStirrupsPage : public CShearSteelPage2
{
	DECLARE_DYNAMIC(CClosurePourStirrupsPage)

public:
	CClosurePourStirrupsPage();
	virtual ~CClosurePourStirrupsPage();

protected:

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};
