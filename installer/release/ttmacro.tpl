; ********** PLEASE DO NOT MODIFY THE FOLLOWING FILE DESCRIPTION **************
; FILE NAME:    ttmacro.tpl 
; AUTHOR:       Boris Maisuradze
; LAST UPDATED: 10-Jun-2007
; DESCRIPTION:
;  This file contains macro Templates used by TTLEdit and LogMETTc applications.
;  The file can store any number of templates each starting with header
;  [MACRO_TEMPLATE <template name in File menu>] followed by the actual macro 
;  code. Templates end either with the header of next the template, or the end 
;  of ttmacro.tpl file. Special header type [MACRO_TEMPLATE SEPARATOR] allows to 
;  visually separate/group template names in the menu. Separator header is not 
;  followed by macro code. 
;
;  This file is part of TTLEditor/LogMeTT freeware package developed for 
;  TeraTerm PRO versions 4.x and higher. 
;
; For the latest versions and support visit http://www.neocom.ca/forum
;
; Copyright (c) Neocom Ltd. 2007.
; ********** PLEASE DO NOT MODIFY THE TEXT ABOVE THIS LINE ********************

[MACRO_TEMPLATE Telnet Session] 
; ******* LogMeTT Macro Template for Telnet session ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Openning telnet connection using port 23 
connect '<server_name_or_IP_address>:23 /nossh' 
; 
; Entering synchronous communication mode (useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connecting <Server_name_or_site_name>' 
; Another way of assigning title using LogMeTT key word 
; settitle 'Connecting $connection$' 
; 
; Waiting for Login prompt from remote side 
wait 'ogin:' 
; 
; Sending user name 
sendln '<user_name>' 
; Another way of sending user name using LogMeTT key word 
; sendln '$user$' 
; 
; Waiting for Password prompt from remote side 
wait 'assword:' 
; 
; Calling Popup window to type in password 
passwordbox 'Enter password' 'Login' 
sendln inputstr 
; Previous 2 lines can be replaced with hardcoded password (not recommended) 
; sendln '<password>' 
; 
; Assigning final title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SEPARATOR] 

[MACRO_TEMPLATE SSH1 Session] 
; ******* LogMeTT Macro Template for SSH1 session ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Openning SSH1 connection using port 22. 
; User will be prompted for username and password 
connect '<server_name_or_IP_address> /ssh /1' 
; 
; Entering synchronous communication mode(useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SSH1 Session with Credentials] 
; ******* LogMeTT Macro Template for SSH1 session with credentials ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Calling Popup window to get username 
inputbox 'Enter username' 'Login' 
; Assigning the value entered by user to variable _user 
_user=inputstr 
; Calling Popup window to get password 
passwordbox 'Enter password' 'Login' 
; Building connection string 
_constr='<server_name_or_IP_address> /ssh /1 /auth=password /user=' 
strconcat _constr _user 
strconcat _constr ' /passwd=' 
strconcat _constr inputstr 
; Openning SSH1 conection 
connect _constr 
; Alternative way with hardcoded username and password (not recommended) 
; The following line replaces lines 5..17 
; connect '<server_name_or_IP_address> /ssh /1 /auth=password /user=<username> /passwd=<password>' 
; 
; Entering synchronous communication mode (useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SSH1 Session with Public Key] 
; ******* LogMeTT Macro Template for SSH1 session with public key ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Building connection string 
_constr='<server_name_or_IP_address> /ssh /1 /auth=publickey /user=' 
; Calling Popup window to get username 
inputbox 'Enter username' 'Login' 
strconcat _constr inputstr 
strconcat _constr ' /passwd=' 
; Calling Popup window to get password 
passwordbox 'Enter password' 'Login' 
strconcat _constr inputstr 
strconcat _constr ' /keyfile=' 
; Calling Popup window to get key file name 
inputbox 'Key file' 'Login' 
strconcat _constr inputstr 
; Openning SSH1 conection 
connect _constr 
; Alternative way with hardcoded username and password (not recommended) 
; The following line replaces lines 5..19 
; connect '<server_name_or_IP_address> /ssh /1 /auth=keyfile /user=<username> /passwd=<password> /keyfile=<private>' 
; 
; Entering synchronous communication mode (useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SEPARATOR] 

[MACRO_TEMPLATE SSH2 Session] 
; ******* LogMeTT Macro Template for SSH2 session ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Openning SSH2 connection using port 22. 
; User will be prompted for username and password 
connect '<server_name_or_IP_address> /ssh /2' 
; 
; Entering synchronous communication mode(useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SSH2 Session with Credentials] 
; ******* LogMeTT Macro Template for SSH2 session with credentials ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Calling Popup window to get username 
inputbox 'Enter username' 'Login' 
; Assigning the value entered by user to variable _user 
_user=inputstr 
; Calling Popup window to get password 
passwordbox 'Enter password' 'Login' 
; Building connection string 
_constr='<server_name_or_IP_address> /ssh /2 /auth=password /user=' 
strconcat _constr _user 
strconcat _constr ' /passwd=' 
strconcat _constr inputstr 
; Openning SSH2 conection 
connect _constr 
; Alternative way with hardcoded username and password (not recommended) 
; The following line replaces lines 5..17 
; connect '<server_name_or_IP_address> /ssh /2 /auth=password /user=<username> /passwd=<password>' 
; 
; Entering synchronous communication mode (useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SSH2 Session with Public Key] 
; ******* LogMeTT Macro Template for SSH2 session with public key ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Building connection string 
_constr='<server_name_or_IP_address> /ssh /2 /auth=publickey /user=' 
; Calling Popup window to get username 
inputbox 'Enter username' 'Login' 
strconcat _constr inputstr 
strconcat _constr ' /passwd=' 
; Calling Popup window to get password 
passwordbox 'Enter password' 'Login' 
strconcat _constr inputstr 
strconcat _constr ' /keyfile=' 
; Calling Popup window to get key file name 
inputbox 'Key file' 'Login' 
strconcat _constr inputstr 
; Openning SSH2 conection 
connect _constr 
; Alternative way with hardcoded username and password (not recommended) 
; The following line replaces lines 5..19 
; connect '<server_name_or_IP_address> /ssh /2 /auth=keyfile /user=<username> /passwd=<password> /keyfile=<private>' 
; 
; Entering synchronous communication mode (useful in most cases) 
setsync 1 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
; 

[MACRO_TEMPLATE SEPARATOR] 


[MACRO_TEMPLATE Serial Connection] 
; ******* LogMeTT Macro Template for Serial connection ******* 
; Substitute words inside angle brackets with corresponding values 
; and delete angle brackets. 
; 
; Openning Serial connection using COM port. 
; Valid value for port number is 1..6. 
connect '/C=<port_number>' 
; 
; If COM port is connected to modem the following lines can be used 
; sendln 'ATZ' 
; wait 'OK' 
; ATDT<phone> 
; 
; Assigning new title to TeraTerm window 
settitle 'Connected to <Server_name_or_site_name>' 
;