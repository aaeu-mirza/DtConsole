/*-----------------------------------------------------------------------

PROGRAM: DtConsole.cpp

PURPOSE:
    Open Layers data acquisition example showing how to implement a 
    continuous analog input operation using windows messaging in a 
    console environment.
    
****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "oldaapi.h"

#define CHECKERROR( ecode )                           \
do                                                    \
{                                                     \
   ECODE olStatus;                                    \
   if( OLSUCCESS != ( olStatus = ( ecode ) ) )        \
   {                                                  \
      printf("OpenLayers Error %d\n", olStatus );  \
      exit(1);                                        \
   }                                                  \
}                                                     \
while(0)      

#define NUM_OL_BUFFERS 4

int counter = 0;
BOOL    tfileopen = 0;
DBL     textfile_time = 0;
ULNG glist_resume = 0; 

BOOL save_data(HDASS hAD, HBUF hBuf)
{
/*
This function writes the specified buffer to the volts output file:
ADCFILE2.  The function returns TRUE if successful.  Otherwise, 
it returns FALSE and writes an error message to the specified
string ( strlen is the max length of the input string )
*/
    UINT strlen = 80;
    char lpstr[80];
    UINT size=0L;
    ECODE status=OLNOERROR;
    FILE *stream;
	BOOL rval=FALSE;
	DBL max=0, min=0;
	UINT resolution, listsize;
	UINT encoding;
	ULNG samples;
	ULNG value;
	PWORD pBuffer= NULL;
	PDWORD pBuffer32 = NULL;
	DBL voltage, freq;
	ULNG i = 0, j = 0;
	DBL gainlist[1024];
	DBL currentglistentry;

	/* get sub system information for code/volts conversion */
    
	status = olDaGetRange(hAD,&max,&min);
	if( status == OLNOERROR )
		status = olDaGetEncoding(hAD,&encoding);
	if( status == OLNOERROR )
		status = olDaGetResolution(hAD,&resolution);
	if( status == OLNOERROR )	
		status = olDmGetValidSamples( hBuf, &samples );
    if( status == OLNOERROR )
        status = olDmGetDataWidth( hBuf, &size );
	if(	status == OLNOERROR ) 
		olDaGetClockFrequency(hAD, &freq);
	if(	status == OLNOERROR ) 
		status = olDaGetChannelListSize(hAD, &listsize);
	if( status != OLNOERROR ) {
        olDaGetErrorString( status, lpstr, strlen );
    	return FALSE;
    }
	
	for (j=0;j<listsize; j++) {
		olDaGetGainListEntry(hAD, j, &currentglistentry);
		gainlist[j] = currentglistentry;
	}

	j = glist_resume;  // set current channel list element to either 0 or the next one in the list
						// this is encase the buffersize is not divisible by the channel list size

    // write the data
    rval = TRUE;
    if (tfileopen)
		stream = fopen( "volts.csv", "a+" );  //append to existing file
	else {
		stream = fopen( "volts.csv", "w" );	  //open new file
		tfileopen = 1;
		textfile_time = 0;
		fprintf( stream, "Time,Volts\n");
	}
            /* get pointer to the buffer */
            if (resolution > 16)
            {
				CHECKERROR (olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer32));
				
				while (i < samples)
				{
					
					value = pBuffer32[i];
					olDaCodeToVolts(min, max, gainlist[j]/*gain*/, resolution, encoding, value, &voltage);
                    LowpassFilter_put(&lowpass_volt, voltage);
                    voltage = LowpassFilter_get(&lowpass_volt);

					fprintf( stream, "%.3f,%f\n", textfile_time, voltage );
					// fprintf( stream, "%.3f \t%f volts", textfile_time, voltage );
					textfile_time += (1/freq);
					i++;
					if (j==listsize-1)
						j = 0;
					else 
						j++;					
				}
				glist_resume = j;  // hold current list element position and gain for next buffer
			}

            else
			{
				CHECKERROR (olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer));
				
				while (i < samples)
				{
					value = pBuffer[i];
					olDaCodeToVolts(min, max, gainlist[j]/*gain*/, resolution, encoding, value, &voltage);
					fprintf( stream, "%.3f,%f\n", textfile_time, voltage );
					// fprintf( stream, "%.3f \t%f volts\n", textfile_time, voltage );
					textfile_time += (1/freq);
					i++;
					if (j==listsize-1)
						j = 0;
					else 
						j++;
				}
				glist_resume = j;  // hold current list element position and gain for next buffer
			}
	
			
   fclose( stream );
   //system( "type volts.txt" );

    return rval;
}

void 
process_data(HDASS hAD, HBUF hBuffer)
{
   DBL min=0,max=0;
   DBL volts;
   ULNG value;
   ULNG samples;
   UINT encoding=0,resolution=0;
   PDWORD  pBuffer32 = NULL;
   PWORD  pBuffer = NULL;

   if( hBuffer )
   {

      /* get sub system information for code/volts conversion */
      olDaGetRange(hAD,&max,&min);
      olDaGetEncoding(hAD,&encoding);
      olDaGetResolution(hAD,&resolution);
    //   printf("Max: %f Min: %f\n", max, min);
    //   printf("Enc: %d\n", encoding);
    //   printf("Res: %d\n", resolution);

      /* get max samples in input buffer */
      olDmGetValidSamples( hBuffer, &samples );
    //   printf("Samp: %ld\n", samples);

      /* get pointer to the buffer */
      if (resolution > 16)
      {
         olDmGetBufferPtr( hBuffer,(LPVOID*)&pBuffer32);
         /* get last sample in buffer */
         value = pBuffer32[samples-1];
      }
      else
      {
         olDmGetBufferPtr( hBuffer,(LPVOID*)&pBuffer);
         /* get last sample in buffer */
         value = pBuffer[samples-1];
      }
      /*  convert value to volts */

      if (encoding != OL_ENC_BINARY) 
      {
         /* convert to offset binary by inverting the sign bit */

         value ^= 1L << (resolution-1);
         value &= (1L << resolution) - 1;     /* zero upper bits */
      }

      volts = ((float)max-(float)min)/(1L<<resolution)*value + (float)min;
    //   printf("%lf mV - ", volts*1000);
      fflush(stdout);
      printf("%lf\r", volts*1000);
    }
}


LRESULT WINAPI 
WndProc( HWND hWnd, UINT msg, WPARAM hAD, LPARAM lParam )
{
    switch( msg )
    {
       case OLDA_WM_BUFFER_DONE:
          printf( "Buffer Done Count: %ld \r", counter );
          HBUF hBuf;
          counter++;
          olDaGetBuffer( (HDASS)hAD, &hBuf );
        //   process_data( (HDASS)hAD, hBuf );
          save_data( (HDASS)hAD, hBuf );
          olDaPutBuffer( (HDASS)hAD, hBuf );
          break;

       case OLDA_WM_QUEUE_DONE:
          printf( "\nAcquisition stopped, rate too fast for current options." );
          PostQuitMessage(0);
          break;

       case OLDA_WM_TRIGGER_ERROR:
          printf( "\nTrigger error: acquisition stopped." );
          PostQuitMessage(0);
          break;

       case OLDA_WM_OVERRUN_ERROR:
          printf( "\nInput overrun error: acquisition stopped." );
          PostQuitMessage(0);
          break;

       default: 
          return DefWindowProc( hWnd, msg, hAD, lParam );
    }
    
    return 0;
}


BOOL CALLBACK 
EnumBrdProc( LPSTR lpszBrdName, LPSTR lpszDriverName, LPARAM lParam )
{

   // Make sure we can Init Board
   if( OLSUCCESS != ( olDaInitialize( lpszBrdName, (LPHDEV)lParam ) ) )
   {
      return TRUE;  // try again
   }

   // Make sure Board has an A/D Subsystem 
   UINT uiCap = 0;
   olDaGetDevCaps ( *((LPHDEV)lParam), OLDC_ADELEMENTS, &uiCap );
   if( uiCap < 1 )
   {
      return TRUE;  // try again
   }

   printf( "%s succesfully initialized.\n", lpszBrdName );
   return FALSE;    // all set , board handle in lParam
}

int main()
{
   int i = 0;
   printf( "Open Layers Continuous A/D Win32 Console Example\n" );

   // create a window for messages
   WNDCLASS wc;
   memset( &wc, 0, sizeof(wc) );
   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "DtConsoleClass";
   RegisterClass( &wc );

   HWND hWnd = CreateWindow( wc.lpszClassName,
                           NULL, 
                           NULL, 
                           0, 0, 0, 0,
                           NULL,
                           NULL,
                           NULL,
                           NULL );

   if( !hWnd )
      exit( 1 );

   HDEV hDev = NULL;
   CHECKERROR( olDaEnumBoards( EnumBrdProc, (LPARAM)&hDev ) );

   HDASS hAD = NULL;
   CHECKERROR( olDaGetDASS( hDev, OLSS_AD, 0, &hAD ) ); 

   CHECKERROR( olDaSetWndHandle( hAD, hWnd, 0 ) ); 

   CHECKERROR( olDaSetDataFlow( hAD, OL_DF_CONTINUOUS ) ); 
   CHECKERROR( olDaSetChannelListSize( hAD, 1 ) );
   CHECKERROR( olDaSetChannelListEntry( hAD, 0, 0 ) );
   CHECKERROR( olDaSetGainListEntry( hAD, 0, 1 ) );
   CHECKERROR( olDaSetTrigger( hAD, OL_TRG_SOFT ) );
   CHECKERROR( olDaSetClockSource( hAD, OL_CLK_INTERNAL ) ); 
   CHECKERROR( olDaSetClockFrequency( hAD, 1000.0 ) );
   CHECKERROR( olDaSetWrapMode( hAD, OL_WRP_NONE ) );
   CHECKERROR( olDaConfig( hAD ) );

   HBUF hBufs[NUM_OL_BUFFERS];
   for( int i=0; i < NUM_OL_BUFFERS; i++ )
   {
      if( OLSUCCESS != olDmAllocBuffer( GHND, 1000, &hBufs[i] ) )
      {
         for ( i--; i>=0; i-- )
         {
            olDmFreeBuffer( hBufs[i] );
         }   
         exit( 1 );   
      }
      olDaPutBuffer( hAD,hBufs[i] );
   }

   if( OLSUCCESS != ( olDaStart( hAD ) ) )
   {
      printf( "A/D Operation Start Failed...hit any key to terminate.\n" );
   }
   else
   {
      printf( "A/D Operation Started...hit any key to terminate.\n\n" );
      printf( "Buffer Done Count : %ld \r", counter );
   }   

   MSG msg;                                    
   SetMessageQueue( 50 );                      // Increase the our message queue size so
                                             // we don't lose any data acq messages

   // Acquire and dispatch messages until a key is hit...since we are a console app 
   // we are using a mix of Windows messages for data acquistion and console approaches
   // for keyboard input.
   //
   while( GetMessage( &msg,        // message structure              
                     hWnd,        // handle of window receiving the message 
                     0,           // lowest message to examine          
                     0 ) )        // highest message to examine         
   {
      TranslateMessage( &msg );    // Translates virtual key codes       
      DispatchMessage( &msg );     // Dispatches message to window 
      if( _kbhit() )
      {
         _getch();
         PostQuitMessage(0);      
      }
   }

   // abort A/D operation
   olDaAbort( hAD );
   printf( "\nA/D Operation Terminated \n" );

   for( i=0; i<NUM_OL_BUFFERS; i++ ) 
   {
        olDmFreeBuffer( hBufs[i] );
   }   

   olDaTerminate( hDev );
   exit( 0 );

   return 0;
}
