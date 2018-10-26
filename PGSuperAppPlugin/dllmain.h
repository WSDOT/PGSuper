// dllmain.h : Declaration of module class.

class CPGSuperAppPluginModule : public CAtlDllModuleT< CPGSuperAppPluginModule >
{
public :
	DECLARE_LIBID(LIBID_PGSuperAppPluginLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_PGSUPERAPPPLUGIN, "{FFFAA047-B757-41E5-9235-D106F534558C}")
};

extern class CPGSuperAppPluginModule _AtlModule;
