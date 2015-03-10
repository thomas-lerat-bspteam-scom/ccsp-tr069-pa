/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/**********************************************************************

    module:	ccsp_cwmp_proco_mpaif.c

        For CCSP CWMP protocol implemenetation

    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc. 2005 ~ 2011
        All Rights Reserved.

    ---------------------------------------------------------------

    description:

        This module implements the advanced interface functions
        of the CCSP CWMP Database Object.

        *   CcspCwmppoMpaLockWriteAccess
        *   CcspCwmppoMpaUnlockWriteAccess
        *   CcspCwmppoMpaSetParameterValues
        *   CcspCwmppoMpaSetParameterValuesWithWriteID
        *   CcspCwmppoMpaGetParameterValues
        *   CcspCwmppoMpaGetParameterNames
        *   CcspCwmppoMpaSetParameterAttributes
        *   CcspCwmppoMpaGetParameterAttributes 
        *   CcspCwmppoMpaAddObject
        *   CcspCwmppoMpaDeleteObject

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Kang Quan

    ---------------------------------------------------------------

    revision:

        06/15/11    initial revision.
        06/22/11    decouple TR-069 PA from Data Model Manager
                    and make it work with CCSP architecture.
        07/19/11    implement CCSP normalized actions

**********************************************************************/

#include "ccsp_cwmp_proco_global.h"

/**********************************************************************
                                  MACROS
**********************************************************************/

#define  CcspCwmppoMpaMapParamInstNumCwmpToDmInt(pParam)                        \
            /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/                           \
            {                                                                   \
                CCSP_STRING     pReturnStr  = NULL;                             \
                                                                                \
                CcspTr069PaTraceDebug(("%s - Param CWMP to DmInt\n", __FUNCTION__));\
                                                                                \
                pReturnStr =                                                    \
                    CcspTr069PA_MapInstNumCwmpToDmInt                           \
                        (                                                       \
                            pParam                                              \
                        );                                                      \
                                                                                \
                if ( pReturnStr )                                               \
                {                                                               \
                    /* we are responsible for releasing the original string */  \
                    CcspTr069PaFreeMemory(pParam);                              \
                    pParam = pReturnStr;                                        \
                }                                                               \
            }

#define  CcspCwmppoMpaMapInvalidParamInstNumDmIntToCwmp(pParamName)                 \
            /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/                               \
            {                                                                       \
                CCSP_STRING     pReturnStr  = NULL;                                 \
                                                                                    \
                CcspTr069PaTraceWarning(("%s - Invalid Param DmInt to CWMP\n", __FUNCTION__));\
                                                                                    \
                pReturnStr =                                                        \
                    CcspTr069PA_MapInstNumDmIntToCwmp(pParamName);                  \
                                                                                    \
                if ( pReturnStr )                                                   \
                {                                                                   \
                    /* we are responsible for releasing the original string */      \
                    CcspTr069PaFreeMemory(pParamName);                              \
                    pParamName = pReturnStr;                                        \
                }                                                                   \
            }

#define  CcspCwmppoMpaMapParamInstNumDmIntToCwmp(pParam)                        \
            {                                                                       \
                CCSP_STRING     pReturnStr  = NULL;                                 \
                                                                                    \
                CcspTr069PaTraceWarning(("%s - Param DmInt to CWMP\n", __FUNCTION__));\
                                                                                    \
                pReturnStr =                                                        \
                    CcspTr069PA_MapInstNumDmIntToCwmp(pParam);                      \
                                                                                    \
                if ( pReturnStr )                                                   \
                {                                                                   \
                    /* we are responsible for releasing the original string */      \
                    CcspTr069PaFreeMemory(pParam);                                  \
                    pParam = pReturnStr;                                            \
                }                                                                   \
            }

/**********************************************************************
                                  MACROS
**********************************************************************/

static
void
CcspCwmppoAddFunctionalComponents
    (
        char*                       pSubsystem,
        componentStruct_t**         ppComp,
        ULONG                       NumComp,
        char***                     pppSubsysArray,
        char***                     pppFcNameArray,
        char***                     pppDbusPathArray,
        PULONG                      pulFcArraySize
    )
{
    int                         k = -1;
    int                         j;
    char**                      ppSubsysArray       = *pppSubsysArray;
    char**                      ppFcNameArray       = *pppFcNameArray;
    char**                      ppDbusPathArray     = *pppDbusPathArray;
    ULONG                       ulFcArraySize       = *pulFcArraySize;

    if ( ulFcArraySize == 0 )
    {
        ppSubsysArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*NumComp);
        ppFcNameArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*NumComp);
        ppDbusPathArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*NumComp);
        if ( !ppSubsysArray || !ppFcNameArray || !ppDbusPathArray )
        {
            CcspTr069PaFreeMemory(ppSubsysArray);
            CcspTr069PaFreeMemory(ppFcNameArray);
            CcspTr069PaFreeMemory(ppDbusPathArray);
        }
        else
        {
            k = 0;
        }
    }
    else
    {
        char**                  ppSsArray = NULL;
        char**                  ppFnArray = NULL;
        char**                  ppDpArray = NULL;

        ppSsArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*(ulFcArraySize+NumComp));
        ppFnArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*(ulFcArraySize+NumComp));
        ppDpArray = (char**)CcspTr069PaAllocateMemory(sizeof(char*)*(ulFcArraySize+NumComp));

        if ( !ppSsArray || !ppFnArray || !ppDpArray )
        {
            if ( ppSsArray ) CcspTr069PaFreeMemory(ppSsArray);
            if ( ppFnArray ) CcspTr069PaFreeMemory(ppFnArray);
            if ( ppDpArray ) CcspTr069PaFreeMemory(ppDpArray);
            goto EXIT;
        }

        for ( j = 0; j < ulFcArraySize; j ++ )
        {
            ppSsArray[j] = ppSubsysArray[j];
            ppFnArray[j] = ppFcNameArray[j];
            ppDpArray[j] = ppDbusPathArray[j];
        }
        CcspTr069PaFreeMemory(ppSubsysArray);
        CcspTr069PaFreeMemory(ppFcNameArray);
        CcspTr069PaFreeMemory(ppDbusPathArray);
        ppSubsysArray = ppSsArray;
        ppFcNameArray = ppFnArray;
        ppDbusPathArray = ppDpArray;

        k = ulFcArraySize;
    }

    if ( k >= 0 )
    {
        for ( j = 0; j < NumComp; j ++ )
        {
            ppSubsysArray[k+j] = CcspTr069PaCloneString(pSubsystem);
            ppFcNameArray[k+j] = CcspTr069PaCloneString(ppComp[j]->componentName);
            ppDbusPathArray[k+j] = CcspTr069PaCloneString(ppComp[j]->dbusPath);
        }
        ulFcArraySize += NumComp;
    }

EXIT:
    *pppSubsysArray     = ppSubsysArray;
    *pppFcNameArray     = ppFcNameArray;
    *pppDbusPathArray   = ppDbusPathArray;
    *pulFcArraySize     = ulFcArraySize;
}


static
int
CcspCwmppoDiscoverFunctionalComponent
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pParamName,
        BOOL                        bNextLevel,
        char**                      Subsystems,
        int                         NumSubsystems,
        char***                     pppSubsysArray,
        char***                     pppFcNameArray,
        char***                     pppDbusPathArray,
        PULONG                      pulFcArraySize
    )
{
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController   = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr           = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    int                             n;
    componentStruct_t**             ppComp               = NULL;
    ULONG                           NumComp              = 0;
    ULONG                           ulFcArraySize        = 0;
    int                             nRet                 = 0;

    *pppSubsysArray     = NULL;
    *pppFcNameArray     = NULL;
    *pppDbusPathArray   = NULL;
    *pulFcArraySize     = 0;

    if ( !pCcspNsMgr )
    {
        CcspTr069PaTraceError(("Namespace manager object has not created!\n"));
        return  CCSP_FAILURE;
    }

    for ( n = 0; n < NumSubsystems; n ++ )
    {
        nRet =
            pCcspNsMgr->DiscoverNamespace
                (
                    (ANSC_HANDLE)pCcspNsMgr,
                    pParamName,
                    Subsystems[n],
                    bNextLevel,
                    (PVOID**)&ppComp,
                    &NumComp
                );

        if ( nRet == CCSP_SUCCESS && NumComp > 0 )
        {
            CcspCwmppoAddFunctionalComponents
                (
                    Subsystems[n],
                    ppComp,
                    NumComp,
                    pppSubsysArray,
                    pppFcNameArray,
                    pppDbusPathArray,
                    pulFcArraySize
                );

            free_componentStruct_t(pCcspCwmpCpeController->hMsgBusHandle, NumComp, ppComp);
            NumComp = 0;
            ppComp = NULL;
        }
    }

    return  nRet;
}


#define  CCSP_TR069PA_DISCOVER_FC(pParamName, bNextLevel)                                               \
    do {                                                                                                \
        if ( !pMyObject->bRemoteCRsSyncDone )                                                           \
        {                                                                                               \
            ULONG                   ulTimeNow = AnscGetTickInSeconds();                                 \
                                                                                                        \
            if ( ulTimeNow - pMyObject->LastRemoteCrSyncTime > CCSP_CWMPPO_REMOTE_CR_RESYNC_INTERVAL )   \
            {                                                                                           \
                ULONG               ulLastSyncTime = pMyObject->LastRemoteCrSyncTime;                   \
                CcspTr069PaTraceDebug(("PA sync again with remote sub-system(s).\n"));                  \
                pMyObject->SyncNamespacesWithCR((ANSC_HANDLE)pMyObject, TRUE);                          \
                CcspTr069PaTraceDebug                                                                   \
                    ((                                                                                  \
                        "PA re-sync, time now = %u, last time = %u.\n",                                 \
                        (unsigned int)ulTimeNow,                                                        \
                        (unsigned int)ulLastSyncTime                                                    \
                    ));                                                                                 \
            }                                                                                           \
        }                                                                                               \
                                                                                                        \
        nRet =                                                                                          \
            CcspCwmppoDiscoverFunctionalComponent                                                        \
                (                                                                                       \
                    hThisObject,                                                                        \
                    pParamName,                                                                         \
                    bNextLevel,                                                                         \
                    Subsystems,                                                                         \
                    NumSubsystems,                                                                      \
                    &ppSubsysArray,                                                                     \
                    &ppFcNameArray,                                                                     \
                    &ppDbusPathArray,                                                                   \
                    &ulFcArraySize                                                                      \
                );                                                                                      \
    } while (0)


#define  CcspTr069PaVisibleToCloudServer(bExcInv, pNsSubsystem, pParamName, bNsInvisibleToCloudServer)  \
    do {                                                                                                \
        bNsInvisibleToCloudServer = TRUE;                                                               \
        /* filter out namespace that is not supported by this PA */                                     \
        if ( pCcspCwmpCpeController->SubsysName &&                                                          \
             AnscEqualString(pNsSubsystem, pCcspCwmpCpeController->SubsysName, TRUE) )                      \
        {                                                                                               \
            /* assumption: local namespace is always supported */                                       \
            bNsInvisibleToCloudServer = FALSE;                                                          \
        }                                                                                               \
        else if ( CcspTr069PA_IsNamespaceSupported                                                      \
                    (                                                                                   \
                        pCcspCwmpCpeController->hTr069PaMapper,                                             \
                        pParamName,                                                                     \
                        pNsSubsystem,                                                                   \
                        !bExcInv                                                                        \
                    ) )                                                                                 \
        {                                                                                               \
            bNsInvisibleToCloudServer = FALSE;                                                          \
        }                                                                                               \
                                                                                                        \
        /* filter out namespace that is invisible to cloud server through this PA */                    \
        if ( !bNsInvisibleToCloudServer && bExcInv &&                                                   \
             CcspTr069PA_IsNamespaceInvisible                                                           \
                (                                                                                       \
                    pCcspCwmpCpeController->hTr069PaMapper,                                                 \
                    pParamName                                                                          \
                ) )                                                                                     \
        {                                                                                               \
            bNsInvisibleToCloudServer = TRUE;                                                           \
        }                                                                                               \
    } while (0)


/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        CcspCwmppoMpaLockWriteAccess
            (
                ANSC_HANDLE                 hThisObject,
            );

    description:

        This function is called to lock the write access on the
        parameter database. If it succeeds, no other entities can write
        into the database until UnlockWriteAccess() is called.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     TRUE or FALSE.

**********************************************************************/

BOOL
CcspCwmppoMpaLockWriteAccess
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus   = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject      = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    BOOL                            bWriteLocked   = FALSE;

    /* 
     * Nothing to do under current CCSP architecture
     */

    return  bWriteLocked;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaUnlockWriteAccess
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to release the write lock previously
        acquired by calling LockWriteAccess().

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaUnlockWriteAccess
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus   = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject      = (PCCSP_CWMP_PROCESSOR_OBJECT  )hThisObject;

    /* 
     * Nothing to do under current CCSP architecture
     */

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaSetParameterValues
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pParamValueArray,
                ULONG                       ulArraySize,
                int*                        piStatus,
                ANSC_HANDLE*                phSoapFault
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to configure the value of the
        specified parameter.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pParamValueArray
                Specifies the array of ParameterValue structures.

                ULONG                       ulArraySize
                Specifies the number of elements in the
                pParamValueArray.

                int*                        piStatus
                Returns the status of the SET operation: 0 = Parameter
                changes have been validated and applied; 1 = Parameter
                changes have been validated and committed, but not yet
                applied.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate if invisible namespaces would be excluded.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaSetParameterValues
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pParamValueArray,
        ULONG                       ulArraySize,
        int*                        piStatus,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_MPA_INTERFACE             pCcspCwmpMpaIf           = (PCCSP_CWMP_MPA_INTERFACE        )pMyObject->hCcspCwmpMpaIf;

    return 
        pCcspCwmpMpaIf->SetParameterValuesWithWriteID
            (
                pCcspCwmpMpaIf->hOwnerContext,
                pParamValueArray,
                ulArraySize,
                CCSP_TR069PA_WRITE_ID,
                piStatus,
                phSoapFault,
                bExcludeInvNs
            );
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaSetParameterValuesWithWriteID
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pParamValueArray,
                ULONG                       ulArraySize,
                ULONG                       ulWriteID,
                int*                        piStatus,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to configure the value of the
        specified parameter.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pParamValueArray
                Specifies the array of ParameterValue structures.

                ULONG                       ulArraySize
                Specifies the number of elements in the
                pParamValueArray.

                ULONG                       ulWriteID
                Write ID of the component.

                int*                        piStatus
                Returns the status of the SET operation: 0 = Parameter
                changes have been validated and applied; 1 = Parameter
                changes have been validated and committed, but not yet
                applied.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaSetParameterValuesWithWriteID
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pParamValueArray,
        ULONG                       ulArraySize,
        ULONG                       ulWriteID,
        int*                        piStatus,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController   = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr           = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    ULONG                           ulSessionID          = 0;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault       = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    PCCSP_CWMP_PARAM_VALUE          pParameterValueArray = (PCCSP_CWMP_PARAM_VALUE     )pParamValueArray;
    ULONG                           ulParameterCount     = (ULONG                      )0;
    char*                           pFaultParamName      = (char*                      )NULL;
    BOOL                            bFaultEncountered    = (BOOL                       )FALSE;
    ULONG                           i                    = 0;
    QUEUE_HEADER                    FcNsListQueue;
    int                             nRet;
    char**                          ppFcNameArray        = NULL;
    char**                          ppDbusPathArray      = NULL;
    char**                          ppSubsysArray        = NULL;
    ULONG                           ulFcArraySize        = 0;
    int                             j;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems        = 0;
    char*                           pInvalidParamName    = NULL;
    PCCSP_TR069PA_FC_NSLIST         pFcNsList            = NULL;
    BOOL                            bSucc                = TRUE;
    int                             nCcspError           = CCSP_SUCCESS;
    PCCSP_TR069PA_NSLIST            pNsList              = NULL;
#ifndef  _CCSP_TR069_PA_INTERCEPT_ACS_CREDENTIAL_
    BOOL                            bAcsCredChanged      = FALSE;
#endif

    *piStatus    = 0;
    *phSoapFault = (ANSC_HANDLE)NULL;

    AnscQueueInitializeHeader(&FcNsListQueue);

    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT) + sizeof(CCSP_CWMP_SET_PARAM_FAULT) * ulArraySize);

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT1;
    }

    if ( (ulArraySize == 0) || !pParameterValueArray )
    {
        *piStatus    = 0;
        *phSoapFault = (ANSC_HANDLE)NULL;
        returnStatus = ANSC_STATUS_SUCCESS;

        goto  EXIT1;
    }

    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    /*
     * Sanity checks
     */
    for ( i = 0; i < ulArraySize; i++ )
    {
        if ( !pParameterValueArray[i].Name || CcspCwmpIsPartialName(pParameterValueArray[i].Name) )
        {
            bFaultEncountered = TRUE;

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);

            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = CcspTr069PaCloneString(pParameterValueArray[i].Name);
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName;
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamName);
            pCwmpSoapFault->SetParamValuesFaultCount++;

            continue;
        }
        else if ( !pParameterValueArray[i].Value )
        {
            bFaultEncountered = TRUE;

            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = CcspTr069PaCloneString(pParameterValueArray[i].Name);
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamValue;
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamValue);
            pCwmpSoapFault->SetParamValuesFaultCount++;

            continue;
        }
        else if ( pParameterValueArray[i].Tr069DataType == CCSP_CWMP_TR069_DATA_TYPE_Unspecified )
        {
            pParameterValueArray[i].Tr069DataType = pCcspCwmpCpeController->GetParamDataType((ANSC_HANDLE)pCcspCwmpCpeController, pParameterValueArray[i].Name);
        }
    }

    if ( bFaultEncountered )
    {
        returnStatus = ANSC_STATUS_BAD_PARAMETER;
        goto EXIT2;
    }

    /* 
     * Parse all parameters and identify owners - come up a list of parameters
     * for each FC and make SPV MBus call to each FC. 
     * If there're more than one FC involved, two phases for SPV.
     * First to call SetParameterValues, and then call SetCommit
     * for each FC with 'commit' flag set to TRUE if all FCs return success
     * on previous SetParameterValues action call. Otherwise set 'commit'
     * flag to FALSE to indicate all FCs that have finished SPV calls
     * to rollback changes.
     */
    
    for ( i = 0; i < ulArraySize; i++ )
    {
#ifndef  _CCSP_TR069_PA_INTERCEPT_ACS_CREDENTIAL_
        if ( !bAcsCredChanged && 
             ( _ansc_strstr(pParameterValueArray[i].Name, ".ManagementServer.Username") || 
              _ansc_strstr(pParameterValueArray[i].Name, ".ManagementServer.Password") ) )
        {
            bAcsCredChanged = TRUE;
        }
#endif

        /* identify which sub-system(s) the parameter resides */
        NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

        CcspTr069PA_GetNamespaceSubsystems
            (
                pCcspCwmpCpeController->hTr069PaMapper,
                pParameterValueArray[i].Name,
                Subsystems,
                &NumSubsystems,
                !bExcludeInvNs
            );
        
        if ( NumSubsystems <= 0 )
        {
            Subsystems[0] = CcspTr069PaCloneString(pCcspCwmpCpeController->SubsysName);    /* assume 'local' sub-system will be used */
            NumSubsystems = 1;
        }
        else if ( NumSubsystems > 1 )
        {
            /* found more than one namespace, error! */
        	CcspTr069PaTraceError(("More than one namespace matches '%s' in mapper file.\n", pParameterValueArray[i].Name));
            returnStatus = ANSC_STATUS_INTERNAL_ERROR;
            pInvalidParamName = pParameterValueArray[i].Name;

            goto EXIT2;
        }

        /* query namespace manager for the namespace on identified sub-system */
        CCSP_TR069PA_DISCOVER_FC
            (
                pParameterValueArray[i].Name,
                TRUE
            );


        if ( nRet != CCSP_SUCCESS )
        {
            returnStatus = ANSC_STATUS_BAD_PARAMETER;

            pInvalidParamName = pParameterValueArray[i].Name;

            goto EXIT2;
        } 
        else if ( ulFcArraySize != 1 )    /* must be only one FC that matches the namespace */
        {
            pInvalidParamName = pParameterValueArray[i].Name;
            returnStatus = ANSC_STATUS_BAD_PARAMETER;
            goto EXIT2;
        }

        CcspTr069PaAddFcIntoFcNsList(&FcNsListQueue, ppSubsysArray[0], ppFcNameArray[0], ppDbusPathArray[0], pFcNsList);

        if ( ppFcNameArray )
        {
            CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
            ppFcNameArray = NULL;
        }

        if ( ppDbusPathArray )
        {
            CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
            ppDbusPathArray = NULL;
        }

        if ( ppSubsysArray )
        {
            CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
            ppSubsysArray = NULL;
        }

        if ( !pFcNsList )
        {
            returnStatus = ANSC_STATUS_RESOURCES;
            goto EXIT2;
        }

        pNsList = 
            (PCCSP_TR069PA_NSLIST)CcspTr069PaAllocateMemory
                (
                    sizeof(CCSP_TR069PA_NSLIST)
                );

        if ( !pNsList )
        {
            returnStatus = ANSC_STATUS_RESOURCES;
            goto EXIT2;
        }
        else
        {
            PCCSP_PARAM_VALUE_INFO  pValueInfo;

            pNsList->NaType = CCSP_NORMALIZED_ACTION_TYPE_SPV;
            pValueInfo      = &pNsList->Args.paramValueInfo;

            pValueInfo->parameterName  = pParameterValueArray[i].Name;
            pValueInfo->parameterValue = pParameterValueArray[i].Value->Variant.varString;
            pValueInfo->type           = CcspTr069PA_Cwmp2CcspType(pParameterValueArray[i].Tr069DataType);

            AnscQueuePushEntry(&pFcNsList->NsList, &pNsList->Linkage);
        }
    }

    ulSessionID = 
        pCcspCwmpCpeController->AcqCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                CCSP_TR069PA_SESSION_PRIORITY_WRTIABLE
            );

    if ( TRUE )
    {
        /* go through FcNsList queue and construct a request for
         * each FC and make SetParameterValues call (and
         * possibly with a setCommit if more than one FC is 
         * involved in this request 
         */
        parameterValStruct_t*       pParamValues    = NULL;
        int                         nParamCount     = 0;
        int                         nNumFCs         = AnscQueueQueryDepth(&FcNsListQueue);
        PSINGLE_LINK_ENTRY          pSLinkEntry     = NULL;
        PSINGLE_LINK_ENTRY          pSLinkEntryNs   = NULL;
        int                         k;
        int                         nNsCount;
        PCCSP_TR069PA_NSLIST        pNsList;
        int                         nResult         = CCSP_SUCCESS;
        char*                       pInvalidParam   = NULL;

        CcspTr069PaTraceDebug(("SPV involves %d Functional Component(s), sessionID = %u.\n", nNumFCs, (unsigned int)ulSessionID));

        for ( j = 0; j < nNumFCs; j ++ )
        {
            pSLinkEntry = AnscQueueSearchEntryByIndex(&FcNsListQueue, j);
            pFcNsList   = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);

            nNsCount = AnscQueueQueryDepth(&pFcNsList->NsList);

            pParamValues = 
                (parameterValStruct_t*)CcspTr069PaAllocateMemory
                    (
                        sizeof(parameterValStruct_t) * nNsCount
                    );

            if ( !pParamValues )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            k = 0;

            pSLinkEntryNs = AnscSListGetFirstEntry(&pFcNsList->NsList);
            while ( pSLinkEntryNs )
            {
                pNsList       = ACCESS_CCSP_TR069PA_NSLIST(pSLinkEntryNs);
                pSLinkEntryNs = AnscQueueGetNextEntry(pSLinkEntryNs);

                /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/
                CcspCwmppoMpaMapParamInstNumCwmpToDmInt(pNsList->Args.paramValueInfo.parameterName);
                CcspCwmppoMpaMapParamInstNumCwmpToDmInt(pNsList->Args.paramValueInfo.parameterValue);

                pParamValues[k++] = pNsList->Args.paramValueInfo;
            }

            /* we're ready to make SPV call! */
            CcspTr069PaTraceDebug(("Calling SPV to FC <%s>, DBus path <%s>.\n", pFcNsList->FCName, pFcNsList->DBusPath));

            pInvalidParam = NULL;


            nResult = 
                CcspBaseIf_setParameterValues
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        pFcNsList->FCName,
                        pFcNsList->DBusPath,
                        ulSessionID,
                        ulWriteID, 
                        pParamValues,
                        k,
                        (nNumFCs == 1),
                        &pInvalidParam
                    );


            CcspTr069PaFreeMemory(pParamValues);

            if ( nResult != CCSP_SUCCESS )
            {
                if ( pInvalidParam )
                {
                    ULONG           spvFaultCode = nResult;

                    /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/
                    CcspCwmppoMpaMapInvalidParamInstNumDmIntToCwmp(pInvalidParam);

                    switch ( spvFaultCode )
                    {
                        case    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName:
                                
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = 
                                    CcspTr069PaCloneString(pInvalidParam);
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = 
                                    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName;
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = 
                                    CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamName);
                                /* set the primary fault code to 9003 */
                                nResult = CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs;

                                break;

                        case    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamType:
                                
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = 
                                    CcspTr069PaCloneString(pInvalidParam);
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = 
                                    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamType;
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = 
                                    CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamType);
                                /* set the primary fault code to 9003 */
                                nResult = CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs;

                                break;

                        case    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamValue:
                                
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = 
                                    CcspTr069PaCloneString(pInvalidParam);
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = 
                                    CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamValue;
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = 
                                    CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamValue);
                                /* set the primary fault code to 9003 */
                                nResult = CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs;

                                break;

                        case    CCSP_CWMP_CPE_CWMP_FaultCode_notWritable:
                                
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = 
                                    CcspTr069PaCloneString(pInvalidParam);
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = 
                                    CCSP_CWMP_CPE_CWMP_FaultCode_notWritable;
                                pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = 
                                    CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_notWritable);
                                /* set the primary fault code to 9003 */
                                nResult = CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs;

                                break;
                    }

                    pCwmpSoapFault->SetParamValuesFaultCount++;

                    CcspTr069PaFreeMemory(pInvalidParam);
                }

            	CcspTr069PaTraceDebug(("SPV failure on FC %s, error = %d\n", pFcNsList->FCName, nResult));
                bSucc = FALSE;
                nCcspError = nResult;

                break;
            }

            if ( pInvalidParam )
            {
                CcspTr069PaFreeMemory(pInvalidParam);
            }
        }

        if ( nResult == CCSP_SUCCESS )
        {
            j = nNumFCs;
        }

        if ( nNumFCs > 1 )
        {
            for ( k = 0; k < j; k ++ )
            {
                pSLinkEntry = AnscQueueSearchEntryByIndex(&FcNsListQueue, k);
                pFcNsList   = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);

                /* given FCs that have passed validation a chance to discard uncommitted changes */
                CcspTr069PaTraceDebug(("Invoking SetCommit on FC <%s>, DBus path <%s>\n", pFcNsList->FCName, pFcNsList->DBusPath));
                nResult = 
                    CcspBaseIf_setCommit
                        (
                            pCcspCwmpCpeController->hMsgBusHandle,
                            pFcNsList->FCName,
                            pFcNsList->DBusPath,
                            ulSessionID,
                            ulWriteID,
                            (dbus_bool)(nResult == CCSP_SUCCESS)
                        );

                CcspTr069PaTraceDebug(("SetCommit on FC %s, status = %d\n", pFcNsList->FCName, nResult));
            }
        }
    }

    if ( bSucc )
    {
        *phSoapFault = (ANSC_HANDLE)NULL;
        returnStatus = ANSC_STATUS_SUCCESS;

#ifndef  _CCSP_TR069_PA_INTERCEPT_ACS_CREDENTIAL_
        if ( bAcsCredChanged ) 
        {
            *piStatus = 1;
        }
#endif

        goto EXIT1;
    }
    else
    {
        returnStatus = ANSC_STATUS_FAILURE;
    }

    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pCwmpSoapFault )
    {
        if ( nCcspError != CCSP_SUCCESS )
        {
            CCSP_INT                nCwmpError = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nCcspError);

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nCwmpError);
        }
        else if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }

        if ( pInvalidParamName )
        {
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].ParameterName = CcspTr069PaCloneString(pInvalidParamName);
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultCode     = CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName;
            pCwmpSoapFault->SetParamValuesFaultArray[pCwmpSoapFault->SetParamValuesFaultCount].FaultString   = CcspTr069PaCloneString(CCSP_CWMP_CPE_CWMP_FaultText_invalidParamName);
            pCwmpSoapFault->SetParamValuesFaultCount++;
        }
    }

    *phSoapFault = (ANSC_HANDLE)pCwmpSoapFault;
    pCwmpSoapFault = NULL;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    CcspTr069PaFreeAllFcNsList(&FcNsListQueue);

    if ( returnStatus == ANSC_STATUS_SUCCESS && pCwmpSoapFault )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
        ppFcNameArray = NULL;
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
        ppDbusPathArray = NULL;
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
        ppSubsysArray = NULL;
    }

    if ( ulSessionID != 0 )
    {
        pCcspCwmpCpeController->RelCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                ulSessionID
            );
    }


    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaGetParameterValues
            (
                ANSC_HANDLE                 hThisObject,
                SLAP_STRING_ARRAY*          pParamNameArray,
				ULONG						uMaxEntry,
                void**                      ppParamValueArray,
                PULONG                      pulArraySize,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to retrieve the value of the
        specified parameters.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

				ULONG						uMaxEntry,
				The maxi number of entries allowed;

                SLAP_STRING_ARRAY*          pParamNameArray
                Specifies the name of the parameters whose values are
                to be returned.

                void**                      ppParamValueArray
                Returns the array of ParameterValue structures.

                PULONG                      pulArraySize
                Specifies the number of elements in the returned
                *ppParamValueArray.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaGetParameterValues
    (
        ANSC_HANDLE                 hThisObject,
        SLAP_STRING_ARRAY*          pParamNameArray,
		ULONG						uMaxEntry,
        void**                      ppParamValueArray,
        PULONG                      pulArraySize,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController   = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr           = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    ULONG                           ulSessionID          = 0;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault       = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    PCCSP_CWMP_PARAM_VALUE          pParameterValueArray = (PCCSP_CWMP_PARAM_VALUE     )NULL;
    ULONG                           ulParameterIndex     = (ULONG                      )0;
    ULONG                           i                    = 0;
    char*                           pParamName           = NULL;
    QUEUE_HEADER                    FcNsListQueue;
    QUEUE_HEADER                    FcGpvResultListQueue;
    int                             nRet;
    char**                          ppFcNameArray        = NULL;
    char**                          ppDbusPathArray      = NULL;
    char**                          ppSubsysArray        = NULL;
    ULONG                           ulFcArraySize        = 0;
    int                             j;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems        = 0;
    PCCSP_TR069PA_FC_NSLIST         pFcNsList            = NULL;
    CCSP_INT                        nCcspError           = CCSP_SUCCESS;
    PCCSP_TR069PA_NSLIST            pNsList              = NULL;
    BOOL                            bParamNameArrayEmpty = FALSE;
    char*                           pRootObjName         = pCcspCwmpCpeController->GetRootObject((ANSC_HANDLE)pCcspCwmpCpeController);

    *ppParamValueArray = NULL;
    *pulArraySize      = 0;
    *phSoapFault       = (ANSC_HANDLE)NULL;

    AnscQueueInitializeHeader(&FcNsListQueue);
    AnscQueueInitializeHeader(&FcGpvResultListQueue);

    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT) + sizeof(CCSP_CWMP_SET_PARAM_FAULT));

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT1;
    }
    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    if ( !pParamNameArray || pParamNameArray->VarCount == 0 )
    {
        bParamNameArrayEmpty = TRUE;
        SlapAllocStringArray2(1, pParamNameArray);
    }

    /*
     * query namespace cache for each parameter and come up with a parameter list for each
     * FC. Make GPV to each FC with parameters the FC owns, and each FC returns a list of 
     * parameter struct on success. Partial path will be simply indicated to each FC which is
     * responsible for returning all parameters that match the partial path name.
     */

    for ( i = 0; i < pParamNameArray->VarCount; i++ )
    {
        pParamName = pParamNameArray->Array.arrayString[i];

        if ( !pParamName || *pParamName == 0 )
        {
            /* empty string */
            pParamName = pRootObjName; 
        }

        /* identify which sub-system(s) the parameter or partial name could reside */
        NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

        CcspTr069PA_GetNamespaceSubsystems
            (
                pCcspCwmpCpeController->hTr069PaMapper,
                pParamName,
                Subsystems,
                &NumSubsystems,
                !bExcludeInvNs
            );
        
        /* query namespace manager for the namespace on identified sub-system */
        CCSP_TR069PA_DISCOVER_FC
            (
                pParamName,
                FALSE
            );


	//dpotter
        if ( ulFcArraySize == 0 ) 
        {
            /* there must be at least one FC that matches the namespace */
            CcspTr069PaTraceDebug(("GPV - no FC owns namespace <%s>\n", pParamName));
            returnStatus = ANSC_STATUS_BAD_NAME; 
         
            //Do not thow a SoapFault causes 9005 error
            goto EXIT1;
        } 

        for ( j = 0; j < ulFcArraySize; j ++ )
        {
            CcspTr069PaAddFcIntoFcNsList(&FcNsListQueue, ppSubsysArray[j], ppFcNameArray[j], ppDbusPathArray[j], pFcNsList);

            if ( !pFcNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            /* Push requested namespace into all owning FCs' queues */
            CcspTr069PaPushGpvNsInQueue
                (
                    &pFcNsList->NsList,
                    pParamName,
                    NULL,
                    ccsp_string,
                    pNsList
                );

            if ( !pNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                break;
            }
        }

        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            break;
        }

        if ( ppFcNameArray )
        {
            CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
            ppFcNameArray = NULL;
        }

        if ( ppDbusPathArray )
        {
            CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
            ppDbusPathArray = NULL;
        }

        if ( ppSubsysArray )
        {
            CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
            ppSubsysArray = NULL;
        }

    }

    ulSessionID =
        pCcspCwmpCpeController->AcqCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                CCSP_TR069PA_SESSION_PRIORITY_READONLY
            );

    if ( TRUE )
    {
        /* go through FcNsList queue and construct a request for
         * each FC and make GetParameterValues call 
         */
        char**                      pParamNames     = NULL;
        int                         nNumParamNames  = 0;
        parameterValStruct_t**      pParamValues    = NULL;
        int                         nParamCount     = 0;
        int                         nNumFCs         = AnscQueueQueryDepth(&FcNsListQueue);
        PSINGLE_LINK_ENTRY          pSLinkEntry     = NULL;
        PSINGLE_LINK_ENTRY          pSLinkEntryNs   = NULL;
        PCCSP_TR069PA_FC_NSLIST     pFcGpvResNsList = NULL;
        int                         k;
        int                         nNsCount;
        PCCSP_TR069PA_NSLIST        pNsList;
        int                         nResult         = CCSP_SUCCESS;
        char*                       pPaSubsystem    = pCcspCwmpCpeController->SubsysName;

        CcspTr069PaTraceDebug(("GPV involves %d Functional Component(s), sessionID = %u.\n", nNumFCs, (unsigned int)ulSessionID));
        
        /* for result NS list, we don't care sub-system, FC name, and DBus path */
        CcspTr069PaAddFcIntoFcNsList(&FcGpvResultListQueue, NULL, NULL, NULL, pFcGpvResNsList);

        for ( j = 0; j < nNumFCs; j ++ )
        {
            pSLinkEntry = AnscQueueSearchEntryByIndex(&FcNsListQueue, j);
            pFcNsList   = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);

            nNsCount = AnscQueueQueryDepth(&pFcNsList->NsList);

            pParamNames = 
                (char**)CcspTr069PaAllocateMemory
                    (
                        sizeof(char *) * nNsCount
                    );

            if ( !pParamNames )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            k = 0;

            pSLinkEntryNs = AnscSListGetFirstEntry(&pFcNsList->NsList);
            while ( pSLinkEntryNs )
            {
                pNsList       = ACCESS_CCSP_TR069PA_NSLIST(pSLinkEntryNs);
                pSLinkEntryNs = AnscQueueGetNextEntry(pSLinkEntryNs);

                /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/
                CcspCwmppoMpaMapParamInstNumCwmpToDmInt(pNsList->Args.paramValueInfo.parameterName);

                pParamNames[k++] = pNsList->Args.paramValueInfo.parameterName;
                pNsList->Args.paramValueInfo.parameterName = NULL;
            }


            /* we're ready to make GPV call! */
            CcspTr069PaTraceDebug(("Invoking GPV on FC <%s>, DBus path <%s>\n, pParamNames : <%s>, num = %d\n", pFcNsList->FCName, pFcNsList->DBusPath, pParamNames[0], k));
            nResult =
                CcspBaseIf_getParameterValues
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        pFcNsList->FCName,
                        pFcNsList->DBusPath,
                        pParamNames,
                        k,
                        &nParamCount,
                        &pParamValues
                    );

            CcspTr069PaFreeMemory(pParamNames);


            if ( nResult != CCSP_SUCCESS )
            {
            	CcspTr069PaTraceDebug(("GPV failure on FC %s, error = %d\n", pFcNsList->FCName, nResult));
                nCcspError = CCSP_SUCCESS;
                returnStatus = ANSC_STATUS_INTERNAL_ERROR;
                break;
            }
            else
            {
                BOOL                bNsInvisibleToCloudServer = FALSE;

                for ( k = 0; k < nParamCount; k ++ )
                {
                    if ( !pParamValues[k] ) continue;      /* some FC returns NULL, robustness check */
                    
                    /*CWMP_2_DM_INT_INSTANCE_NUMBER_MAPPING*/
                    CcspCwmppoMpaMapParamInstNumDmIntToCwmp(pParamValues[k]->parameterName);
                    CcspCwmppoMpaMapParamInstNumDmIntToCwmp(pParamValues[k]->parameterValue);

                    /* filter out namespace that is not supported by this PA, or invisible
                     * to cloud server through this PA
                     */
                    CcspTr069PaVisibleToCloudServer
                        (
                            bExcludeInvNs,
                            pFcNsList->Subsystem,
                            pParamValues[k]->parameterName, 
                            bNsInvisibleToCloudServer
                        );

                    if ( bNsInvisibleToCloudServer )
                    {
                        if ( pParamValues[k]->parameterName )
                        {
                            CcspTr069PaFreeMemory(pParamValues[k]->parameterName);
                            pParamValues[k]->parameterName = NULL;
                        }
                        if ( pParamValues[k]->parameterValue )
                        {
                            CcspTr069PaFreeMemory(pParamValues[k]->parameterValue);
                            pParamValues[k]->parameterValue = NULL;
                        }
                    }
                    else
                    {
                        CcspTr069PaPushGpvNsInQueue
                            (
                                &pFcGpvResNsList->NsList,
                                pParamValues[k]->parameterName,
                                pParamValues[k]->parameterValue,
                                pParamValues[k]->type,
                                pNsList
                            );

                        if ( !pNsList )
                        {
                            returnStatus = ANSC_STATUS_RESOURCES;
                            break;
                        }

                        pParamValues[k]->parameterName  = NULL;
                        pParamValues[k]->parameterValue = NULL;

                        if ( AnscQueueQueryDepth(&pFcGpvResNsList->NsList) > uMaxEntry )
                        {
                            returnStatus = ANSC_STATUS_RESOURCES;
                            break;
                        }
                    }
                }
            }
            
            free_parameterValStruct_t(pCcspCwmpCpeController->hMsgBusHandle, nParamCount, pParamValues);

            if ( returnStatus != ANSC_STATUS_SUCCESS )
            {
                break;
            }
        }

        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            goto EXIT2;
        }
        else 
        {
            ULONG                   ulTotalParamCount = AnscQueueQueryDepth(&pFcGpvResNsList->NsList);
            PSINGLE_LINK_ENTRY      pSLinkEntry;
            PCCSP_TR069PA_NSLIST    pNsList;
            PCCSP_CWMP_PARAM_VALUE  pCwmpPV;

            pParameterValueArray = 
                (PCCSP_CWMP_PARAM_VALUE)CcspTr069PaAllocateMemory
                    (
                        sizeof(CCSP_CWMP_PARAM_VALUE) * ulTotalParamCount
                    );

            if ( !pParameterValueArray )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }
            
            pSLinkEntry = AnscQueueGetFirstEntry(&pFcGpvResNsList->NsList);
            while ( pSLinkEntry )                                   
            {                                                       
                pNsList = ACCESS_CCSP_TR069PA_NSLIST(pSLinkEntry);
                pSLinkEntry = AnscQueueGetNextEntry(pSLinkEntry);

                pCwmpPV = &pParameterValueArray[ulParameterIndex++];

                pCwmpPV->Name = pNsList->Args.paramValueInfo.parameterName;
                pNsList->Args.paramValueInfo.parameterName = NULL;

                SlapAllocVariable(pCwmpPV->Value);
                if ( pCwmpPV->Value )
                {
                    pCwmpPV->Value->Syntax              = SLAP_VAR_SYNTAX_string;
                    pCwmpPV->Value->Variant.varString   = AnscCloneString(pNsList->Args.paramValueInfo.parameterValue); /* cannot use CcspTr069PaCloneString which causes crash */
                    CcspTr069PaFreeMemory(pNsList->Args.paramValueInfo.parameterValue);
                    pNsList->Args.paramValueInfo.parameterValue = NULL;
                }
                else
                {
                    if ( pNsList->Args.paramValueInfo.parameterValue )
                    {
                        CcspTr069PaFreeMemory(pNsList->Args.paramValueInfo.parameterValue);
                        pNsList->Args.paramValueInfo.parameterValue = NULL;
                    }
                }

                pCwmpPV->Tr069DataType = CcspTr069PA_Ccsp2CwmpType(pNsList->Args.paramValueInfo.type);
            }                                                           
        }
    }


    if ( ulParameterIndex == 0 )
    {
        CcspTr069PaTraceDebug(("GPV will return error since PA returns no parameters.\n"));

        returnStatus = ANSC_STATUS_BAD_NAME;
        goto EXIT2;
    }

    *ppParamValueArray = pParameterValueArray;
    *pulArraySize      = ulParameterIndex;
    returnStatus       = ANSC_STATUS_SUCCESS;

    goto EXIT1;

    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pParameterValueArray )
    {
        for ( i = 0; i < ulParameterIndex; i ++ )
        {
            CcspCwmpCleanParamValue((pParameterValueArray + i));
        }

        CcspTr069PaFreeMemory(pParameterValueArray);
    }

    if ( pCwmpSoapFault )
    {
        if ( nCcspError != CCSP_SUCCESS )
        {
            CCSP_INT                nCwmpError;
            
            nCwmpError = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nCcspError);

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nCwmpError);
        }
        else if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_NAME )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
    }

    *phSoapFault   = (ANSC_HANDLE)pCwmpSoapFault;
    pCwmpSoapFault = NULL;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    CcspTr069PaFreeAllFcNsList(&FcNsListQueue);
    CcspTr069PaFreeAllFcNsList(&FcGpvResultListQueue);

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }

    if ( pCwmpSoapFault && returnStatus == ANSC_STATUS_SUCCESS )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ulSessionID != 0 )
    {
        pCcspCwmpCpeController->RelCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                ulSessionID
            );
    }

    if ( bParamNameArrayEmpty )
    {
        SlapFreeVarArray(pParamNameArray);
    }

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaGetParameterNames
            (
                ANSC_HANDLE                 hThisObject,
                char*                       pParamPath,
                BOOL                        bNextLevel,
                void**                      ppParamInfoArray,
                PULONG                      pulArraySize,
                ANSC_HANDLE*                phSoapFault
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to retrieve the list of the parameters
        supported by the CPE.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                char*                       pParamPath
                Specifies a string containing either a complete
                parameter name, or a partial path name representing a
                subset of the name hierarchy.

                BOOL                        bNextLevel
                If false, the response lists the full path name of all
                parameters whose name begins with the string given by
                the ParameterPath argument. If true, the response lists
                only the partial path one level below the specified
                ParameterPath.

                void**                      ppParamInfoArray
                Returns the array of ParameterInfo structures.

                PULONG                      pulArraySize
                Specifies the number of elements in the returned
                *ppParamInfoArray.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaGetParameterNames
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pParamPath,
        BOOL                        bNextLevel,
        void**                      ppParamInfoArray,
        PULONG                      pulArraySize,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject           = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController  = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr          = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    ULONG                           ulSessionID         = 0;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault      = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    PCCSP_CWMP_PARAM_INFO           pCwmpParamInfoArray = (PCCSP_CWMP_PARAM_INFO      )NULL;
    ULONG                           ulParameterCount    = (ULONG                      )0;
    char*                           pRootObjName        = pCcspCwmpCpeController->GetRootObject((ANSC_HANDLE)pCcspCwmpCpeController);
    ULONG                           i                   = 0;
    char*                           pParamName          = NULL;
    QUEUE_HEADER                    FcGpnResultListQueue;
    int                             nRet;
    char**                          ppFcNameArray        = NULL;
    char**                          ppDbusPathArray      = NULL;
    char**                          ppSubsysArray        = NULL;
    ULONG                           ulFcArraySize        = 0;
    int                             j;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems        = 0;
    parameterInfoStruct_t**         ParamInfoArray       = NULL;
    int                             nParamInfoArraySize  = 0;
    PCCSP_TR069PA_FC_NSLIST         pFcNsList            = NULL;
    PCCSP_TR069PA_NSLIST            pNsList              = NULL;
    BOOL                            bNsInvisibleToCloudServer;
    int                             ParamInfoArraySize;
    PSINGLE_LINK_ENTRY              pSLinkEntry          = NULL;
    BOOL                            bDuplicateNs         = FALSE;

    *ppParamInfoArray = NULL;
    *pulArraySize     = 0;
    *phSoapFault      = (ANSC_HANDLE)NULL;

    AnscQueueInitializeHeader(&FcGpnResultListQueue);

    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT) + sizeof(CCSP_CWMP_SET_PARAM_FAULT));

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT1;
    }
    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    /*
     * The GetParameterNames() operation is divided into two steps: the first round we calculate
     * the total number of parameters/objects that need to be returned.
     */
    if ( !pParamPath || (AnscSizeOfString(pParamPath) == 0) )
    {
        pParamPath = pRootObjName;
    }

    if ( bNextLevel && !CcspCwmpIsPartialName(pParamPath) )
    {
        returnStatus = ANSC_STATUS_BAD_PARAMETER;
        goto  EXIT2;
    }

    /*
     * In TR-069 Plugfests, there is a test case to trigger the 9004 Resources Exceeded fault by
     * retrieving information on the Entire Object Model in a single "GetParameterNames",
     * "GetParameterValues" or "GetParameterAttributes" call. As Bin suggested, to address this
     * issue, we should impose an upper limit on the maximum number of parameters that can be
     * returned in a single response. CCSP_CWMPDO_MAX_PARAMS_IN_RESPONSE is defined for this purpose.
     * By default, it's set to 256. This value should be adjusted based on the available DRAM on
     * the target platform.
     */

    /*
     * query namespace cache and come up with a list of FCs that own parameters under
     * the given path. We will get a list of parameter info as result. And then we construct
     * response.
     */


    /* identify which sub-system(s) the parameter or partial name could reside */
    NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

    CcspTr069PA_GetNamespaceSubsystems
        (
            pCcspCwmpCpeController->hTr069PaMapper,
            pParamPath,
            Subsystems,
            &NumSubsystems,
            !bExcludeInvNs
        );
    
    /* query namespace manager for the namespace on identified sub-system */
    CCSP_TR069PA_DISCOVER_FC
        (
            pParamPath,
            bNextLevel
        );


    if ( nRet != CCSP_SUCCESS || ulFcArraySize == 0 ) 
    {
        /* there must be at least one FC that matches the namespace */
        CcspTr069PaTraceDebug(("GPN - no FC owns namespace <%s>\n", pParamPath));

        returnStatus = ANSC_STATUS_BAD_NAME;
        goto EXIT2;
    } 

    CcspTr069PaAddFcIntoFcNsList(&FcGpnResultListQueue, NULL, NULL, NULL, pFcNsList);

    for ( i = 0; i < ulFcArraySize; i ++ )
    {
        nRet = 
            CcspBaseIf_getParameterNames
                (
                    pCcspCwmpCpeController->hMsgBusHandle,
                    ppFcNameArray[i],
                    ppDbusPathArray[i],
                    pParamPath,
                    bNextLevel,
                    &ParamInfoArraySize,
                    &ParamInfoArray
                );


        if ( nRet != CCSP_SUCCESS )
        {
            CcspTr069PaTraceDebug(("GPN - FC <%s> returned error %d.\n", ppFcNameArray[i], nRet));
            continue;
        }

        for ( j = 0; j < ParamInfoArraySize; j ++ )
        {
            /* filter out namespace that is not supported by this PA, or invisible
             * to cloud server through this PA
             */
            if ( !ParamInfoArray[j] ) continue;      /* some FC returns NULL, robustness check */

            CcspTr069PaVisibleToCloudServer
                (
                    bExcludeInvNs,
                    ppSubsysArray[i],
                    ParamInfoArray[j]->parameterName, 
                    bNsInvisibleToCloudServer
                );

            if ( bNsInvisibleToCloudServer )
            {
                continue;
            }

            /* check if the namespace is already in the result list */
#ifndef  _NO_CCSP_TR069PA_GPN_RESULT_FILTERING
            bDuplicateNs = 
                CcspTr069PaIsGpnNsInQueue
                    (
                        &pFcNsList->NsList,
                        ParamInfoArray[j]->parameterName
                    );
#endif

            if ( !bDuplicateNs )
            {
                CcspTr069PaPushGpnNsInQueue
                    (
                        &pFcNsList->NsList, 
                        ParamInfoArray[j]->parameterName,
                        ParamInfoArray[j]->writable,
                        pNsList
                    );

                if ( !pNsList )
                {
                    returnStatus = ANSC_STATUS_RESOURCES;
                    break;
                }
                else
                {
                    ParamInfoArray[j]->parameterName = NULL;
                }
            }
        }

        free_parameterInfoStruct_t(pCcspCwmpCpeController->hMsgBusHandle, ParamInfoArraySize, ParamInfoArray);

        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            break;
        }
    }

    /* construct result name info array */
    ulParameterCount = AnscQueueQueryDepth(&pFcNsList->NsList);

    if ( ulParameterCount == 0 )
    {
        CcspTr069PaTraceDebug(("GPN will return Invalid Arg since PA returns no parameters under the specified path <%s>.\n", pParamPath));
        returnStatus = ANSC_STATUS_BAD_NAME;
        goto EXIT2;
    }

/*
    CcspTr069PaTraceDebug(("The ulParameterCount = %u\n", (unsigned int)ulParameterCount));
*/
    pCwmpParamInfoArray = (PCCSP_CWMP_PARAM_INFO)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_PARAM_INFO) * ulParameterCount);

    if ( !pCwmpParamInfoArray )
    {
        returnStatus = ANSC_STATUS_RESOURCES;
        goto  EXIT2;
    }

    CcspTr069PaTraceDebug(("Total parameter count in 'GetParameterNames' == %u\n", (unsigned int)ulParameterCount));

    i = 0;
    pSLinkEntry = AnscQueueGetFirstEntry(&pFcNsList->NsList);
    while ( pSLinkEntry )                                   
    {                                                       
        pNsList     = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);
        pSLinkEntry = AnscQueueGetNextEntry(pSLinkEntry);

        pCwmpParamInfoArray[i].Name      = pNsList->Args.paramInfo.parameterName;
        pCwmpParamInfoArray[i++].bWritable = pNsList->Args.paramInfo.writable;

        pNsList->Args.paramInfo.parameterName = NULL;
    }


    *ppParamInfoArray = pCwmpParamInfoArray;
    *pulArraySize     = ulParameterCount;
    returnStatus      = ANSC_STATUS_SUCCESS;
    goto EXIT1;

    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pCwmpParamInfoArray )
    {
        for ( i = 0; i < ulParameterCount; i ++ )
        {
            CcspCwmpCleanParamInfo((pCwmpParamInfoArray + i));
        }

        CcspTr069PaFreeMemory(pCwmpParamInfoArray);
    }

    if ( pCwmpSoapFault )
    {
        if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_NAME )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
    }

    *phSoapFault = (ANSC_HANDLE)pCwmpSoapFault;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    CcspTr069PaFreeAllFcNsList(&FcGpnResultListQueue);

    if ( pCwmpSoapFault && returnStatus == ANSC_STATUS_SUCCESS )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }


    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaSetParameterAttributes
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pSetParamAttribArray,
                ULONG                       ulArraySize,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
        );

    description:

        This function is called to configure the attribute of the
        specified parameters.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pSetParamAttribArray
                Specifies the array of ParameterAttribute structures.

                ULONG                       ulArraySize
                Specifies the number of elements in the
                pParamAttributeArray.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaSetParameterAttributes
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pSetParamAttribArray,
        ULONG                       ulArraySize,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController   = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr           = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault       = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    PCCSP_CWMP_SET_PARAM_ATTRIB     pSetParamAttrArray   = (PCCSP_CWMP_SET_PARAM_ATTRIB)pSetParamAttribArray;
    ULONG                           i                    = 0;
    char*                           pRootObjName         = pCcspCwmpCpeController->GetRootObject((ANSC_HANDLE)pCcspCwmpCpeController);
    ULONG                           ulSessionID          = 0;
    QUEUE_HEADER                    FcNsListQueue;
    int                             nRet;
    char**                          ppFcNameArray        = NULL;
    char**                          ppDbusPathArray      = NULL;
    char**                          ppSubsysArray        = NULL;
    ULONG                           ulFcArraySize        = 0;
    int                             j;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems        = 0;
    PCCSP_TR069PA_FC_NSLIST         pFcNsList            = NULL;
    char*                           pParamName           = NULL;
    PCCSP_TR069PA_NSLIST            pNsList              = NULL;

    *phSoapFault = (ANSC_HANDLE)NULL;

    AnscQueueInitializeHeader(&FcNsListQueue);


    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT));

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT1;
    }
    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    if ( (ulArraySize == 0) || !pSetParamAttrArray )
    {
        returnStatus = ANSC_STATUS_SUCCESS;

        goto  EXIT1;
    }

    /* 
     * Parse all parameters and identify owners - come up a list of parameters
     * for each FC and make SPA MBus call to each FC. With current architecture,
     * SPA must succeeds.
     */
    
    for ( i = 0; i < ulArraySize; i++ )
    {
        /* identify which sub-system(s) the parameter resides */
        NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

        pParamName = pSetParamAttrArray[i].Name;
        if ( !pParamName                        || 
             AnscSizeOfString(pParamName) == 0  )
        {
            pParamName = pRootObjName;
        }

        CcspTr069PA_GetNamespaceSubsystems
            (
                pCcspCwmpCpeController->hTr069PaMapper,
                pParamName,
                Subsystems,
                &NumSubsystems,
                !bExcludeInvNs
            );

        /* query namespace manager for the namespace on identified sub-system */
        CCSP_TR069PA_DISCOVER_FC
            (
                pParamName,
                FALSE
            );


        if ( nRet != CCSP_SUCCESS || ulFcArraySize == 0 )
        {
            returnStatus = ANSC_STATUS_BAD_PARAMETER;

            if ( nRet == CCSP_CR_ERR_UNSUPPORTED_NAMESPACE )
            {
                returnStatus = ANSC_STATUS_NOT_SUPPORTED;
            }

            goto EXIT2;
        } 

        for ( j = 0; j < ulFcArraySize; j ++ )
        {
            CcspTr069PaAddFcIntoFcNsList(&FcNsListQueue, ppSubsysArray[j], ppFcNameArray[j], ppDbusPathArray[j], pFcNsList);

            if ( !pFcNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            pNsList = 
                (PCCSP_TR069PA_NSLIST)CcspTr069PaAllocateMemory
                    (
                        sizeof(CCSP_TR069PA_NSLIST)
                    );

            if ( !pNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }
            else
            {
                PCCSP_PARAM_ATTR_INFO    pAttrInfo;

                pNsList->NaType = CCSP_NORMALIZED_ACTION_TYPE_SPA;
                pAttrInfo       = &pNsList->Args.paramAttrInfo;

                pAttrInfo->parameterName        = pParamName;
                pAttrInfo->notificationChanged  = pSetParamAttrArray[i].bNotificationChange;
                pAttrInfo->notification         = pSetParamAttrArray[i].Notification != CCSP_CWMP_NOTIFICATION_off;
                pAttrInfo->access               = CCSP_RW;      /* TODO: do not change access */
                pAttrInfo->accessControlChanged = pSetParamAttrArray[i].bAccessListChange;
                pAttrInfo->accessControlBitmask = CCSP_NS_ACCESS_ACSONLY;

                if ( pSetParamAttrArray[i].bAccessListChange &&
                     AnscEqualString(pSetParamAttrArray[i].AccessList, "Subscriber", FALSE) )
                {
                    pAttrInfo->accessControlBitmask = CCSP_NS_ACCESS_SUBSCRIBER;
                }

                AnscQueuePushEntry(&pFcNsList->NsList, &pNsList->Linkage);
            }
        }

        if ( ppFcNameArray )
        {
            CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
            ppFcNameArray = NULL;
        }

        if ( ppDbusPathArray )
        {
            CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
            ppDbusPathArray = NULL;
        }

        if ( ppSubsysArray )
        {
            CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
            ppSubsysArray = NULL;
        }

    }

    ulSessionID =
        pCcspCwmpCpeController->AcqCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                CCSP_TR069PA_SESSION_PRIORITY_WRTIABLE
            );

    if ( TRUE )
    {
        /* go through FcNsList queue and construct a request for
         * each FC and make SetParameterAttributes call
         */
        parameterAttributeStruct_t* pParamAttributes= NULL;
        int                         nParamCount     = 0;
        int                         nNumFCs         = AnscQueueQueryDepth(&FcNsListQueue);
        PSINGLE_LINK_ENTRY          pSLinkEntry     = NULL;
        PSINGLE_LINK_ENTRY          pSLinkEntryNs   = NULL;
        int                         k;
        int                         nNsCount;
        PCCSP_TR069PA_NSLIST        pNsList;
        int                         nResult         = CCSP_SUCCESS;


        CcspTr069PaTraceDebug(("SPA involves %d Functional Component(s), sessionID = %u.\n", nNumFCs, (unsigned int)ulSessionID));

        for ( j = 0; j < nNumFCs; j ++ )
        {
            pSLinkEntry = AnscQueueSearchEntryByIndex(&FcNsListQueue, j);
            pFcNsList   = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);

            nNsCount = AnscQueueQueryDepth(&pFcNsList->NsList);

            pParamAttributes = 
                (parameterAttributeStruct_t*)CcspTr069PaAllocateMemory
                    (
                        sizeof(parameterAttributeStruct_t) * nNsCount
                    );

            if ( !pParamAttributes )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            k = 0;

            pSLinkEntryNs = AnscSListGetFirstEntry(&pFcNsList->NsList);
            while ( pSLinkEntryNs )
            {
                pNsList       = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntryNs);
                pSLinkEntryNs = AnscQueueGetNextEntry(pSLinkEntryNs);

                pParamAttributes[k++] = pNsList->Args.paramAttrInfo;

                pNsList->Args.paramAttrInfo.parameterName = NULL;
            }


            /* we're ready to make SPA call! */
            CcspTr069PaTraceDebug(("Invoking SPA on FC <%s>, DBus path <%s>.\n", pFcNsList->FCName, pFcNsList->DBusPath));
            nResult = 
                CcspBaseIf_setParameterAttributes
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        pFcNsList->FCName,
                        pFcNsList->DBusPath,
                        ulSessionID,
                        pParamAttributes,
                        k
                    );

            CcspTr069PaFreeMemory(pParamAttributes);

            if ( nResult != CCSP_SUCCESS )
            {
            	CcspTr069PaTraceDebug(("SPA failure on FC %s, error = %d, ignored.\n", pFcNsList->FCName, nResult));
                if ( nResult >= CCSP_ERR_NOT_CONNECT && nResult <= CCSP_ERR_NOT_SUPPORT )
                {
                    returnStatus = ANSC_STATUS_INTERNAL_ERROR;
                }
                else
                {
                    returnStatus = ANSC_STATUS_FAILURE;
                }   
                goto EXIT2;
            }
        }
    
    }

    /* update local parameter notification cache */
    pMyObject->UpdateParamAttrCache
        (
            (ANSC_HANDLE)pMyObject,
            pSetParamAttribArray,
            ulArraySize
        );

    *phSoapFault = (ANSC_HANDLE)NULL;
    returnStatus = ANSC_STATUS_SUCCESS;

    goto EXIT1;

    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pCwmpSoapFault )
    {
        if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER || returnStatus == ANSC_STATUS_NOT_SUPPORTED )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else if ( returnStatus == ANSC_STATUS_INTERNAL_ERROR )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_notifyRejected);
        }
    }

    *phSoapFault   = (ANSC_HANDLE)pCwmpSoapFault;
    pCwmpSoapFault = NULL;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    CcspTr069PaFreeAllFcNsList(&FcNsListQueue);

    if ( returnStatus == ANSC_STATUS_SUCCESS && pCwmpSoapFault )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }

    if ( ulSessionID != 0 )
    {
        pCcspCwmpCpeController->RelCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                ulSessionID
            );
    }

   
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaGetParameterAttributes
            (
                ANSC_HANDLE                 hThisObject,
                SLAP_STRING_ARRAY*          pParamNameArray,
				ULONG						uMaxEntry,
                void**                      ppParamAttribArray,
                PULONG                      pulArraySize,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
        );

    description:

        This function is called to retrieve the attibutes of the
        specified parameters.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                SLAP_STRING_ARRAY*          pParamNameArray
                Specifies the name of the parameters whose attributes
                are to be returned.

				ULONG						uMaxEntry,
				The maxi amount of entries allowed;

                void**                      ppParamAttributeArray
                Returns the array of ParameterAttribute structures.

                PULONG                      pulArraySize
                Specifies the number of elements in the returned
                *ppParamAttributeArray.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaGetParameterAttributes
    (
        ANSC_HANDLE                 hThisObject,
        SLAP_STRING_ARRAY*          pParamNameArray,
		ULONG						uMaxEntry,
        void**                      ppParamAttribArray,
        PULONG                      pulArraySize,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject            = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController   = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr           = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault       = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    PCCSP_CWMP_PARAM_ATTRIB         pParamAttrArray      = (PCCSP_CWMP_PARAM_ATTRIB    )NULL;
    ULONG                           ulParamAttrArraySize = 0;
    ULONG                           i                    = 0;
    char*                           pRootObjName         = pCcspCwmpCpeController->GetRootObject((ANSC_HANDLE)pCcspCwmpCpeController);
    ULONG                           ulSessionID          = 0;
    QUEUE_HEADER                    FcNsListQueue;
    QUEUE_HEADER                    FcGpaResultListQueue;
    int                             nRet;
    char**                          ppFcNameArray        = NULL;
    char**                          ppDbusPathArray      = NULL;
    char**                          ppSubsysArray        = NULL;
    ULONG                           ulFcArraySize        = 0;
    int                             j;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems        = 0;
    PCCSP_TR069PA_FC_NSLIST         pFcNsList            = NULL;
    parameterAttributeStruct_t**    ppCcspAttrArray      = NULL;
    int                             nCcspAttrArraySize   = 0;
    char*                           pParamName           = NULL;
    CCSP_INT                        nCcspError           = CCSP_SUCCESS;
    PCCSP_TR069PA_NSLIST            pNsList              = NULL;
    BOOL                            bParamNameArrayEmpty = FALSE;

    *phSoapFault = (ANSC_HANDLE)NULL;

    AnscQueueInitializeHeader(&FcNsListQueue);
    AnscQueueInitializeHeader(&FcGpaResultListQueue);


    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT) + sizeof(CCSP_CWMP_SET_PARAM_FAULT));

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT1;
    }

    if ( !pParamNameArray || pParamNameArray->VarCount == 0 )
    {
        bParamNameArrayEmpty = TRUE;
        SlapAllocStringArray2(1, pParamNameArray);
    }

    /* 
     * Parse all parameters and identify owners - come up a list of parameters
     * for each FC and make GPA MBus call to each FC. With current architecture,
     * GPA must succeeds.
     */
    
    for ( i = 0; i < pParamNameArray->VarCount; i++ )
    {
        /* identify which sub-system(s) the parameter resides */
        NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

        pParamName = pParamNameArray->Array.arrayString[i];
        if ( !pParamName || AnscSizeOfString(pParamName) == 0 )
        {
            pParamName = pRootObjName;
        }

        CcspTr069PA_GetNamespaceSubsystems
            (
                pCcspCwmpCpeController->hTr069PaMapper,
                pParamName,
                Subsystems,
                &NumSubsystems,
                !bExcludeInvNs
            );

        /* query namespace manager for the namespace on identified sub-system */
        CCSP_TR069PA_DISCOVER_FC
            (
                pParamName,
                FALSE
            );


        if ( nRet != CCSP_SUCCESS || ulFcArraySize == 0 )
        {
            returnStatus = ANSC_STATUS_BAD_NAME;
            goto EXIT2;
        } 

        for ( j = 0; j < ulFcArraySize; j ++ )
        {
            CcspTr069PaAddFcIntoFcNsList(&FcNsListQueue, ppSubsysArray[j], ppFcNameArray[j], ppDbusPathArray[j], pFcNsList);

            if ( !pFcNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            pNsList = 
                (PCCSP_TR069PA_NSLIST)CcspTr069PaAllocateMemory
                    (
                        sizeof(CCSP_TR069PA_NSLIST)
                    );

            if ( !pNsList )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }
            else
            {
                PCCSP_PARAM_ATTR_INFO    pAttrInfo;

                pNsList->NaType = CCSP_NORMALIZED_ACTION_TYPE_GPA;
                pAttrInfo       = &pNsList->Args.paramAttrInfo;

                pAttrInfo->parameterName = pParamName;

                AnscQueuePushEntry(&pFcNsList->NsList, &pNsList->Linkage);
            }
        }

        if ( ppFcNameArray )
        {
            CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
            ppFcNameArray = NULL;
        }

        if ( ppDbusPathArray )
        {
            CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
            ppDbusPathArray = NULL;
        }

        if ( ppSubsysArray )
        {
            CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
            ppSubsysArray = NULL;
        }

    }

    ulSessionID =
        pCcspCwmpCpeController->AcqCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                CCSP_TR069PA_SESSION_PRIORITY_READONLY
            );

    if ( TRUE )
    {
        /* go through FcNsList queue and construct a request for
         * each FC and make GetParameterAttributes call 
         */
        char**                      pParamNames     = NULL;
        int                         nNumParamNames  = 0;
        int                         nNumFCs         = AnscQueueQueryDepth(&FcNsListQueue);
        PSINGLE_LINK_ENTRY          pSLinkEntry     = NULL;
        PSINGLE_LINK_ENTRY          pSLinkEntryNs   = NULL;
        PCCSP_TR069PA_FC_NSLIST     pFcGpaResNsList = NULL;
        int                         k;
        int                         nNsCount;
        PCCSP_TR069PA_NSLIST        pNsList;
        int                         nResult         = CCSP_SUCCESS;
        char*                       pPaSubsystem    = pCcspCwmpCpeController->SubsysName;

        CcspTr069PaTraceDebug(("GPA involves %d Functional Component(s), sessionID = %u.\n", nNumFCs, (unsigned int)ulSessionID));
        
        /* for result NS list, we don't care sub-system, FC name, and DBus path */
        CcspTr069PaAddFcIntoFcNsList(&FcGpaResultListQueue, NULL, NULL, NULL, pFcGpaResNsList);

        for ( j = 0; j < nNumFCs; j ++ )
        {
            pSLinkEntry = AnscQueueSearchEntryByIndex(&FcNsListQueue, j);
            pFcNsList   = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntry);

            nNsCount = AnscQueueQueryDepth(&pFcNsList->NsList);

            pParamNames = 
                (char**)CcspTr069PaAllocateMemory
                    (
                        sizeof(char *) * nNsCount
                    );

            if ( !pParamNames )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }

            k = 0;

            pSLinkEntryNs = AnscSListGetFirstEntry(&pFcNsList->NsList);
            while ( pSLinkEntryNs )
            {
                pNsList       = ACCESS_CCSP_TR069PA_FC_NSLIST(pSLinkEntryNs);
                pSLinkEntryNs = AnscQueueGetNextEntry(pSLinkEntryNs);

                pParamNames[k++] = pNsList->Args.paramValueInfo.parameterName;
                pNsList->Args.paramValueInfo.parameterName = NULL;
            }


            /* we're ready to make SPV call! */
            nResult = 
                CcspBaseIf_getParameterAttributes
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        pFcNsList->FCName,
                        pFcNsList->DBusPath,
                        pParamNames,
                        k,
                        &nCcspAttrArraySize,
                        &ppCcspAttrArray
                    );

            CcspTr069PaFreeMemory(pParamNames);


            if ( nResult != CCSP_SUCCESS )
            {
            	CcspTr069PaTraceDebug(("GPA failure on FC %s, error = %d.\n", pFcNsList->FCName, nResult));
                nCcspError = nResult;
                returnStatus = ANSC_STATUS_INTERNAL_ERROR;

                break;
            }
            else
            {
                BOOL                bNsInvisibleToCloudServer = FALSE;

                for ( k = 0; k < nCcspAttrArraySize; k ++ )
                {
                    if ( !ppCcspAttrArray[k] ) continue;      /* some FC returns NULL, robustness check */
            
                    /* filter out namespace that is not supported by this PA, or invisible
                     * to cloud server through this PA
                     */
                    CcspTr069PaVisibleToCloudServer
                        (
                            bExcludeInvNs,
                            pFcNsList->Subsystem,
                            ppCcspAttrArray[k]->parameterName, 
                            bNsInvisibleToCloudServer
                        );

                    if ( bNsInvisibleToCloudServer )
                    {
                        if ( ppCcspAttrArray[k]->parameterName )
                        {
                            CcspTr069PaFreeMemory(ppCcspAttrArray[k]->parameterName);
                            ppCcspAttrArray[k]->parameterName = NULL;
                        }
                    }
                    else
                    {
                        CcspTr069PaPushGpaNsInQueue
                            (
                                &pFcGpaResNsList->NsList,
                                ppCcspAttrArray[k]->parameterName,
                                ppCcspAttrArray[k]->notification,
                                ppCcspAttrArray[k]->accessControlBitmask,
                                pNsList
                            );

                        if ( !pNsList )
                        {
                            returnStatus = ANSC_STATUS_RESOURCES;
                            break;
                        }

                        ppCcspAttrArray[k]->parameterName  = NULL;

                        if ( AnscQueueQueryDepth(&pFcGpaResNsList->NsList) > uMaxEntry )
                        {
                            returnStatus = ANSC_STATUS_RESOURCES;
                            break;
                        }
                    }
                }

                free_parameterAttributeStruct_t(pCcspCwmpCpeController->hMsgBusHandle, nCcspAttrArraySize, ppCcspAttrArray);
            }

            if ( returnStatus != ANSC_STATUS_SUCCESS )
            {
                break;
            }
        }

        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            goto EXIT2;
        }
        else 
        {
            ULONG                   ulTotalParamCount = AnscQueueQueryDepth(&pFcGpaResNsList->NsList);
            PSINGLE_LINK_ENTRY      pSLinkEntry;
            PCCSP_TR069PA_NSLIST    pNsList;
            PCCSP_CWMP_PARAM_ATTRIB pCwmpPA;

            pParamAttrArray = 
                (PCCSP_CWMP_PARAM_ATTRIB)CcspTr069PaAllocateMemory
                    (
                        sizeof(CCSP_CWMP_PARAM_ATTRIB) * ulTotalParamCount
                    );

            if ( !pParamAttrArray )
            {
                returnStatus = ANSC_STATUS_RESOURCES;
                goto EXIT2;
            }
            else
            {
                AnscZeroMemory
                    (
                        pParamAttrArray, 
                        sizeof(CCSP_CWMP_PARAM_ATTRIB) * ulTotalParamCount
                    );
            }

            pSLinkEntry = AnscQueueGetFirstEntry(&pFcGpaResNsList->NsList);
            while ( pSLinkEntry )                                   
            {                                                       
                pNsList = ACCESS_CCSP_TR069PA_NSLIST(pSLinkEntry);
                pSLinkEntry = AnscQueueGetNextEntry(pSLinkEntry);

                pCwmpPA = &pParamAttrArray[ulParamAttrArraySize++];

                pCwmpPA->Name = pNsList->Args.paramAttrInfo.parameterName;
                pNsList->Args.paramAttrInfo.parameterName = NULL;

                pCwmpPA->Notification = CCSP_CWMP_NOTIFICATION_off;

                if ( pNsList->Args.paramAttrInfo.notification )
                {
                    /* check local cache to see whether it's Active or Passive notification */
                    pCwmpPA->Notification = 
                        pMyObject->CheckParamAttrCache
                            (
                                (ANSC_HANDLE)pMyObject,
                                pCwmpPA->Name
                            );

                    if ( pCwmpPA->Notification == CCSP_CWMP_NOTIFICATION_off )
                    {
                        /* Alert: for reasons that CWMP notification attribute
                         * got messed (either not saved into PSM or local cache), 
                         * we will not be able to determine the actual attribute,
                         * and return passive notification always.
                         */
                        pCwmpPA->Notification = CCSP_CWMP_NOTIFICATION_passive;
                    	CcspTr069PaTraceWarning
                            (
                                (
                                    "Namespace '%s' notification attribute cannot be determined correctly!\n", 
                                    pCwmpPA->Name
                                )
                            );
                    }
                }

                pCwmpPA->AccessList = NULL;
                if ( pNsList->Args.paramAttrInfo.accessControlBitmask == CCSP_NS_ACCESS_SUBSCRIBER )
                {
                    pCwmpPA->AccessList = CcspTr069PaCloneString("Subscriber");
                }
            }                                                           
       }
    }


    if ( returnStatus == ANSC_STATUS_SUCCESS && nCcspError == CCSP_SUCCESS )
    {
        *phSoapFault = (ANSC_HANDLE)NULL;

        if ( ulParamAttrArraySize == 0 )
        {
            CcspTr069PaTraceDebug(("GPA will return error since PA returns no namespaces.\n"));

            returnStatus = ANSC_STATUS_BAD_NAME;
            goto EXIT2;
        }

        *ppParamAttribArray = pParamAttrArray;
        *pulArraySize       = ulParamAttrArraySize;

        goto EXIT1;
    }

    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pParamAttrArray )
    {
        for ( i = 0; i < ulParamAttrArraySize;  i ++ )
        {
            CcspCwmpCleanParamAttrib((pParamAttrArray+i));
        }

        CcspTr069PaFreeMemory(pParamAttrArray);
    }

    if ( pCwmpSoapFault )
    {
        if ( nCcspError != CCSP_SUCCESS )
        {
            CCSP_INT                nCwmpError = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nCcspError);

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nCwmpError);
        }
        else if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_NAME )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
    }

    *phSoapFault = (ANSC_HANDLE)pCwmpSoapFault;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    CcspTr069PaFreeAllFcNsList(&FcNsListQueue);
    CcspTr069PaFreeAllFcNsList(&FcGpaResultListQueue);

    if ( returnStatus == ANSC_STATUS_SUCCESS && pCwmpSoapFault )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }

    if ( ulSessionID != 0 )
    {
        pCcspCwmpCpeController->RelCrSessionID
            (
                (ANSC_HANDLE)pCcspCwmpCpeController,
                ulSessionID
            );
    }

    if ( bParamNameArrayEmpty )
    {
        SlapFreeVarArray(pParamNameArray);
    }

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaAddObject
            (
                ANSC_HANDLE                 hThisObject,
                char*                       pObjName,
                PULONG                      pulObjInsNumber,
                int*                        piStatus,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to create a new instance of a multi-
        instance object.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                char*                       pObjName
                Specifies the path name of the collection of objects
                for which a new instance is to be created.

                PULONG                      pulObjInsNumber
                Returns the object instance of the newly created
                object.

                int*                        piStatus
                Returns the status of the object creation.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaAddObject
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pObjName,
        PULONG                      pulObjInsNumber,
        int*                        piStatus,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject           = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController  = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr          = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault      = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    char**                          ppDbusPathArray     = NULL;
    char**                          ppFcNameArray       = NULL;
    char**                          ppSubsysArray       = NULL;
    ULONG                           ulFcArraySize       = 0;
    ULONG                           i;
    int                             nRet;
    CCSP_INT                        nCcspError          = CCSP_SUCCESS;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems       = 0;

    *pulObjInsNumber = 0;
    *piStatus        = 0;
    *phSoapFault     = (ANSC_HANDLE)NULL;

    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-0" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT));
    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;
        goto  EXIT2;
    }
    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    if ( !pObjName || (AnscSizeOfString(pObjName) == 0) || !CcspCwmpIsPartialName(pObjName) )
    {
        returnStatus = ANSC_STATUS_BAD_PARAMETER;
        goto  EXIT2;
    }

    /*
     * query namespace cache and get the FC that owns the
     * table object. Call AddTblRow to the FC.
     */

    NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

    CcspTr069PA_GetNamespaceSubsystems
        (
            pCcspCwmpCpeController->hTr069PaMapper,
            pObjName,
            Subsystems,
            &NumSubsystems,
            !bExcludeInvNs
        );
    
    if ( NumSubsystems <= 0 )
    {
        Subsystems[0] = CcspTr069PaCloneString(pCcspCwmpCpeController->SubsysName);    /* assume 'local' sub-system will be used */
        NumSubsystems = 1;
    }
    else if ( NumSubsystems > 1 )
    {
        /* found more than one namespace, error! */
        CcspTr069PaTraceError(("More than one namespace matches '%s' in mapper file.\n", pObjName));
        returnStatus = ANSC_STATUS_INTERNAL_ERROR;

        goto EXIT2;
    }

    /* query namespace manager for the namespace on identified sub-system */
    CCSP_TR069PA_DISCOVER_FC
        (
            pObjName,
            TRUE
        );

    if ( nRet != CCSP_SUCCESS || ulFcArraySize != 1 )
    {
        returnStatus = ANSC_STATUS_BAD_NAME;
        goto EXIT2;
    } 
    else
    {
        PCCSP_CWMP_CPE_CONTROLLER_OBJECT pCcspCwmpCpeController  = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
        ULONG                       ulSessionID         = 0;

        ulSessionID =
            pCcspCwmpCpeController->AcqCrSessionID
                (
                    (ANSC_HANDLE)pCcspCwmpCpeController,
                    CCSP_TR069PA_SESSION_PRIORITY_WRTIABLE
                );

        if ( TRUE )
        {
            nRet = 
                CcspBaseIf_AddTblRow
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        ppFcNameArray[0],
                        ppDbusPathArray[0],
                        ulSessionID,
                        pObjName,
                        (int*)pulObjInsNumber
                    );

            if ( ulSessionID != 0 )
            {
                pCcspCwmpCpeController->RelCrSessionID
                    (
                        (ANSC_HANDLE)pCcspCwmpCpeController,
                        ulSessionID
                    );
            }

            if ( nRet != CCSP_SUCCESS )
            {
                returnStatus = ANSC_STATUS_BAD_PARAMETER;
                nCcspError   = nRet;

                goto  EXIT2;
            }
        }
    }

    *piStatus    = 0;
    *phSoapFault = (ANSC_HANDLE)NULL;
    returnStatus = ANSC_STATUS_SUCCESS;

    goto  EXIT1;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pCwmpSoapFault )
    {
        if ( nCcspError != CCSP_SUCCESS )
        {
            CCSP_INT                nCwmpError = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nCcspError);

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nCwmpError);
        }
        else if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_NAME )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
    }

    *phSoapFault = (ANSC_HANDLE)pCwmpSoapFault;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    if ( returnStatus == ANSC_STATUS_SUCCESS && pCwmpSoapFault )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        CcspCwmppoMpaDeleteObject
            (
                ANSC_HANDLE                 hThisObject,
                char*                       pObjName,
                int*                        piStatus,
                ANSC_HANDLE*                phSoapFault,
                BOOL                        bExcludeInvNs
            );

    description:

        This function is called to create a new instance of a multi-
        instance object.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                char*                       pObjName
                Specifies the path name of the object instance to be
                removed.

                int*                        piStatus
                Returns the status of the object deletion.

                ANSC_HANDLE*                phSoapFault
                Returns the error information of the operation.

                BOOL                        bExcludeInvNs
                Indicate whether to exclude invisible namespaces.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
CcspCwmppoMpaDeleteObject
    (
        ANSC_HANDLE                 hThisObject,
        char*                       pObjName,
        int*                        piStatus,
        ANSC_HANDLE*                phSoapFault,
        BOOL                        bExcludeInvNs
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PCCSP_CWMP_PROCESSOR_OBJECT      pMyObject           = (PCCSP_CWMP_PROCESSOR_OBJECT )hThisObject;
    PCCSP_CWMP_CPE_CONTROLLER_OBJECT     pCcspCwmpCpeController  = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
    PCCSP_NAMESPACE_MGR_OBJECT      pCcspNsMgr          = (PCCSP_NAMESPACE_MGR_OBJECT )pMyObject->hCcspNamespaceMgr;
    PCCSP_CWMP_SOAP_FAULT           pCwmpSoapFault      = (PCCSP_CWMP_SOAP_FAULT      )NULL;
    char**                          ppDbusPathArray     = NULL;
    char**                          ppFcNameArray       = NULL;
    char**                          ppSubsysArray       = NULL;
    ULONG                           ulFcArraySize       = 0;
    ULONG                           i;
    int                             nRet;
    CCSP_INT                        nCcspError          = CCSP_SUCCESS;
    CCSP_STRING                     Subsystems[CCSP_SUBSYSTEM_MAX_COUNT];
    int                             NumSubsystems       = 0;

    *piStatus    = 0;
    *phSoapFault = (ANSC_HANDLE)NULL;

    /*
     * A fault response MUST make use of the SOAP Fault element using the following conventions:
     *
     *      - The SOAP faultcode element MUST indicate the source of the fault, either
     *        Client or Server, as appropriate for the particular fault. In this usage,
     *        Client represents the originator of the SOAP request, and Server represents
     *        the SOAP responder.
     *      - The SOAP faultstring sub-element MUST contain the string "CWMP fault".
     *      - The SOAP detail element MUST contain a Fault structure defined in the
     *        "urn:dslforum-org:cwmp-1-2" namespace.
     */
    pCwmpSoapFault = (PCCSP_CWMP_SOAP_FAULT)CcspTr069PaAllocateMemory(sizeof(CCSP_CWMP_SOAP_FAULT));

    if ( !pCwmpSoapFault )
    {
        returnStatus = ANSC_STATUS_RESOURCES;
        goto  EXIT2;
    }
    pCwmpSoapFault->SetParamValuesFaultCount = 0;

    if ( !pObjName || (AnscSizeOfString(pObjName) == 0) || !CcspCwmpIsPartialName(pObjName) )
    {
        returnStatus = ANSC_STATUS_BAD_NAME;
        goto  EXIT2;
    }

    /*
     * query namespace cache and get the FC that owns the
     * table object. Call DeleteTblRow to the FC.
     */
    NumSubsystems = CCSP_SUBSYSTEM_MAX_COUNT;

    CcspTr069PA_GetNamespaceSubsystems
        (
            pCcspCwmpCpeController->hTr069PaMapper,
            pObjName,
            Subsystems,
            &NumSubsystems,
            !bExcludeInvNs
        );
    
    if ( NumSubsystems <= 0 )
    {
        Subsystems[0] = CcspTr069PaCloneString(pCcspCwmpCpeController->SubsysName);    /* assume 'local' sub-system will be used */
        NumSubsystems = 1;
    }
    else if ( NumSubsystems > 1 )
    {
        /* found more than one namespace, error! */
        CcspTr069PaTraceError(("More than one namespace matches '%s' in mapper file.\n", pObjName));
        returnStatus = ANSC_STATUS_INTERNAL_ERROR;

        goto EXIT2;
    }

    /* query namespace manager for the namespace on identified sub-system */
    CCSP_TR069PA_DISCOVER_FC
        (
            pObjName,
            TRUE
        );

    if ( nRet != CCSP_SUCCESS || ulFcArraySize != 1 )
    {
        returnStatus = ANSC_STATUS_BAD_NAME;
        goto EXIT2;
    } 
    else
    {
        PCCSP_CWMP_CPE_CONTROLLER_OBJECT pCcspCwmpCpeController  = (PCCSP_CWMP_CPE_CONTROLLER_OBJECT)pMyObject->hCcspCwmpCpeController;
        ULONG                       ulSessionID         = 0;

        ulSessionID =
            pCcspCwmpCpeController->AcqCrSessionID
                (
                    (ANSC_HANDLE)pCcspCwmpCpeController,
                    CCSP_TR069PA_SESSION_PRIORITY_WRTIABLE
                );

        if ( TRUE )
        {

            nRet = 
                CcspBaseIf_DeleteTblRow
                    (
                        pCcspCwmpCpeController->hMsgBusHandle,
                        ppFcNameArray[0],
                        ppDbusPathArray[0],
                        ulSessionID,
                        pObjName
                    );

            if ( ulSessionID != 0 )
            {
                pCcspCwmpCpeController->RelCrSessionID
                    (
                        (ANSC_HANDLE)pCcspCwmpCpeController,
                        ulSessionID
                    );
            }

            nRet = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nRet);

            if ( nRet != CCSP_CWMP_CPE_CWMP_FaultCode_noError )
            {
                CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nRet);

                returnStatus = ANSC_STATUS_INTERNAL_ERROR;

                goto  EXIT2;
            }
        }
    }

    *piStatus    = 0;
    *phSoapFault = (ANSC_HANDLE)NULL;
    returnStatus = ANSC_STATUS_SUCCESS;

    goto  EXIT1;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( pCwmpSoapFault )
    {
        if ( nCcspError != CCSP_SUCCESS )
        {
            CCSP_INT                nCwmpError = CcspTr069PA_MapCcspErrCode(pCcspCwmpCpeController->hTr069PaMapper, nCcspError);

            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, nCwmpError);
        }
        else if ( returnStatus == ANSC_STATUS_RESOURCES )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_resources);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_PARAMETER )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidArgs);
        }
        else if ( returnStatus == ANSC_STATUS_BAD_NAME )
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_invalidParamName);
        }
        else
        {
            CCSP_CWMP_SET_SOAP_FAULT(pCwmpSoapFault, CCSP_CWMP_CPE_CWMP_FaultCode_internalError);
        }
    }

    *phSoapFault = (ANSC_HANDLE)pCwmpSoapFault;

EXIT1:

    CcspTr069FreeStringArray(Subsystems, NumSubsystems, FALSE);

    if ( returnStatus == ANSC_STATUS_SUCCESS && pCwmpSoapFault )
    {
        CcspCwmpFreeSoapFault(pCwmpSoapFault);
    }

    if ( ppFcNameArray )
    {
        CcspTr069FreeStringArray(ppFcNameArray, ulFcArraySize, TRUE);
    }

    if ( ppDbusPathArray )
    {
        CcspTr069FreeStringArray(ppDbusPathArray, ulFcArraySize, TRUE);
    }

    if ( ppSubsysArray )
    {
        CcspTr069FreeStringArray(ppSubsysArray, ulFcArraySize, TRUE);
    }

    return  returnStatus;
}
