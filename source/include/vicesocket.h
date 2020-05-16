/*! \file vicesocket.h \n
 *  \author Spiro Trikaliotis\n
 *  \brief  Abstraction from network sockets.
 *
 * socket.h - Abstraction from network sockets.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_SOCKET_H
#define VICE_SOCKET_H

#include "types.h"
#include "socketimpl.h"

/*! \internal \brief Access to socket addresses
 *
 * This union is used to access socket addresses.
 * It replaces the casts needed otherwise.
 *
 * Furthermore, it ensures we have always enough
 * memory for any type needed. (sizeof struct sockaddr_in6
 * is bigger than struct sockaddr).
 *
 * Note, however, that there is no consensus if the C standard
 * guarantees that all union members start at the same
 * address. There are arguments for and against this.
 *
 * However, in practice, this approach works.
 */
union socket_addresses_u {
    struct sockaddr generic;     /*!< the generic type needed for calling the socket API */

#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    struct sockaddr_un local;    /*!< an Unix Domain Socket (file system) socket address */
#endif

    struct sockaddr_in ipv4;     /*!< an IPv4 socket address */

#ifdef HAVE_IPV6
    struct sockaddr_in6 ipv6;    /*!< an IPv6 socket address */
#endif
};

/*! \internal \brief opaque structure describing an address for use with socket functions
 *
 */
struct vice_network_socket_address_s {
    unsigned int used;      /*!< 1 if this entry is being used, 0 else.
                             * This is used for debugging the buffer
                             * allocation strategy.
                             */
    int domain;             /*!< the address family (AF_INET, ...) of this address */
    int protocol;           /*!< the protocol of this address. This can be used to distinguish between different types of an address family. */
#ifdef HAVE_SOCKLEN_T
    socklen_t len;          /*!< the length of the socket address */
#else
    int len;
#endif
    union socket_addresses_u address; /* the socket address */
};

typedef struct vice_network_socket_address_s vice_network_socket_address_t;

/*! \internal \brief opaque structure describing a socket */
struct vice_network_socket_s {
    SOCKET sockfd;           /*!< the socket handle */
    vice_network_socket_address_t address; /*!< place for an address. It is updated on accept(). */
    unsigned int used;      /*!< 1 if this entry is being used, 0 else.
                             * This is used for debugging the buffer
                             * allocation strategy.
                             */
};

typedef struct vice_network_socket_s vice_network_socket_t;

vice_network_socket_t * vice_network_server(const vice_network_socket_address_t * server_address);
vice_network_socket_t * vice_network_client(const vice_network_socket_address_t * server_address);

vice_network_socket_address_t * vice_network_address_generate(const char * address, unsigned short port);
void vice_network_address_close(vice_network_socket_address_t *);

vice_network_socket_t * vice_network_accept(vice_network_socket_t * sockfd);

int vice_network_socket_close(vice_network_socket_t * sockfd);

int vice_network_send(vice_network_socket_t * sockfd, const void * buffer, size_t buffer_length, int flags);
int vice_network_receive(vice_network_socket_t * sockfd, void * buffer, size_t buffer_length, int flags);

int vice_network_select_poll_one(vice_network_socket_t * readsockfd);

int vice_network_get_errorcode(void);

#endif /* VICE_SOCKET_H */
