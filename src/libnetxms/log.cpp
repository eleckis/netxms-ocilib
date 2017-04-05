/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: log.cpp
**
**/

#include "libnetxms.h"
#include <nxstat.h>

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#define MAX_LOG_HISTORY_SIZE	128

/**
 * Static data
 */
#ifdef _WIN32
static HANDLE m_eventLogHandle = NULL;
static HMODULE m_msgModuleHandle = NULL;
#else
static unsigned int m_numMessages;
static const TCHAR **m_messages;
static char s_syslogName[64];
#endif
static TCHAR m_logFileName[MAX_PATH] = _T("");
static FILE *m_logFileHandle = NULL;
static MUTEX m_mutexLogAccess = INVALID_MUTEX_HANDLE;
static UINT32 s_flags = 0;
static int s_debugLevel = 0;
static DWORD s_debugMsg = 0;
static int s_rotationMode = NXLOG_ROTATION_BY_SIZE;
static UINT64 s_maxLogSize = 4096 * 1024;	// 4 MB
static int s_logHistorySize = 4;		// Keep up 4 previous log files
static TCHAR s_dailyLogSuffixTemplate[64] = _T("%Y%m%d");
static time_t m_currentDayStart = 0;
static NxLogConsoleWriter m_consoleWriter = (NxLogConsoleWriter)_tprintf;
static String s_logBuffer;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static CONDITION s_writerStopCondition = INVALID_CONDITION_HANDLE;
static NxLogDebugWriter s_debugWriter = NULL;

/**
 * Set debug level
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level(int level)
{
   if ((level >= 0) && (level <= 9))
      s_debugLevel = level;
}

/**
 * Get current debug level
 */
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level()
{
   return s_debugLevel;
}

/**
 * Set additional debug writer callback. It will be called for each line written with nxlog_debug.
 */
extern "C" void LIBNETXMS_EXPORTABLE nxlog_set_debug_writer(NxLogDebugWriter writer)
{
   s_debugWriter = writer;
}

/**
 * Format current time for output
 */
static TCHAR *FormatLogTimestamp(TCHAR *buffer)
{
	INT64 now = GetCurrentTimeMs();
	time_t t = now / 1000;
#if HAVE_LOCALTIME_R
	struct tm ltmBuffer;
	struct tm *loc = localtime_r(&t, &ltmBuffer);
#else
	struct tm *loc = localtime(&t);
#endif
	_tcsftime(buffer, 32, _T("[%d-%b-%Y %H:%M:%S"), loc);
	_sntprintf(&buffer[21], 8, _T(".%03d]"), (int)(now % 1000));
	return buffer;
}

/**
 * Set timestamp of start of the current day
 */
static void SetDayStart()
{
	time_t now = time(NULL);
	struct tm dayStart;
#if HAVE_LOCALTIME_R
	localtime_r(&now, &dayStart);
#else
	struct tm *ltm = localtime(&now);
	memcpy(&dayStart, ltm, sizeof(struct tm));
#endif
	dayStart.tm_hour = 0;
	dayStart.tm_min = 0;
	dayStart.tm_sec = 0;
	m_currentDayStart = mktime(&dayStart);
}

/**
 * Set log rotation policy
 * Setting log size to 0 or mode to NXLOG_ROTATION_DISABLED disables log rotation
 */
bool LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, UINT64 maxLogSize, int historySize, const TCHAR *dailySuffix)
{
	bool isValid = true;

	if ((rotationMode >= 0) || (rotationMode <= 2))
	{
		s_rotationMode = rotationMode;
		if (rotationMode == NXLOG_ROTATION_BY_SIZE)
		{
			if ((maxLogSize == 0) || (maxLogSize >= 1024))
			{
				s_maxLogSize = maxLogSize;
			}
			else
			{
				s_maxLogSize = 1024;
				isValid = false;
			}

			if ((historySize >= 0) && (historySize <= MAX_LOG_HISTORY_SIZE))
			{
				s_logHistorySize = historySize;
			}
			else
			{
				if (historySize > MAX_LOG_HISTORY_SIZE)
					s_logHistorySize = MAX_LOG_HISTORY_SIZE;
				isValid = false;
			}
		}
		else if (rotationMode == NXLOG_ROTATION_DAILY)
		{
			if ((dailySuffix != NULL) && (dailySuffix[0] != 0))
				nx_strncpy(s_dailyLogSuffixTemplate, dailySuffix, sizeof(s_dailyLogSuffixTemplate) / sizeof(TCHAR));
			SetDayStart();
		}
	}
	else
	{
		isValid = false;
	}

	if (isValid)
	   nxlog_debug(0, _T("Log rotation policy set to %d (size=") UINT64_FMT _T(", count=%d)"), rotationMode, maxLogSize, historySize);

	return isValid;
}

/**
 * Set console writer
 */
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(NxLogConsoleWriter writer)
{
	m_consoleWriter = writer;
}

/**
 * Rotate log
 */
static bool RotateLog(bool needLock)
{
	int i;
	TCHAR oldName[MAX_PATH], newName[MAX_PATH];

	if (s_flags & NXLOG_USE_SYSLOG)
		return FALSE;	// Cannot rotate system logs

	if (needLock)
		MutexLock(m_mutexLogAccess);

	if ((m_logFileHandle != NULL) && (s_flags & NXLOG_IS_OPEN))
	{
		fclose(m_logFileHandle);
		s_flags &= ~NXLOG_IS_OPEN;
	}

	if (s_rotationMode == NXLOG_ROTATION_BY_SIZE)
	{
		// Delete old files
		for(i = MAX_LOG_HISTORY_SIZE; i >= s_logHistorySize; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), m_logFileName, i);
			_tunlink(oldName);
		}

		// Shift file names
		for(; i >= 0; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), m_logFileName, i);
			_sntprintf(newName, MAX_PATH, _T("%s.%d"), m_logFileName, i + 1);
			_trename(oldName, newName);
		}

		// Rename current log to name.0
		_sntprintf(newName, MAX_PATH, _T("%s.0"), m_logFileName);
		_trename(m_logFileName, newName);
	}
	else if (s_rotationMode == NXLOG_ROTATION_DAILY)
	{
#if HAVE_LOCALTIME_R
      struct tm ltmBuffer;
      struct tm *loc = localtime_r(&m_currentDayStart, &ltmBuffer);
#else
      struct tm *loc = localtime(&m_currentDayStart);
#endif
		TCHAR buffer[64];
      _tcsftime(buffer, 64, s_dailyLogSuffixTemplate, loc);

		// Rename current log to name.suffix
		_sntprintf(newName, MAX_PATH, _T("%s.%s"), m_logFileName, buffer);
		_trename(m_logFileName, newName);

		SetDayStart();
	}

   // Reopen log
#if HAVE_FOPEN64
   m_logFileHandle = _tfopen64(m_logFileName, _T("w"));
#else
   m_logFileHandle = _tfopen(m_logFileName, _T("w"));
#endif
   if (m_logFileHandle != NULL)
   {
      s_flags |= NXLOG_IS_OPEN;
      TCHAR buffer[32];
      _ftprintf(m_logFileHandle, _T("%s Log file truncated.\n"), FormatLogTimestamp(buffer));
      fflush(m_logFileHandle);
#ifndef _WIN32
      int fd = fileno(m_logFileHandle);
      fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif
   }

	if (needLock)
		MutexUnlock(m_mutexLogAccess);

	return (s_flags & NXLOG_IS_OPEN) ? true : false;
}

/**
 * API for application to force log rotation
 */
bool LIBNETXMS_EXPORTABLE nxlog_rotate()
{
	return RotateLog(true);
}

/**
 * Background writer thread
 */
static THREAD_RESULT THREAD_CALL BackgroundWriterThread(void *arg)
{
   bool stop = false;
   while(!stop)
   {
      stop = ConditionWait(s_writerStopCondition, 1000);

	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= m_currentDayStart + 86400))
	   {
		   RotateLog(FALSE);
	   }

      MutexLock(m_mutexLogAccess);
      if (!s_logBuffer.isEmpty())
      {
         if (s_flags & NXLOG_PRINT_TO_STDOUT)
            m_consoleWriter(_T("%s"), s_logBuffer.getBuffer());

         size_t buflen = s_logBuffer.length();
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         MutexUnlock(m_mutexLogAccess);

         if (s_flags & NXLOG_DEBUG_MODE)
         {
            char marker[64];
            sprintf(marker, "##(" INT64_FMTA ")" INT64_FMTA " @" INT64_FMTA "\n",
                    (INT64)buflen, (INT64)strlen(data), GetCurrentTimeMs());
#ifdef _WIN32
            fwrite(marker, 1, strlen(marker), m_logFileHandle);
#else
            write(fileno(m_logFileHandle), marker, strlen(marker));
#endif
         }

#ifdef _WIN32
			fwrite(data, 1, strlen(data), m_logFileHandle);
#else
         // write is used here because on linux fwrite is not working
         // after calling fwprintf on a stream
			size_t size = strlen(data);
			size_t offset = 0;
			do
			{
			   int bw = write(fileno(m_logFileHandle), &data[offset], size);
			   if (bw < 0)
			      break;
			   size -= bw;
			   offset += bw;
			} while(size > 0);
#endif
         free(data);

	      // Check log size
	      if ((m_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	      {
	         NX_STAT_STRUCT st;
		      NX_FSTAT(fileno(m_logFileHandle), &st);
		      if ((UINT64)st.st_size >= s_maxLogSize)
			      RotateLog(FALSE);
	      }
      }
      else
      {
         MutexUnlock(m_mutexLogAccess);
      }
   }
   return THREAD_OK;
}

/**
 * Initialize log
 */
bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags,
                                     const TCHAR *msgModule, unsigned int msgCount, const TCHAR **messages, DWORD debugMsg)
{
	s_flags = flags & 0x7FFFFFFF;
#ifdef _WIN32
	m_msgModuleHandle = GetModuleHandle(msgModule);
#else
	m_numMessages = msgCount;
	m_messages = messages;
#endif
	s_debugMsg = debugMsg;

   if (s_flags & NXLOG_USE_SYSLOG)
   {
#ifdef _WIN32
      m_eventLogHandle = RegisterEventSource(NULL, logName);
		if (m_eventLogHandle != NULL)
			s_flags |= NXLOG_IS_OPEN;
#else
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, logName, -1, s_syslogName, 64, NULL, NULL);
		s_syslogName[63] = 0;
#else
		nx_strncpy(s_syslogName, logName, 64);
#endif
      openlog(s_syslogName, LOG_PID, LOG_DAEMON);
		s_flags |= NXLOG_IS_OPEN;
#endif
   }
   else
   {
      TCHAR buffer[32];

		nx_strncpy(m_logFileName, logName, MAX_PATH);
#if HAVE_FOPEN64
      m_logFileHandle = _tfopen64(logName, _T("a"));
#else
      m_logFileHandle = _tfopen(logName, _T("a"));
#endif
      if (m_logFileHandle != NULL)
      {
			s_flags |= NXLOG_IS_OPEN;
         _ftprintf(m_logFileHandle, _T("\n%s Log file opened (rotation policy %d, max size ") UINT64_FMT _T(")\n"),
                   FormatLogTimestamp(buffer), s_rotationMode, s_maxLogSize);
         fflush(m_logFileHandle);

#ifndef _WIN32
         int fd = fileno(m_logFileHandle);
         fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif

         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            s_logBuffer.setAllocationStep(8192);
            s_writerStopCondition = ConditionCreate(TRUE);
            s_writerThread = ThreadCreateEx(BackgroundWriterThread, 0, NULL);
         }
      }

      m_mutexLogAccess = MutexCreate();
		SetDayStart();
   }
	return (s_flags & NXLOG_IS_OPEN) ? true : false;
}

/**
 * Close log
 */
void LIBNETXMS_EXPORTABLE nxlog_close()
{
   if (s_flags & NXLOG_IS_OPEN)
   {
      if (s_flags & NXLOG_USE_SYSLOG)
      {
#ifdef _WIN32
         DeregisterEventSource(m_eventLogHandle);
#else
         closelog();
#endif
      }
      else
      {
         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            ConditionSet(s_writerStopCondition);
            ThreadJoin(s_writerThread);
            ConditionDestroy(s_writerStopCondition);
         }
         if (m_logFileHandle != NULL)
            fclose(m_logFileHandle);
         if (m_mutexLogAccess != INVALID_MUTEX_HANDLE)
            MutexDestroy(m_mutexLogAccess);
      }
	   s_flags &= ~NXLOG_IS_OPEN;
   }
}

/**
 * Write record to log file
 */
static void WriteLogToFile(TCHAR *message, const WORD wType)
{
   TCHAR buffer[64];
   TCHAR loglevel[64];

   switch(wType) 
   {
      case EVENTLOG_ERROR_TYPE:
	      _sntprintf(loglevel, 16, _T("[%s] "), _T("ERROR"));
	      break;
      case EVENTLOG_WARNING_TYPE:
	      _sntprintf(loglevel, 16, _T("[%s] "), _T("WARN "));
	      break;
      case EVENTLOG_INFORMATION_TYPE:
	      _sntprintf(loglevel, 16, _T("[%s] "), _T("INFO "));
	      break;
      case EVENTLOG_DEBUG_TYPE:
	      _sntprintf(loglevel, 16, _T("[%s] "), _T("DEBUG"));
	      break;
      default:
    	  _tcsncpy(loglevel, _T(""), 1);
	      break;
   }

   if (s_flags & NXLOG_BACKGROUND_WRITER)
   {
      MutexLock(m_mutexLogAccess);

	   FormatLogTimestamp(buffer);
      s_logBuffer.append(buffer);
      s_logBuffer.append(_T(" "));
      s_logBuffer.append(loglevel);
      s_logBuffer.append(message);

      MutexUnlock(m_mutexLogAccess);
   }
   else
   {
      // Prevent simultaneous write to log file
      MutexLock(m_mutexLogAccess);

	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= m_currentDayStart + 86400))
	   {
		   RotateLog(FALSE);
	   }

	   FormatLogTimestamp(buffer);
      if (m_logFileHandle != NULL)
	   {
         _ftprintf(m_logFileHandle, _T("%s %s%s"), buffer, loglevel, message);
		   fflush(m_logFileHandle);
	   }
      if (s_flags & NXLOG_PRINT_TO_STDOUT)
         m_consoleWriter(_T("%s %s%s"), buffer, loglevel, message);

	   // Check log size
	   if ((m_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	   {
	      NX_STAT_STRUCT st;
		   NX_FSTAT(fileno(m_logFileHandle), &st);
		   if ((UINT64)st.st_size >= s_maxLogSize)
			   RotateLog(FALSE);
	   }

      MutexUnlock(m_mutexLogAccess);
   }
}

/**
 * Format message (UNIX version)
 */
#ifndef _WIN32

static TCHAR *FormatMessageUX(UINT32 dwMsgId, TCHAR **ppStrings)
{
   const TCHAR *p;
   TCHAR *pMsg;
   int i, iSize, iLen;

   if (dwMsgId >= m_numMessages)
   {
      // No message with this ID
      pMsg = (TCHAR *)malloc(64 * sizeof(TCHAR));
      _sntprintf(pMsg, 64, _T("MSG 0x%08X - Unable to find message text\n"), (unsigned int)dwMsgId);
   }
   else
   {
      iSize = (_tcslen(m_messages[dwMsgId]) + 2) * sizeof(TCHAR);
      pMsg = (TCHAR *)malloc(iSize);

      for(i = 0, p = m_messages[dwMsgId]; *p != 0; p++)
         if (*p == _T('%'))
         {
            p++;
            if ((*p >= _T('1')) && (*p <= _T('9')))
            {
               iLen = _tcslen(ppStrings[*p - _T('1')]);
               iSize += iLen * sizeof(TCHAR);
               pMsg = (TCHAR *)realloc(pMsg, iSize);
               _tcscpy(&pMsg[i], ppStrings[*p - _T('1')]);
               i += iLen;
            }
            else
            {
               if (*p == 0)   // Handle single % character at the string end
                  break;
               pMsg[i++] = *p;
            }
         }
         else
         {
            pMsg[i++] = *p;
         }
      pMsg[i++] = _T('\n');
      pMsg[i] = 0;
   }

   return pMsg;
}

#endif   /* ! _WIN32 */

/**
 * Write log record
 * Parameters:
 * msg    - Message ID
 * wType  - Message type (see ReportEvent() for details)
 * format - Parameter format string, each parameter represented by one character.
 *          The following format characters can be used:
 *             s - String (multibyte or UNICODE, depending on build)
 *             m - multibyte string
 *             u - UNICODE string
 *             d - Decimal integer
 *             x - Hex integer
 *             e - System error code (will appear in log as textual description)
 *             a - IP address in network byte order
 *             A - IPv6 address in network byte order
 *             H - IPv6 address in network byte order (will appear in [])
 *             I - pointer to InetAddress object
 */
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...)
{
   va_list args;
   TCHAR *strings[16], *pMsg, szBuffer[256];
   int numStrings = 0;
   UINT32 error;
#if defined(UNICODE) && !defined(_WIN32)
	char *mbMsg;
#endif

	if (!(s_flags & NXLOG_IS_OPEN) || (msg == 0))
		return;

   memset(strings, 0, sizeof(TCHAR *) * 16);

   if (format != NULL)
   {
      va_start(args, format);

      for(; (format[numStrings] != 0) && (numStrings < 16); numStrings++)
      {
         switch(format[numStrings])
         {
            case 's':
               strings[numStrings] = _tcsdup(va_arg(args, TCHAR *));
               break;
            case 'm':
#ifdef UNICODE
					strings[numStrings] = WideStringFromMBString(va_arg(args, char *));
#else
               strings[numStrings] = strdup(va_arg(args, char *));
#endif
               break;
            case 'u':
#ifdef UNICODE
               strings[numStrings] = wcsdup(va_arg(args, WCHAR *));
#else
					strings[numStrings] = MBStringFromWideString(va_arg(args, WCHAR *));
#endif
               break;
            case 'd':
               strings[numStrings] = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 16, _T("%d"), (int)(va_arg(args, LONG)));
               break;
            case 'x':
               strings[numStrings] = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 16, _T("0x%08X"), (unsigned int)(va_arg(args, UINT32)));
               break;
            case 'a':
               strings[numStrings] = (TCHAR *)malloc(20 * sizeof(TCHAR));
               IpToStr(va_arg(args, UINT32), strings[numStrings]);
               break;
            case 'A':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               Ip6ToStr(va_arg(args, BYTE *), strings[numStrings]);
               break;
            case 'H':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               strings[numStrings][0] = _T('[');
               Ip6ToStr(va_arg(args, BYTE *), strings[numStrings] + 1);
               _tcscat(strings[numStrings], _T("]"));
               break;
            case 'I':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               va_arg(args, InetAddress *)->toString(strings[numStrings]);
               break;
            case 'e':
               error = va_arg(args, UINT32);
#ifdef _WIN32
               if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM | 
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, error,
                                 MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                                 (LPTSTR)&pMsg, 0, NULL) > 0)
               {
                  pMsg[_tcscspn(pMsg, _T("\r\n"))] = 0;
                  strings[numStrings] = (TCHAR *)malloc((_tcslen(pMsg) + 1) * sizeof(TCHAR));
                  _tcscpy(strings[numStrings], pMsg);
                  LocalFree(pMsg);
               }
               else
               {
                  strings[numStrings] = (TCHAR *)malloc(64 * sizeof(TCHAR));
                  _sntprintf(strings[numStrings], 64, _T("MSG 0x%08X - Unable to find message text"), error);
               }
#else   /* _WIN32 */
#if HAVE_STRERROR_R
#if HAVE_POSIX_STRERROR_R
					strings[numStrings] = (TCHAR *)malloc(256 * sizeof(TCHAR));
					_tcserror_r((int)error, strings[numStrings], 256);
#else
					strings[numStrings] = _tcsdup(_tcserror_r((int)error, szBuffer, 256));
#endif
#else
					strings[numStrings] = _tcsdup(_tcserror((int)error));
#endif
#endif
               break;
            default:
               strings[numStrings] = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 32, _T("BAD FORMAT (0x%08X)"), (unsigned int)(va_arg(args, UINT32)));
               break;
         }
      }
      va_end(args);
   }

#ifdef _WIN32
   if (!(s_flags & NXLOG_USE_SYSLOG) || (s_flags & NXLOG_PRINT_TO_STDOUT))
   {
      LPVOID lpMsgBuf;

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        m_msgModuleHandle, msg, 0, (LPTSTR)&lpMsgBuf, 0, (va_list *)strings) > 0)
      {
         TCHAR *pCR;

         // Replace trailing CR/LF pair with LF
         pCR = _tcschr((TCHAR *)lpMsgBuf, _T('\r'));
         if (pCR != NULL)
         {
            *pCR = _T('\n');
            pCR++;
            *pCR = 0;
         }
			if (s_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), (TCHAR *)lpMsgBuf);
			}
			else
			{
				WriteLogToFile((TCHAR *)lpMsgBuf, wType);
			}
         LocalFree(lpMsgBuf);
      }
      else
      {
         TCHAR message[64];

         _sntprintf(message, 64, _T("MSG 0x%08X - cannot format message"), msg);
			if (s_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), message);
			}
			else
			{
	         WriteLogToFile(message, wType);
			}
      }
   }

   if (s_flags & NXLOG_USE_SYSLOG)
	{
      ReportEvent(m_eventLogHandle, (wType == EVENTLOG_DEBUG_TYPE) ? EVENTLOG_INFORMATION_TYPE : wType, 0, msg, NULL, numStrings, 0, (const TCHAR **)strings, NULL);
	}
#else  /* _WIN32 */
   pMsg = FormatMessageUX(msg, strings);
   if (s_flags & NXLOG_USE_SYSLOG)
   {
      int level;

      switch(wType)
      {
         case EVENTLOG_ERROR_TYPE:
            level = LOG_ERR;
            break;
         case EVENTLOG_WARNING_TYPE:
            level = LOG_WARNING;
            break;
         case EVENTLOG_INFORMATION_TYPE:
            level = LOG_NOTICE;
            break;
         case EVENTLOG_DEBUG_TYPE:
            level = LOG_DEBUG;
            break;
         default:
            level = LOG_INFO;
            break;
      }
#ifdef UNICODE
		mbMsg = MBStringFromWideString(pMsg);
      syslog(level, "%s", mbMsg);
		free(mbMsg);
#else
      syslog(level, "%s", pMsg);
#endif

		if (s_flags & NXLOG_PRINT_TO_STDOUT)
		{
			m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), pMsg);
		}
   }
   else
   {
      WriteLogToFile(pMsg, wType);
   }
   free(pMsg);
#endif /* _WIN32 */

   while(--numStrings >= 0)
      free(strings[numStrings]);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug(int level, const TCHAR *format, ...)
{
   if (level > s_debugLevel)
      return;

   va_list args;
   va_start(args, format);
   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);
   nxlog_write(s_debugMsg, NXLOG_DEBUG, "s", buffer);

   if (s_debugWriter != NULL)
      s_debugWriter(buffer);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug2(int level, const TCHAR *format, va_list args)
{
   if (level > s_debugLevel)
      return;

   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   nxlog_write(s_debugMsg, NXLOG_DEBUG, "s", buffer);

   if (s_debugWriter != NULL)
      s_debugWriter(buffer);
}
