/*
	ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "ch_test.h"

#include "chprintf.h"

#include "ff.h"


/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static virtual_timer_t tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static event_source_t inserted_event, removed_event;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc( void *p )
{
	BaseBlockDevice *bbdp = p;

	chSysLockFromISR();
	if( cnt > 0 )
	{
		if( blkIsInserted( bbdp ) )
		{
			if( --cnt == 0 )
				chEvtBroadcastI( &inserted_event );
		}
		else
			cnt = POLLING_INTERVAL;
	}
	else
	{
		if( !blkIsInserted( bbdp ) )
		{
			cnt = POLLING_INTERVAL;
			chEvtBroadcastI( &removed_event );
		}
	}
	chVTSetI( &tmr, TIME_MS2I( POLLING_DELAY ), tmrfunc, bbdp );
	chSysUnlockFromISR();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init( void *p )
{
	chEvtObjectInit( &inserted_event );
	chEvtObjectInit( &removed_event );
	chSysLock();
	cnt = POLLING_INTERVAL;
	chVTSetI( &tmr, TIME_MS2I( POLLING_DELAY ), tmrfunc, p );
	chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fs_ready = FALSE;

/* Generic large buffer.*/
static uint8_t fbuff[1024];

static FRESULT scan_files( BaseSequentialStream *chp, char *path )
{
	static FILINFO fno;
	FRESULT res;
	DIR dir;
	size_t i;
	char *fn;

	res = f_opendir( &dir, path );
	if( res == FR_OK )
	{
		i = strlen( path );
		while( ( ( res = f_readdir( &dir, &fno ) ) == FR_OK ) && fno.fname[0] )
		{
			if( FF_FS_RPATH && fno.fname[0] == '.' )
				continue;
			fn = fno.fname;
			if( fno.fattrib & AM_DIR )
			{
				*( path + i ) = '/';
				strcpy( path + i + 1, fn );
				res = scan_files( chp, path );
				*( path + i ) = '\0';
				if( res != FR_OK )
					break;
			}
			else
				chprintf( chp, "%s/%s\r\n", path, fn );
		}
	}
	return res;
}

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

static void cmd_tree( BaseSequentialStream *chp, int argc, char *argv[] )
{
	FRESULT err;
	uint32_t fre_clust, fre_sect, tot_sect;
	FATFS *fsp;

	( void )argv;
	if( argc > 0 )
	{
		chprintf( chp, "Usage: tree\r\n" );
		return;
	}
	if( !fs_ready )
	{
		chprintf( chp, "File System not mounted\r\n" );
		return;
	}
	err = f_getfree( "/", &fre_clust, &fsp );
	if( err != FR_OK )
	{
		chprintf( chp, "FS: f_getfree() failed\r\n" );
		return;
	}
	chprintf( chp,
			  "FS: %lu free clusters with %lu sectors (%lu bytes) per cluster\r\n",
			  fre_clust, ( uint32_t )fsp->csize, ( uint32_t )fsp->csize * 512 );
	chprintf( chp,
			  "    %lu bytes (%lu MB) free of %lu MB\r\n",
			  fre_sect * 512,
			  fre_sect / 2 / 1024,
			  tot_sect / 2 / 1024 );
	fbuff[0] = 0;
	scan_files( chp, ( char * )fbuff );
}

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

// SD I/O Thread
thread_t* sdThread = 0;
static THD_WORKING_AREA( sdThreadArea, 8192 );
static THD_FUNCTION( SDThread, arg )
{
	scan_files( &SD4, "/" );
	/*FRESULT res;
	cmd_tree( &SD6, 0, 0 );
	FIL file;
	res = f_open( &file, "/readme.txt", FA_OPEN_ALWAYS | FA_WRITE );
	uint bw;
	res = f_write( &file, "Kyky epta ", sizeof( "Kyky epta " ), &bw );
	res = f_close( &file );*/

	while( true )
		;
}

/*
 * Card insertion event.
 */
static void InsertHandler( eventid_t id )
{
	FRESULT err;

	( void )id;
	/*
	 * On insertion SDC initialization and FS mount.
	 */
	if( sdcConnect( &SDCD1 ) )
		return;

	err = f_mount( &SDC_FS, "/", 1 );
	//static uint8_t bb[1024];
	//err = f_mkfs( "/", FM_FAT32, 1024, bb, 1024 );
	if( err != FR_OK )
	{
		sdcDisconnect( &SDCD1 );
		return;
	}
	fs_ready = TRUE;

	sdThread = chThdCreateStatic( sdThreadArea, sizeof( sdThreadArea ), NORMALPRIO - 1, SDThread, NULL );
}

/*
 * Card removal event.
 */
static void RemoveHandler( eventid_t id )
{
	( void )id;
	sdcDisconnect( &SDCD1 );
	fs_ready = FALSE;
}

/*
 * Application entry point.
 */
int main( void )
{
	static const evhandler_t evhndl[] = {
	  InsertHandler,
	  RemoveHandler,
	};
	event_listener_t el0, el1;

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	/*
	 * Activates the serial driver 6 and SDC driver 1 using default
	 * configuration.
	 */
	sdStart( &SD4, NULL );
	palSetPadMode( GPIOA, 0, PAL_MODE_ALTERNATE( GPIO_AF_UART4 ) );
	palSetPadMode( GPIOA, 1, PAL_MODE_ALTERNATE( GPIO_AF_UART4 ) );

	palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	sdcStart( &SDCD1, NULL );

	/*
	 * Activates the card insertion monitor.
	 */
	tmr_init( &SDCD1 );

	/*
	 * Normal main() thread activity, handling SD card events.
	 */
	chEvtRegister( &inserted_event, &el0, 0 );
	chEvtRegister( &removed_event, &el1, 1 );
	while( true )
		chEvtDispatch( evhndl, chEvtWaitOneTimeout( ALL_EVENTS, TIME_MS2I( 500 ) ) );
}
