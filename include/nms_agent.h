/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: nms_agent.h
**
**/

#ifndef _nms_agent_h_
#define _nms_agent_h_

#ifdef _WIN32
#ifdef LIBNXAGENT_EXPORTS
#define LIBNXAGENT_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXAGENT_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXAGENT_EXPORTABLE
#endif


#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <nxdbapi.h>
#include <nxcpapi.h>

/**
 * Initialization function declaration macro
 */
#if defined(_STATIC_AGENT) || defined(_NETWARE)
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" BOOL NxSubAgentRegister_##name(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#else
#ifdef _WIN32
#define DECLSPEC_EXPORT __declspec(dllexport) __cdecl
#else
#define DECLSPEC_EXPORT
#endif
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" BOOL DECLSPEC_EXPORT NxSubAgentRegister(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#endif

/**
 * Constants
 */
#define AGENT_LISTEN_PORT        4700
#define AGENT_TUNNEL_PORT        4703
#define AGENT_PROTOCOL_VERSION   2
#define MAX_RESULT_LENGTH        256
#define MAX_CMD_LEN              256
#define COMMAND_TIMEOUT          60
#define MAX_SUBAGENT_NAME        64
#define MAX_INSTANCE_COLUMNS     8

/**
 * Agent policy types
 */
#define AGENT_POLICY_CONFIG      1
#define AGENT_POLICY_LOG_PARSER  2

/**
 * Agent log parser policy folder
 */
#define LOGPARSER_AP_FOLDER _T("logparser_ap")
#define CONFIG_AP_FOLDER _T("config_ap")

/**
 * Error codes
 */
#define ERR_SUCCESS                 ((UINT32)0)
#define ERR_UNKNOWN_COMMAND         ((UINT32)400)
#define ERR_AUTH_REQUIRED           ((UINT32)401)
#define ERR_ACCESS_DENIED           ((UINT32)403)
#define ERR_UNKNOWN_PARAMETER       ((UINT32)404)
#define ERR_REQUEST_TIMEOUT         ((UINT32)408)
#define ERR_AUTH_FAILED             ((UINT32)440)
#define ERR_ALREADY_AUTHENTICATED   ((UINT32)441)
#define ERR_AUTH_NOT_REQUIRED       ((UINT32)442)
#define ERR_INTERNAL_ERROR          ((UINT32)500)
#define ERR_NOT_IMPLEMENTED         ((UINT32)501)
#define ERR_OUT_OF_RESOURCES        ((UINT32)503)
#define ERR_NOT_CONNECTED           ((UINT32)900)
#define ERR_CONNECTION_BROKEN       ((UINT32)901)
#define ERR_BAD_RESPONSE            ((UINT32)902)
#define ERR_IO_FAILURE              ((UINT32)903)
#define ERR_RESOURCE_BUSY           ((UINT32)904)
#define ERR_EXEC_FAILED             ((UINT32)905)
#define ERR_ENCRYPTION_REQUIRED     ((UINT32)906)
#define ERR_NO_CIPHERS              ((UINT32)907)
#define ERR_INVALID_PUBLIC_KEY      ((UINT32)908)
#define ERR_INVALID_SESSION_KEY     ((UINT32)909)
#define ERR_CONNECT_FAILED          ((UINT32)910)
#define ERR_MALFORMED_COMMAND       ((UINT32)911)
#define ERR_SOCKET_ERROR            ((UINT32)912)
#define ERR_BAD_ARGUMENTS           ((UINT32)913)
#define ERR_SUBAGENT_LOAD_FAILED    ((UINT32)914)
#define ERR_FILE_OPEN_ERROR         ((UINT32)915)
#define ERR_FILE_STAT_FAILED        ((UINT32)916)
#define ERR_MEM_ALLOC_FAILED        ((UINT32)917)
#define ERR_FILE_DELETE_FAILED      ((UINT32)918)
#define ERR_NO_SESSION_AGENT        ((UINT32)919)
#define ERR_SERVER_ID_UNSET         ((UINT32)920)
#define ERR_NO_SUCH_INSTANCE        ((UINT32)921)
#define ERR_OUT_OF_STATE_REQUEST    ((UINT32)922)
#define ERR_ENCRYPTION_ERROR        ((UINT32)923)

/**
 * Bulk data reconciliation DCI processing status codes
 */
#define BULK_DATA_REC_RETRY      0
#define BULK_DATA_REC_SUCCESS    1
#define BULK_DATA_REC_FAILURE    2

/**
 * Max bulk data block size
 */
#define MAX_BULK_DATA_BLOCK_SIZE 8192

/**
 * Parameter handler return codes
 */
#define SYSINFO_RC_SUCCESS             0
#define SYSINFO_RC_UNSUPPORTED         1
#define SYSINFO_RC_ERROR               2
#define SYSINFO_RC_NO_SUCH_INSTANCE    3

/**
 * WinPerf features
 */
#define WINPERF_AUTOMATIC_SAMPLE_COUNT    ((UINT32)0x00000001)
#define WINPERF_REMOTE_COUNTER_CONFIG     ((UINT32)0x00000002)

/**
 * User session states (used by session agents)
 */
#define USER_SESSION_ACTIVE         0
#define USER_SESSION_CONNECTED      1
#define USER_SESSION_DISCONNECTED   2
#define USER_SESSION_IDLE           3
#define USER_SESSION_OTHER          4

/**
 * Descriptions for common parameters
 */
#define DCIDESC_FS_AVAIL                          _T("Available space on file system {instance}")
#define DCIDESC_FS_AVAILPERC                      _T("Percentage of available space on file system {instance}")
#define DCIDESC_FS_FREE                           _T("Free space on file system {instance}")
#define DCIDESC_FS_FREEPERC                       _T("Percentage of free space on file system {instance}")
#define DCIDESC_FS_TOTAL                          _T("Total space on file system {instance}")
#define DCIDESC_FS_TYPE                           _T("Type of file system {instance}")
#define DCIDESC_FS_USED                           _T("Used space on file system {instance}")
#define DCIDESC_FS_USEDPERC                       _T("Percentage of used space on file system {instance}")
#define DCIDESC_LVM_LV_SIZE                       _T("Size of logical volume {instance}")
#define DCIDESC_LVM_LV_STATUS                     _T("Status of logical volume {instance}")
#define DCIDESC_LVM_PV_FREE                       _T("Free space on physical volume {instance}")
#define DCIDESC_LVM_PV_FREEPERC                   _T("Percentage of free space on physical volume {instance}")
#define DCIDESC_LVM_PV_RESYNC_PART                _T("Number of resynchronizing partitions on physical volume {instance}")
#define DCIDESC_LVM_PV_STALE_PART                 _T("Number of stale partitions on physical volume {instance}")
#define DCIDESC_LVM_PV_STATUS                     _T("Status of physical volume {instance}")
#define DCIDESC_LVM_PV_TOTAL                      _T("Total size of physical volume {instance}")
#define DCIDESC_LVM_PV_USED                       _T("Used space on physical volume {instance}")
#define DCIDESC_LVM_PV_USEDPERC                   _T("Percentage of used space on physical volume {instance}")
#define DCIDESC_LVM_VG_FREE                       _T("Free space in volume group {instance}")
#define DCIDESC_LVM_VG_FREEPERC                   _T("Percentage of free space in volume group {instance}")
#define DCIDESC_LVM_VG_LVOL_TOTAL                 _T("Number of logical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_PVOL_ACTIVE                _T("Number of active physical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_PVOL_TOTAL                 _T("Number of physical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_RESYNC_PART                _T("Number of resynchronizing partitions in volume group {instance}")
#define DCIDESC_LVM_VG_STALE_PART                 _T("Number of stale partitions in volume group {instance}")
#define DCIDESC_LVM_VG_STATUS                     _T("Status of volume group {instance}")
#define DCIDESC_LVM_VG_TOTAL                      _T("Total size of volume group {instance}")
#define DCIDESC_LVM_VG_USED                       _T("Used space in volume group {instance}")
#define DCIDESC_LVM_VG_USEDPERC                   _T("Percentage of used space in volume group {instance}")
#define DCIDESC_NET_INTERFACE_64BITCOUNTERS       _T("Is 64bit interface counters supported")
#define DCIDESC_NET_INTERFACE_ADMINSTATUS         _T("Administrative status of interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESIN             _T("Number of input bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESOUT            _T("Number of output bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_DESCRIPTION         _T("Description of interface {instance}")
#define DCIDESC_NET_INTERFACE_INERRORS            _T("Number of input errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_LINK                _T("Link status for interface {instance}")
#define DCIDESC_NET_INTERFACE_MTU                 _T("MTU for interface {instance}")
#define DCIDESC_NET_INTERFACE_OPERSTATUS          _T("Operational status of interface {instance}")
#define DCIDESC_NET_INTERFACE_OUTERRORS           _T("Number of output errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSIN           _T("Number of input packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSOUT          _T("Number of output packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_SPEED               _T("Speed of interface {instance}")
#define DCIDESC_NET_IP_FORWARDING                 _T("IP forwarding status")
#define DCIDESC_NET_IP6_FORWARDING                _T("IPv6 forwarding status")
#define DCIDESC_NET_RESOLVER_ADDRBYNAME           _T("Resolver: address for name {instance}")
#define DCIDESC_NET_RESOLVER_NAMEBYADDR           _T("Resolver: name for address {instance}")
#define DCIDESC_PHYSICALDISK_FIRMWARE             _T("Firmware version of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_MODEL                _T("Model of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SERIALNUMBER         _T("Serial number of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SMARTATTR            _T("")
#define DCIDESC_PHYSICALDISK_SMARTSTATUS          _T("Status of hard disk {instance} reported by SMART")
#define DCIDESC_PHYSICALDISK_TEMPERATURE          _T("Temperature of hard disk {instance}")
#define DCIDESC_SYSTEM_CPU_CACHE_SIZE             _T("CPU {instance}: cache size (KB)")
#define DCIDESC_SYSTEM_CPU_CORE_ID                _T("CPU {instance}: core ID")
#define DCIDESC_SYSTEM_CPU_COUNT                  _T("Number of CPU in the system")
#define DCIDESC_SYSTEM_CPU_FREQUENCY              _T("CPU {instance}: frequency")
#define DCIDESC_SYSTEM_CPU_MODEL                  _T("CPU {instance}: model")
#define DCIDESC_SYSTEM_CPU_PHYSICAL_ID            _T("CPU {instance}: physical ID")
#define DCIDESC_SYSTEM_HOSTNAME                   _T("Host name")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE  _T("Available physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT _T("Percentage of available physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS    _T("Physical memory used for buffers")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS_PCT _T("Percentage of physical memory used for buffers")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED     _T("Physical memory used for cache")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT _T("Percentage of physical memory used for cache")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE       _T("Free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT   _T("Percentage of free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL      _T("Total amount of physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED       _T("Used physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT   _T("Percentage of used physical memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE      _T("Active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT  _T("Percentage of active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE        _T("Free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT    _T("Percentage of free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL       _T("Total amount of virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED        _T("Used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT    _T("Percentage of used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE   _T("Available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT _T("Percentage of available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE           _T("Free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT       _T("Percentage of free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL          _T("Total amount of swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED           _T("Used swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT       _T("Percentage of used swap space")
#define DCIDESC_SYSTEM_MSGQUEUE_BYTES             _T("Message queue {instance}: bytes in queue")
#define DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX         _T("Message queue {instance}: maximum allowed bytes in queue")
#define DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME       _T("Message queue {instance}: last change time")
#define DCIDESC_SYSTEM_MSGQUEUE_MESSAGES          _T("Message queue {instance}: number of messages")
#define DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME         _T("Message queue {instance}: last receive time")
#define DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME         _T("Message queue {instance}: last send time")
#define DCIDESC_SYSTEM_UNAME                      _T("System uname")
#define DCIDESC_AGENT_ACCEPTEDCONNECTIONS         _T("Number of connections accepted by agent")
#define DCIDESC_AGENT_ACCEPTERRORS                _T("Number of accept() call errors")
#define DCIDESC_AGENT_ACTIVECONNECTIONS           _T("Number of active connections to agent")
#define DCIDESC_AGENT_AUTHENTICATIONFAILURES      _T("Number of authentication failures")
#define DCIDESC_AGENT_CONFIG_SERVER               _T("Configuration server address set on agent startup")
#define DCIDESC_AGENT_DATACOLLQUEUESIZE           _T("Agent data collector queue size")
#define DCIDESC_AGENT_FAILEDREQUESTS              _T("Number of failed requests to agent")
#define DCIDESC_AGENT_GENERATED_TRAPS             _T("Number of traps generated by agent")
#define DCIDESC_AGENT_IS_SUBAGENT_LOADED          _T("Check if given subagent is loaded")
#define DCIDESC_AGENT_LAST_TRAP_TIME              _T("Timestamp of last generated trap")
#define DCIDESC_AGENT_LOCALDB_FAILED_QUERIES      _T("Agent local database: failed queries")
#define DCIDESC_AGENT_LOCALDB_SLOW_QUERIES        _T("Agent local database: long running queries")
#define DCIDESC_AGENT_LOCALDB_STATUS              _T("Agent local database: status")
#define DCIDESC_AGENT_LOCALDB_TOTAL_QUERIES       _T("Agent local database: total queries executed")
#define DCIDESC_AGENT_LOG_STATUS                  _T("Agent log status")
#define DCIDESC_AGENT_PROCESSEDREQUESTS           _T("Number of requests processed by agent")
#define DCIDESC_AGENT_PROXY_ACTIVESESSIONS        _T("Number of active proxy sessions")
#define DCIDESC_AGENT_PROXY_CONNECTIONREQUESTS    _T("Number of proxy connection requests")
#define DCIDESC_AGENT_PROXY_ISENABLED             _T("Check if agent proxy is enabled")
#define DCIDESC_AGENT_REGISTRAR                   _T("Registrar server address set on agent startup")
#define DCIDESC_AGENT_REJECTEDCONNECTIONS         _T("Number of connections rejected by agent")
#define DCIDESC_AGENT_SENT_TRAPS                  _T("Number of traps successfully sent to server")
#define DCIDESC_AGENT_SNMP_ISPROXYENABLED         _T("Check if SNMP proxy is enabled")
#define DCIDESC_AGENT_SNMP_REQUESTS               _T("Number of SNMP requests sent")
#define DCIDESC_AGENT_SNMP_RESPONSES              _T("Number of SNMP responses received")
#define DCIDESC_AGENT_SNMP_SERVERREQUESTS         _T("Number of SNMP proxy requests received from server")
#define DCIDESC_AGENT_SOURCEPACKAGESUPPORT        _T("Check if source packages are supported")
#define DCIDESC_AGENT_SUPPORTEDCIPHERS            _T("List of ciphers supported by agent")
#define DCIDESC_AGENT_SYSLOGPROXY_ISENABLED       _T("Check if syslog proxy is enabled")
#define DCIDESC_AGENT_SYSLOGPROXY_RECEIVEDMSGS    _T("Number of syslog messages received by agent")
#define DCIDESC_AGENT_SYSLOGPROXY_QUEUESIZE       _T("Agent syslog proxy queue size")
#define DCIDESC_AGENT_THREADPOOL_ACTIVEREQUESTS   _T("Agent thread pool {instance}: active requests")
#define DCIDESC_AGENT_THREADPOOL_CURRSIZE         _T("Agent thread pool {instance}: current size")
#define DCIDESC_AGENT_THREADPOOL_LOAD             _T("Agent thread pool {instance}: current load")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG          _T("Agent thread pool {instance}: load average (1 minute)")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG_5        _T("Agent thread pool {instance}: load average (5 minutes)")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG_15       _T("Agent thread pool {instance}: load average (15 minutes)")
#define DCIDESC_AGENT_THREADPOOL_MAXSIZE          _T("Agent thread pool {instance}: max size")
#define DCIDESC_AGENT_THREADPOOL_MINSIZE          _T("Agent thread pool {instance}: min size")
#define DCIDESC_AGENT_THREADPOOL_USAGE            _T("Agent thread pool {instance}: usage")
#define DCIDESC_AGENT_TIMEDOUTREQUESTS            _T("Number of timed out requests to agent")
#define DCIDESC_AGENT_UNSUPPORTEDREQUESTS         _T("Number of requests for unsupported parameters")
#define DCIDESC_AGENT_UPTIME                      _T("Agent's uptime")
#define DCIDESC_AGENT_VERSION                     _T("Agent's version")
#define DCIDESC_FILE_COUNT                        _T("Number of files {instance}")
#define DCIDESC_FILE_FOLDERCOUNT                  _T("Number of folders {instance}")
#define DCIDESC_FILE_HASH_CRC32                   _T("CRC32 checksum of {instance}")
#define DCIDESC_FILE_HASH_MD5                     _T("MD5 hash of {instance}")
#define DCIDESC_FILE_HASH_SHA1                    _T("SHA1 hash of {instance}")
#define DCIDESC_FILE_SIZE                         _T("Size of file {instance}")
#define DCIDESC_FILE_TIME_ACCESS                  _T("Time of last access to file {instance}")
#define DCIDESC_FILE_TIME_CHANGE                  _T("Time of last status change of file {instance}")
#define DCIDESC_FILE_TIME_MODIFY                  _T("Time of last modification of file {instance}")
#define DCIDESC_SYSTEM_CURRENTTIME                _T("Current system time")
#define DCIDESC_SYSTEM_PLATFORMNAME               _T("Platform name")
#define DCIDESC_PROCESS_COUNT                     _T("Number of {instance} processes")
#define DCIDESC_PROCESS_COUNTEX                   _T("Number of {instance} processes (extended)")
#define DCIDESC_PROCESS_CPUTIME                   _T("Total execution time for process {instance}")
#define DCIDESC_PROCESS_GDIOBJ                    _T("GDI objects used by process {instance}")
#define DCIDESC_PROCESS_HANDLES                   _T("Number of handles in process {instance}")
#define DCIDESC_PROCESS_IO_OTHERB                 _T("")
#define DCIDESC_PROCESS_IO_OTHEROP                _T("")
#define DCIDESC_PROCESS_IO_READB                  _T("")
#define DCIDESC_PROCESS_IO_READOP                 _T("")
#define DCIDESC_PROCESS_IO_WRITEB                 _T("")
#define DCIDESC_PROCESS_IO_WRITEOP                _T("")
#define DCIDESC_PROCESS_KERNELTIME                _T("Total execution time in kernel mode for process {instance}")
#define DCIDESC_PROCESS_PAGEFAULTS                _T("Page faults for process {instance}")
#define DCIDESC_PROCESS_SYSCALLS                  _T("Number of system calls made by process {instance}")
#define DCIDESC_PROCESS_THREADS                   _T("Number of threads in process {instance}")
#define DCIDESC_PROCESS_USEROBJ                   _T("USER objects used by process {instance}")
#define DCIDESC_PROCESS_USERTIME                  _T("Total execution time in user mode for process {instance}")
#define DCIDESC_PROCESS_VMSIZE                    _T("Virtual memory used by process {instance}")
#define DCIDESC_PROCESS_WKSET                     _T("Physical memory used by process {instance}")
#define DCIDESC_PROCESS_ZOMBIE_COUNT              _T("Number of {instance} zombie processes")
#define DCIDESC_SYSTEM_APPADDRESSSPACE            _T("Address space available to applications (MB)")
#define DCIDESC_SYSTEM_CONNECTEDUSERS             _T("Number of logged in users")
#define DCIDESC_SYSTEM_HANDLECOUNT                _T("Total number of handles")
#define DCIDESC_SYSTEM_PROCESSCOUNT               _T("Total number of processes")
#define DCIDESC_SYSTEM_SERVICESTATE               _T("State of {instance} service")
#define DCIDESC_SYSTEM_THREADCOUNT                _T("Total number of threads")
#define DCIDESC_PDH_COUNTERVALUE                  _T("Value of PDH counter {instance}")
#define DCIDESC_PDH_VERSION                       _T("Version of PDH.DLL")
#define DCIDESC_SYSTEM_UPTIME                     _T("System uptime")

#define DCIDESC_SYSTEM_CPU_LOADAVG                _T("Average CPU load for last minute")
#define DCIDESC_SYSTEM_CPU_LOADAVG5               _T("Average CPU load for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_LOADAVG15              _T("Average CPU load for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_INTERRUPTS             _T("CPU interrupt count")
#define DCIDESC_SYSTEM_CPU_INTERRUPTS_EX          _T("CPU {instance} interrupt count")
#define DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES       _T("CPU context switch count")

#define DCIDESC_SYSTEM_CPU_USAGE_EX               _T("Average CPU {instance} utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_EX              _T("Average CPU {instance} utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_EX             _T("Average CPU {instance} utilization for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE                  _T("Average CPU utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5                 _T("Average CPU utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15                _T("Average CPU utilization for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR              _T("Current CPU utilization")
#define DCIDESC_SYSTEM_CPU_USAGECURR_EX           _T("Current CPU {instance} utilization")

#define DCIDESC_SYSTEM_CPU_USAGE_USER_EX          _T("Average CPU {instance} utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER_EX         _T("Average CPU {instance} utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER_EX        _T("Average CPU {instance} utilization (user) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_USER             _T("Average CPU utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER            _T("Average CPU utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER           _T("Average CPU utilization (user) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_USER         _T("Current CPU utilization (user)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_USER_EX      _T("Current CPU {instance} utilization (user)")

#define DCIDESC_SYSTEM_CPU_USAGE_NICE_EX          _T("Average CPU {instance} utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX         _T("Average CPU {instance} utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX        _T("Average CPU {instance} utilization (nice) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_NICE             _T("Average CPU utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE            _T("Average CPU utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE           _T("Average CPU utilization (nice) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_NICE         _T("Current CPU utilization (nice)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_NICE_EX      _T("Current CPU {instance} utilization (nice)")

#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX        _T("Average CPU {instance} utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX       _T("Average CPU {instance} utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX      _T("Average CPU {instance} utilization (system) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM           _T("Average CPU utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM          _T("Average CPU utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM         _T("Average CPU utilization (system) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM       _T("Current CPU utilization (system)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM_EX    _T("Current CPU {instance} utilization (system)")

#define DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX          _T("Average CPU {instance} utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX         _T("Average CPU {instance} utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX        _T("Average CPU {instance} utilization (idle) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IDLE             _T("Average CPU utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE            _T("Average CPU utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE           _T("Average CPU utilization (idle) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IDLE         _T("Current CPU utilization (idle)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IDLE_EX      _T("Current CPU {instance} utilization (idle)")

#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX        _T("Average CPU {instance} utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX       _T("Average CPU {instance} utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX      _T("Average CPU {instance} utilization (iowait) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT           _T("Average CPU utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT          _T("Average CPU utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT         _T("Average CPU utilization (iowait) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IOWAIT       _T("Current CPU utilization (iowait)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IOWAIT_EX    _T("Current CPU {instance} utilization (iowait)")

#define DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX           _T("Average CPU {instance} utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX          _T("Average CPU {instance} utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX         _T("Average CPU {instance} utilization (irq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IRQ              _T("Average CPU utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ             _T("Average CPU utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ            _T("Average CPU utilization (irq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IRQ          _T("Current CPU utilization (irq)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IRQ_EX       _T("Current CPU {instance} utilization (irq)")

#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX       _T("Average CPU {instance} utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX      _T("Average CPU {instance} utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX     _T("Average CPU {instance} utilization (softirq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ          _T("Average CPU utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ         _T("Average CPU utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ        _T("Average CPU utilization (softirq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SOFTIRQ      _T("Current CPU utilization (softirq)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SOFTIRQ_EX   _T("Current CPU {instance} utilization (softirq)")

#define DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX         _T("Average CPU {instance} utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX        _T("Average CPU {instance} utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX       _T("Average CPU {instance} utilization (steal) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_STEAL            _T("Average CPU utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL           _T("Average CPU utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL          _T("Average CPU utilization (steal) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_STEAL        _T("Current CPU utilization (steal)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_STEAL_EX     _T("Current CPU {instance} utilization (steal)")

#define DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX         _T("Average CPU {instance} utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX        _T("Average CPU {instance} utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX       _T("Average CPU {instance} utilization (guest) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_GUEST            _T("Average CPU utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST           _T("Average CPU utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST          _T("Average CPU utilization (guest) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_GUEST        _T("Current CPU utilization (guest)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_GUEST_EX     _T("Current CPU {instance} utilization (guest)")

#define DCIDESC_SYSTEM_IO_DISKQUEUE               _T("Average disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MIN           _T("Minimum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MAX           _T("Maximum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX            _T("Average disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MIN        _T("Minimum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MAX        _T("Maximum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME                _T("Percent of CPU time spent on I/O for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME_EX             _T("Percent of CPU time spent on I/O on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WAITTIME                _T("Average I/O request wait time")
#define DCIDESC_SYSTEM_IO_WAITTIME_EX             _T("Average I/O request wait time for device {instance}")
#define DCIDESC_SYSTEM_IO_READS                   _T("Average number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MIN               _T("Minimum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MAX               _T("Maximum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX                _T("Average number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MIN            _T("Minimum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MAX            _T("Maximum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES                  _T("Average number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MIN              _T("Minimum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MAX              _T("Maximum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX               _T("Average number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MIN           _T("Minimum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MAX           _T("Maximum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS                   _T("Average number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MIN               _T("Minimum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MAX               _T("Maximum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX                _T("Average number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MIN            _T("Minimum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MAX            _T("Maximum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS               _T("Average number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MIN           _T("Minimum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MAX           _T("Maximum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX            _T("Average number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MIN        _T("Minimum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MAX        _T("Maximum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES              _T("Average number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MIN          _T("Minimum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MAX          _T("Maximum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX           _T("Average number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MIN       _T("Minimum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MAX       _T("Maximum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_OPENFILES               _T("Number of open files")


#define DCIDESC_DEPRECATED                        _T("<deprecated>")


#define DCTDESC_AGENT_SESSION_AGENTS              _T("Registered session agents")
#define DCTDESC_AGENT_SUBAGENTS                   _T("Loaded subagents")
#define DCTDESC_FILESYSTEM_VOLUMES                _T("File system volumes")
#define DCTDESC_LVM_VOLUME_GROUPS                 _T("LVM volume groups")
#define DCTDESC_LVM_LOGICAL_VOLUMES               _T("Logical volumes in volume group {instance}")
#define DCTDESC_LVM_PHYSICAL_VOLUMES              _T("Physical volumes in volume group {instance}")
#define DCTDESC_SYSTEM_INSTALLED_PRODUCTS         _T("Installed products")
#define DCTDESC_SYSTEM_OPEN_FILES                 _T("Open files")
#define DCTDESC_SYSTEM_PROCESSES                  _T("Processes")

/**
 * Class that stores information about file that will be received
 */
class LIBNXAGENT_EXPORTABLE DownloadFileInfo
{
protected:
   TCHAR *m_fileName;
   time_t m_lastModTime;
   int m_file;
   StreamCompressor *m_compressor;  // stream compressor for file transfer

public:
   DownloadFileInfo(const TCHAR *name, time_t lastModTime = 0);
   virtual ~DownloadFileInfo();

   virtual bool open();
   virtual bool write(const BYTE *data, size_t dataSize, bool compressedStream);
   virtual void close(bool success);
};

/**
 * API for CommSession
 */
class AbstractCommSession : public RefCountObject
{
public:
   virtual UINT32 getId() = 0;

   virtual UINT64 getServerId() = 0;
   virtual const InetAddress& getServerAddress() = 0;

   virtual bool isMasterServer() = 0;
   virtual bool isControlServer() = 0;
   virtual bool canAcceptData() = 0;
   virtual bool canAcceptTraps() = 0;
   virtual bool canAcceptFileUpdates() = 0;
   virtual bool isBulkReconciliationSupported() = 0;
   virtual bool isIPv6Aware() = 0;

   virtual bool sendMessage(NXCPMessage *msg) = 0;
   virtual void postMessage(NXCPMessage *msg) = 0;
   virtual bool sendRawMessage(NXCP_MESSAGE *msg) = 0;
   virtual void postRawMessage(NXCP_MESSAGE *msg) = 0;
	virtual bool sendFile(UINT32 requestId, const TCHAR *file, long offset, bool allowCompression) = 0;
   virtual UINT32 doRequest(NXCPMessage *msg, UINT32 timeout) = 0;
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, UINT32 timeout) = 0;
   virtual UINT32 generateRequestId() = 0;
   virtual UINT32 openFile(TCHAR* nameOfFile, UINT32 requestId, time_t fileModTime = 0) = 0;
   virtual void debugPrintf(int level, const TCHAR *format, ...) = 0;
};

/**
 * Execute server command
 */
class LIBNXAGENT_EXPORTABLE CommandExec
{
private:
   UINT32 m_streamId;
   THREAD m_outputThread;
#ifdef _WIN32
   HANDLE m_phandle;
   HANDLE m_pipe;
#else
   pid_t m_pid;
   int m_pipe[2];
#endif

protected:
   TCHAR *m_cmd;
   bool m_sendOutput;

#ifdef _WIN32
   HANDLE getOutputPipe() { return m_pipe; }
#else
   int getOutputPipe() { return m_pipe[0]; }
#endif

   static THREAD_RESULT THREAD_CALL readOutput(void *pArg);

   virtual void onOutput(const char *text);
   virtual void endOfOutput();

public:
   CommandExec(const TCHAR *cmd);
   CommandExec();
   virtual ~CommandExec();

   UINT32 getStreamId() const { return m_streamId; }
   const TCHAR *getCommand() const { return m_cmd; }

   bool execute();
   void stop();
};

/**
 * Subagent's parameter information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
   const TCHAR *arg;
   int dataType;		// Use DT_DEPRECATED to indicate deprecated parameter
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_PARAM;

/**
 * Subagent's push parameter information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   int dataType;
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_PUSHPARAM;

/**
 * Subagent's list information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
   const TCHAR *arg;
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_LIST;

/**
 * Subagent's table column information
 */
typedef struct
{
   TCHAR name[MAX_COLUMN_NAME];
   TCHAR displayName[MAX_COLUMN_NAME];
   int dataType;
   bool isInstance;
} NETXMS_SUBAGENT_TABLE_COLUMN;

/**
 * Subagent's table information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *);
   const TCHAR *arg;
   TCHAR instanceColumns[MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS];
   TCHAR description[MAX_DB_STRING];
   int numColumns;
   NETXMS_SUBAGENT_TABLE_COLUMN *columns;
} NETXMS_SUBAGENT_TABLE;

/**
 * Subagent's action information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, StringList *, const TCHAR *, AbstractCommSession *);
   const TCHAR *arg;
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_ACTION;

#define NETXMS_SUBAGENT_INFO_MAGIC     ((UINT32)0x20161127)

class NXCPMessage;

/**
 * Subagent initialization structure
 */
typedef struct
{
   UINT32 magic;    // Magic number to check if subagent uses correct version of this structure
   TCHAR name[MAX_SUBAGENT_NAME];
   TCHAR version[32];
	BOOL (* init)(Config *);   // Called to initialize subagent. Can be NULL.
   void (* shutdown)();       // Called at subagent unload. Can be NULL.
   BOOL (* commandHandler)(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session);
   UINT32 numParameters;
   NETXMS_SUBAGENT_PARAM *parameters;
   UINT32 numLists;
   NETXMS_SUBAGENT_LIST *lists;
   UINT32 numTables;
   NETXMS_SUBAGENT_TABLE *tables;
   UINT32 numActions;
   NETXMS_SUBAGENT_ACTION *actions;
	UINT32 numPushParameters;
	NETXMS_SUBAGENT_PUSHPARAM *pushParameters;
} NETXMS_SUBAGENT_INFO;

/**
 * Inline functions for returning parameters
 */
inline void ret_string(TCHAR *rbuf, const TCHAR *value)
{
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
}

inline void ret_wstring(TCHAR *rbuf, const WCHAR *value)
{
#ifdef UNICODE
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
#else
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, value, -1, rbuf, MAX_RESULT_LENGTH, NULL, NULL);
	rbuf[MAX_RESULT_LENGTH - 1] = 0;
#endif
}

inline void ret_mbstring(TCHAR *rbuf, const char *value)
{
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, rbuf, MAX_RESULT_LENGTH);
	rbuf[MAX_RESULT_LENGTH - 1] = 0;
#else
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
#endif
}

inline void ret_int(TCHAR *rbuf, LONG value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%ld"), (long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%ld"), (long)value);
#endif
}

inline void ret_uint(TCHAR *rbuf, UINT32 value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%lu"), (unsigned long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%lu"), (unsigned long)value);
#endif
}

inline void ret_double(TCHAR *rbuf, double value, int digits = 6)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%1.*f"), digits, value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%1.*f"), digits, value);
#endif
}

inline void ret_int64(TCHAR *rbuf, INT64 value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64d"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, INT64_FMT, value);
#endif   /* _WIN32 */
}

inline void ret_uint64(TCHAR *rbuf, QWORD value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64u"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, UINT64_FMT, value);
#endif   /* _WIN32 */
}

/**
 * API for subagents
 */
bool LIBNXAGENT_EXPORTABLE AgentGetParameterArgA(const TCHAR *param, int index, char *arg, int maxSize, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetParameterArgW(const TCHAR *param, int index, WCHAR *arg, int maxSize, bool inBrackets = true);
#ifdef UNICODE
#define AgentGetParameterArg AgentGetParameterArgW
#else
#define AgentGetParameterArg AgentGetParameterArgA
#endif

void LIBNXAGENT_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void LIBNXAGENT_EXPORTABLE AgentWriteLog2(int logLevel, const TCHAR *format, va_list args);
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog(int level, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog2(int level, const TCHAR *format, va_list args);

void LIBNXAGENT_EXPORTABLE AgentSendTrap(UINT32 dwEvent, const TCHAR *eventName, const char *pszFormat, ...);
void LIBNXAGENT_EXPORTABLE AgentSendTrap2(UINT32 dwEvent, const TCHAR *eventName, int nCount, TCHAR **ppszArgList);

bool LIBNXAGENT_EXPORTABLE AgentEnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data);
AbstractCommSession LIBNXAGENT_EXPORTABLE *AgentFindServerSession(UINT64 serverId);

bool LIBNXAGENT_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset, bool allowCompression);

bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, UINT32 value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, INT64 value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, QWORD value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value);

CONDITION LIBNXAGENT_EXPORTABLE AgentGetShutdownCondition();
bool LIBNXAGENT_EXPORTABLE AgentSleepAndCheckForShutdown(UINT32 sleepTime);
const TCHAR LIBNXAGENT_EXPORTABLE *AgentGetDataDirectory();

DB_HANDLE LIBNXAGENT_EXPORTABLE AgentGetLocalDatabaseHandle();

TCHAR LIBNXAGENT_EXPORTABLE *ReadRegistryAsString(const TCHAR *attr, TCHAR *buffer = NULL, int bufSize = 0, const TCHAR *defaultValue = NULL);
INT32 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt32(const TCHAR *attr, INT32 defaultValue);
INT64 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt64(const TCHAR *attr, INT64 defaultValue);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT32 value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT64 value);
bool LIBNXAGENT_EXPORTABLE DeleteRegistryEntry(const TCHAR *attr);

#endif   /* _nms_agent_h_ */
