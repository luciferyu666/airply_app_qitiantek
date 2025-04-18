/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2003-2004, Apple Computer, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdlib.h>

#include "dnssd_ipc.h"

static int gDaemonErr = kDNSServiceErr_NoError;

#if defined(_WIN32)

	#define _SSIZE_T
	#include <CommonServices.h>
	#include <DebugServices.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#include <stdarg.h>
	
	#define sockaddr_mdns sockaddr_in
	#define AF_MDNS AF_INET
	
	// Disable warning: "'type cast' : from data pointer 'void *' to function pointer"
	#pragma warning(disable:4055)
	
	// Disable warning: "nonstandard extension, function/data pointer conversion in expression"
	#pragma warning(disable:4152)
	
	extern BOOL IsSystemServiceDisabled();
	
	#define sleep(X) Sleep((X) * 1000)
	
	static int g_initWinsock = 0;
	#define LOG_WARNING kDebugLevelWarning
	#define LOG_INFO kDebugLevelInfo
	static void syslog( int priority, const char * message, ...)
		{
		va_list args;
		int len;
		char * buffer;
		DWORD err = WSAGetLastError();
		(void) priority;
		va_start( args, message );
		len = _vscprintf( message, args ) + 1;
		buffer = malloc( len * sizeof(char) );
		if ( buffer ) { vsprintf( buffer, message, args ); OutputDebugString( buffer ); free( buffer ); }
		WSASetLastError( err );
		}
#else
#ifndef __ANDROID__
	#include <sys/fcntl.h>		// For O_RDWR etc.
#else
	#include <fcntl.h>
	#define LOG_TAG "libmdnsd"
#endif
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <syslog.h>
	
	#define sockaddr_mdns sockaddr_un
	#define AF_MDNS AF_LOCAL

#endif

#if 0 // fix 4.0-4.4 error
#undef FD_SET
#define FD_SET(fd, fdsetp)   (((fd_set *)(fdsetp))->fds_bits[(fd) >> 5] |= (1<<((fd) & 31)))
#endif

// <rdar://problem/4096913> Specifies how many times we'll try and connect to the server.

#define DNSSD_CLIENT_MAXTRIES 4

// Uncomment the line below to use the old error return mechanism of creating a temporary named socket (e.g. in /var/tmp)
//#define USE_NAMED_ERROR_RETURN_SOCKET 1

#define DNSSD_CLIENT_TIMEOUT 10  // In seconds

#ifndef CTL_PATH_PREFIX
#define CTL_PATH_PREFIX "/var/tmp/dnssd_result_socket."
#endif

typedef struct
	{
	ipc_msg_hdr         ipc_hdr;
	DNSServiceFlags     cb_flags;
	uint32_t            cb_interface;
	DNSServiceErrorType cb_err;
	} CallbackHeader;

typedef struct _DNSServiceRef_t DNSServiceOp;
typedef struct _DNSRecordRef_t DNSRecord;

// client stub callback to process message from server and deliver results to client application
typedef void (*ProcessReplyFn)(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *msg, const char *const end);

#define ValidatorBits 0x12345678
#define DNSServiceRefValid(X) (dnssd_SocketValid((X)->sockfd) && (((X)->sockfd ^ (X)->validator) == ValidatorBits))

// When using kDNSServiceFlagsShareConnection, there is one primary _DNSServiceOp_t, and zero or more subordinates
// For the primary, the 'next' field points to the first subordinate, and its 'next' field points to the next, and so on.
// For the primary, the 'primary' field is NULL; for subordinates the 'primary' field points back to the associated primary
//
// _DNS_SD_LIBDISPATCH is defined where libdispatch/GCD is available. This does not mean that the application will use the
// DNSServiceSetDispatchQueue API. Hence any new code guarded with _DNS_SD_LIBDISPATCH should still be backwards compatible.
struct _DNSServiceRef_t
	{
	DNSServiceOp     *next;				// For shared connection
	DNSServiceOp     *primary;			// For shared connection
	dnssd_sock_t      sockfd;			// Connected socket between client and daemon
	dnssd_sock_t      validator;		// Used to detect memory corruption, double disposals, etc.
	client_context_t  uid;				// For shared connection requests, each subordinate DNSServiceRef has its own ID,
										// unique within the scope of the same shared parent DNSServiceRef
	uint32_t          op;				// request_op_t or reply_op_t
	uint32_t          max_index;		// Largest assigned record index - 0 if no additional records registered
	uint32_t          logcounter;		// Counter used to control number of syslog messages we write
	int              *moreptr;			// Set while DNSServiceProcessResult working on this particular DNSServiceRef
	ProcessReplyFn    ProcessReply;		// Function pointer to the code to handle received messages
	void             *AppCallback;		// Client callback function and context
	void             *AppContext;
	DNSRecord        *rec;
#if _DNS_SD_LIBDISPATCH
	dispatch_source_t disp_source;
	dispatch_queue_t  disp_queue;
#endif
	};

struct _DNSRecordRef_t
	{
	DNSRecord		*recnext;
	void *AppContext;
	DNSServiceRegisterRecordReply AppCallback;
	DNSRecordRef recref;
	uint32_t record_index;  // index is unique to the ServiceDiscoveryRef
	DNSServiceOp *sdr;
	};

// Write len bytes. Return 0 on success, -1 on error
static int write_all(dnssd_sock_t sd, char *buf, size_t len)
	{
	// Don't use "MSG_WAITALL"; it returns "Invalid argument" on some Linux versions; use an explicit while() loop instead.
	//if (send(sd, buf, len, MSG_WAITALL) != len) return -1;
	while (len)
		{
		ssize_t num_written = send(sd, buf, (long)len, 0);
		if (num_written < 0 || (size_t)num_written > len)
			{
			// Should never happen. If it does, it indicates some OS bug,
			// or that the mDNSResponder daemon crashed (which should never happen).
			#if !defined(__ppc__) && defined(SO_ISDEFUNCT)
			int defunct;
			socklen_t dlen = sizeof (defunct);
			if (getsockopt(sd, SOL_SOCKET, SO_ISDEFUNCT, &defunct, &dlen) < 0)
				syslog(LOG_WARNING, "dnssd_clientstub write_all: SO_ISDEFUNCT failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
			if (!defunct)
				syslog(LOG_WARNING, "dnssd_clientstub write_all(%d) failed %ld/%ld %d %s", sd,
					(long)num_written, (long)len,
					(num_written < 0) ? dnssd_errno                 : 0,
					(num_written < 0) ? dnssd_strerror(dnssd_errno) : "");
			else
				syslog(LOG_INFO, "dnssd_clientstub write_all(%d) DEFUNCT", sd);
			#else
			syslog(LOG_WARNING, "dnssd_clientstub write_all(%d) failed %ld/%ld %d %s", sd,
				(long)num_written, (long)len,
				(num_written < 0) ? dnssd_errno                 : 0,
				(num_written < 0) ? dnssd_strerror(dnssd_errno) : "");
			#endif
			return -1;
			}
		buf += num_written;
		len -= num_written;
		}
	return 0;
	}

enum { read_all_success = 0, read_all_fail = -1, read_all_wouldblock = -2 };

// Read len bytes. Return 0 on success, read_all_fail on error, or read_all_wouldblock for
static int read_all(dnssd_sock_t sd, char *buf, int len)
	{
	// Don't use "MSG_WAITALL"; it returns "Invalid argument" on some Linux versions; use an explicit while() loop instead.
	//if (recv(sd, buf, len, MSG_WAITALL) != len) return -1;

	while (len)
		{
		ssize_t num_read = recv(sd, buf, len, 0);
		// It is valid to get an interrupted system call error e.g., somebody attaching
		// in a debugger, retry without failing
		if ((num_read < 0) && (errno == EINTR)) { syslog(LOG_INFO, "dnssd_clientstub read_all: EINTR continue"); continue; }
		if ((num_read == 0) || (num_read < 0) || (num_read > len))
			{
			int printWarn = 0;
			int defunct = 0;
			// Should never happen. If it does, it indicates some OS bug,
			// or that the mDNSResponder daemon crashed (which should never happen).
#if defined(WIN32)
			// <rdar://problem/7481776> Suppress logs for "A non-blocking socket operation
			//                          could not be completed immediately"
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				printWarn = 1;
#endif
#if !defined(__ppc__) && defined(SO_ISDEFUNCT)
			{
			socklen_t dlen = sizeof (defunct);
			if (getsockopt(sd, SOL_SOCKET, SO_ISDEFUNCT, &defunct, &dlen) < 0)
				syslog(LOG_WARNING, "dnssd_clientstub read_all: SO_ISDEFUNCT failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
			}
			if (!defunct)
				printWarn = 1;
#endif
			if (printWarn)
				syslog(LOG_WARNING, "dnssd_clientstub read_all(%d) failed %ld/%ld %d %s", sd,
					(long)num_read, (long)len,
					(num_read < 0) ? dnssd_errno                 : 0,
					(num_read < 0) ? dnssd_strerror(dnssd_errno) : "");
			else if (defunct)
				syslog(LOG_INFO, "dnssd_clientstub read_all(%d) DEFUNCT", sd);
			return (num_read < 0 && dnssd_errno == dnssd_EWOULDBLOCK) ? read_all_wouldblock : read_all_fail;
			}
		buf += num_read;
		len -= num_read;
		}
	return read_all_success;
	}

// Returns 1 if more bytes remain to be read on socket descriptor sd, 0 otherwise
static int more_bytes(dnssd_sock_t sd)
	{
	struct timeval tv = { 0, 0 };
	fd_set readfds;
	fd_set *fs;
	int ret;

	if (sd < FD_SETSIZE)
		{
		fs = &readfds;
		FD_ZERO(fs);
		}
	else 
		{
		// Compute the number of integers needed for storing "sd". Internally fd_set is stored
		// as an array of ints with one bit for each fd and hence we need to compute
		// the number of ints needed rather than the number of bytes. If "sd" is 32, we need
		// two ints and not just one.
		int nfdbits = sizeof (int) * 8;
		int nints = (sd/nfdbits) + 1;
		fs = (fd_set *)calloc(nints, sizeof(int));
		if (fs == NULL) { syslog(LOG_WARNING, "dnssd_clientstub more_bytes: malloc failed"); return 0; }
		}
	FD_SET(sd, fs);
	ret = select((int)sd+1, fs, (fd_set*)NULL, (fd_set*)NULL, &tv);
	if (fs != &readfds) free(fs);
	return (ret > 0);
	}

// Wait for daemon to write to socket
static int wait_for_daemon(dnssd_sock_t sock, int timeout)
	{
#ifndef WIN32
	// At this point the next operation (accept() or read()) on this socket may block for a few milliseconds waiting
	// for the daemon to respond, but that's okay -- the daemon is a trusted service and we know if won't take more
	// than a few milliseconds to respond.  So we'll forego checking for readability of the socket.
	(void) sock;
	(void) timeout;
#else
	// Windows on the other hand suffers from 3rd party software (primarily 3rd party firewall software) that
	// interferes with proper functioning of the TCP protocol stack. Because of this and because we depend on TCP
	// to communicate with the system service, we want to make sure that the next operation on this socket (accept() or
	// read()) doesn't block indefinitely.
	if (!gDaemonErr)
		{
		struct timeval tv;
		fd_set set;

		FD_ZERO(&set);
		FD_SET(sock, &set);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if (!select((int)(sock + 1), &set, NULL, NULL, &tv))
			{
				syslog(LOG_WARNING, "dnssd_clientstub wait_for_daemon timed out");
				gDaemonErr = kDNSServiceErr_Timeout;
			}
		}
#endif
	return gDaemonErr;
	}

/* create_hdr
 *
 * allocate and initialize an ipc message header. Value of len should initially be the
 * length of the data, and is set to the value of the data plus the header. data_start
 * is set to point to the beginning of the data section. SeparateReturnSocket should be
 * non-zero for calls that can't receive an immediate error return value on their primary
 * socket, and therefore require a separate return path for the error code result.
 * if zero, the path to a control socket is appended at the beginning of the message buffer.
 * data_start is set past this string.
 */
static ipc_msg_hdr *create_hdr(uint32_t op, size_t *len, char **data_start, int SeparateReturnSocket, DNSServiceOp *ref)
	{
	char *msg = NULL;
	ipc_msg_hdr *hdr;
	int datalen;
#if !defined(USE_TCP_LOOPBACK)
	char ctrl_path[64] = "";	// "/var/tmp/dnssd_result_socket.xxxxxxxxxx-xxx-xxxxxx"
#endif

	if (SeparateReturnSocket)
		{
#if defined(USE_TCP_LOOPBACK)
		*len += 2;  // Allocate space for two-byte port number
#elif defined(USE_NAMED_ERROR_RETURN_SOCKET)
		struct timeval tv;
		if (gettimeofday(&tv, NULL) < 0)
			{ syslog(LOG_WARNING, "dnssd_clientstub create_hdr: gettimeofday failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno)); return NULL; }
		sprintf(ctrl_path, "%s%d-%.3lx-%.6lu", CTL_PATH_PREFIX, (int)getpid(),
			(unsigned long)(tv.tv_sec & 0xFFF), (unsigned long)(tv.tv_usec));
		*len += strlen(ctrl_path) + 1;
#else
		*len += 1;		// Allocate space for single zero byte (empty C string)
#endif
		}

	datalen = (int) *len;
	*len += sizeof(ipc_msg_hdr);

	// Write message to buffer
	msg = malloc(*len);
	if (!msg) { syslog(LOG_WARNING, "dnssd_clientstub create_hdr: malloc failed"); return NULL; }

	memset(msg, 0, *len);
	hdr = (ipc_msg_hdr *)msg;
	hdr->version                = VERSION;
	hdr->datalen                = datalen;
	hdr->ipc_flags              = 0;
	hdr->op                     = op;
	hdr->client_context         = ref->uid;
	hdr->reg_index              = 0;
	*data_start = msg + sizeof(ipc_msg_hdr);
#if defined(USE_TCP_LOOPBACK)
	// Put dummy data in for the port, since we don't know what it is yet.
	// The data will get filled in before we send the message. This happens in deliver_request().
	if (SeparateReturnSocket) put_uint16(0, data_start);
#else
	if (SeparateReturnSocket) put_string(ctrl_path, data_start);
#endif
	return hdr;
	}

static void FreeDNSRecords(DNSServiceOp *sdRef)
	{
	DNSRecord *rec = sdRef->rec;
	while (rec)
		{
		DNSRecord *next = rec->recnext;
		free(rec);
		rec = next;
		}
	}

static void FreeDNSServiceOp(DNSServiceOp *x)
	{
	// We don't use our DNSServiceRefValid macro here because if we're cleaning up after a socket() call failed
	// then sockfd could legitimately contain a failing value (e.g. dnssd_InvalidSocket)
	if ((x->sockfd ^ x->validator) != ValidatorBits)
		syslog(LOG_WARNING, "dnssd_clientstub attempt to dispose invalid DNSServiceRef %p %08X %08X", x, x->sockfd, x->validator);
	else
		{
		x->next         = NULL;
		x->primary      = NULL;
		x->sockfd       = dnssd_InvalidSocket;
		x->validator    = 0xDDDDDDDD;
		x->op           = request_op_none;
		x->max_index    = 0;
		x->logcounter   = 0;
		x->moreptr      = NULL;
		x->ProcessReply = NULL;
		x->AppCallback  = NULL;
		x->AppContext   = NULL;
#if _DNS_SD_LIBDISPATCH
		if (x->disp_source)	dispatch_release(x->disp_source);
		x->disp_source  = NULL;
		x->disp_queue   = NULL;
#endif
		// DNSRecords may have been added to subordinate sdRef e.g., DNSServiceRegister/DNSServiceAddRecord
		// or on the main sdRef e.g., DNSServiceCreateConnection/DNSServiveRegisterRecord. DNSRecords may have
		// been freed if the application called DNSRemoveRecord
		FreeDNSRecords(x);
		free(x);
		}
	}

// Return a connected service ref (deallocate with DNSServiceRefDeallocate)
static DNSServiceErrorType ConnectToServer(DNSServiceRef *ref, DNSServiceFlags flags, uint32_t op, ProcessReplyFn ProcessReply, void *AppCallback, void *AppContext)
	{
	#if APPLE_OSX_mDNSResponder
	int NumTries = DNSSD_CLIENT_MAXTRIES;
	#else
	int NumTries = 0;
	#endif

	dnssd_sockaddr_t saddr;
	DNSServiceOp *sdr;

	if (!ref) { syslog(LOG_WARNING, "dnssd_clientstub DNSService operation with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }

	if (flags & kDNSServiceFlagsShareConnection)
		{
		if (!*ref)
			{
			syslog(LOG_WARNING, "dnssd_clientstub kDNSServiceFlagsShareConnection used with NULL DNSServiceRef");
			return kDNSServiceErr_BadParam;
			}
		if (!DNSServiceRefValid(*ref) || (*ref)->op != connection_request || (*ref)->primary)
			{
			syslog(LOG_WARNING, "dnssd_clientstub kDNSServiceFlagsShareConnection used with invalid DNSServiceRef %p %08X %08X",
				(*ref), (*ref)->sockfd, (*ref)->validator);
			*ref = NULL;
			return kDNSServiceErr_BadReference;
			}
		}

	#if defined(_WIN32)
	if (!g_initWinsock)
		{
		WSADATA wsaData;
		g_initWinsock = 1;
		if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) { *ref = NULL; return kDNSServiceErr_ServiceNotRunning; }
		}
	// <rdar://problem/4096913> If the system service is disabled, we only want to try to connect once
	if (IsSystemServiceDisabled()) NumTries = DNSSD_CLIENT_MAXTRIES;
	#endif

	sdr = malloc(sizeof(DNSServiceOp));
	if (!sdr) { syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: malloc failed"); *ref = NULL; return kDNSServiceErr_NoMemory; }
	sdr->next          = NULL;
	sdr->primary       = NULL;
	sdr->sockfd        = dnssd_InvalidSocket;
	sdr->validator     = sdr->sockfd ^ ValidatorBits;
	sdr->op            = op;
	sdr->max_index     = 0;
	sdr->logcounter    = 0;
	sdr->moreptr       = NULL;
	sdr->uid.u32[0]    = 0;
	sdr->uid.u32[1]    = 0;
	sdr->ProcessReply  = ProcessReply;
	sdr->AppCallback   = AppCallback;
	sdr->AppContext    = AppContext;
	sdr->rec           = NULL;
#if _DNS_SD_LIBDISPATCH
	sdr->disp_source   = NULL;
	sdr->disp_queue    = NULL;
#endif

	if (flags & kDNSServiceFlagsShareConnection)
		{
		DNSServiceOp **p = &(*ref)->next;		// Append ourselves to end of primary's list
		while (*p) p = &(*p)->next;
		*p = sdr;
		// Preincrement counter before we use it -- it helps with debugging if we know the all-zeroes ID should never appear
		if (++(*ref)->uid.u32[0] == 0) ++(*ref)->uid.u32[1];	// In parent DNSServiceOp increment UID counter
		sdr->primary    = *ref;					// Set our primary pointer
		sdr->sockfd     = (*ref)->sockfd;		// Inherit primary's socket
		sdr->validator  = (*ref)->validator;
		sdr->uid        = (*ref)->uid;
		//printf("ConnectToServer sharing socket %d\n", sdr->sockfd);
		}
	else
		{
		#ifdef SO_NOSIGPIPE
		const unsigned long optval = 1;
		#endif
		*ref = NULL;
		sdr->sockfd    = socket(AF_DNSSD, SOCK_STREAM, 0);
		sdr->validator = sdr->sockfd ^ ValidatorBits;
		if (!dnssd_SocketValid(sdr->sockfd))
			{
			syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: socket failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
			FreeDNSServiceOp(sdr);
			return kDNSServiceErr_NoMemory;
			}
		#ifdef SO_NOSIGPIPE
		// Some environments (e.g. OS X) support turning off SIGPIPE for a socket
		if (setsockopt(sdr->sockfd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)) < 0)
			syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: SO_NOSIGPIPE failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
		#endif
		#if defined(USE_TCP_LOOPBACK)
		saddr.sin_family      = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(MDNS_TCP_SERVERADDR);
		saddr.sin_port        = htons(MDNS_TCP_SERVERPORT);
		#else
		saddr.sun_family      = AF_LOCAL;
		strcpy(saddr.sun_path, MDNS_UDS_SERVERPATH);
		#if !defined(__ppc__) && defined(SO_DEFUNCTOK)
		{
		int defunct = 1;
		if (setsockopt(sdr->sockfd, SOL_SOCKET, SO_DEFUNCTOK, &defunct, sizeof(defunct)) < 0)
			syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: SO_DEFUNCTOK failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
		}
		#endif	
		#endif
	
		while (1)
			{
			int err = connect(sdr->sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
			if (!err) break; // If we succeeded, return sdr
			// If we failed, then it may be because the daemon is still launching.
			// This can happen for processes that launch early in the boot process, while the
			// daemon is still coming up. Rather than fail here, we'll wait a bit and try again.
			// If, after four seconds, we still can't connect to the daemon,
			// then we give up and return a failure code.
			if (++NumTries < DNSSD_CLIENT_MAXTRIES) sleep(1); // Sleep a bit, then try again
			else { dnssd_close(sdr->sockfd); FreeDNSServiceOp(sdr); return kDNSServiceErr_ServiceNotRunning; }
			}
		//printf("ConnectToServer opened socket %d\n", sdr->sockfd);
		}

	*ref = sdr;
	return kDNSServiceErr_NoError;
	}

#define deliver_request_bailout(MSG) \
	do { syslog(LOG_WARNING, "dnssd_clientstub deliver_request: %s failed %d (%s)", (MSG), dnssd_errno, dnssd_strerror(dnssd_errno)); goto cleanup; } while(0)

static DNSServiceErrorType deliver_request(ipc_msg_hdr *hdr, DNSServiceOp *sdr)
	{
	uint32_t datalen = hdr->datalen;	// We take a copy here because we're going to convert hdr->datalen to network byte order
	#if defined(USE_TCP_LOOPBACK) || defined(USE_NAMED_ERROR_RETURN_SOCKET)
	char *const data = (char *)hdr + sizeof(ipc_msg_hdr);
	#endif
	dnssd_sock_t listenfd = dnssd_InvalidSocket, errsd = dnssd_InvalidSocket;
	DNSServiceErrorType err = kDNSServiceErr_Unknown;	// Default for the "goto cleanup" cases
	int MakeSeparateReturnSocket = 0;

	// Note: need to check hdr->op, not sdr->op.
	// hdr->op contains the code for the specific operation we're currently doing, whereas sdr->op
	// contains the original parent DNSServiceOp (e.g. for an add_record_request, hdr->op will be
	// add_record_request but the parent sdr->op will be connection_request or reg_service_request)
	if (sdr->primary ||
		hdr->op == reg_record_request || hdr->op == add_record_request || hdr->op == update_record_request || hdr->op == remove_record_request)
		MakeSeparateReturnSocket = 1;

	if (!DNSServiceRefValid(sdr))
		{
		syslog(LOG_WARNING, "dnssd_clientstub deliver_request: invalid DNSServiceRef %p %08X %08X", sdr, sdr->sockfd, sdr->validator);
		return kDNSServiceErr_BadReference;
		}

	if (!hdr) { syslog(LOG_WARNING, "dnssd_clientstub deliver_request: !hdr"); return kDNSServiceErr_Unknown; }

	if (MakeSeparateReturnSocket)
		{
		#if defined(USE_TCP_LOOPBACK)
			{
			union { uint16_t s; u_char b[2]; } port;
			dnssd_sockaddr_t caddr;
			dnssd_socklen_t len = (dnssd_socklen_t) sizeof(caddr);
			listenfd = socket(AF_DNSSD, SOCK_STREAM, 0);
			if (!dnssd_SocketValid(listenfd)) deliver_request_bailout("TCP socket");

			caddr.sin_family      = AF_INET;
			caddr.sin_port        = 0;
			caddr.sin_addr.s_addr = inet_addr(MDNS_TCP_SERVERADDR);
			if (bind(listenfd, (struct sockaddr*) &caddr, sizeof(caddr)) < 0) deliver_request_bailout("TCP bind");
			if (getsockname(listenfd, (struct sockaddr*) &caddr, &len)   < 0) deliver_request_bailout("TCP getsockname");
			if (listen(listenfd, 1)                                      < 0) deliver_request_bailout("TCP listen");
			port.s = caddr.sin_port;
			data[0] = port.b[0];  // don't switch the byte order, as the
			data[1] = port.b[1];  // daemon expects it in network byte order
			}
		#elif defined(USE_NAMED_ERROR_RETURN_SOCKET)
			{
			mode_t mask;
			int bindresult;
			dnssd_sockaddr_t caddr;
			listenfd = socket(AF_DNSSD, SOCK_STREAM, 0);
			if (!dnssd_SocketValid(listenfd)) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET socket");

			caddr.sun_family = AF_LOCAL;
			// According to Stevens (section 3.2), there is no portable way to
			// determine whether sa_len is defined on a particular platform.
			#ifndef NOT_HAVE_SA_LEN
			caddr.sun_len = sizeof(struct sockaddr_un);
			#endif
			strcpy(caddr.sun_path, data);
			mask = umask(0);
			bindresult = bind(listenfd, (struct sockaddr *)&caddr, sizeof(caddr));
			umask(mask);
			if (bindresult          < 0) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET bind");
			if (listen(listenfd, 1) < 0) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET listen");
			}
		#else
			{
			dnssd_sock_t sp[2];
			if (socketpair(AF_DNSSD, SOCK_STREAM, 0, sp) < 0) deliver_request_bailout("socketpair");
			else
				{
				errsd    = sp[0];	// We'll read our four-byte error code from sp[0]
				listenfd = sp[1];	// We'll send sp[1] to the daemon
				#if !defined(__ppc__) && defined(SO_DEFUNCTOK)
				{
				int defunct = 1;
				if (setsockopt(errsd, SOL_SOCKET, SO_DEFUNCTOK, &defunct, sizeof(defunct)) < 0)
					syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: SO_DEFUNCTOK failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
				}
				#endif	
				}
			}
		#endif
		}

#if !defined(USE_TCP_LOOPBACK) && !defined(USE_NAMED_ERROR_RETURN_SOCKET)
	// If we're going to make a separate error return socket, and pass it to the daemon
	// using sendmsg, then we'll hold back one data byte to go with it.
	// On some versions of Unix (including Leopard) sending a control message without
	// any associated data does not work reliably -- e.g. one particular issue we ran
	// into is that if the receiving program is in a kqueue loop waiting to be notified
	// of the received message, it doesn't get woken up when the control message arrives.
	if (MakeSeparateReturnSocket || sdr->op == send_bpf) datalen--;		// Okay to use sdr->op when checking for op == send_bpf
#endif

	// At this point, our listening socket is set up and waiting, if necessary, for the daemon to connect back to
	ConvertHeaderBytes(hdr);
	//syslog(LOG_WARNING, "dnssd_clientstub deliver_request writing %lu bytes", (unsigned long)(datalen + sizeof(ipc_msg_hdr)));
	//if (MakeSeparateReturnSocket) syslog(LOG_WARNING, "dnssd_clientstub deliver_request name is %s", data);
#if TEST_SENDING_ONE_BYTE_AT_A_TIME
	unsigned int i;
	for (i=0; i<datalen + sizeof(ipc_msg_hdr); i++)
		{
		syslog(LOG_WARNING, "dnssd_clientstub deliver_request writing %d", i);
		if (write_all(sdr->sockfd, ((char *)hdr)+i, 1) < 0)
			{ syslog(LOG_WARNING, "write_all (byte %u) failed", i); goto cleanup; }
		usleep(10000);
		}
#else
	if (write_all(sdr->sockfd, (char *)hdr, datalen + sizeof(ipc_msg_hdr)) < 0)
		{
		// write_all already prints an error message if there is an error writing to
		// the socket except for DEFUNCT. Logging here is unnecessary and also wrong
		// in the case of DEFUNCT sockets
		syslog(LOG_INFO, "dnssd_clientstub deliver_request ERROR: write_all(%d, %lu bytes) failed",
			sdr->sockfd, (unsigned long)(datalen + sizeof(ipc_msg_hdr)));
		goto cleanup;
		}
#endif

	if (!MakeSeparateReturnSocket) errsd = sdr->sockfd;
	if (MakeSeparateReturnSocket || sdr->op == send_bpf)	// Okay to use sdr->op when checking for op == send_bpf
		{
#if defined(USE_TCP_LOOPBACK) || defined(USE_NAMED_ERROR_RETURN_SOCKET)
		// At this point we may block in accept for a few milliseconds waiting for the daemon to connect back to us,
		// but that's okay -- the daemon is a trusted service and we know if won't take more than a few milliseconds to respond.
		dnssd_sockaddr_t daddr;
		dnssd_socklen_t len = sizeof(daddr);
		if ((err = wait_for_daemon(listenfd, DNSSD_CLIENT_TIMEOUT)) != kDNSServiceErr_NoError) goto cleanup;
		errsd = accept(listenfd, (struct sockaddr *)&daddr, &len);
		if (!dnssd_SocketValid(errsd)) deliver_request_bailout("accept");
#else

#if APPLE_OSX_mDNSResponder
// On Leopard, the stock definitions of the CMSG_* macros in /usr/include/sys/socket.h,
// while arguably correct in theory, nonetheless in practice produce code that doesn't work on 64-bit machines
// For details see <rdar://problem/5565787> Bonjour API broken for 64-bit apps (SCM_RIGHTS sendmsg fails)
#undef  CMSG_DATA
#define CMSG_DATA(cmsg) ((unsigned char *)(cmsg) + (sizeof(struct cmsghdr)))
#undef  CMSG_SPACE
#define CMSG_SPACE(l)   ((sizeof(struct cmsghdr)) + (l))
#undef  CMSG_LEN
#define CMSG_LEN(l)     ((sizeof(struct cmsghdr)) + (l))
#endif

		struct iovec vec = { ((char *)hdr) + sizeof(ipc_msg_hdr) + datalen, 1 }; // Send the last byte along with the SCM_RIGHTS
		struct msghdr msg;
		struct cmsghdr *cmsg;
		char cbuf[CMSG_SPACE(sizeof(dnssd_sock_t))];

		if (sdr->op == send_bpf)	// Okay to use sdr->op when checking for op == send_bpf
			{
			int i;
			char p[12];		// Room for "/dev/bpf999" with terminating null
			for (i=0; i<100; i++)
				{
				snprintf(p, sizeof(p), "/dev/bpf%d", i);
				listenfd = open(p, O_RDWR, 0);
				//if (dnssd_SocketValid(listenfd)) syslog(LOG_WARNING, "Sending fd %d for %s", listenfd, p);
				if (!dnssd_SocketValid(listenfd) && dnssd_errno != EBUSY)
					syslog(LOG_WARNING, "Error opening %s %d (%s)", p, dnssd_errno, dnssd_strerror(dnssd_errno));
				if (dnssd_SocketValid(listenfd) || dnssd_errno != EBUSY) break;
				}
			}

		msg.msg_name       = 0;
		msg.msg_namelen    = 0;
		msg.msg_iov        = &vec;
		msg.msg_iovlen     = 1;
		msg.msg_control    = cbuf;
		msg.msg_controllen = CMSG_LEN(sizeof(dnssd_sock_t));
		msg.msg_flags      = 0;
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_len     = CMSG_LEN(sizeof(dnssd_sock_t));
		cmsg->cmsg_level   = SOL_SOCKET;
		cmsg->cmsg_type    = SCM_RIGHTS;
		*((dnssd_sock_t *)CMSG_DATA(cmsg)) = listenfd;

#if TEST_KQUEUE_CONTROL_MESSAGE_BUG
		sleep(1);
#endif

#if DEBUG_64BIT_SCM_RIGHTS
		syslog(LOG_WARNING, "dnssd_clientstub sendmsg read sd=%d write sd=%d %ld %ld %ld/%ld/%ld/%ld",
			errsd, listenfd, sizeof(dnssd_sock_t), sizeof(void*),
			sizeof(struct cmsghdr) + sizeof(dnssd_sock_t),
			CMSG_LEN(sizeof(dnssd_sock_t)), (long)CMSG_SPACE(sizeof(dnssd_sock_t)),
			(long)((char*)CMSG_DATA(cmsg) + 4 - cbuf));
#endif // DEBUG_64BIT_SCM_RIGHTS

		if (sendmsg(sdr->sockfd, &msg, 0) < 0)
			{
			syslog(LOG_WARNING, "dnssd_clientstub deliver_request ERROR: sendmsg failed read sd=%d write sd=%d errno %d (%s)",
				errsd, listenfd, dnssd_errno, dnssd_strerror(dnssd_errno));
			err = kDNSServiceErr_Incompatible;
			goto cleanup;
			}

#if DEBUG_64BIT_SCM_RIGHTS
		syslog(LOG_WARNING, "dnssd_clientstub sendmsg read sd=%d write sd=%d okay", errsd, listenfd);
#endif // DEBUG_64BIT_SCM_RIGHTS

#endif
		// Close our end of the socketpair *before* blocking in read_all to get the four-byte error code.
		// Otherwise, if the daemon closes our socket (or crashes), we block in read_all() forever
		// because the socket is not closed (we still have an open reference to it ourselves).
		dnssd_close(listenfd);
		listenfd = dnssd_InvalidSocket;		// Make sure we don't close it a second time in the cleanup handling below
		}

	// At this point we may block in read_all for a few milliseconds waiting for the daemon to send us the error code,
	// but that's okay -- the daemon is a trusted service and we know if won't take more than a few milliseconds to respond.
	if (sdr->op == send_bpf)	// Okay to use sdr->op when checking for op == send_bpf
		err = kDNSServiceErr_NoError;
	else if ((err = wait_for_daemon(errsd, DNSSD_CLIENT_TIMEOUT)) == kDNSServiceErr_NoError)
		{
		if (read_all(errsd, (char*)&err, (int)sizeof(err)) < 0)
			err = kDNSServiceErr_ServiceNotRunning; // On failure read_all will have written a message to syslog for us
		else
			err = ntohl(err);
		}

	//syslog(LOG_WARNING, "dnssd_clientstub deliver_request: retrieved error code %d", err);

cleanup:
	if (MakeSeparateReturnSocket)
		{
		if (dnssd_SocketValid(listenfd)) dnssd_close(listenfd);
		if (dnssd_SocketValid(errsd))    dnssd_close(errsd);
#if defined(USE_NAMED_ERROR_RETURN_SOCKET)
		// syslog(LOG_WARNING, "dnssd_clientstub deliver_request: removing UDS: %s", data);
		if (unlink(data) != 0)
			syslog(LOG_WARNING, "dnssd_clientstub WARNING: unlink(\"%s\") failed errno %d (%s)", data, dnssd_errno, dnssd_strerror(dnssd_errno));
		// else syslog(LOG_WARNING, "dnssd_clientstub deliver_request: removed UDS: %s", data);
#endif
		}

	free(hdr);
	return err;
	}

int DNSSD_API DNSServiceRefSockFD(DNSServiceRef sdRef)
	{
	if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD called with NULL DNSServiceRef"); return dnssd_InvalidSocket; }

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD called with invalid DNSServiceRef %p %08X %08X",
			sdRef, sdRef->sockfd, sdRef->validator);
		return dnssd_InvalidSocket;
		}

	if (sdRef->primary)
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD undefined for kDNSServiceFlagsShareConnection subordinate DNSServiceRef %p", sdRef);
		return dnssd_InvalidSocket;
		}

	return (int) sdRef->sockfd;
	}

#if _DNS_SD_LIBDISPATCH
static void CallbackWithError(DNSServiceRef sdRef, DNSServiceErrorType error)
	{
	DNSServiceOp *sdr = sdRef;
	DNSServiceOp *sdrNext;
	DNSRecord *rec;
	DNSRecord *recnext;
	int morebytes;
	
	while (sdr)
		{
		// We can't touch the sdr after the callback as it can be deallocated in the callback
		sdrNext = sdr->next;
		morebytes = 1;
		sdr->moreptr = &morebytes;
		switch (sdr->op)
			{
			case resolve_request:
				if (sdr->AppCallback)((DNSServiceResolveReply)    sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, 0, 0, NULL,    sdr->AppContext);
				break;
			case query_request:
				if (sdr->AppCallback)((DNSServiceQueryRecordReply)sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, 0, 0, NULL, 0, sdr->AppContext);
				break;
			case addrinfo_request:
				if (sdr->AppCallback)((DNSServiceGetAddrInfoReply)sdr->AppCallback)(sdr, 0, 0, error, NULL, NULL, 0,          sdr->AppContext);
				break;
			case browse_request:
				if (sdr->AppCallback)((DNSServiceBrowseReply)     sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, NULL,          sdr->AppContext);
				break;
			case reg_service_request:
				if (sdr->AppCallback)((DNSServiceRegisterReply)   sdr->AppCallback)(sdr, 0,    error, NULL, 0, NULL,          sdr->AppContext);
				break;
			case enumeration_request:
				if (sdr->AppCallback)((DNSServiceDomainEnumReply) sdr->AppCallback)(sdr, 0, 0, error, NULL,                   sdr->AppContext);
				break;
			case sethost_request:
				if (sdr->AppCallback)((DNSServiceSetHostReply)    sdr->AppCallback)(sdr, 0,    error, NULL,                   sdr->AppContext);
				break;
			case connection_request:
				// This means Register Record, walk the list of DNSRecords to do the callback
				rec = sdr->rec;
				while (rec)
					{
					recnext = rec->recnext;
					if (rec->AppCallback) ((DNSServiceRegisterRecordReply)rec->AppCallback)(sdr, 0, 0, error, rec->AppContext);
					// The Callback can call DNSServiceRefDeallocate which in turn frees sdr and all the records.
					// Detect that and return early
					if (!morebytes){syslog(LOG_WARNING, "dnssdclientstub:Record: CallbackwithError morebytes zero"); return;}
					rec = recnext;
					}
				break;
			case port_mapping_request:
				if (sdr->AppCallback)((DNSServiceNATPortMappingReply)sdr->AppCallback)(sdr, 0, 0, error, 0, 0, 0, 0, 0, sdr->AppContext);
				break;
			default:
				syslog(LOG_WARNING, "dnssd_clientstub CallbackWithError called with bad op %d", sdr->op);
			}
		// If DNSServiceRefDeallocate was called in the callback, morebytes will be zero. It means
		// all other sdrefs have been freed. This happens for shared connections where the
		// DNSServiceRefDeallocate on the first sdRef frees all other sdrefs.
		if (!morebytes){syslog(LOG_WARNING, "dnssdclientstub:sdRef: CallbackwithError morebytes zero"); return;}
		sdr = sdrNext;
		}
	}
#endif // _DNS_SD_LIBDISPATCH

// Handle reply from server, calling application client callback. If there is no reply
// from the daemon on the socket contained in sdRef, the call will block.
DNSServiceErrorType DNSSD_API DNSServiceProcessResult(DNSServiceRef sdRef)
	{
	int morebytes = 0;

	if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return kDNSServiceErr_BadReference;
		}

	if (sdRef->primary)
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult undefined for kDNSServiceFlagsShareConnection subordinate DNSServiceRef %p", sdRef);
		return kDNSServiceErr_BadReference;
		}

	if (!sdRef->ProcessReply)
		{
		static int num_logs = 0;
		if (num_logs < 10) syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with DNSServiceRef with no ProcessReply function");
		if (num_logs < 1000) num_logs++; else sleep(1);
		return kDNSServiceErr_BadReference;
		}

	do
		{
		CallbackHeader cbh;
		char *data;
	
		// return NoError on EWOULDBLOCK. This will handle the case
		// where a non-blocking socket is told there is data, but it was a false positive.
		// On error, read_all will write a message to syslog for us, so don't need to duplicate that here
		// Note: If we want to properly support using non-blocking sockets in the future
		int result = read_all(sdRef->sockfd, (void *)&cbh.ipc_hdr, sizeof(cbh.ipc_hdr));
		if (result == read_all_fail)
			{
			// Set the ProcessReply to NULL before callback as the sdRef can get deallocated
			// in the callback.
			sdRef->ProcessReply = NULL;
#if _DNS_SD_LIBDISPATCH
			// Call the callbacks with an error if using the dispatch API, as DNSServiceProcessResult
			// is not called by the application and hence need to communicate the error. Cancel the
			// source so that we don't get any more events
			if (sdRef->disp_source)
				{
				dispatch_source_cancel(sdRef->disp_source);
				dispatch_release(sdRef->disp_source);
				sdRef->disp_source = NULL;
				CallbackWithError(sdRef, kDNSServiceErr_ServiceNotRunning);
				}
#endif
			// Don't touch sdRef anymore as it might have been deallocated
			return kDNSServiceErr_ServiceNotRunning;
			}
		else if (result == read_all_wouldblock)
			{
			if (morebytes && sdRef->logcounter < 100)
				{
				sdRef->logcounter++;
				syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult error: select indicated data was waiting but read_all returned EWOULDBLOCK");
				}
			return kDNSServiceErr_NoError;
			}
	
		ConvertHeaderBytes(&cbh.ipc_hdr);
		if (cbh.ipc_hdr.version != VERSION)
			{
			syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult daemon version %d does not match client version %d", cbh.ipc_hdr.version, VERSION);
			sdRef->ProcessReply = NULL;
			return kDNSServiceErr_Incompatible;
			}
	
		data = malloc(cbh.ipc_hdr.datalen);
		if (!data) return kDNSServiceErr_NoMemory;
		if (read_all(sdRef->sockfd, data, cbh.ipc_hdr.datalen) < 0) // On error, read_all will write a message to syslog for us
			{
			// Set the ProcessReply to NULL before callback as the sdRef can get deallocated
			// in the callback.
			sdRef->ProcessReply = NULL;
#if _DNS_SD_LIBDISPATCH
			// Call the callbacks with an error if using the dispatch API, as DNSServiceProcessResult
			// is not called by the application and hence need to communicate the error. Cancel the
			// source so that we don't get any more events
			if (sdRef->disp_source)
				{
				dispatch_source_cancel(sdRef->disp_source);
				dispatch_release(sdRef->disp_source);
				sdRef->disp_source = NULL;
				CallbackWithError(sdRef, kDNSServiceErr_ServiceNotRunning);
				}
#endif
			// Don't touch sdRef anymore as it might have been deallocated
			free(data);
			return kDNSServiceErr_ServiceNotRunning;
			}
		else
			{
			const char *ptr = data;
			cbh.cb_flags     = get_flags     (&ptr, data + cbh.ipc_hdr.datalen);
			cbh.cb_interface = get_uint32    (&ptr, data + cbh.ipc_hdr.datalen);
			cbh.cb_err       = get_error_code(&ptr, data + cbh.ipc_hdr.datalen);

			// CAUTION: We have to handle the case where the client calls DNSServiceRefDeallocate from within the callback function.
			// To do this we set moreptr to point to morebytes. If the client does call DNSServiceRefDeallocate(),
			// then that routine will clear morebytes for us, and cause us to exit our loop.
			morebytes = more_bytes(sdRef->sockfd);
			if (morebytes)
				{
				cbh.cb_flags |= kDNSServiceFlagsMoreComing;
				sdRef->moreptr = &morebytes;
				}
			if (ptr) sdRef->ProcessReply(sdRef, &cbh, ptr, data + cbh.ipc_hdr.datalen);
			// Careful code here:
			// If morebytes is non-zero, that means we set sdRef->moreptr above, and the operation was not
			// cancelled out from under us, so now we need to clear sdRef->moreptr so we don't leave a stray
			// dangling pointer pointing to a long-gone stack variable.
			// If morebytes is zero, then one of two thing happened:
			// (a) morebytes was 0 above, so we didn't set sdRef->moreptr, so we don't need to clear it
			// (b) morebytes was 1 above, and we set sdRef->moreptr, but the operation was cancelled (with DNSServiceRefDeallocate()),
			//     so we MUST NOT try to dereference our stale sdRef pointer.
			if (morebytes) sdRef->moreptr = NULL;
			}
		free(data);
		} while (morebytes);

	return kDNSServiceErr_NoError;
	}

void DNSSD_API DNSServiceRefDeallocate(DNSServiceRef sdRef)
	{
	if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefDeallocate called with NULL DNSServiceRef"); return; }

	if (!DNSServiceRefValid(sdRef))		// Also verifies dnssd_SocketValid(sdRef->sockfd) for us too
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefDeallocate called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return;
		}

	// If we're in the middle of a DNSServiceProcessResult() invocation for this DNSServiceRef, clear its morebytes flag to break it out of its while loop
	if (sdRef->moreptr) *(sdRef->moreptr) = 0;

	if (sdRef->primary)		// If this is a subordinate DNSServiceOp, just send a 'stop' command
		{
		DNSServiceOp **p = &sdRef->primary->next;
		while (*p && *p != sdRef) p = &(*p)->next;
		if (*p)
			{
			char *ptr;
			size_t len = 0;
			ipc_msg_hdr *hdr = create_hdr(cancel_request, &len, &ptr, 0, sdRef);
			if (hdr)
				{
				ConvertHeaderBytes(hdr);
				write_all(sdRef->sockfd, (char *)hdr, len);
				free(hdr);
				}
			*p = sdRef->next;
			FreeDNSServiceOp(sdRef);
			}
		}
	else					// else, make sure to terminate all subordinates as well
		{
#if _DNS_SD_LIBDISPATCH
		// The cancel handler will close the fd if a dispatch source has been set
		if (sdRef->disp_source)
			{
			// By setting the ProcessReply to NULL, we make sure that we never call
			// the application callbacks ever, after returning from this function. We
			// assume that DNSServiceRefDeallocate is called from the serial queue
			// that was passed to DNSServiceSetDispatchQueue. Hence, dispatch_source_cancel
			// should cancel all the blocks on the queue and hence there should be no more
			// callbacks when we return from this function. Setting ProcessReply to NULL
			// provides extra protection.
			sdRef->ProcessReply = NULL;
			dispatch_source_cancel(sdRef->disp_source);
			dispatch_release(sdRef->disp_source);
			sdRef->disp_source = NULL;
			}
			// if disp_queue is set, it means it used the DNSServiceSetDispatchQueue API. In that case,
			// when the source was cancelled, the fd was closed in the handler. Currently the source
			// is cancelled only when the mDNSResponder daemon dies
		else if (!sdRef->disp_queue) dnssd_close(sdRef->sockfd);
#else
		dnssd_close(sdRef->sockfd);
#endif
		// Free DNSRecords added in DNSRegisterRecord if they have not
		// been freed in DNSRemoveRecord
		while (sdRef)
			{
			DNSServiceOp *p = sdRef;
			sdRef = sdRef->next;
			FreeDNSServiceOp(p);
			}
		}
	}

DNSServiceErrorType DNSSD_API DNSServiceGetProperty(const char *property, void *result, uint32_t *size)
	{
	char *ptr;
	size_t len = strlen(property) + 1;
	ipc_msg_hdr *hdr;
	DNSServiceOp *tmp;
	uint32_t actualsize;

	DNSServiceErrorType err = ConnectToServer(&tmp, 0, getproperty_request, NULL, NULL, NULL);
	if (err) return err;

	hdr = create_hdr(getproperty_request, &len, &ptr, 0, tmp);
	if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

	put_string(property, &ptr);
	err = deliver_request(hdr, tmp);		// Will free hdr for us
	if (read_all(tmp->sockfd, (char*)&actualsize, (int)sizeof(actualsize)) < 0)
		{ DNSServiceRefDeallocate(tmp); return kDNSServiceErr_ServiceNotRunning; }

	actualsize = ntohl(actualsize);
	if (read_all(tmp->sockfd, (char*)result, actualsize < *size ? actualsize : *size) < 0)
		{ DNSServiceRefDeallocate(tmp); return kDNSServiceErr_ServiceNotRunning; }
	DNSServiceRefDeallocate(tmp);

	// Swap version result back to local process byte order
	if (!strcmp(property, kDNSServiceProperty_DaemonVersion) && *size >= 4)
		*(uint32_t*)result = ntohl(*(uint32_t*)result);

	*size = actualsize;
	return kDNSServiceErr_NoError;
	}

static void handle_resolve_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *end)
	{
	char fullname[kDNSServiceMaxDomainName];
	char target[kDNSServiceMaxDomainName];
	uint16_t txtlen;
	union { uint16_t s; u_char b[2]; } port;
	unsigned char *txtrecord;

	get_string(&data, end, fullname, kDNSServiceMaxDomainName);
	get_string(&data, end, target,   kDNSServiceMaxDomainName);
	if (!data || data + 2 > end) goto fail;

	port.b[0] = *data++;
	port.b[1] = *data++;
	txtlen = get_uint16(&data, end);
	txtrecord = (unsigned char *)get_rdata(&data, end, txtlen);

	if (!data) goto fail;
	((DNSServiceResolveReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, fullname, target, port.s, txtlen, txtrecord, sdr->AppContext);
	return;
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
fail:
	syslog(LOG_WARNING, "dnssd_clientstub handle_resolve_response: error reading result from daemon");
	}

DNSServiceErrorType DNSSD_API DNSServiceResolve
	(
	DNSServiceRef                 *sdRef,
	DNSServiceFlags               flags,
	uint32_t                      interfaceIndex,
	const char                    *name,
	const char                    *regtype,
	const char                    *domain,
	DNSServiceResolveReply        callBack,
	void                          *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err;

	if (!name || !regtype || !domain || !callBack) return kDNSServiceErr_BadParam;

	// Need a real InterfaceID for WakeOnResolve
	if ((flags & kDNSServiceFlagsWakeOnResolve) != 0 &&
		((interfaceIndex == kDNSServiceInterfaceIndexAny) ||
		(interfaceIndex == kDNSServiceInterfaceIndexLocalOnly) ||
		(interfaceIndex == kDNSServiceInterfaceIndexUnicast) ||
		(interfaceIndex == kDNSServiceInterfaceIndexP2P)))
		{
		return kDNSServiceErr_BadParam;
		}
		
	err = ConnectToServer(sdRef, flags, resolve_request, handle_resolve_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	// Calculate total message length
	len = sizeof(flags);
	len += sizeof(interfaceIndex);
	len += strlen(name) + 1;
	len += strlen(regtype) + 1;
	len += strlen(domain) + 1;

	hdr = create_hdr(resolve_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(name, &ptr);
	put_string(regtype, &ptr);
	put_string(domain, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

static void handle_query_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	uint32_t ttl;
	char name[kDNSServiceMaxDomainName];
	uint16_t rrtype, rrclass, rdlen;
	const char *rdata;

	get_string(&data, end, name, kDNSServiceMaxDomainName);
	rrtype  = get_uint16(&data, end);
	rrclass = get_uint16(&data, end);
	rdlen   = get_uint16(&data, end);
	rdata   = get_rdata(&data, end, rdlen);
	ttl     = get_uint32(&data, end);

	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_query_response: error reading result from daemon");
	else ((DNSServiceQueryRecordReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, name, rrtype, rrclass, rdlen, rdata, ttl, sdr->AppContext);
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSServiceQueryRecord
	(
	DNSServiceRef              *sdRef,
	DNSServiceFlags             flags,
	uint32_t                    interfaceIndex,
	const char                 *name,
	uint16_t                    rrtype,
	uint16_t                    rrclass,
	DNSServiceQueryRecordReply  callBack,
	void                       *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err = ConnectToServer(sdRef, flags, query_request, handle_query_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	if (!name) name = "\0";

	// Calculate total message length
	len = sizeof(flags);
	len += sizeof(uint32_t);  // interfaceIndex
	len += strlen(name) + 1;
	len += 2 * sizeof(uint16_t);  // rrtype, rrclass

	hdr = create_hdr(query_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(name, &ptr);
	put_uint16(rrtype, &ptr);
	put_uint16(rrclass, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

static void handle_addrinfo_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	char hostname[kDNSServiceMaxDomainName];
	uint16_t rrtype, rrclass, rdlen;
	const char *rdata;
	uint32_t ttl;

	get_string(&data, end, hostname, kDNSServiceMaxDomainName);
	rrtype  = get_uint16(&data, end);
	rrclass = get_uint16(&data, end);
	rdlen   = get_uint16(&data, end);
	rdata   = get_rdata (&data, end, rdlen);
	ttl     = get_uint32(&data, end);

	// We only generate client callbacks for A and AAAA results (including NXDOMAIN results for
	// those types, if the client has requested those with the kDNSServiceFlagsReturnIntermediates).
	// Other result types, specifically CNAME referrals, are not communicated to the client, because
	// the DNSServiceGetAddrInfoReply interface doesn't have any meaningful way to communiate CNAME referrals.
	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_addrinfo_response: error reading result from daemon");
	else if (rrtype == kDNSServiceType_A || rrtype == kDNSServiceType_AAAA)
		{
		struct sockaddr_in  sa4;
		struct sockaddr_in6 sa6;
		const struct sockaddr *const sa = (rrtype == kDNSServiceType_A) ? (struct sockaddr*)&sa4 : (struct sockaddr*)&sa6;
		if (rrtype == kDNSServiceType_A)
			{
			memset(&sa4, 0, sizeof(sa4));
			#ifndef NOT_HAVE_SA_LEN
			sa4.sin_len = sizeof(struct sockaddr_in);
			#endif
			sa4.sin_family = AF_INET;
			//  sin_port   = 0;
			if (!cbh->cb_err) memcpy(&sa4.sin_addr, rdata, rdlen);
			}
		else
			{
			memset(&sa6, 0, sizeof(sa6));
			#ifndef NOT_HAVE_SA_LEN
			sa6.sin6_len = sizeof(struct sockaddr_in6);
			#endif
			sa6.sin6_family     = AF_INET6;
			//  sin6_port     = 0;
			//  sin6_flowinfo = 0;
			//  sin6_scope_id = 0;
			if (!cbh->cb_err)
				{
				memcpy(&sa6.sin6_addr, rdata, rdlen);
				if (IN6_IS_ADDR_LINKLOCAL(&sa6.sin6_addr)) sa6.sin6_scope_id = cbh->cb_interface;
				}
			}
		((DNSServiceGetAddrInfoReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, hostname, sa, ttl, sdr->AppContext);
		// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
		}
	}

DNSServiceErrorType DNSSD_API DNSServiceGetAddrInfo
	(
	DNSServiceRef                    *sdRef,
	DNSServiceFlags                  flags,
	uint32_t                         interfaceIndex,
	uint32_t                         protocol,
	const char                       *hostname,
	DNSServiceGetAddrInfoReply       callBack,
	void                             *context          /* may be NULL */
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err;

	if (!hostname) return kDNSServiceErr_BadParam;

	err = ConnectToServer(sdRef, flags, addrinfo_request, handle_addrinfo_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	// Calculate total message length
	len = sizeof(flags);
	len += sizeof(uint32_t);      // interfaceIndex
	len += sizeof(uint32_t);      // protocol
	len += strlen(hostname) + 1;

	hdr = create_hdr(addrinfo_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_uint32(protocol, &ptr);
	put_string(hostname, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}
	
static void handle_browse_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	char replyName[256], replyType[kDNSServiceMaxDomainName], replyDomain[kDNSServiceMaxDomainName];
	get_string(&data, end, replyName, 256);
	get_string(&data, end, replyType, kDNSServiceMaxDomainName);
	get_string(&data, end, replyDomain, kDNSServiceMaxDomainName);
	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_browse_response: error reading result from daemon");
	else ((DNSServiceBrowseReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, replyName, replyType, replyDomain, sdr->AppContext);
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSServiceBrowse
	(
	DNSServiceRef         *sdRef,
	DNSServiceFlags        flags,
	uint32_t               interfaceIndex,
	const char            *regtype,
	const char            *domain,
	DNSServiceBrowseReply  callBack,
	void                  *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err = ConnectToServer(sdRef, flags, browse_request, handle_browse_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	if (!domain) domain = "";
	len = sizeof(flags);
	len += sizeof(interfaceIndex);
	len += strlen(regtype) + 1;
	len += strlen(domain) + 1;

	hdr = create_hdr(browse_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(regtype, &ptr);
	put_string(domain, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

static void handle_hostname_changed_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	char replyHostname[256];

	get_string(&data, end, replyHostname, sizeof(replyHostname));
	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_hostname_changed_response: error reading result from daemon");
	else ((DNSHostnameChangedReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_err, replyHostname, sdr->AppContext);
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSSetHostname
	(
	DNSServiceRef           *sdRef,
	DNSServiceFlags          flags,
	const char              *hostname,
	DNSHostnameChangedReply  callBack,
	void                    *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err = ConnectToServer(sdRef, flags, sethost_request, handle_hostname_changed_response, callBack, context);
	if (err) return err;
	len = sizeof(flags);
	len += strlen(hostname) + 1;

	hdr = create_hdr(sethost_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_string(hostname, &ptr);
	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

DNSServiceErrorType DNSSD_API DNSServiceSetDefaultDomainForUser(DNSServiceFlags flags, const char *domain);
DNSServiceErrorType DNSSD_API DNSServiceSetDefaultDomainForUser(DNSServiceFlags flags, const char *domain)
	{
	DNSServiceOp *tmp;
	char *ptr;
	size_t len = sizeof(flags) + strlen(domain) + 1;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err = ConnectToServer(&tmp, 0, setdomain_request, NULL, NULL, NULL);
	if (err) return err;

	hdr = create_hdr(setdomain_request, &len, &ptr, 0, tmp);
	if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_string(domain, &ptr);
	err = deliver_request(hdr, tmp);		// Will free hdr for us
	DNSServiceRefDeallocate(tmp);
	return err;
	}

static void handle_regservice_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	char name[256], regtype[kDNSServiceMaxDomainName], domain[kDNSServiceMaxDomainName];
	get_string(&data, end, name, 256);
	get_string(&data, end, regtype, kDNSServiceMaxDomainName);
	get_string(&data, end, domain,  kDNSServiceMaxDomainName);
	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_regservice_response: error reading result from daemon");
	else ((DNSServiceRegisterReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_err, name, regtype, domain, sdr->AppContext);
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSServiceRegister
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *name,
	const char                          *regtype,
	const char                          *domain,
	const char                          *host,
	uint16_t                            PortInNetworkByteOrder,
	uint16_t                            txtLen,
	const void                          *txtRecord,
	DNSServiceRegisterReply             callBack,
	void                                *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err;
	union { uint16_t s; u_char b[2]; } port = { PortInNetworkByteOrder };

	if (!name) name = "";
	if (!regtype) return kDNSServiceErr_BadParam;
	if (!domain) domain = "";
	if (!host) host = "";
	if (!txtRecord) txtRecord = (void*)"";

	// No callback must have auto-rename
	if (!callBack && (flags & kDNSServiceFlagsNoAutoRename)) return kDNSServiceErr_BadParam;

	err = ConnectToServer(sdRef, flags, reg_service_request, callBack ? handle_regservice_response : NULL, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	len = sizeof(DNSServiceFlags);
	len += sizeof(uint32_t);  // interfaceIndex
	len += strlen(name) + strlen(regtype) + strlen(domain) + strlen(host) + 4;
	len += 2 * sizeof(uint16_t);  // port, txtLen
	len += txtLen;

	hdr = create_hdr(reg_service_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }
	if (!callBack) hdr->ipc_flags |= IPC_FLAGS_NOREPLY;

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(name, &ptr);
	put_string(regtype, &ptr);
	put_string(domain, &ptr);
	put_string(host, &ptr);
	*ptr++ = port.b[0];
	*ptr++ = port.b[1];
	put_uint16(txtLen, &ptr);
	put_rdata(txtLen, txtRecord, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

static void handle_enumeration_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	char domain[kDNSServiceMaxDomainName];
	get_string(&data, end, domain, kDNSServiceMaxDomainName);
	if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_enumeration_response: error reading result from daemon");
	else ((DNSServiceDomainEnumReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, domain, sdr->AppContext);
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSServiceEnumerateDomains
	(
	DNSServiceRef             *sdRef,
	DNSServiceFlags            flags,
	uint32_t                   interfaceIndex,
	DNSServiceDomainEnumReply  callBack,
	void                      *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err;

	int f1 = (flags & kDNSServiceFlagsBrowseDomains) != 0;
	int f2 = (flags & kDNSServiceFlagsRegistrationDomains) != 0;
	if (f1 + f2 != 1) return kDNSServiceErr_BadParam;

	err = ConnectToServer(sdRef, flags, enumeration_request, handle_enumeration_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	len = sizeof(DNSServiceFlags);
	len += sizeof(uint32_t);

	hdr = create_hdr(enumeration_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

static void ConnectionResponse(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *const data, const char *const end)
	{
	DNSRecordRef rref = cbh->ipc_hdr.client_context.context;
	(void)data; // Unused

	//printf("ConnectionResponse got %d\n", cbh->ipc_hdr.op);
	if (cbh->ipc_hdr.op != reg_record_reply_op)
		{
		// When using kDNSServiceFlagsShareConnection, need to search the list of associated DNSServiceOps
		// to find the one this response is intended for, and then call through to its ProcessReply handler.
		// We start with our first subordinate DNSServiceRef -- don't want to accidentally match the parent DNSServiceRef.
		DNSServiceOp *op = sdr->next;
		while (op && (op->uid.u32[0] != cbh->ipc_hdr.client_context.u32[0] || op->uid.u32[1] != cbh->ipc_hdr.client_context.u32[1]))
			op = op->next;
		// Note: We may sometimes not find a matching DNSServiceOp, in the case where the client has
		// cancelled the subordinate DNSServiceOp, but there are still messages in the pipeline from the daemon
		if (op && op->ProcessReply) op->ProcessReply(op, cbh, data, end);
		// WARNING: Don't touch op or sdr after this -- client may have called DNSServiceRefDeallocate
		return;
		}

	if (sdr->op == connection_request)
		rref->AppCallback(rref->sdr, rref, cbh->cb_flags, cbh->cb_err, rref->AppContext);
	else
		{
		syslog(LOG_WARNING, "dnssd_clientstub ConnectionResponse: sdr->op != connection_request");
		rref->AppCallback(rref->sdr, rref, 0, kDNSServiceErr_Unknown, rref->AppContext);
		}
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
	}

DNSServiceErrorType DNSSD_API DNSServiceCreateConnection(DNSServiceRef *sdRef)
	{
	char *ptr;
	size_t len = 0;
	ipc_msg_hdr *hdr;
	DNSServiceErrorType err = ConnectToServer(sdRef, 0, connection_request, ConnectionResponse, NULL, NULL);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL
	
	hdr = create_hdr(connection_request, &len, &ptr, 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

DNSServiceErrorType DNSSD_API DNSServiceRegisterRecord
	(
	DNSServiceRef                  sdRef,
	DNSRecordRef                  *RecordRef,
	DNSServiceFlags                flags,
	uint32_t                       interfaceIndex,
	const char                    *fullname,
	uint16_t                       rrtype,
	uint16_t                       rrclass,
	uint16_t                       rdlen,
	const void                    *rdata,
	uint32_t                       ttl,
	DNSServiceRegisterRecordReply  callBack,
	void                          *context
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr = NULL;
	DNSRecordRef rref = NULL;
	DNSRecord **p;
	int f1 = (flags & kDNSServiceFlagsShared) != 0;
	int f2 = (flags & kDNSServiceFlagsUnique) != 0;
	if (f1 + f2 != 1) return kDNSServiceErr_BadParam;

	if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return kDNSServiceErr_BadReference;
		}

	if (sdRef->op != connection_request)
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with non-DNSServiceCreateConnection DNSServiceRef %p %d", sdRef, sdRef->op);
		return kDNSServiceErr_BadReference;
		}

	*RecordRef = NULL;

	len = sizeof(DNSServiceFlags);
	len += 2 * sizeof(uint32_t);  // interfaceIndex, ttl
	len += 3 * sizeof(uint16_t);  // rrtype, rrclass, rdlen
	len += strlen(fullname) + 1;
	len += rdlen;

	hdr = create_hdr(reg_record_request, &len, &ptr, 1, sdRef);
	if (!hdr) return kDNSServiceErr_NoMemory;

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(fullname, &ptr);
	put_uint16(rrtype, &ptr);
	put_uint16(rrclass, &ptr);
	put_uint16(rdlen, &ptr);
	put_rdata(rdlen, rdata, &ptr);
	put_uint32(ttl, &ptr);

	rref = malloc(sizeof(DNSRecord));
	if (!rref) { free(hdr); return kDNSServiceErr_NoMemory; }
	rref->AppContext = context;
	rref->AppCallback = callBack;
	rref->record_index = sdRef->max_index++;
	rref->sdr = sdRef;
	rref->recnext = NULL;
	*RecordRef = rref;
	hdr->client_context.context = rref;
	hdr->reg_index = rref->record_index;

	p = &(sdRef)->rec;
	while (*p) p = &(*p)->recnext;
	*p = rref;

	return deliver_request(hdr, sdRef);		// Will free hdr for us
	}

// sdRef returned by DNSServiceRegister()
DNSServiceErrorType DNSSD_API DNSServiceAddRecord
	(
	DNSServiceRef    sdRef,
	DNSRecordRef    *RecordRef,
	DNSServiceFlags  flags,
	uint16_t         rrtype,
	uint16_t         rdlen,
	const void      *rdata,
	uint32_t         ttl
	)
	{
	ipc_msg_hdr *hdr;
	size_t len = 0;
	char *ptr;
	DNSRecordRef rref;
	DNSRecord **p;

	if (!sdRef)     { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with NULL DNSServiceRef");        return kDNSServiceErr_BadParam; }
	if (!RecordRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with NULL DNSRecordRef pointer"); return kDNSServiceErr_BadParam; }
	if (sdRef->op != reg_service_request)
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with non-DNSServiceRegister DNSServiceRef %p %d", sdRef, sdRef->op);
		return kDNSServiceErr_BadReference;
		}

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return kDNSServiceErr_BadReference;
		}

	*RecordRef = NULL;

	len += 2 * sizeof(uint16_t);  // rrtype, rdlen
	len += rdlen;
	len += sizeof(uint32_t);
	len += sizeof(DNSServiceFlags);

	hdr = create_hdr(add_record_request, &len, &ptr, 1, sdRef);
	if (!hdr) return kDNSServiceErr_NoMemory;
	put_flags(flags, &ptr);
	put_uint16(rrtype, &ptr);
	put_uint16(rdlen, &ptr);
	put_rdata(rdlen, rdata, &ptr);
	put_uint32(ttl, &ptr);

	rref = malloc(sizeof(DNSRecord));
	if (!rref) { free(hdr); return kDNSServiceErr_NoMemory; }
	rref->AppContext = NULL;
	rref->AppCallback = NULL;
	rref->record_index = sdRef->max_index++;
	rref->sdr = sdRef;
	rref->recnext = NULL;
	*RecordRef = rref;
	hdr->reg_index = rref->record_index;

	p = &(sdRef)->rec;
	while (*p) p = &(*p)->recnext;
	*p = rref;

	return deliver_request(hdr, sdRef);		// Will free hdr for us
	}

// DNSRecordRef returned by DNSServiceRegisterRecord or DNSServiceAddRecord
DNSServiceErrorType DNSSD_API DNSServiceUpdateRecord
	(
	DNSServiceRef    sdRef,
	DNSRecordRef     RecordRef,
	DNSServiceFlags  flags,
	uint16_t         rdlen,
	const void      *rdata,
	uint32_t         ttl
	)
	{
	ipc_msg_hdr *hdr;
	size_t len = 0;
	char *ptr;

	if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceUpdateRecord called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceUpdateRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return kDNSServiceErr_BadReference;
		}

	// Note: RecordRef is allowed to be NULL

	len += sizeof(uint16_t);
	len += rdlen;
	len += sizeof(uint32_t);
	len += sizeof(DNSServiceFlags);

	hdr = create_hdr(update_record_request, &len, &ptr, 1, sdRef);
	if (!hdr) return kDNSServiceErr_NoMemory;
	hdr->reg_index = RecordRef ? RecordRef->record_index : TXT_RECORD_INDEX;
	put_flags(flags, &ptr);
	put_uint16(rdlen, &ptr);
	put_rdata(rdlen, rdata, &ptr);
	put_uint32(ttl, &ptr);
	return deliver_request(hdr, sdRef);		// Will free hdr for us
	}

DNSServiceErrorType DNSSD_API DNSServiceRemoveRecord
	(
	DNSServiceRef    sdRef,
	DNSRecordRef     RecordRef,
	DNSServiceFlags  flags
	)
	{
	ipc_msg_hdr *hdr;
	size_t len = 0;
	char *ptr;
	DNSServiceErrorType err;

	if (!sdRef)            { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }
	if (!RecordRef)        { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with NULL DNSRecordRef");  return kDNSServiceErr_BadParam; }
	if (!sdRef->max_index) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with bad DNSServiceRef");  return kDNSServiceErr_BadReference; }

	if (!DNSServiceRefValid(sdRef))
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
		return kDNSServiceErr_BadReference;
		}

	len += sizeof(flags);
	hdr = create_hdr(remove_record_request, &len, &ptr, 1, sdRef);
	if (!hdr) return kDNSServiceErr_NoMemory;
	hdr->reg_index = RecordRef->record_index;
	put_flags(flags, &ptr);
	err = deliver_request(hdr, sdRef);		// Will free hdr for us
	if (!err)
		{
		// This RecordRef could have been allocated in DNSServiceRegisterRecord or DNSServiceAddRecord.
		// If so, delink from the list before freeing
		DNSRecord **p = &sdRef->rec;
		while (*p && *p != RecordRef) p = &(*p)->recnext;
		if (*p) *p = RecordRef->recnext;
		free(RecordRef);
		}
	return err;
	}

DNSServiceErrorType DNSSD_API DNSServiceReconfirmRecord
	(
	DNSServiceFlags  flags,
	uint32_t         interfaceIndex,
	const char      *fullname,
	uint16_t         rrtype,
	uint16_t         rrclass,
	uint16_t         rdlen,
	const void      *rdata
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	DNSServiceOp *tmp;

	DNSServiceErrorType err = ConnectToServer(&tmp, flags, reconfirm_record_request, NULL, NULL, NULL);
	if (err) return err;

	len = sizeof(DNSServiceFlags);
	len += sizeof(uint32_t);
	len += strlen(fullname) + 1;
	len += 3 * sizeof(uint16_t);
	len += rdlen;
	hdr = create_hdr(reconfirm_record_request, &len, &ptr, 0, tmp);
	if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_string(fullname, &ptr);
	put_uint16(rrtype, &ptr);
	put_uint16(rrclass, &ptr);
	put_uint16(rdlen, &ptr);
	put_rdata(rdlen, rdata, &ptr);

	err = deliver_request(hdr, tmp);		// Will free hdr for us
	DNSServiceRefDeallocate(tmp);
	return err;
	}

static void handle_port_mapping_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const char *data, const char *const end)
	{
	union { uint32_t l; u_char b[4]; } addr;
	uint8_t protocol;
	union { uint16_t s; u_char b[2]; } internalPort;
	union { uint16_t s; u_char b[2]; } externalPort;
	uint32_t ttl;

	if (!data || data + 13 > end) goto fail;

	addr        .b[0] = *data++;
	addr        .b[1] = *data++;
	addr        .b[2] = *data++;
	addr        .b[3] = *data++;
	protocol          = *data++;
	internalPort.b[0] = *data++;
	internalPort.b[1] = *data++;
	externalPort.b[0] = *data++;
	externalPort.b[1] = *data++;
	ttl               = get_uint32(&data, end);
	if (!data) goto fail;

	((DNSServiceNATPortMappingReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, addr.l, protocol, internalPort.s, externalPort.s, ttl, sdr->AppContext);
	return;
	// MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function

fail:
	syslog(LOG_WARNING, "dnssd_clientstub handle_port_mapping_response: error reading result from daemon");
	}

DNSServiceErrorType DNSSD_API DNSServiceNATPortMappingCreate
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	uint32_t                            protocol,     /* TCP and/or UDP */
	uint16_t                            internalPortInNetworkByteOrder,
	uint16_t                            externalPortInNetworkByteOrder,
	uint32_t                            ttl,          /* time to live in seconds */
	DNSServiceNATPortMappingReply       callBack,
	void                                *context      /* may be NULL */
	)
	{
	char *ptr;
	size_t len;
	ipc_msg_hdr *hdr;
	union { uint16_t s; u_char b[2]; } internalPort = { internalPortInNetworkByteOrder };
	union { uint16_t s; u_char b[2]; } externalPort = { externalPortInNetworkByteOrder };

	DNSServiceErrorType err = ConnectToServer(sdRef, flags, port_mapping_request, handle_port_mapping_response, callBack, context);
	if (err) return err;	// On error ConnectToServer leaves *sdRef set to NULL

	len = sizeof(flags);
	len += sizeof(interfaceIndex);
	len += sizeof(protocol);
	len += sizeof(internalPort);
	len += sizeof(externalPort);
	len += sizeof(ttl);

	hdr = create_hdr(port_mapping_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
	if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

	put_flags(flags, &ptr);
	put_uint32(interfaceIndex, &ptr);
	put_uint32(protocol, &ptr);
	*ptr++ = internalPort.b[0];
	*ptr++ = internalPort.b[1];
	*ptr++ = externalPort.b[0];
	*ptr++ = externalPort.b[1];
	put_uint32(ttl, &ptr);

	err = deliver_request(hdr, *sdRef);		// Will free hdr for us
	if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
	return err;
	}

#if _DNS_SD_LIBDISPATCH
DNSServiceErrorType DNSSD_API DNSServiceSetDispatchQueue
	(
	DNSServiceRef service,
	dispatch_queue_t queue
	)
	{
	int dnssd_fd  = DNSServiceRefSockFD(service);
	if (dnssd_fd == dnssd_InvalidSocket) return kDNSServiceErr_BadParam;
	if (!queue)
		{
		syslog(LOG_WARNING, "dnssd_clientstub: DNSServiceSetDispatchQueue dispatch queue NULL");
		return kDNSServiceErr_BadParam;
		}
	if (service->disp_queue)
		{
		syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSetDispatchQueue dispatch queue set already");
		return kDNSServiceErr_BadParam;
		}
	if (service->disp_source)
		{
		syslog(LOG_WARNING, "DNSServiceSetDispatchQueue dispatch source set already");
		return kDNSServiceErr_BadParam;
		}
	service->disp_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, dnssd_fd, 0, queue);
	if (!service->disp_source)
		{
		syslog(LOG_WARNING, "DNSServiceSetDispatchQueue dispatch_source_create failed");
		return kDNSServiceErr_NoMemory;
		}
	service->disp_queue = queue;
	dispatch_source_set_event_handler(service->disp_source, ^{DNSServiceProcessResult(service);});
	dispatch_source_set_cancel_handler(service->disp_source, ^{dnssd_close(dnssd_fd);});
	dispatch_resume(service->disp_source);
	return kDNSServiceErr_NoError;
	}
#endif // _DNS_SD_LIBDISPATCH
