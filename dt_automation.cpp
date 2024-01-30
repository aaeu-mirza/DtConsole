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

      /* get max samples in input buffer */
      olDmGetValidSamples( hBuffer, &samples );

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
      printf("%lf V - ", volts);
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
          process_data((HDASS)hAD, hBuf);
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
