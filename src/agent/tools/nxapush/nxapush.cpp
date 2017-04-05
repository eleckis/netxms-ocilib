/* 
** nxapush - command line tool used to push DCI values to NetXMS server
**           via local NetXMS agent
** Copyright (C) 2006-2016 Raden Solutions
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcpapi.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

/**
 * Pipe handle
 */
#ifdef _WIN32
static HANDLE s_hPipe = NULL;
#else
static int s_hPipe = -1;
#endif

/**
 * Data to send
 */
static StringMap *s_data = new StringMap;

/**
 * options
 */
static int s_optVerbose = 1;
static UINT32 s_optObjectId = 0;
static time_t s_timestamp = 0;

/**
 * Values parser - clean string and split by '='
 */
static BOOL AddValue(TCHAR *pair)
{
	BOOL ret = FALSE;
	TCHAR *p = pair;
	TCHAR *value = NULL;

	for (p = pair; *p != 0; p++)
	{
		if (*p == _T('=') && value == NULL)
		{
			value = p;
		}
		if (*p == 0x0D || *p == 0x0A)
		{
			*p = 0;
			break;
		}
	}

	if (value != NULL)
	{
		*value++ = 0;
		s_data->set(pair, value);
		ret = TRUE;
	}

	return ret;
}

/**
 * Initialize and connect to the agent
 */
static BOOL Startup()
{
#ifdef _WIN32
reconnect:
	s_hPipe = CreateFile(_T("\\\\.\\pipe\\nxagentd.push"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (s_hPipe == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_PIPE_BUSY)
		{
			if (WaitNamedPipe(_T("\\\\.\\pipe\\nxagentd.push"), 5000))
				goto reconnect;
		}
		return FALSE;
	}

	DWORD pipeMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(s_hPipe, &pipeMode, NULL, NULL);
#else
	s_hPipe = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s_hPipe == INVALID_SOCKET)
		return FALSE;

	struct sockaddr_un remote;
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, "/tmp/.nxagentd.push");
	if (connect(s_hPipe, (struct sockaddr *)&remote, SUN_LEN(&remote)) == -1)
	{
		close(s_hPipe);
		s_hPipe = -1;
		return FALSE;
	}
#endif

	if (s_optVerbose > 2)
		_tprintf(_T("Connected to NetXMS agent\n"));

	return TRUE;
}

/**
 * Send all DCIs
 */
static BOOL Send()
{
	BOOL success = FALSE;

	NXCPMessage msg;
	msg.setCode(CMD_PUSH_DCI_DATA);
   msg.setField(VID_OBJECT_ID, s_optObjectId);
   msg.setFieldFromTime(VID_TIMESTAMP, s_timestamp);
   s_data->fillMessage(&msg, VID_NUM_ITEMS, VID_PUSH_DCI_DATA_BASE);

	// Send response to pipe
	NXCP_MESSAGE *rawMsg = msg.createMessage();
#ifdef _WIN32
	DWORD bytes;
	if (!WriteFile(s_hPipe, rawMsg, ntohl(rawMsg->size), &bytes, NULL))
		goto cleanup;
	if (bytes != ntohl(rawMsg->size))
		goto cleanup;
#else
	int bytes = SendEx(s_hPipe, rawMsg, ntohl(rawMsg->size), 0, NULL); 
	if (bytes != (int)ntohl(rawMsg->size))
		goto cleanup;
#endif

	success = TRUE;

cleanup:
	free(rawMsg);
	return success;
}

/**
 * Disconnect and cleanup
 */
static BOOL Teardown()
{
#ifdef _WIN32
	if (s_hPipe != NULL)
	{
		CloseHandle(s_hPipe);
	}
#else
	if (s_hPipe != -1)
	{
		close(s_hPipe);
	}
#endif
	delete s_data;
	return TRUE;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"help",           no_argument,       NULL,        'h'},
	{ (char *)"object",         required_argument, NULL,        'o'},
	{ (char *)"quiet",          no_argument,       NULL,        'q'},
	{ (char *)"timestamp-unix", required_argument, NULL,        't'},
	{ (char *)"timestamp-text", required_argument, NULL,        'T'},
	{ (char *)"verbose",        no_argument,       NULL,        'v'},
	{ (char *)"version",        no_argument,       NULL,        'V'},
	{ NULL, 0, NULL, 0}
};
#endif

#define SHORT_OPTIONS "ho:qt:T:vV"

/**
 * Show online help
 */
static void usage(char *argv0)
{
	_tprintf(
_T("NetXMS Agent PUSH  Version ") NETXMS_VERSION_STRING _T("\n")
_T("Copyright (c) 2006-2013 Raden Solutions\n\n")
_T("Usage: %hs [OPTIONS] [@batch_file] [values]\n")
_T("  \n")
_T("Options:\n")
#if HAVE_GETOPT_LONG
_T("  -h, --help                  Display this help message.\n")
_T("  -o, --object <id>           Push data on behalf of object with given id.\n")
_T("  -q, --quiet                 Suppress all messages.\n")
_T("  -t, --timestamp-unix <time> Specify timestamp for data as UNIX timestamp.\n")
_T("  -T, --timestamp-text <time> Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v, --verbose               Enable verbose messages. Add twice for debug\n")
_T("  -V, --version               Display version information.\n\n")
#else
_T("  -h             Display this help message.\n")
_T("  -o <id>        Push data on behalf of object with given id.\n")
_T("  -q             Suppress all messages.\n")
_T("  -t <time>      Specify timestamp for data as UNIX timestamp.\n")
_T("  -T <time>      Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v             Enable verbose messages. Add twice for debug\n")
_T("  -V             Display version information.\n\n")
#endif
_T("Notes:\n")
_T("  * Values should be given in the following format:\n")
_T("    dci=value\n")
_T("    where dci can be specified by it's name\n")
_T("  * Name of batch file cannot contain character = (equality sign)\n")
_T("\n")
_T("Examples:\n")
_T("  Push two values:\n")
_T("      nxapush PushParam1=1 PushParam2=4\n\n")
_T("  Push values from file:\n")
_T("      nxapush @file\n")
	, argv0);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	InitNetXMSProcess(true);

	opterr = 0;
#if HAVE_DECL_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
#else
	while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
	{
		switch(c)
		{
		   case 'h': // help
			   usage(argv[0]);
			   exit(0);
			   break;
		   case 'o': // object ID
			   s_optObjectId = strtoul(optarg, NULL, 0);
			   break;
		   case 'q': // quiet
			   s_optVerbose = 0;
			   break;
		   case 't': // timestamp as UNIX time
			   s_timestamp = (time_t)strtoull(optarg, NULL, 0);
			   break;
		   case 'T': // timestamp as YYYYMMDDhhmmss
			   s_timestamp = ParseDateTimeA(optarg, 0);
			   break;
		   case 'v': // verbose
			   s_optVerbose++;
			   break;
		   case 'V': // version
			   _tprintf(_T("nxapush (") NETXMS_VERSION_STRING _T(")\n"));
			   exit(0);
			   break;
		   case '?':
			   exit(3);
			   break;
		}
	}
	
	if (optind == argc)
	{
		if (s_optVerbose > 0)
		{
			printf("Not enough arguments\n\n");
#if HAVE_GETOPT_LONG
			printf("Try `%s --help' for more information.\n", argv[0]);
#else
			printf("Try `%s -h' for more information.\n", argv[0]);
#endif
		}
		exit(1);
	}

	// Parse
	if (optind < argc)
	{
		while (optind < argc)
		{
			char *p = argv[optind];

			if (((*p == '@') && (strchr(p, '=') == NULL)) || (*p == '-'))
			{
				FILE *fileHandle = stdin;

				if (*p != '-')
				{
					fileHandle = fopen(p + 1, "r");
				}

				if (fileHandle != NULL)
				{
					char buffer[1024];

					while (fgets(buffer, sizeof(buffer), fileHandle) != NULL)
					{
#ifdef UNICODE
						WCHAR *wvalue = WideStringFromMBString(buffer);
						AddValue(wvalue);
						free(wvalue);
#else
						AddValue(buffer);
#endif
					}

					if (fileHandle != stdin)
					{
						fclose(fileHandle);
					}
				}
				else
				{
					if (s_optVerbose > 0)
					{
						printf("Cannot open \"%s\": %s\n", p + 1, strerror(errno));
					}
				}
			}
			else
			{
#ifdef UNICODE
				WCHAR *wvalue = WideStringFromMBString(argv[optind]);
				AddValue(wvalue);
				free(wvalue);
#else
				AddValue(argv[optind]);
#endif
			}

			optind++;
		}
	}

	if (s_data->size() > 0)
	{
		if (Startup())
		{
			if (Send() != TRUE)
			{
				ret = 3;
			}
		}
	}
	else
	{
		if (s_optVerbose > 0)
		{
			printf("No valid pairs found; nothing to send\n");
			ret = 2;
		}
	}
	Teardown();

	return ret;
}
