/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: datacoll.cpp
**
**/

#include "nxcore.h"

/**
 * Interval between DCI polling
 */
#define ITEM_POLLING_INTERVAL    1

/**
 * Externals
 */
extern Queue g_syslogProcessingQueue;
extern Queue g_syslogWriteQueue;

/**
 * Global data
 */
double g_dAvgPollerQueueSize = 0;
double g_dAvgDBWriterQueueSize = 0;
double g_dAvgIDataWriterQueueSize = 0;
double g_dAvgRawDataWriterQueueSize = 0;
double g_dAvgDBAndIDataWriterQueueSize = 0;
double g_dAvgSyslogProcessingQueueSize = 0;
double g_dAvgSyslogWriterQueueSize = 0;
UINT32 g_dwAvgDCIQueuingTime = 0;
Queue g_dataCollectionQueue(4096, 256);
Queue g_dciCacheLoaderQueue;

/**
 * Collect data for DCI
 */
static void *GetItemData(DataCollectionTarget *dcTarget, DCItem *pItem, TCHAR *pBuffer, UINT32 *error)
{
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (pItem->isAggregateOnCluster())
      {
         *error = ((Cluster *)dcTarget)->collectAggregatedData(pItem, pBuffer);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(pItem->getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameters (like status)
            *error = dcTarget->getInternalItem(pItem->getName(), MAX_LINE_SIZE, pBuffer);
            break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
				   *error = ((Node *)dcTarget)->getItemFromSNMP(pItem->getSnmpPort(), pItem->getName(), MAX_LINE_SIZE,
					   pBuffer, pItem->isInterpretSnmpRawValue() ? (int)pItem->getSnmpRawValueType() : SNMP_RAWTYPE_NONE);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_CHECKPOINT_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
	            *error = ((Node *)dcTarget)->getItemFromCheckPointSNMP(pItem->getName(), MAX_LINE_SIZE, pBuffer);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
	            *error = ((Node *)dcTarget)->getItemFromAgent(pItem->getName(), MAX_LINE_SIZE, pBuffer);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_WINPERF:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
			   {
				   TCHAR name[MAX_PARAM_NAME];
				   _sntprintf(name, MAX_PARAM_NAME, _T("PDH.CounterValue(\"%s\",%d)"), (const TCHAR *)EscapeStringForAgent(pItem->getName()), pItem->getSampleCount());
	            *error = ((Node *)dcTarget)->getItemFromAgent(name, MAX_LINE_SIZE, pBuffer);
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_SSH:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               UINT32 proxyId = ((Node *)dcTarget)->getSshProxy();
               if (proxyId == 0)
               {
                  if (IsZoningEnabled())
                  {
                     Zone *zone = (Zone *)FindZoneByGUID(((Node *)dcTarget)->getZoneId());
                     if ((zone != NULL) && (zone->getProxyNodeId() != 0))
                        proxyId = zone->getProxyNodeId();
                     else
                        proxyId = g_dwMgmtNode;
                  }
                  else
                  {
                     proxyId = g_dwMgmtNode;
                  }
               }
               Node *proxy = (Node *)FindObjectById(proxyId, OBJECT_NODE);
               if (proxy != NULL)
               {
                  TCHAR name[MAX_PARAM_NAME], ipAddr[64];
                  _sntprintf(name, MAX_PARAM_NAME, _T("SSH.Command(%s,\"%s\",\"%s\",\"%s\")"),
                             ((Node *)dcTarget)->getIpAddress().toString(ipAddr),
                             (const TCHAR *)EscapeStringForAgent(((Node *)dcTarget)->getSshLogin()),
                             (const TCHAR *)EscapeStringForAgent(((Node *)dcTarget)->getSshPassword()),
                             (const TCHAR *)EscapeStringForAgent(pItem->getName()));
                  *error = proxy->getItemFromAgent(name, MAX_LINE_SIZE, pBuffer);
               }
               else
               {
                  *error = DCE_COMM_ERROR;
               }
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SMCLP:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
	            *error = ((Node *)dcTarget)->getItemFromSMCLP(pItem->getName(), MAX_LINE_SIZE, pBuffer);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SCRIPT:
            *error = dcTarget->getScriptItem(pItem->getName(), MAX_LINE_SIZE, pBuffer);
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
      }
   }
	return pBuffer;
}

/**
 * Collect data for table
 */
static void *GetTableData(DataCollectionTarget *dcTarget, DCTable *table, UINT32 *error)
{
	Table *result = NULL;
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (table->isAggregateOnCluster())
      {
         *error = ((Cluster *)dcTarget)->collectAggregatedData(table, &result);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(table->getDataSource())
      {
		   case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
				   *error = ((Node *)dcTarget)->getTableFromAgent(table->getName(), &result);
               if ((*error == DCE_SUCCESS) && (result != NULL))
                  table->updateResultColumns(result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
			   break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               *error = ((Node *)dcTarget)->getTableFromSNMP(table->getSnmpPort(), table->getName(), table->getColumns(), &result);
               if ((*error == DCE_SUCCESS) && (result != NULL))
                  table->updateResultColumns(result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
	   }
   }
	return result;
}

/**
 * Data collector
 */
static THREAD_RESULT THREAD_CALL DataCollector(void *pArg)
{
   UINT32 dwError;

   TCHAR *pBuffer = (TCHAR *)malloc(MAX_LINE_SIZE * sizeof(TCHAR));
   while(!IsShutdownInProgress())
   {
      DCObject *pItem = (DCObject *)g_dataCollectionQueue.getOrBlock();
		DataCollectionTarget *target = (DataCollectionTarget *)pItem->getOwner();

		if (pItem->isScheduledForDeletion())
		{
	      nxlog_debug(7, _T("DataCollector(): about to destroy DC object %d \"%s\" owner=%d"),
			            pItem->getId(), pItem->getName(), (target != NULL) ? (int)target->getId() : -1);
			pItem->deleteFromDatabase();
			delete pItem;
			continue;
		}

		if (target == NULL)
		{
         nxlog_debug(3, _T("DataCollector: attempt to collect information for non-existing node (DCI=%d \"%s\")"),
                     pItem->getId(), pItem->getName());

         // Update item's last poll time and clear busy flag so item can be polled again
         pItem->setLastPollTime(time(NULL));
         pItem->clearBusyFlag();
		   continue;
		}

      DbgPrintf(8, _T("DataCollector(): processing DC object %d \"%s\" owner=%d sourceNode=%d"),
		          pItem->getId(), pItem->getName(), (target != NULL) ? (int)target->getId() : -1, pItem->getSourceNode());
      UINT32 sourceNodeId = target->getEffectiveSourceNode(pItem);
		if (sourceNodeId != 0)
		{
			Node *sourceNode = (Node *)FindObjectById(sourceNodeId, OBJECT_NODE);
			if (sourceNode != NULL)
			{
				if (((target->getObjectClass() == OBJECT_CHASSIS) && (((Chassis *)target)->getControllerId() == sourceNodeId)) ||
				    sourceNode->isTrustedNode(target->getId()))
				{
					target = sourceNode;
					target->incRefCount();
				}
				else
				{
               // Change item's status to _T("not supported")
               pItem->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
               target->decRefCount();
               target = NULL;
				}
			}
			else
			{
            target->decRefCount();
            target = NULL;
			}
		}

      time_t currTime = time(NULL);
      if (target != NULL)
      {
         if (!IsShutdownInProgress())
         {
            void *data;
            switch(pItem->getType())
            {
               case DCO_TYPE_ITEM:
                  data = GetItemData(target, (DCItem *)pItem, pBuffer, &dwError);
                  break;
               case DCO_TYPE_TABLE:
                  data = GetTableData(target, (DCTable *)pItem, &dwError);
                  break;
               default:
                  data = NULL;
                  dwError = DCE_NOT_SUPPORTED;
                  break;
            }

            // Transform and store received value into database or handle error
            switch(dwError)
            {
               case DCE_SUCCESS:
                  if (pItem->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                     pItem->setStatus(ITEM_STATUS_ACTIVE, true);
                  if (!((DataCollectionTarget *)pItem->getOwner())->processNewDCValue(pItem, currTime, data))
                  {
                     // value processing failed, convert to data collection error
                     pItem->processNewError(false);
                  }
                  break;
               case DCE_COLLECTION_ERROR:
                  if (pItem->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                     pItem->setStatus(ITEM_STATUS_ACTIVE, true);
                  pItem->processNewError(false);
                  break;
               case DCE_NO_SUCH_INSTANCE:
                  if (pItem->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                     pItem->setStatus(ITEM_STATUS_ACTIVE, true);
                  pItem->processNewError(true);
                  break;
               case DCE_COMM_ERROR:
                  pItem->processNewError(false);
                  break;
               case DCE_NOT_SUPPORTED:
                  // Change item's status
                  pItem->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
                  break;
            }
         }

         // Send session notification when force poll is performed
         if (pItem->getPollingSession() != NULL)
         {
            ClientSession *session = pItem->processForcePoll();
            session->notify(NX_NOTIFY_FORCE_DCI_POLL, pItem->getOwnerId());
            session->decRefCount();
         }

         // Decrement node's usage counter
         target->decRefCount();
			if ((pItem->getSourceNode() != 0) && (pItem->getOwner() != NULL))
			{
				pItem->getOwner()->decRefCount();
			}
      }
      else     /* target == NULL */
      {
			Template *n = pItem->getOwner();
         nxlog_debug(5, _T("DataCollector: attempt to collect information for non-existing or inaccessible node (DCI=%d \"%s\" target=%d sourceNode=%d)"),
			            pItem->getId(), pItem->getName(), (n != NULL) ? (int)n->getId() : -1, sourceNodeId);
      }

		// Update item's last poll time and clear busy flag so item can be polled again
      pItem->setLastPollTime(currTime);
      pItem->clearBusyFlag();
   }

   free(pBuffer);
   DbgPrintf(1, _T("Data collector thread terminated"));
   return THREAD_OK;
}

/**
 * Callback for queueing DCIs
 */
static void QueueItems(NetObj *object, void *data)
{
   if (IsShutdownInProgress())
      return;

   WatchdogNotify(*((UINT32 *)data));
	nxlog_debug(8, _T("ItemPoller: calling DataCollectionTarget::queueItemsForPolling for object %s [%d]"),
				   object->getName(), object->getId());
	((DataCollectionTarget *)object)->queueItemsForPolling(&g_dataCollectionQueue);
}

/**
 * Item poller thread: check nodes' items and put into the
 * data collector queue when data polling required
 */
static THREAD_RESULT THREAD_CALL ItemPoller(void *pArg)
{
   UINT32 dwSum, currPos = 0;
   UINT32 dwTimingHistory[60 / ITEM_POLLING_INTERVAL];
   INT64 qwStart;

   UINT32 watchdogId = WatchdogAddThread(_T("Item Poller"), 10);
   memset(dwTimingHistory, 0, sizeof(UINT32) * (60 / ITEM_POLLING_INTERVAL));

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(ITEM_POLLING_INTERVAL))
         break;      // Shutdown has arrived
      WatchdogNotify(watchdogId);
		DbgPrintf(8, _T("ItemPoller: wakeup"));

      qwStart = GetCurrentTimeMs();
		g_idxNodeById.forEach(QueueItems, &watchdogId);
		g_idxClusterById.forEach(QueueItems, &watchdogId);
		g_idxMobileDeviceById.forEach(QueueItems, &watchdogId);
      g_idxChassisById.forEach(QueueItems, &watchdogId);

      // Save last poll time
      dwTimingHistory[currPos] = (UINT32)(GetCurrentTimeMs() - qwStart);
      currPos++;
      if (currPos == (60 / ITEM_POLLING_INTERVAL))
         currPos = 0;

      // Calculate new average for last minute
		dwSum = 0;
      for(int i = 0; i < (60 / ITEM_POLLING_INTERVAL); i++)
         dwSum += dwTimingHistory[i];
      g_dwAvgDCIQueuingTime = dwSum / (60 / ITEM_POLLING_INTERVAL);
   }
   DbgPrintf(1, _T("Item poller thread terminated"));
   return THREAD_OK;
}

/**
 * Statistics collection thread
 */
static THREAD_RESULT THREAD_CALL StatCollector(void *pArg)
{
   UINT32 i, currPos = 0;
   UINT32 pollerQS[12], dbWriterQS[12];
   UINT32 iDataWriterQS[12], rawDataWriterQS[12], dbAndIDataWriterQS[12];
   UINT32 syslogProcessingQS[12], syslogWriterQS[12];
   double sum1, sum2, sum3, sum4, sum5, sum8, sum9;

   memset(pollerQS, 0, sizeof(UINT32) * 12);
   memset(dbWriterQS, 0, sizeof(UINT32) * 12);
   memset(iDataWriterQS, 0, sizeof(UINT32) * 12);
   memset(rawDataWriterQS, 0, sizeof(UINT32) * 12);
   memset(dbAndIDataWriterQS, 0, sizeof(UINT32) * 12);
   memset(syslogProcessingQS, 0, sizeof(UINT32) * 12);
   memset(syslogWriterQS, 0, sizeof(UINT32) * 12);
   g_dAvgPollerQueueSize = 0;
   g_dAvgDBWriterQueueSize = 0;
   g_dAvgIDataWriterQueueSize = 0;
   g_dAvgRawDataWriterQueueSize = 0;
   g_dAvgDBAndIDataWriterQueueSize = 0;
   g_dAvgSyslogProcessingQueueSize = 0;
   g_dAvgSyslogWriterQueueSize = 0;
   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(5))
         break;      // Shutdown has arrived

      // Get current values
      pollerQS[currPos] = g_dataCollectionQueue.size();
      dbWriterQS[currPos] = g_dbWriterQueue->size();
      iDataWriterQS[currPos] = g_dciDataWriterQueue->size();
      rawDataWriterQS[currPos] = g_dciRawDataWriterQueue->size();
      dbAndIDataWriterQS[currPos] = g_dbWriterQueue->size() + g_dciDataWriterQueue->size() + g_dciRawDataWriterQueue->size();
      syslogProcessingQS[currPos] = g_syslogProcessingQueue.size();
      syslogWriterQS[currPos] = g_syslogWriteQueue.size();
      currPos++;
      if (currPos == 12)
         currPos = 0;

      // Calculate new averages
      for(i = 0, sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0, sum5 = 0, sum8 = 0, sum9 = 0; i < 12; i++)
      {
         sum1 += pollerQS[i];
         sum2 += dbWriterQS[i];
         sum3 += iDataWriterQS[i];
         sum4 += rawDataWriterQS[i];
         sum5 += dbAndIDataWriterQS[i];
         sum8 += syslogProcessingQS[i];
         sum9 += syslogWriterQS[i];
      }
      g_dAvgPollerQueueSize = sum1 / 12;
      g_dAvgDBWriterQueueSize = sum2 / 12;
      g_dAvgIDataWriterQueueSize = sum3 / 12;
      g_dAvgRawDataWriterQueueSize = sum4 / 12;
      g_dAvgDBAndIDataWriterQueueSize = sum5 / 12;
      g_dAvgSyslogProcessingQueueSize = sum8 / 12;
      g_dAvgSyslogWriterQueueSize = sum9 / 12;
   }
   return THREAD_OK;
}

/**
 * DCI cache loader
 */
THREAD_RESULT THREAD_CALL CacheLoader(void *arg)
{
   DbgPrintf(2, _T("DCI cache loader thread started"));
   while(true)
   {
      DCItem *dci = (DCItem *)g_dciCacheLoaderQueue.getOrBlock();
      if (dci == INVALID_POINTER_VALUE)
         break;

      DbgPrintf(6, _T("Loading cache for DCI %s [%d] on %s [%d]"),
                dci->getName(), dci->getId(), dci->getOwnerName(), dci->getOwnerId());
      dci->reloadCache();
      dci->getOwner()->decRefCount();
   }
   DbgPrintf(2, _T("DCI cache loader thread stopped"));
   return THREAD_OK;
}

/**
 * Initialize data collection subsystem
 */
BOOL InitDataCollector()
{
   int i, iNumCollectors;

   // Start data collection threads
   iNumCollectors = ConfigReadInt(_T("NumberOfDataCollectors"), 10);
   for(i = 0; i < iNumCollectors; i++)
      ThreadCreate(DataCollector, 0, NULL);

   ThreadCreate(ItemPoller, 0, NULL);
   ThreadCreate(StatCollector, 0, NULL);
   ThreadCreate(CacheLoader, 0, NULL);

   return TRUE;
}

/**
 * Update parameter list from node
 */
static void UpdateParamList(NetObj *object, void *data)
{
	ObjectArray<AgentParameterDefinition> *fullList = (ObjectArray<AgentParameterDefinition> *)data;

	ObjectArray<AgentParameterDefinition> *paramList;
	((Node *)object)->openParamList(&paramList);
	if ((paramList != NULL) && (paramList->size() > 0))
	{
		for(int i = 0; i < paramList->size(); i++)
		{
			int j;
			for(j = 0; j < fullList->size(); j++)
			{
				if (!_tcsicmp(paramList->get(i)->getName(), fullList->get(j)->getName()))
					break;
			}

			if (j == fullList->size())
			{
            fullList->add(new AgentParameterDefinition(paramList->get(i)));
			}
		}
	}
	((Node *)object)->closeParamList();
}

/**
 * Update table list from node
 */
static void UpdateTableList(NetObj *object, void *data)
{
	ObjectArray<AgentTableDefinition> *fullList = (ObjectArray<AgentTableDefinition> *)data;

   ObjectArray<AgentTableDefinition> *tableList;
	((Node *)object)->openTableList(&tableList);
	if ((tableList != NULL) && (tableList->size() > 0))
	{
		for(int i = 0; i < tableList->size(); i++)
		{
			int j;
			for(j = 0; j < fullList->size(); j++)
			{
				if (!_tcsicmp(tableList->get(i)->getName(), fullList->get(j)->getName()))
					break;
			}

			if (j == fullList->size())
			{
            fullList->add(new AgentTableDefinition(tableList->get(i)));
			}
		}
	}
	((Node *)object)->closeTableList();
}

/**
 * Write full (from all nodes) agent parameters list to NXCP message
 */
void WriteFullParamListToMessage(NXCPMessage *pMsg, WORD flags)
{
   // Gather full parameter list
	if (flags & 0x01)
	{
		ObjectArray<AgentParameterDefinition> fullList(64, 64, true);
		g_idxNodeById.forEach(UpdateParamList, &fullList);

		// Put list into the message
		pMsg->setField(VID_NUM_PARAMETERS, (UINT32)fullList.size());
      UINT32 varId = VID_PARAM_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}

   // Gather full table list
	if (flags & 0x02)
	{
		ObjectArray<AgentTableDefinition> fullList(64, 64, true);
		g_idxNodeById.forEach(UpdateTableList, &fullList);

		// Put list into the message
		pMsg->setField(VID_NUM_TABLES, (UINT32)fullList.size());
      UINT32 varId = VID_TABLE_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}
}

/**
 * Get type of data collection object
 */
int GetDCObjectType(UINT32 nodeId, UINT32 dciId)
{
   Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
   if (node != NULL)
   {
      DCObject *dco = node->getDCObjectById(dciId);
      if (dco != NULL)
      {
         return dco->getType();
      }
   }
   return DCO_TYPE_ITEM;   // default
}
