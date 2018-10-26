// dllmain.h : Declaration of module class.

class CPGSuperLibraryModule : public CAtlDllModuleT< CPGSuperLibraryModule >
{
public :
	DECLARE_LIBID(LIBID_PGSuperLibrary)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LIBAPPPLUGIN, "{C42666D1-05FF-43f8-9869-2EBF6AA2A666}")
};

extern class CPGSuperLibraryModule _AtlModule;
