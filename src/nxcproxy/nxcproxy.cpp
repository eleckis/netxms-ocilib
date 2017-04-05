/* 
** NetXMS client proxy
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxcproxy.cpp
**
**/

#include "nxcproxy.h"
#include <nxconfig.h>

#if defined(_WIN32)
#include <conio.h>
#include <locale.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

/**
 * Listener thread
 */
THREAD_RESULT THREAD_CALL ListenerThread(void *);

/**
 * Messages generated by mc.pl (for UNIX version only)
 */
#ifndef _WIN32
extern unsigned int g_dwNumMessages;
extern const TCHAR *g_szMessages[];
#endif

/**
 * Valid options for getopt()
 */
#if defined(_WIN32)
#define VALID_OPTIONS   "c:dD:fhIRsSv"
#else
#define VALID_OPTIONS   "c:dD:fhp:v"
#endif

/**
 * Actions
 */
#define ACTION_NONE                    0
#define ACTION_RUN_PROXY               1
#define ACTION_INSTALL_SERVICE         2
#define ACTION_REMOVE_SERVICE          3
#define ACTION_START_SERVICE           4
#define ACTION_STOP_SERVICE            5
#define ACTION_HELP							6

/**
 * Global variables
 */
UINT32 g_flags = 0;
TCHAR g_logFile[MAX_PATH] = NXCPROXY_DEFAULT_LOG;
TCHAR g_configFile[MAX_PATH] = NXCPROXY_DEFAULT_CONFIG;
TCHAR g_listenAddress[MAX_PATH] = _T("*");
WORD g_listenPort = 4701;
TCHAR g_serverAddress[MAX_PATH] = _T("127.0.0.1");
WORD g_serverPort = 4701;
UINT32 g_debugLevel = (UINT32)NXCONFIG_UNINITIALIZED_VALUE;

#if !defined(_WIN32)
TCHAR g_pidFile[MAX_PATH] = _T("/var/run/nxcproxy.pid");
#endif

/**
 * Static variables
 */
static UINT32 m_dwMaxLogSize = 16384 * 1024;
static UINT32 m_dwLogHistorySize = 4;
static UINT32 m_dwLogRotationMode = NXLOG_ROTATION_BY_SIZE;
static TCHAR m_szDailyLogFileSuffix[64] = _T("");
static TCHAR s_dumpDir[MAX_PATH] = _T("C:\\");
static THREAD m_listenerThread = INVALID_THREAD_HANDLE;

#if defined(_WIN32)
static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
#endif

#if !defined(_WIN32)
static pid_t m_pid;
#endif

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CreateCrashDumps"), CT_BOOLEAN, 0, 0, AF_CATCH_EXCEPTIONS, 0, &g_flags, NULL },
   { _T("DailyLogFileSuffix"), CT_STRING, 0, 0, MAX_PATH, 0, m_szDailyLogFileSuffix, NULL },
	{ _T("DebugLevel"), CT_LONG, 0, 0, 0, 0, &g_debugLevel, &g_debugLevel },
   { _T("DumpDirectory"), CT_STRING, 0, 0, MAX_PATH, 0, s_dumpDir, NULL },
   { _T("FullCrashDumps"), CT_BOOLEAN, 0, 0, AF_WRITE_FULL_DUMP, 0, &g_flags, NULL },
   { _T("ListenAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_listenAddress, NULL },
   { _T("ListenPort"), CT_WORD, 0, 0, 0, 0, &g_listenPort, NULL },
   { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, g_logFile, NULL },
   { _T("LogHistorySize"), CT_LONG, 0, 0, 0, 0, &m_dwLogHistorySize, NULL },
   { _T("LogRotationMode"), CT_LONG, 0, 0, 0, 0, &m_dwLogRotationMode, NULL },
   { _T("MaxLogSize"), CT_LONG, 0, 0, 0, 0, &m_dwMaxLogSize, NULL },
   { _T("ServerAddress"), CT_STRING, 0, 0, MAX_PATH, 0, g_serverAddress, NULL },
   { _T("ServerPort"), CT_WORD, 0, 0, 0, 0, &g_serverPort, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
};

/**
 * Help text
 */
static TCHAR m_szHelpText[] =
   _T("Usage: nxcproxy [options]\n")
   _T("Where valid options are:\n")
   _T("   -c <file>  : Use configuration file <file> (default ") NXCPROXY_DEFAULT_CONFIG _T(")\n")
   _T("   -d         : Run as daemon/service\n")
	_T("   -D <level> : Set debug level (0..9)\n")
   _T("   -h         : Display help and exit\n")
   _T("   -f         : Run in foreground\n")
#ifdef _WIN32
   _T("   -I         : Install Windows service\n")
   _T("   -R         : Remove Windows service\n")
   _T("   -s         : Start Windows servive\n")
   _T("   -S         : Stop Windows service\n")
#else
   _T("   -p         : Path to pid file (default: /var/run/nxcproxy.pid)\n")
#endif
   _T("   -v         : Display version and exit\n")
   _T("\n");

/**
 * Signal handler for UNIX platforms
 */
#if !defined(_WIN32)

static THREAD_RESULT THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGPIPE);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);

	sigprocmask(SIG_BLOCK, &signals, NULL);

	while(1)
	{
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
					goto stop_handler;
				case SIGSEGV:
					abort();
					break;
				default:
					break;
			}
		}
		else
		{
			ThreadSleepMs(100);
		}
	}

stop_handler:
	sigprocmask(SIG_UNBLOCK, &signals, NULL);
	return THREAD_OK;
}

#endif

/**
 * Proxy initialization
 */
bool Initialize()
{
   if (g_debugLevel == (UINT32)NXCONFIG_UNINITIALIZED_VALUE)
      g_debugLevel = 0;

   // Open log file
	if (!(g_flags & AF_USE_SYSLOG))
	{
		if (!nxlog_set_rotation_policy((int)m_dwLogRotationMode, (int)m_dwMaxLogSize, (int)m_dwLogHistorySize, m_szDailyLogFileSuffix))
			if (!(g_flags & AF_DAEMON))
				_tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
	}
   if (!nxlog_open((g_flags & AF_USE_SYSLOG) ? NXCPROXY_SYSLOG_NAME : g_logFile,
	                ((g_flags & AF_USE_SYSLOG) ? NXLOG_USE_SYSLOG : 0) |
	                   ((g_flags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT),
	                _T("NXCPROXY.EXE"),
#ifdef _WIN32
                   0, NULL, MSG_DEBUG))
#else
	                g_dwNumMessages, g_szMessages, MSG_DEBUG))
#endif
	{
		_ftprintf(stderr, _T("FATAL ERROR: Cannot open log file\n"));
		return false;
	}
	nxlog_write(MSG_DEBUG_LEVEL, NXLOG_INFO, "d", g_debugLevel);

#ifdef _WIN32
   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      nxlog_write(MSG_WSASTARTUP_FAILED, NXLOG_ERROR, "e", wrc);
      return false;
   }
#endif

   // Initialize cryptografy
   if (!InitCryptoLib(0xFFFF))
   {
      nxlog_write(MSG_INIT_CRYPTO_FAILED, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      return false;
   }

	m_listenerThread = ThreadCreateEx(ListenerThread, 0, NULL);

   return true;
}

/**
 * Shutdown proxy
 */
void Shutdown()
{
	DebugPrintf(2, _T("Shutdown() called"));
   g_flags |= AF_SHUTDOWN;

	ThreadJoin(m_listenerThread);
   nxlog_write(MSG_PROXY_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
   nxlog_close();

   // Notify main thread about shutdown
#ifdef _WIN32
   ConditionSet(m_hCondShutdown);
#endif
   
   // Remove PID file
#if !defined(_WIN32)
   _tremove(g_pidFile);
#endif
}

/**
 * Common Main()
 */
void Main()
{
   nxlog_write(MSG_PROXY_STARTED, NXLOG_INFO, NULL);

   if (g_flags & AF_DAEMON)
   {
#if defined(_WIN32)
      ConditionWait(m_hCondShutdown, INFINITE);
#else
      StartMainLoop(SignalHandler, NULL);
#endif
   }
   else
   {
#if defined(_WIN32)
      _tprintf(_T("Client proxy running. Press ESC to shutdown.\n"));
      while(1)
      {
         if (_getch() == 27)
            break;
      }
      _tprintf(_T("Client proxy shutting down...\n"));
      Shutdown();
#else
      _tprintf(_T("Client proxy running. Press Ctrl+C to shutdown.\n"));
      StartMainLoop(SignalHandler, NULL);
      _tprintf(_T("\nStopping client proxy...\n"));
#endif
   }
}

/**
 * Application entry point
 */
int main(int argc, char *argv[])
{
   int ch, iExitCode = 0, iAction = ACTION_RUN_PROXY;
	char *eptr;
#ifdef _WIN32
   TCHAR szModuleName[MAX_PATH];
   HKEY hKey;
   DWORD dwSize;
#else
   TCHAR *pszEnv;
#endif

   InitNetXMSProcess(false);

#if defined(__sun) || defined(_AIX) || defined(__hpux)
   signal(SIGPIPE, SIG_IGN);
   signal(SIGHUP, SIG_IGN);
   signal(SIGINT, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGUSR1, SIG_IGN);
   signal(SIGUSR2, SIG_IGN);
#endif

   // Check for alternate config file location
#ifdef _WIN32
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\ClientProxy"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, (BYTE *)g_configFile, &dwSize);
      RegCloseKey(hKey);
   }
#else
   pszEnv = _tgetenv(_T("NXCPROXY_CONFIG"));
   if (pszEnv != NULL)
      nx_strncpy(g_configFile, pszEnv, MAX_PATH);
#endif

   // Parse command line
	if (argc == 1)
		iAction = ACTION_HELP;
   opterr = 1;
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            iAction = ACTION_HELP;
            break;
         case 'd':   // Run as daemon
            g_flags |= AF_DAEMON;
            break;
			case 'f':	// Run in foreground
            g_flags &= ~AF_DAEMON;
				break;
         case 'D':   // Turn on debug output
				g_debugLevel = strtoul(optarg, &eptr, 0);
				if ((*eptr != 0) || (g_debugLevel > 9))
				{
					fprintf(stderr, "Invalid debug level: %s\n", optarg);
					iAction = -1;
					iExitCode = 1;
				}
            break;
         case 'c':   // Configuration file
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_configFile, MAX_PATH);
				g_configFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(g_configFile, optarg, MAX_PATH);
#endif
            break;
#if !defined(_WIN32)
         case 'p':   // PID file
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, g_pidFile, MAX_PATH);
				g_pidFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(g_pidFile, optarg, MAX_PATH);
#endif
            break;
#endif
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS Client Proxy Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T("\n"));
            iAction = ACTION_NONE;
            break;
#ifdef _WIN32
         case 'I':   // Install Windows service
            iAction = ACTION_INSTALL_SERVICE;
            break;
         case 'R':   // Remove Windows service
            iAction = ACTION_REMOVE_SERVICE;
            break;
         case 's':   // Start Windows service
            iAction = ACTION_START_SERVICE;
            break;
         case 'S':   // Stop Windows service
            iAction = ACTION_STOP_SERVICE;
            break;
#endif
         case '?':
            iAction = ACTION_HELP;
            iExitCode = 1;
            break;
         default:
            break;
      }
   }

#if !defined(_WIN32)
	if (!_tcscmp(g_configFile, _T("{search}")))
	{
		if (_taccess(PREFIX _T("/etc/nxcproxy.conf"), 4) == 0)
		{
			_tcscpy(g_configFile, PREFIX _T("/etc/nxcproxy.conf"));
		}
		else if (_taccess(_T("/usr/etc/nxcproxy.conf"), 4) == 0)
		{
			_tcscpy(g_configFile, _T("/usr/etc/nxcproxy.conf"));
		}
		else
		{
			_tcscpy(g_configFile, _T("/etc/nxcproxy.conf"));
		}
	}
#endif

	Config *config = new Config();
	config->setTopLevelTag(_T("config"));

   // Do requested action
   switch(iAction)
   {
      case ACTION_RUN_PROXY:
			if (config->loadConfig(g_configFile, _T("proxy")))
			{
				if (config->parseTemplate(_T("proxy"), m_cfgTemplate))
				{
					// Set exception handler
#ifdef _WIN32
					if (g_flags & AF_CATCH_EXCEPTIONS)
						SetExceptionHandler(SEHServiceExceptionHandler, SEHServiceExceptionDataWriter, s_dumpDir,
												  _T("nxcproxy"), MSG_EXCEPTION, g_flags & AF_WRITE_FULL_DUMP, !(g_flags & AF_DAEMON));
					__try {
#endif
					if ((!_tcsicmp(g_logFile, _T("{syslog}"))) || 
						 (!_tcsicmp(g_logFile, _T("{eventlog}"))))
						g_flags |= AF_USE_SYSLOG;

#ifdef _WIN32
					if (g_flags & AF_DAEMON)
					{
						InitService();
					}
					else
					{
						if (Initialize())
						{
							Main();
						}
						else
						{
							ConsolePrintf(_T("Client proxy initialization failed\n"));
							nxlog_close();
							iExitCode = 3;
						}
					}
#else    /* _WIN32 */
					if (g_flags & AF_DAEMON)
						if (daemon(0, 0) == -1)
						{
							perror("Unable to setup itself as a daemon");
							iExitCode = 4;
						}
					if (iExitCode == 0)
               {
						m_pid = getpid();
						if (Initialize())
						{
							FILE *fp;

							// Write PID file
							fp = _tfopen(g_pidFile, _T("w"));
							if (fp != NULL)
							{
								_ftprintf(fp, _T("%d"), m_pid);
								fclose(fp);
							}   
							Main();
							Shutdown();
						}
						else
						{
							ConsolePrintf(_T("Client proxy initialization failed\n"));
							nxlog_close();
							iExitCode = 3;
						}
	            }
#endif   /* _WIN32 */

#if defined(_WIN32)
					if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
						ConditionDestroy(m_hCondShutdown);
#endif
#ifdef _WIN32
					LIBNETXMS_EXCEPTION_HANDLER
#endif
				}
				else
				{
					ConsolePrintf(_T("Error parsing configuration file\n"));
					iExitCode = 2;
				}
         }
         else
         {
            ConsolePrintf(_T("Error loading configuration file\n"));
            iExitCode = 2;
         }
         break;
#ifdef _WIN32
      case ACTION_INSTALL_SERVICE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallService(szModuleName, NULL, NULL);
         break;
      case ACTION_REMOVE_SERVICE:
         RemoveService();
         break;
      case ACTION_START_SERVICE:
         StartProxyService();
         break;
      case ACTION_STOP_SERVICE:
         StopProxyService();
         break;
#endif
		case ACTION_HELP:
         _fputts(m_szHelpText, stdout);
			break;
      default:
         break;
   }
   delete config;

   return iExitCode;
}
