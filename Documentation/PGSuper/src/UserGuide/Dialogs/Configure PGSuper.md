Configure PGSuper {#ug_dialogs_configure}
==============================================
This window is used to select a Configuration.

> NOTE: The first time you run BridgeLink, you must configure PGSuper. To get started quickly, select the default configuration or select the WSDOT or TxDOT configurations from the Internet, then press [Update Configuration Now]. You can always come back and change the configuration later.

> NOTE: You can use configurations for agencies other that WSDOT or TxDOT. Check with the agency to see if they publish a configuration. Several agency specific configurations are available from third party developers. You can also create and publish your own configurations.

Configuration Information
------------------
There are two basic configuration options. 

Option | Description
------|--------------
Use the default configuration | A default configuration was installed with the BridgeLink. This configuration does not change and can become out of date over time. However, it provides a configuration when online configurations are not available. Think of this as a "fail safe" configuration.
Use a configuration published on the local file system, network, or the Internet | These configurations are shared by users over the Internet. Using this type of configuration is the most common option. See Shared Configurations below for more information.

### Shared Configurations ###
Shared configurations are "in the cloud" and are shared by many engineers. Use the drop down list to select a Configuration Server. Then select a specific configuration from the list. For example, the WSDOT server provides a WSDOT configuration and an AASHTO configuration.

Use [Manage] to create or modify your list of available @ref ug_dialogs_configuration_servers.

For more information about a configuration, click the "More about this configuration..." hyperlink. This will open your web browser and go to an information page for the selected configuration.

### Check for configuration updates ###
Use the drop down list to specify the frequency with which the configuration server is checked for updates.

#### Updating the Configuration ###
Press [Update Configuration Now] to download and install the selected configuration.

> TIP: Once you've selected a configuration, you aren't "locked in". You can change at any time.

> TIP: Once a configuration has been installed, you do not need to be connected to your local network or the Internet. The configuration information is stored on your computer. Reconnect to the configuration server on a regular basis to check for updates.

User Information
-----------------
When creating a new projects, one of the first things that you will do is supply some basic information about the project. These properties are used to describe a bridge project and to identify the engineer performing the work. Your personal information will be the same for every project. So you don't have to enter your personal information every time you create a new project, you can specify that information in the User Information fields. Enter your name and company in the input fields. This information will be put onto reports and other output.

> NOTE: This information is never set to WSDOT or anyone else.
