# GRID_PROPS Environment Variable {#grid_props_environment_variable}

PGSuper requires the GRID_PROPS environment variable to be defined. This environment variable indicates the location of the MSBuild property sheet file for the ObjectiveGrid product. This variable allows for developers to use different versions of the grid control.

If it is not defined, you'll get an error similar to this:

~~~
C:\ARP\PGSuper\PgsExt\PgsExt.vcxproj : error : The value "" of the "Project" attribute in element <Import> is invalid.
~~~

Define the environment variable for the grid control version you are using. For example:

~~~
GRID_PROPS = "C:\Program Files\Perforce\Stingray Studio 2021.1\Src\SS-X64-PropSheet16.props"
~~~

![Edit User Variable dialog showing GRID_PROPS](GridProps_EditUserVariable.png)
