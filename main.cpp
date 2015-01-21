
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <list>

#include <boost/foreach.hpp>

#include "pkcs11/pkcs11u.h"
#include "pkcs11/pkcs11.h"

#include "tools.h"
#include "soft_token.h"


std::auto_ptr<soft_token_t> soft_token;

static void log(const std::string& str) {
    std::cout << str << std::endl;
}



template <int ID>
struct func_t {
    static CK_RV not_supported() {
        std::cout << "function " << ID << " not supported" << std::endl;
        return CKR_FUNCTION_NOT_SUPPORTED;
    }
};


struct session_t {
    
    static std::list<session_t>::iterator create() {
        return _sessions.insert(_sessions.end(), session_t(++_id));
    };
    
    static void destroy(CK_SESSION_HANDLE id) {
        auto it = find(id);
        if (it != _sessions.end()) {
            _sessions.erase(it);
        }
    }
    
    static std::list<session_t>::iterator find(CK_SESSION_HANDLE id) {
        return std::find(_sessions.begin(), _sessions.end(), id);
    };
    
    static std::list<session_t>::iterator end() {
        return _sessions.end();
    }

    operator CK_SESSION_HANDLE() const {return id;}
    
    const CK_SESSION_HANDLE id;
    ids_iterator_t ids_iterator;
    
private:
    session_t(CK_SESSION_HANDLE id) : id(id) {}

private:
    static CK_SESSION_HANDLE _id;
    static std::list<session_t> _sessions;
};


CK_SESSION_HANDLE session_t::_id = 0;
std::list<session_t> session_t::_sessions = std::list<session_t>();



extern "C" {
  
  
CK_RV C_Initialize(CK_VOID_PTR a)
{
    CK_C_INITIALIZE_ARGS_PTR args = reinterpret_cast<CK_C_INITIALIZE_ARGS_PTR>(a);
    log("Initialize");

    std::string rcfile;
    try {
        rcfile = std::string(std::getenv("SOFTPKCS11RC"));
    }
    catch(...) {
        const std::string home = std::string(std::getenv("HOME"));
        rcfile = home + "/.soft-token.rc";
    }

    soft_token.reset(new soft_token_t(rcfile));

    return CKR_OK;
}

CK_RV C_Finalize(CK_VOID_PTR args)
{
    log("Finalize");
    soft_token.reset();

    return CKR_OK;
}

static void snprintf_fill(char *str, size_t size, char fillchar, const char *fmt, ...)
{
    int len;
    va_list ap;
    len = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    if (len < 0 || len > size)
  return;
    while(len < size)
  str[len++] = fillchar;
}

CK_RV C_GetInfo(CK_INFO_PTR args)
{
    log("** GetInfo");
    
    memset(args, 17, sizeof(*args));
    args->cryptokiVersion.major = 2;
    args->cryptokiVersion.minor = 10;
    snprintf_fill((char *)args->manufacturerID, 
      sizeof(args->manufacturerID),
      ' ',
      "SoftToken");
    snprintf_fill((char *)args->libraryDescription, 
      sizeof(args->libraryDescription), ' ',
      "SoftToken");
    args->libraryVersion.major = 1;
    args->libraryVersion.minor = 8;

    return CKR_OK;
}

extern CK_FUNCTION_LIST funcs;

CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR_PTR ppFunctionList)
{
    *ppFunctionList = &funcs;
    return CKR_OK;
}

CK_RV C_GetSlotList(CK_BBOOL tokenPresent, CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR   pulCount)
{
    st_logf("C_GetSlotList\n");

    if (pSlotList) {
        pSlotList[0] = 1;
    }
    
    *pulCount = 1;

    return CKR_OK;
}

CK_RV C_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo)
{
    st_logf("GetSlotInfo: slot: %d\n", (int) slotID);

    memset(pInfo, 18, sizeof(*pInfo));

     if (slotID != 1) return CKR_ARGUMENTS_BAD;


    snprintf_fill((char *)pInfo->slotDescription, 
      sizeof(pInfo->slotDescription),
      ' ',
      "SoftToken (slot)");
    snprintf_fill((char *)pInfo->manufacturerID,
      sizeof(pInfo->manufacturerID),
      ' ',
      "SoftToken (slot)");
    pInfo->flags = CKF_TOKEN_PRESENT;
    pInfo->flags |= CKF_HW_SLOT;
    pInfo->hardwareVersion.major = 1;
    pInfo->hardwareVersion.minor = 0;
    pInfo->firmwareVersion.major = 1;
    pInfo->firmwareVersion.minor = 0;

    return CKR_OK;
}

CK_RV C_GetTokenInfo(CK_SLOT_ID slotID, CK_TOKEN_INFO_PTR pInfo)
{
    log("GetTokenInfo: %s"); 

    memset(pInfo, 19, sizeof(*pInfo));

    snprintf_fill((char *)pInfo->label, 
      sizeof(pInfo->label),
      ' ',
      "SoftToken (token)");
    snprintf_fill((char *)pInfo->manufacturerID, 
      sizeof(pInfo->manufacturerID),
      ' ',
      "SoftToken (token)");
    snprintf_fill((char *)pInfo->model,
      sizeof(pInfo->model),
      ' ',
      "SoftToken (token)");
    snprintf_fill((char *)pInfo->serialNumber, 
      sizeof(pInfo->serialNumber),
      ' ',
      "4711");
    pInfo->flags = CKF_TOKEN_INITIALIZED | CKF_USER_PIN_INITIALIZED;

    if (!soft_token->logged_in())
        pInfo->flags |= CKF_LOGIN_REQUIRED;

    pInfo->ulMaxSessionCount = 5;
    pInfo->ulSessionCount = soft_token->open_sessions();
    pInfo->ulMaxRwSessionCount = 5;
    pInfo->ulRwSessionCount = soft_token->open_sessions();
    pInfo->ulMaxPinLen = 1024;
    pInfo->ulMinPinLen = 0;
    pInfo->ulTotalPublicMemory = 4711;
    pInfo->ulFreePublicMemory = 4712;
    pInfo->ulTotalPrivateMemory = 4713;
    pInfo->ulFreePrivateMemory = 4714;
    pInfo->hardwareVersion.major = 2;
    pInfo->hardwareVersion.minor = 0;
    pInfo->firmwareVersion.major = 2;
    pInfo->firmwareVersion.minor = 0;

    return CKR_OK;
}

CK_RV C_GetMechanismList(CK_SLOT_ID slotID, CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pulCount)
{
    log("GetMechanismList\n");

    *pulCount = 2;
    if (pMechanismList == NULL_PTR) return CKR_OK;

    pMechanismList[0] = CKM_RSA_X_509;
    pMechanismList[1] = CKM_RSA_PKCS;

    return CKR_OK;
}

CK_RV C_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type, CK_MECHANISM_INFO_PTR pInfo)
{
    st_logf("GetMechanismInfo: slot %d type: %d\n", (int)slotID, (int)type);
    return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_InitToken(CK_SLOT_ID slotID,
        CK_UTF8CHAR_PTR pPin,
        CK_ULONG ulPinLen,
        CK_UTF8CHAR_PTR pLabel)
{
    st_logf("InitToken: slot %d\n", (int)slotID);
    return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_OpenSession(CK_SLOT_ID slotID,
          CK_FLAGS flags,
          CK_VOID_PTR pApplication,
          CK_NOTIFY Notify,
          CK_SESSION_HANDLE_PTR phSession)
{
    int i;

    st_logf("OpenSession: slot: %d\n", (int)slotID);
    
    auto session = session_t::create();
    *phSession = *session;

    return CKR_OK;
}

CK_RV C_CloseSession(CK_SESSION_HANDLE hSession)
{
    st_logf("CloseSession\n");
    session_t::destroy(hSession);
    return CKR_OK;
}



CK_RV C_FindObjectsInit(CK_SESSION_HANDLE hSession, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
//     struct session_state *state;

    st_logf("FindObjectsInit: Session: %d ulCount: %d\n", hSession, ulCount);

    auto session = session_t::find(hSession);
    if (session == session_t::end()) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    

    
//     VERIFY_SESSION_HANDLE(hSession, &state);

//     if (state->find.next_object != -1) {
//         application_error("application didn't do C_FindObjectsFinal\n");
//         find_object_final(state);
//     }
    if (ulCount) {
//         CK_ULONG i;
//         size_t len;

//         print_attributes(pTemplate, ulCount);

//         state->find.attributes = 
//             calloc(1, ulCount * sizeof(state->find.attributes[0]));
//         if (state->find.attributes == NULL)
//             return CKR_DEVICE_MEMORY;
//         for (i = 0; i < ulCount; i++) {CKR_DEVICE_MEMORY
//             state->find.attributes[i].pValue = 
//             malloc(pTemplate[i].ulValueLen);
//             if (state->find.attributes[i].pValue == NULL) {
//             find_object_final(state); 
//             return CKR_DEVICE_MEMORY;
//             }
//             memcpy(state->find.attributes[i].pValue,
//             pTemplate[i].pValue, pTemplate[i].ulValueLen);
//             state->find.attributes[i].type = pTemplate[i].type;
//             state->find.attributes[i].ulValueLen = pTemplate[i].ulValueLen;
//         }
//         state->find.num_attributes = ulCount;
//         state->find.next_object = 0;
    } else {
        st_logf(" == find all objects\n");
        session->ids_iterator = soft_token->ids_iterator();
    }

    return CKR_OK;
}

CK_RV C_FindObjects(CK_SESSION_HANDLE hSession,
          CK_OBJECT_HANDLE_PTR phObject,
          CK_ULONG ulMaxObjectCount,
          CK_ULONG_PTR pulObjectCount)
{
    st_logf("FindObjects Session: %d ulMaxObjectCount: %d\n", hSession, ulMaxObjectCount);

    if (ulMaxObjectCount == 0) {
        return CKR_ARGUMENTS_BAD;
    }
    
    auto session = session_t::find(hSession);
    if (session == session_t::end()) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    
    *pulObjectCount = 0;

    for(auto id = session->ids_iterator(); id != soft_token->id_invalid(); id = session->ids_iterator()) {
        *phObject++ = id;
        (*pulObjectCount)++;
        ulMaxObjectCount--;
        if (ulMaxObjectCount == 0) break;        
    }
    
    st_logf("  == pulObjectCount: %d\n", *pulObjectCount);
    return CKR_OK;
}

CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE hSession)
{
    st_logf("FindObjectsFinal\n");
    session_t::destroy(hSession);
    return CKR_OK;
}

CK_RV C_GetAttributeValue(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
    struct session_state *state;
    struct st_object *obj;
    CK_ULONG i;
    CK_RV ret;
    int j;

    st_logf("** GetAttributeValue: %lu ulCount: %d\n", hObject, ulCount);
    
    
//     VERIFY_SESSION_HANDLE(hSession, &state);

//     if ((ret = object_handle_to_object(hObject, &obj)) != CKR_OK) {
//     st_logf("object not found: %lx\n",
//         (unsigned long)HANDLE_OBJECT_ID(hObject));
//     return ret;
//     }
    
    st_logf(" input ");
    print_attributes(pTemplate, ulCount);

    auto attrs = soft_token->attributes(hObject);
    
    for (i = 0; i < ulCount; i++) {
        st_logf("   getting 0x%08lx i:%d\n", (unsigned long)pTemplate[i].type, i);

//             std::cout << "assign" << std::endl;
            auto it = attrs.find(pTemplate[i].type);
            
//             std::cout << "tmpl mem size:" << pTemplate[i].ulValueLen << std::endl;
            if (it != attrs.end())
            {
                if (pTemplate[i].pValue != NULL_PTR && pTemplate[i].ulValueLen >= it->second.ulValueLen)
                {
                    memcpy(pTemplate[i].pValue, it->second.pValue, it->second.ulValueLen);
                }

                pTemplate[i].ulValueLen = it->second.ulValueLen;
            }
            
            
            if (it == attrs.end()) {
                st_logf("key type: 0x%08lx not found\n", (unsigned long)pTemplate[i].type);
                pTemplate[i].ulValueLen = (CK_ULONG)-1;
            }


//         
        
        
//         for (j = 0; j < obj->num_attributes; j++) {
//             if (obj->attrs[j].secret) {
//             pTemplate[i].ulValueLen = (CK_ULONG)-1;
//             break;
//             }
//             if (pTemplate[i].type == obj->attrs[j].attribute.type) {
//             if (pTemplate[i].pValue != NULL_PTR && obj->attrs[j].secret == 0) {
//                 if (pTemplate[i].ulValueLen >= obj->attrs[j].attribute.ulValueLen)
//                 memcpy(pTemplate[i].pValue, obj->attrs[j].attribute.pValue,
//                     obj->attrs[j].attribute.ulValueLen);
//             }
//             pTemplate[i].ulValueLen = obj->attrs[j].attribute.ulValueLen;
//             break;
//             }
//         }
//         if (j == obj->num_attributes) {
//             st_logf("key type: 0x%08lx not found\n", (unsigned long)pTemplate[i].type);
//             pTemplate[i].ulValueLen = (CK_ULONG)-1;
//         }

    }
    
    st_logf(" output ");
    print_attributes(pTemplate, ulCount);
    return CKR_OK;
}



CK_FUNCTION_LIST funcs = {
    { 2, 11 },
    C_Initialize,
    C_Finalize,
    C_GetInfo,
    C_GetFunctionList,
    C_GetSlotList,
    C_GetSlotInfo,
    C_GetTokenInfo,
    C_GetMechanismList,
    C_GetMechanismInfo,
    C_InitToken,
    (void *)func_t<1>::not_supported, /* C_InitPIN */
    (void *)func_t<2>::not_supported, /* C_SetPIN */
    C_OpenSession,
    C_CloseSession,
        (void *)func_t<4>::not_supported, //C_CloseAllSessions,
        (void *)func_t<5>::not_supported, //C_GetSessionInfo,
    (void *)func_t<6>::not_supported, /* C_GetOperationState */
    (void *)func_t<7>::not_supported, /* C_SetOperationState */
        (void *)func_t<8>::not_supported, //C_Login,
        (void *)func_t<9>::not_supported, //C_Logout,(void *)func_t::
    (void *)func_t<10>::not_supported, /* C_CreateObject */
    (void *)func_t<11>::not_supported, /* C_CopyObject */
    (void *)func_t<12>::not_supported, /* C_DestroyObject */
    (void *)func_t<13>::not_supported, /* C_GetObjectSize */
    C_GetAttributeValue,
    (void *)func_t<14>::not_supported, /* C_SetAttributeValue */
    C_FindObjectsInit,
    C_FindObjects,
    C_FindObjectsFinal,
        (void *)func_t<16>::not_supported, //C_EncryptInit,
        (void *)func_t<17>::not_supported, //C_Encrypt,
        (void *)func_t<18>::not_supported, //C_EncryptUpdate,
        (void *)func_t<19>::not_supported, //C_EncryptFinal,
        (void *)func_t<20>::not_supported, //C_DecryptInit,
        (void *)func_t<21>::not_supported, //C_Decrypt,
        (void *)func_t<22>::not_supported, //C_DecryptUpdate,
        (void *)func_t<23>::not_supported, //C_DecryptFinal,
        (void *)func_t<24>::not_supported, //C_DigestInit,
    (void *)func_t<25>::not_supported, /* C_Digest */
    (void *)func_t<26>::not_supported, /* C_DigestUpdate */
    (void *)func_t<27>::not_supported, /* C_DigestKey */
    (void *)func_t<28>::not_supported, /* C_DigestFinal */
        (void *)func_t<29>::not_supported, //C_SignInit,
        (void *)func_t<30>::not_supported, //C_Sign,
        (void *)func_t<31>::not_supported, //C_SignUpdate,
        (void *)func_t<32>::not_supported, //C_SignFinal,
    (void *)func_t<33>::not_supported, /* C_SignRecoverInit */
    (void *)func_t<34>::not_supported, /* C_SignRecover */
        (void *)func_t<35>::not_supported, //C_VerifyInit,
        (void *)func_t<36>::not_supported, //C_Verify,
        (void *)func_t<37>::not_supported, //C_VerifyUpdate,
        (void *)func_t<38>::not_supported, //C_VerifyFinal,
    (void *)func_t<39>::not_supported, /* C_VerifyRecoverInit */
    (void *)func_t<40>::not_supported, /* C_VerifyRecover */
    (void *)func_t<41>::not_supported, /* C_DigestEncryptUpdate */
    (void *)func_t<42>::not_supported, /* C_DecryptDigestUpdate */
    (void *)func_t<43>::not_supported, /* C_SignEncryptUpdate */
    (void *)func_t<44>::not_supported, /* C_DecryptVerifyUpdate */
    (void *)func_t<45>::not_supported, /* C_GenerateKey */
    (void *)func_t<46>::not_supported, /* C_GenerateKeyPair */
    (void *)func_t<47>::not_supported, /* C_WrapKey */
    (void *)func_t<48>::not_supported, /* C_UnwrapKey */
    (void *)func_t<49>::not_supported, /* C_DeriveKey */
    (void *)func_t<50>::not_supported, /* C_SeedRandom */
        (void *)func_t<51>::not_supported, //C_GenerateRandom,
    (void *)func_t<52>::not_supported, /* C_GetFunctionStatus */
    (void *)func_t<53>::not_supported, /* C_CancelFunction */
    (void *)func_t<54>::not_supported  /* C_WaitForSlotEvent */
};










}




