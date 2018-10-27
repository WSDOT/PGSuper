#pragma once

#include "ShearSteelPage2.h"

// CClosureJointStirrupsPage dialog

class CClosureJointStirrupsPage : public CShearSteelPage2
{
	DECLARE_DYNAMIC(CClosureJointStirrupsPage)

public:
	CClosureJointStirrupsPage();
	virtual ~CClosureJointStirrupsPage();

protected:

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   virtual void GetLastZoneName(CString& strSymmetric, CString& strEnd) override;

protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};
