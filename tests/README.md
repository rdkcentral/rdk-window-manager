Basic Usage(wherein rdkwmtest is the executable file to be run)
----------------------------------------------------------------
./rdkwmtest [options] [arguments]

Options and Arguments that can be provided for the executable namely rdkwmtest
------------------------------------------------------------------------------
Below are the available options for executable(rdkwmtest):
(please note below options can be provided independently or in combination
For instance:
root@element-teone:~#./rdkwmtest --display main0 --client main0 --resolution 1920 1080 all)

Note: If "testcase/testcases" and "all": is provided with the executable,"all" will be given preference
      If no options are provided with the executable,by default all the test cases will run with default client name provided by testapp
      To find the test result report you can view the file in the path /opt/logs/rdkwmtest

1) -?  
Display the help message and exits

Sample Input:
   root@element-teone:~#./rdkwmtest -?

2) -l
Lists all the testcases and exits

Sample Input:
   root@element-teone:~#./rdkwmtest -l

3)--test "TestCase/TestCases"
( Can query list of the testcases,to get provide testcase name to be entered)

Sample Input:
   root@element-teone:~#./rdkwmtest --test <testcase#1_name> <testcase#4_name>

4)--test all
Runs all the testcases supported

Sample Input:
   root@element-teone:~#./rdkwmtest --test all


5)--display <display_name>
RDK WM creates the display with provided <display_name>

Sample Input:
   root@element-teone:~#./rdkwmtest --display main

6)--client <client_name>
RDK WM creates the display with provided <client_name>

Sample Input:
   root@element-teone:~#./rdkwmtest --client main

7)--resolution <width> <height>
RDK WM creates the display with user provided resolution <width> <height>

Sample Input:
   root@element-teone:~#./rdkwmtest --resolution 1920 1080

8)--virtualdisplay <virtualWidth> <virtualHeight>
RDK WM creates the virtual display with user provided display dimensions <virtualWidth> <virtualHeight>

Sample Input:
   root@element-teone:~#./rdkwmtest --virtualdisplay 1920 1080

9)--topmost <value>
RDK WM created and sets the display as topmost ,<value>:(1) or not (0)

Sample Input:
   root@element-teone:~#./rdkwmtest --topmost 1

10)--focus <value>
RDK WM creates display with setfocus <value>:(1) or not (0)

Sample Input:
   root@element-teone:~#./rdkwmtest --focus 0

Single Client Test Application:
===============================
SSH session: WM Plugin Activation & single test application
-----------
1)Bootup the Element TV device with RDK-E image
2)sky-appsservice needs to be moved to "inactive" state [Can check the status fo sky-appsservice:systemctl status sky-appsservice]
   root@element-teone:~#systemctl stop sky-appsservice

3)Set the power state to ON
  root@element-teone:~# SetPowerState ON

4)Run single test app with main display name 
  root@element-teone:~# export XDG_RUNTIME_DIR=/tmp
  root@element-teone:~# ./rdkwmtest --display main


Multi Client Test Applications:
===============================
Note:Its mandantory to mention client name while running the test (--client)

SSH session1: WM Plugin Activation & First test application
------------

1)Bootup the Element TV device with RDK-E image
2)sky-appsservice needs to be moved to "inactive" state [Can check the status fo sky-appsservice:systemctl status sky-appsservice]
  systemctl stop sky-appsservice

3)Set the power state to ON
  root@element-teone:~# SetPowerState ON

4)Run single test app with main display name 
  root@element-teone:~# export XDG_RUNTIME_DIR=/tmp
  root@element-teone:~# ./rdkwmtest --display main1 --client testapp1
 

SSH session2: Second test application
------------

1)Run second test app with testapp display name 
 root@element-teone:~# export XDG_RUNTIME_DIR=/tmp
 root@element-teone:~# ./rdkwmtest --display main2 --client testapp2

SSH session3: Third test application
------------

1)Run Third test app with testmain display name 
 root@element-teone:~# export XDG_RUNTIME_DIR=/tmp
 root@element-teone:~# ./rdkwmtest --display main3 --client testapp3
