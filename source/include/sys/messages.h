/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

enum {
	/* requests to file-device */
	MSG_FILE_OPEN					= 50,
	MSG_FILE_READ					= 51,
	MSG_FILE_WRITE					= 52,
	MSG_FILE_SIZE					= 53,
	MSG_FILE_CLOSE					= 54,

	/* special device messages */
	MSG_DEV_SHFILE					= 55,
	MSG_DEV_CANCEL					= 56,
	MSG_DEV_CREATSIBL				= 57,

	/* requests to fs */
	MSG_FS_OPEN						= 100,
	MSG_FS_READ						= 101,
	MSG_FS_WRITE					= 102,
	MSG_FS_CLOSE					= 103,
	MSG_FS_SYNCFS					= 104,
	MSG_FS_LINK						= 105,
	MSG_FS_UNLINK					= 106,
	MSG_FS_RENAME					= 107,
	MSG_FS_MKDIR					= 108,
	MSG_FS_RMDIR					= 109,
	MSG_FS_ISTAT					= 110,
	MSG_FS_CHMOD					= 111,
	MSG_FS_CHOWN					= 112,
	MSG_FS_UTIME					= 113,
	MSG_FS_TRUNCATE					= 114,

	/* speaker */
	MSG_SPEAKER_BEEP				= 200,	/* performs a beep */

	/* window manager */
	MSG_WIN_CREATE					= 300,	/* creates a window */
	MSG_WIN_MOVE					= 301,	/* moves a window */
	MSG_WIN_UPDATE					= 302,	/* requests an update of a window */
	MSG_WIN_DESTROY					= 303,	/* destroys a window */
	MSG_WIN_RESIZE					= 304,	/* resizes a window */
	MSG_WIN_ENABLE					= 305,	/* enables the window-manager */
	MSG_WIN_DISABLE					= 306,	/* disables the window-manager */
	MSG_WIN_ADDLISTENER				= 307,	/* announces a listener for CREATE_EV or DESTROY_EV */
	MSG_WIN_REMLISTENER				= 308,	/* removes a listener for CREATE_EV or DESTROY_EV */
	MSG_WIN_SET_ACTIVE				= 309,	/* requests that a window is set to the active one */
	MSG_WIN_ATTACH					= 310,	/* connect an event-channel to a window */
	MSG_WIN_SETMODE					= 311,	/* sets the screen mode */
	MSG_WIN_EVENT					= 312,	/* for all events */

	/* screen */
	MSG_SCR_SETCURSOR				= 500,	/* sets the cursor */
	MSG_SCR_GETMODE					= 501,	/* gets information about the current video-mode */
	MSG_SCR_SETMODE					= 502,	/* sets the video-mode */
	MSG_SCR_GETMODES				= 503,	/* gets all video-modes */
	MSG_SCR_UPDATE					= 504,	/* updates a part of the screen */

	/* vterm */
	MSG_VT_GETFLAG					= 600,	/* gets a flag */
	MSG_VT_SETFLAG					= 601,	/* sets a flag */
	MSG_VT_BACKUP					= 602,	/* backups the screen */
	MSG_VT_RESTORE					= 603,	/* restores the screen */
	MSG_VT_SHELLPID					= 604,	/* gives the vterm the shell-pid */
	MSG_VT_ISVTERM					= 605,	/* dummy message on which only vterm answers with no error */
	MSG_VT_SETMODE					= 606,	/* requests vterm to set the video-mode */

	/* keyboard */
	MSG_KB_EVENT					= 700,	/* events that the keyboard-driver sends */

	/* mouse */
	MSG_MS_EVENT					= 800,	/* events that the mouse-driver sends */

	/* UI manager */
	MSG_UIM_GETKEYMAP				= 900,	/* gets the current keymap path */
	MSG_UIM_SETKEYMAP				= 901,	/* sets a keymap, expects the keymap-path as argument */
	MSG_UIM_EVENT					= 902,	/* the message-id for sending events to the listeners */

	/* PCI */
	MSG_PCI_GET_BY_CLASS			= 1000,	/* searches for a PCI device with given class */
	MSG_PCI_GET_BY_ID				= 1001,	/* searches for a PCI device with given id */
	MSG_PCI_GET_BY_INDEX			= 1002,	/* searches for a PCI device with given index (0..n) */
	MSG_PCI_GET_COUNT				= 1003,	/* gets the number of PCI devices */
	MSG_PCI_READ					= 1004,	/* reads from PCI config space */
	MSG_PCI_WRITE					= 1005,	/* writes to PCI config space */
	MSG_PCI_HAS_CAP					= 1006,	/* checks if a given capability exists for a given device */
	MSG_PCI_ENABLE_MSIS				= 1007,	/* enables MSIs for a given device */

	/* init */
	MSG_INIT_REBOOT					= 1100,	/* requests a reboot */
	MSG_INIT_SHUTDOWN				= 1101,	/* requests a shutdown */
	MSG_INIT_IAMALIVE				= 1102,	/* tells init that the shutdown-request has been received
										 	 * and that you promise to terminate as soon as possible */

	/* NIC */
	MSG_NIC_GETMAC					= 1300,	/* get the MAC address of a NIC */
	MSG_NIC_GETMTU					= 1301,	/* get the MTU of a NIC */

	/* network */
	MSG_NET_LINK_ADD				= 1401,	/* adds a link */
	MSG_NET_LINK_REM				= 1402,	/* removes a link */
	MSG_NET_LINK_CONFIG				= 1403,	/* sets the ip, gateway and subnet-mask of a link */
	MSG_NET_LINK_MAC				= 1404,	/* gets the MAC address of a link */
	MSG_NET_ROUTE_ADD				= 1405,	/* adds an entry to the routing table */
	MSG_NET_ROUTE_REM				= 1406,	/* removes an entry from the routing table */
	MSG_NET_ROUTE_CONFIG			= 1407,	/* sets the status of a routing table entry */
	MSG_NET_ROUTE_GET				= 1408,	/* gets the destination for an IP address */
	MSG_NET_ARP_ADD					= 1409,	/* resolves an IP address and puts it into the ARP table */
	MSG_NET_ARP_REM					= 1410,	/* removes an IP address from the ARP table */

	/* socket */
	MSG_SOCK_CONNECT				= 1500,	/* connects a socket to the other endpoint */
	MSG_SOCK_BIND					= 1501,	/* bind a socket to an address */
	MSG_SOCK_LISTEN					= 1502,	/* puts a socket into listening state */
	MSG_SOCK_RECVFROM				= 1503,	/* receive data from a socket */
	MSG_SOCK_SENDTO					= 1504,	/* send data to a socket */
	MSG_SOCK_ABORT					= 1505,	/* aborts the connection, i.e. sends a RST */

	/* DNS */
	MSG_DNS_RESOLVE					= 1600,	/* resolve a name to an address */
	MSG_DNS_SET_SERVER				= 1601,	/* set the name server */
};

static const size_t IPC_DEF_SIZE	= 256;

/**
 * @param mid the message id
 * @return whether the given message id is a device message, i.e. cannot be sent by the user.
 */
static inline bool isDeviceMsg(msgid_t mid) {
	return mid >= MSG_FILE_OPEN && mid <= MSG_DEV_CREATSIBL;
}
