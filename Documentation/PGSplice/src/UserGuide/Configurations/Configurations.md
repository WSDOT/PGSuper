Configurations {#ug_configurations}
==============================================
The functionality of PGSplice can be customized with Configurations. Configurations model standard bridge components such as girders and traffic barriers, as well as the design policies of bridge owners such as State DOTs. 


Bridge owners, such as WSDOT and TxDOT, publish configurations on the Internet so all of their in-house and consulting engineers are using the same information. These configurations customize PGSplice for a specific agency.

Configurations make using PGSplice easy for consulting engineers who work on projects in different states. Simply change the configuration to match your client's DOT and get to work.

> NOTE: PGSplice comes with configurations from WSDOT. Check with your client to see if they publish a configuration for their DOT. Also, there are third party vendors that supply DOT configurations.

A Configuration consists of a Master Library of predefined objects such as girders, barriers, live loads, and project criteria, and Project Templates which define the default settings for new projects.

To change your configuration:
1. Start BridgeLink but do not open any projects.
2. Select *File > Configure PGSplice...*

![](ConfigurePGSplice.png)

There are two groups of information that can be set in the Configure PGSplice window; Configuration Information and User Information.

Configuration Information
-----------------
The information in this group defines the source of the configuration and its details. You can use the default configuration that was installed with the software or you can use any number of different configurations published on a local file system, network file system, or on the Internet. You can change your configuration any time you like.

Option | Description
-------|---------------
Use a configuration published on the local file system, network or the Internet | A default configuration was installed with the software. This is a good place to start if you don't know which configuration to use. Note that this configuration is based on WSDOT settings and they are not automatically kept up to date over the Internet.
Select a Configuration Server | This is the typically the option you'll want to use if your organization is doing production work for a bridge owner. By selecting this option, your Configuration is automatically kept up to date at the frequency set in this window.
[Manage] | Press to add, edit, and delete configuration servers.
Select a Configuration | Configuration Servers can have many configuration options. Select a configuration from this list.
More about this configuration... | Click on this hyperlink to open a web site with more information about the selected configuration. 
Update Frequency | The Configuration Server is periodically queried to check for updates. This setting controls the frequency of checking for updates.

### Managing Configurations ###
PGSplice comes pre-configured to use the WSDOT configuration server. You may want to use your own configuration server (see the @ref administrator_guide to learn how to create your own configuration server), or use servers created by other DOTs or third party vendors. To use these other configuration servers, you must add their definition to PGSplice. 

To add a Configuration Server Definition:
1. Get the definition parameters from the server owner
2. In the Configure PGSplice window, press [Manage]. This will open the PGSplice Configuration Servers window. ![](ConfigurationServers.png)
3. Press [Add] to add a new server. You can press [Edit] to change a server definition, or select a server and press [Delete] to remove its definition.
4. In the PGSplice Configuration Server Definition window, enter a name for the server. This can be any name you like. Then select the server type and enter the definition parameters you got from the server owner.
5. Press [OK] to save the server definition. Press [OK] again to close the list of configuration servers.
6. The server you just created will be in the drop down list in the Configure PGSplice window. Select the server. The configuration list will be updated with the configurations published on the server.

 
User Information
------------------
When creating a new project, one of the first things that you will do is supply some basic information about the project. These properties are used to describe a bridge project and to identify the engineer performing the work. Your personal information will be the same for every project. So you don't have to enter your personal information every time you create a new project, you can specify that information in the User Information fields. Enter your name and company in the input fields. This information will be put onto reports and other output.

> NOTE: This information is never set to WSDOT or anyone else.

> NOTE: @ref configurations in the @ref administrator_guide for more information about configurations.


