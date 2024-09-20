Command Line Options {#ug_command_line_options}
==============================================
Command line options are parameters that are passed to the program when execution begins. The options alter the normal behavior of the software or provide some other special functionality. This chapter describes the command line options.

PGSplice Command Options
------------------------

PGSplice supports the following command options

Option | Description
-------|--------------
/Configuration=<i>ServerName</i>:<i>PublisherName</i> | Sets the application configuration.
/TestR | Generates NCHRP 12-50 test results for all problem domains
/Test<i>n</i> | Generates NCHRP 12-50 test results for a specified problem domain. Substitute the problem domain ID for <i>n</i>.

Running in Silent Modes (Experimental)
--------------------------------------
With these options PGSuper can be run from the command line without showing User Interface (UI) windows. This is to facilitate running the program from scripts.

Option | Description
-------|--------------
/NoUI   | Normal windows are not displayed during execution. PGsuper's progress messages are redirected (displayed) on the command window (aka, terminal) where the program was started.
/NoUIS   | (Silent Mode) Normal windows are not displayed during execution, and no messages are posted anywhere during execution.

#### Notes ####
  - The program must run to normal completion in order for the Windows process to terminate. If the program does not terminate, the process will remain (orphaned) until the task is ended from the Windows Task Manager or Windows is shut down. Hence, it only makes sense to use these options for PGSuper's testing commands with error-free input files that will run to completion.
  - If an error occurs during execution, the program may show an error dialog. This dialog must be dismissed manually in order for the program to continue.

> TIP: You should always try running your command line without the above silent options to make sure the program will run to termination.
