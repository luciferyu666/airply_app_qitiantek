/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

/* These defines allow us to compile on iOS */
#define _CRT_SECURE_NO_WARNINGS
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 

#include "dnssd.h"
#include "dnssdint.h"
#include "global.h"
#include "compat.h"
#include "utils.h"
#include "raop.h"
//#include "../libcutils/loghack.h"
#define DBG	1
#define LOG_TAG	"dnssd"


#define MAX_DEVICEID 18
#define MAX_SERVNAME 256

#define USE_LIBDL (defined(HAVE_LIBDL) && !defined(__APPLE__))

#if defined(WIN32) || USE_LIBDL
# ifdef WIN32
#  include <stdint.h>
#  if !defined(EFI32) && !defined(EFI64)
#   define DNSSD_STDCALL __stdcall
#  else
#   define DNSSD_STDCALL
#  endif
# else
#  include <dlfcn.h>
#  define DNSSD_STDCALL
# endif

typedef struct _DNSServiceRef_t *DNSServiceRef;
typedef union _TXTRecordRef_t { char PrivateData[16]; char *ForceNaturalAlignment; } TXTRecordRef;

typedef uint32_t DNSServiceFlags;
typedef int32_t  DNSServiceErrorType;

typedef void (DNSSD_STDCALL *DNSServiceRegisterReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    void                                *context
    );

#else
# include <dns_sd.h>
# define DNSSD_STDCALL
#endif

typedef DNSServiceErrorType (DNSSD_STDCALL *DNSServiceRegister_t)
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    const char                          *host,
    uint16_t                            port,
    uint16_t                            txtLen,
    const void                          *txtRecord,
    DNSServiceRegisterReply             callBack,
    void                                *context
    );
typedef void (DNSSD_STDCALL *DNSServiceRefDeallocate_t)(DNSServiceRef sdRef);
typedef void (DNSSD_STDCALL *TXTRecordCreate_t)
    (
    TXTRecordRef     *txtRecord,
    uint16_t         bufferLen,
    void             *buffer
    );
typedef void (DNSSD_STDCALL *TXTRecordDeallocate_t)(TXTRecordRef *txtRecord);
typedef DNSServiceErrorType (DNSSD_STDCALL *TXTRecordSetValue_t)
    (
    TXTRecordRef     *txtRecord,
    const char       *key,
    uint8_t          valueSize,
    const void       *value
    );
typedef uint16_t (DNSSD_STDCALL *TXTRecordGetLength_t)(const TXTRecordRef *txtRecord);
typedef const void * (DNSSD_STDCALL *TXTRecordGetBytesPtr_t)(const TXTRecordRef *txtRecord);

struct dnssd_s {
#ifdef WIN32
	HMODULE module;
#elif USE_LIBDL
	void *module;
#endif

	DNSServiceRegister_t       DNSServiceRegister;
	DNSServiceRefDeallocate_t  DNSServiceRefDeallocate;
	TXTRecordCreate_t          TXTRecordCreate;
	TXTRecordSetValue_t        TXTRecordSetValue;
	TXTRecordGetLength_t       TXTRecordGetLength;
	TXTRecordGetBytesPtr_t     TXTRecordGetBytesPtr;
	TXTRecordDeallocate_t      TXTRecordDeallocate;

	DNSServiceRef raopService;
	DNSServiceRef airplayService;
};

dnssd_t *
dnssd_init(int *error)
{
	dnssd_t *dnssd;
	unsigned char ed_seed[32];
     memcpy(ed_seed,
                   hex2bin("fafe9de4c52e2a3c6de2030d01492f557b46d045680a7e691715af8adaecfaa1", 64),
                   32);
	// unsigned char ed_seed[32] = hex2bin("fafe9de4c52e2a3c6de2030d01492f557b46d045680a7e691715af8adaecfaa1",32);
	// ed25519_create_seed(ed_seed);
	// ed_seed = hex2bin("fafe9de4c52e2a3c6de2030d01492f557b46d045680a7e691715af8adaecfaa1",32);
	ed25519_create_keypair(g_ed_public_key, g_ed_private_key, ed_seed);
	ALOGD(  "g_ed_public_key::::: %s\n",bin2hex(g_ed_public_key,32));
	ALOGD( "g_ed_private_key::::: %s\n",bin2hex(g_ed_private_key,32));
	if (error) *error = DNSSD_ERROR_NOERROR;

	dnssd = calloc(1, sizeof(dnssd_t));
	printf("cccccc");
	if (!dnssd) {
		if (error) *error = DNSSD_ERROR_OUTOFMEM;
		return NULL;
	}
	printf("dddddd");
#ifdef WIN32
	dnssd->module = LoadLibraryA("dnssd.dll");
	if (!dnssd->module) {
		if (error) *error = DNSSD_ERROR_LIBNOTFOUND;
		free(dnssd);
		return NULL;
	}
	dnssd->DNSServiceRegister = (DNSServiceRegister_t)GetProcAddress(dnssd->module, "DNSServiceRegister");
	dnssd->DNSServiceRefDeallocate = (DNSServiceRefDeallocate_t)GetProcAddress(dnssd->module, "DNSServiceRefDeallocate");
	dnssd->TXTRecordCreate = (TXTRecordCreate_t)GetProcAddress(dnssd->module, "TXTRecordCreate");
	dnssd->TXTRecordSetValue = (TXTRecordSetValue_t)GetProcAddress(dnssd->module, "TXTRecordSetValue");
	dnssd->TXTRecordGetLength = (TXTRecordGetLength_t)GetProcAddress(dnssd->module, "TXTRecordGetLength");
	dnssd->TXTRecordGetBytesPtr = (TXTRecordGetBytesPtr_t)GetProcAddress(dnssd->module, "TXTRecordGetBytesPtr");
	dnssd->TXTRecordDeallocate = (TXTRecordDeallocate_t)GetProcAddress(dnssd->module, "TXTRecordDeallocate");

	if (!dnssd->DNSServiceRegister || !dnssd->DNSServiceRefDeallocate || !dnssd->TXTRecordCreate ||
	    !dnssd->TXTRecordSetValue || !dnssd->TXTRecordGetLength || !dnssd->TXTRecordGetBytesPtr ||
	    !dnssd->TXTRecordDeallocate) {
		if (error) *error = DNSSD_ERROR_PROCNOTFOUND;
		FreeLibrary(dnssd->module);
		free(dnssd);
		return NULL;
	}
#elif USE_LIBDL
	dnssd->module = dlopen("libdns_sd.so", RTLD_LAZY);
	if (!dnssd->module) {
		if (error) *error = DNSSD_ERROR_LIBNOTFOUND;
		free(dnssd);
		return NULL;
	}
	dnssd->DNSServiceRegister = (DNSServiceRegister_t)dlsym(dnssd->module, "DNSServiceRegister");
	dnssd->DNSServiceRefDeallocate = (DNSServiceRefDeallocate_t)dlsym(dnssd->module, "DNSServiceRefDeallocate");
	dnssd->TXTRecordCreate = (TXTRecordCreate_t)dlsym(dnssd->module, "TXTRecordCreate");
	dnssd->TXTRecordSetValue = (TXTRecordSetValue_t)dlsym(dnssd->module, "TXTRecordSetValue");
	dnssd->TXTRecordGetLength = (TXTRecordGetLength_t)dlsym(dnssd->module, "TXTRecordGetLength");
	dnssd->TXTRecordGetBytesPtr = (TXTRecordGetBytesPtr_t)dlsym(dnssd->module, "TXTRecordGetBytesPtr");
	dnssd->TXTRecordDeallocate = (TXTRecordDeallocate_t)dlsym(dnssd->module, "TXTRecordDeallocate");

	if (!dnssd->DNSServiceRegister || !dnssd->DNSServiceRefDeallocate || !dnssd->TXTRecordCreate ||
	    !dnssd->TXTRecordSetValue || !dnssd->TXTRecordGetLength || !dnssd->TXTRecordGetBytesPtr ||
	    !dnssd->TXTRecordDeallocate) {
		if (error) *error = DNSSD_ERROR_PROCNOTFOUND;
		dlclose(dnssd->module);
		free(dnssd);
		return NULL;
	}
	printf("bbbbbbb");
#else

#if EMBEDDED_MDNS
	printf("aaaaaaaa");
	dnssd->DNSServiceRegister = &DNSServiceRegister;
	dnssd->DNSServiceRefDeallocate = &DNSServiceRefDeallocate;
	dnssd->TXTRecordCreate = &TXTRecordCreate;
	dnssd->TXTRecordSetValue = &TXTRecordSetValue;
	dnssd->TXTRecordGetLength = &TXTRecordGetLength;
	dnssd->TXTRecordGetBytesPtr = &TXTRecordGetBytesPtr;
	dnssd->TXTRecordDeallocate = &TXTRecordDeallocate;
#endif

#endif

	return dnssd;
}

void
dnssd_destroy(dnssd_t *dnssd)
{
	if (dnssd) {
#ifdef WIN32
		FreeLibrary(dnssd->module);
#elif USE_LIBDL
		dlclose(dnssd->module);
#endif
		free(dnssd);
	}
}

int
dnssd_register_raop(dnssd_t *dnssd, const char *name, unsigned short port, const char *hwaddr, int hwaddrlen, char* password)
{
	TXTRecordRef txtRecord;
	char servname[MAX_SERVNAME];
	int ret;
	char* pk = bin2hex(g_ed_public_key, 32);
	ALOGD(  "pk::::: %s\n", pk);
	assert(dnssd);
	assert(name);
	assert(hwaddr);

	dnssd->TXTRecordCreate(&txtRecord, 0, NULL);
	dnssd->TXTRecordSetValue(&txtRecord, "txtvers", strlen(RAOP_TXTVERS), RAOP_TXTVERS);
	dnssd->TXTRecordSetValue(&txtRecord, "ch", strlen(RAOP_CH), RAOP_CH);
	dnssd->TXTRecordSetValue(&txtRecord, "cn", strlen(RAOP_CN), RAOP_CN);
	dnssd->TXTRecordSetValue(&txtRecord, "et", strlen(RAOP_ET), RAOP_ET);
	dnssd->TXTRecordSetValue(&txtRecord, "sv", strlen(RAOP_SV), RAOP_SV);
	dnssd->TXTRecordSetValue(&txtRecord, "da", strlen(RAOP_DA), RAOP_DA);
	dnssd->TXTRecordSetValue(&txtRecord, "sr", strlen(RAOP_SR), RAOP_SR);
	dnssd->TXTRecordSetValue(&txtRecord, "ss", strlen(RAOP_SS), RAOP_SS);
	if (password) {
		dnssd->TXTRecordSetValue(&txtRecord, "pw", strlen("true"), "true");
	} else {
		dnssd->TXTRecordSetValue(&txtRecord, "pw", strlen("false"), "false");
	}
	dnssd->TXTRecordSetValue(&txtRecord, "vn", strlen(RAOP_VN), RAOP_VN);
	dnssd->TXTRecordSetValue(&txtRecord, "tp", strlen(RAOP_TP), RAOP_TP);
	dnssd->TXTRecordSetValue(&txtRecord, "md", strlen(RAOP_MD), RAOP_MD);
	dnssd->TXTRecordSetValue(&txtRecord, "vs", strlen(GLOBAL_VERSION), GLOBAL_VERSION);
	dnssd->TXTRecordSetValue(&txtRecord, "sm", strlen(RAOP_SM), RAOP_SM);
	dnssd->TXTRecordSetValue(&txtRecord, "ek", strlen(RAOP_EK), RAOP_EK);
	dnssd->TXTRecordSetValue(&txtRecord, "sf", strlen(RAOP_SF), RAOP_SF);
	dnssd->TXTRecordSetValue(&txtRecord, "am", strlen(RAOP_AM), RAOP_AM);
	dnssd->TXTRecordSetValue(&txtRecord, "pk", strlen(pk), pk);

	/* Convert hardware address to string */
	ret = utils_hwaddr_raop(servname, sizeof(servname), hwaddr, hwaddrlen);
	if (ret < 0) {
		/* FIXME: handle better */
		return -1;
	}

	/* Check that we have bytes for 'hw@name' format */
	if (sizeof(servname) < strlen(servname)+1+strlen(name)+1) {
		/* FIXME: handle better */
		return -2;
	}

	strncat(servname, "@", sizeof(servname)-strlen(servname)-1);
	strncat(servname, name, sizeof(servname)-strlen(servname)-1);
	DNSServiceErrorType err;

	/* Register the service */
	err = dnssd->DNSServiceRegister(&dnssd->raopService, 0, 0,
	                          servname, "_raop._tcp",
	                          NULL, NULL,
	                          htons(port),
	                          dnssd->TXTRecordGetLength(&txtRecord),
	                          dnssd->TXTRecordGetBytesPtr(&txtRecord),
	                          NULL, NULL);

	ALOGD(  "dnssd_register_raop::::: %ld\n", (long int)err);

	/* Deallocate TXT record */
	dnssd->TXTRecordDeallocate(&txtRecord);
	return err;
}

int
dnssd_register_airplay(dnssd_t *dnssd, const char *name, unsigned short port, const char *hwaddr, int hwaddrlen)
{
	TXTRecordRef txtRecord;
	char deviceid[3*MAX_HWADDR_LEN];
	char features[32] = {0};
	char flags[] = "0x4";
	char model[] = "AppleTV3,2";
	char pi[] = "b08f5a79-db29-4384-b456-a4784d9e6055";
	//char srcvers[] = "220.68";
	char vv[] = "2";
	int ret;
	char* pk = bin2hex(g_ed_public_key, 32);
	char pushMode[256] = {0};

	strcpy(features, "0x527FFFF7");  //0x5A7FFFE4,0x1E   
	//strcpy(features, "0x527FFFE4");  // youtube mirror                                                 

	assert(dnssd);
	assert(name);
	assert(hwaddr);

	/* Convert hardware address to string */
	ret = utils_hwaddr_airplay(deviceid, sizeof(deviceid), hwaddr, hwaddrlen);
	if (ret < 0) {
		/* FIXME: handle better */
		return -1;
	}

// 	features[sizeof(features)-1] = '\0';
// 	snprintf(features, sizeof(features) - 1, "0x%x", GLOBAL_FEATURES_AIRPLAY);

	dnssd->TXTRecordCreate(&txtRecord, 0, NULL);
	dnssd->TXTRecordSetValue(&txtRecord, "srcvers", strlen(GLOBAL_VERSION), GLOBAL_VERSION);
	dnssd->TXTRecordSetValue(&txtRecord, "deviceid", strlen(deviceid), deviceid);
	dnssd->TXTRecordSetValue(&txtRecord, "features", strlen(features), features);
	dnssd->TXTRecordSetValue(&txtRecord, "model", strlen(model), model);
	dnssd->TXTRecordSetValue(&txtRecord, "flags", strlen(flags), flags);
	dnssd->TXTRecordSetValue(&txtRecord, "pk", strlen(pk), pk);
	dnssd->TXTRecordSetValue(&txtRecord, "pi", strlen(pi), pi);
	dnssd->TXTRecordSetValue(&txtRecord, "vv", strlen(vv), vv);

	DNSServiceErrorType err;
	/* Register the service */
	err = dnssd->DNSServiceRegister(&dnssd->airplayService, 0, 0,
	                          name, "_airplay._tcp",
	                          NULL, NULL,
	                          htons(port),
	                          dnssd->TXTRecordGetLength(&txtRecord),
	                          dnssd->TXTRecordGetBytesPtr(&txtRecord),
	                          NULL, NULL);
  	 
	if(DBG) ALOGD( "dnssd_register_airplay::::: %ld\n", (long int)err);

	/* Deallocate TXT record */
	dnssd->TXTRecordDeallocate(&txtRecord);

	free(pk);
	return err;
}

void
dnssd_unregister_raop(dnssd_t *dnssd)
{
	assert(dnssd);

	if (!dnssd->raopService) {
		return;
	}

	dnssd->DNSServiceRefDeallocate(dnssd->raopService);
	dnssd->raopService = NULL;
}

void
dnssd_unregister_airplay(dnssd_t *dnssd)
{
	assert(dnssd);

	if (!dnssd->airplayService) {
		return;
	}

	dnssd->DNSServiceRefDeallocate(dnssd->airplayService);
	dnssd->airplayService = NULL;
}
