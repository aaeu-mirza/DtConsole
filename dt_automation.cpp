/*-----------------------------------------------------------------------

PROGRAM: dt_automation.cpp

PURPOSE:
    Open Layers data acquisition modified example implementing
    continuous analog input operations for upto 4 channels using
    windows messaging in a console environment.

****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "oldaapi.h" // requires Open Layers Data Aquisition (olDa) packaged lib files.

/* Config Params*/
#define NUM_CHANNELS 4 // Max 4 for DT9837
#define ALL_CHANNEL_GAIN 100
#define CLOCK_FREQUENCY 1000.0
#define EN_MULTIPLE_CH_GAIN 1 //(1-ON,0-OFF)

#if EN_MULTIPLE_CH_GAIN == 1
#define CHANNEL_GAIN_0 100 // Z
#define CHANNEL_GAIN_1 100 // Y
#define CHANNEL_GAIN_2 100 // X
#define CHANNEL_GAIN_3 100 // N/A
#endif

#define SENSITIVITY_VAL_X 101.3 // mV per g [CALIBRATED: DO NOT CHANGE]
#define SENSITIVITY_VAL_Y 99.7  // mV per g [CALIBRATED: DO NOT CHANGE]
#define SENSITIVITY_VAL_Z 101.9 // mV per g [CALIBRATED: DO NOT CHANGE]
/* olDa error checking */
#define CHECKERROR(ecode)                           \
   do                                               \
   {                                                \
      ECODE olStatus;                               \
      if (OLSUCCESS != (olStatus = (ecode)))        \
      {                                             \
         printf("OpenLayers Error %d\n", olStatus); \
         exit(1);                                   \
      }                                             \
   } while (0)

#define NUM_OL_BUFFERS 4

int counter = 0;
BOOL tfileopen = 0;
DBL textfile_time = 0;
ULNG glist_resume = 0;

BOOL save_data(HDASS hAD, HBUF hBuf)
{
   /*
      This function writes the specified buffer to the volts output file:
      volts.csv  The function returns TRUE if successful.  Otherwise,
      it returns FALSE and writes an error message to the specified
      string ( strlen is the max length of the input string )
   */
   UINT strlen = 80;
   char lpstr[80];
   UINT size = 0L;
   ECODE status = OLNOERROR;
   FILE *stream;
   BOOL rval = FALSE;
   DBL max = 0, min = 0;
   UINT resolution, listsize;
   UINT encoding;
   ULNG samples;
   ULNG value;
   PWORD pBuffer = NULL;
   PDWORD pBuffer32 = NULL;
   DBL voltage, freq;
   DBL volt_x, volt_y, volt_z;
   DBL accel_x, accel_y, accel_z;
   ULNG i = 0, j = 0;
   DBL gainlist[1024];
   DBL currentglistentry;

   /* get sub system information for code/volts conversion */

   status = olDaGetRange(hAD, &max, &min);
   if (status == OLNOERROR)
      status = olDaGetEncoding(hAD, &encoding);
   if (status == OLNOERROR)
      status = olDaGetResolution(hAD, &resolution);
   if (status == OLNOERROR)
      status = olDmGetValidSamples(hBuf, &samples);
   if (status == OLNOERROR)
      status = olDmGetDataWidth(hBuf, &size);
   if (status == OLNOERROR)
      olDaGetClockFrequency(hAD, &freq);
   if (status == OLNOERROR)
      status = olDaGetChannelListSize(hAD, &listsize);
   if (status != OLNOERROR)
   {
      olDaGetErrorString(status, lpstr, strlen);
      return FALSE;
   }

   for (j = 0; j < listsize; j++)
   {
      olDaGetGainListEntry(hAD, j, &currentglistentry);
      gainlist[j] = currentglistentry;
   }

   j = glist_resume; // set current channel list element to either 0 or the next one in the list
                     // this is encase the buffersize is not divisible by the channel list size

   // write the data
   rval = TRUE;
   if (tfileopen)
      stream = fopen("accel.csv", "a+"); // append to existing file
   else
   {
      stream = fopen("accel.csv", "w"); // open new file
      tfileopen = 1;
      textfile_time = 0;
      fprintf(stream, "Time,accel(x),accel(y),accel(z)\n");
   }
   /* get pointer to the buffer */
   CHECKERROR(olDmGetBufferPtr(hBuf, (LPVOID *)&pBuffer32));

   while (i < samples)
   {
      if (resolution > 16)
      {
         // Value reserved for channel 4
         // value = pBuffer32[i+3];
         // olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &voltage);

         // Convert and store values
         value = pBuffer32[i + 2];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_x);
         value = pBuffer32[i + 1];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_y);
         value = pBuffer32[i];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_z);
      }
      else
      {
         // Value reserved for channel 4
         // value = pBuffer[i+3];
         // olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &voltage);

         // Convert and store values
         value = pBuffer[i + 2];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_x);
         value = pBuffer[i + 1];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_y);
         value = pBuffer[i];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &volt_z);
      }

      // printf("ValueX: %d\n",volt_x);

      accel_x = volt_x / (SENSITIVITY_VAL_X / 1000);
      accel_y = volt_y / (SENSITIVITY_VAL_Y / 1000);
      accel_z = volt_z / (SENSITIVITY_VAL_Z / 1000);

      // Print voltage values to file
      fprintf(stream, "%.3f,%f,%f,%f\n", textfile_time, accel_x, accel_y, accel_z);

      textfile_time += (1 / freq);
      // i++;
      i += size;
      if (j == listsize - 1)
         j = 0;
      else
         j++;
   }
   glist_resume = j; // hold current list element position and gain for next buffer

   fclose(stream);
   // system( "type volts.txt" );

   return rval;
}

void process_data(HDASS hAD, HBUF hBuffer)
{
   /*
     This function writes the specified buffer record a single voltage
     value from a single channel. It prints the value to the console
  */
   DBL min = 0, max = 0;
   DBL volts;
   ULNG value;
   ULNG samples;
   UINT encoding = 0, resolution = 0;
   PDWORD pBuffer32 = NULL;
   PWORD pBuffer = NULL;

   if (hBuffer)
   {

      /* get sub system information for code/volts conversion */
      olDaGetRange(hAD, &max, &min);
      olDaGetEncoding(hAD, &encoding);
      olDaGetResolution(hAD, &resolution);

      /* get max samples in input buffer */
      olDmGetValidSamples(hBuffer, &samples);

      /* get pointer to the buffer */
      if (resolution > 16)
      {
         olDmGetBufferPtr(hBuffer, (LPVOID *)&pBuffer32);
         /* get last sample in buffer */
         value = pBuffer32[samples - 1];
      }
      else
      {
         olDmGetBufferPtr(hBuffer, (LPVOID *)&pBuffer);
         /* get last sample in buffer */
         value = pBuffer[samples - 1];
      }
      /*  convert value to volts */

      if (encoding != OL_ENC_BINARY)
      {
         /* convert to offset binary by inverting the sign bit */

         value ^= 1L << (resolution - 1);
         value &= (1L << resolution) - 1; /* zero upper bits */
      }

      volts = ((float)max - (float)min) / (1L << resolution) * value + (float)min;
      fflush(stdout);
      printf("%lf\r", volts * 1000);
   }
}

/* This function is a windows api callback for processing the data buffers*/
LRESULT WINAPI
WndProc(HWND hWnd, UINT msg, WPARAM hAD, LPARAM lParam)
{
   switch (msg)
   {
   case OLDA_WM_BUFFER_DONE:
   {
      printf("Buffer Done Count: %ld \r", counter);
      HBUF hBuf = NULL;
      counter++;
      olDaGetBuffer((HDASS)hAD, &hBuf);
      if (hBuf)
      {
         //   process_data( (HDASS)hAD, hBuf );
         save_data((HDASS)hAD, hBuf);
         olDaPutBuffer((HDASS)hAD, hBuf);
      }
   }
   break;

   case OLDA_WM_QUEUE_DONE:
      printf("\nAcquisition stopped, rate too fast for current options.");
      PostQuitMessage(0);
      break;

   case OLDA_WM_TRIGGER_ERROR:
      printf("\nTrigger error: acquisition stopped.");
      PostQuitMessage(0);
      break;

   case OLDA_WM_OVERRUN_ERROR:
      printf("\nInput overrun error: acquisition stopped.");
      PostQuitMessage(0);
      break;

   default:
      return DefWindowProc(hWnd, msg, hAD, lParam);
   }

   return 0;
}

/* Open Layers callback functionto initialize board*/
BOOL CALLBACK
EnumBrdProc(LPSTR lpszBrdName, LPSTR lpszDriverName, LPARAM lParam)
{
   // Make sure we can Init Board
   if (OLSUCCESS != (olDaInitialize(lpszBrdName, (LPHDEV)lParam)))
   {
      return TRUE; // try again
   }

   // Make sure Board has an A/D Subsystem
   UINT uiCap = 0;
   olDaGetDevCaps(*((LPHDEV)lParam), OLDC_ADELEMENTS, &uiCap);
   if (uiCap < 1)
   {
      return TRUE; // try again
   }

   printf("%s succesfully initialized.\n", lpszBrdName);
   return FALSE; // all set , board handle in lParam
}
extern "C"
{
   void measure(bool use_default_values, int num_channels, float clk_freq, int all_channel_gain, int channel_0_gain, int channel_1_gain, int channel_2_gain, int channel_3_gain)
   {
      if (use_default_values)
      {
         num_channels = NUM_CHANNELS;
         all_channel_gain = ALL_CHANNEL_GAIN;
         channel_0_gain = CHANNEL_GAIN_0;
         channel_1_gain = CHANNEL_GAIN_1;
         channel_2_gain = CHANNEL_GAIN_2;
         channel_3_gain = CHANNEL_GAIN_3;
         clk_freq = CLOCK_FREQUENCY;
      }

      int i = 0;
      printf("Open Layers Continuous A/D Win32 Console Example\n");

      // create a window for messages
      WNDCLASS wc;
      memset(&wc, 0, sizeof(wc));
      wc.lpfnWndProc = WndProc;
      wc.lpszClassName = "DtConsoleClass";
      RegisterClass(&wc);

      HWND hWnd = CreateWindow(wc.lpszClassName,
                               NULL,
                               NULL,
                               0, 0, 0, 0,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

      if (!hWnd)
         exit(1);

      /* Configuring the board*/
      HDEV hDev = NULL;
      HDASS hAD = NULL;
      COUPLING_TYPE coup;

      CHECKERROR(olDaEnumBoards(EnumBrdProc, (LPARAM)&hDev));
      CHECKERROR(olDaGetDASS(hDev, OLSS_AD, 0, &hAD));
      CHECKERROR(olDaSetWndHandle(hAD, hWnd, 0));
      CHECKERROR(olDaSetDataFlow(hAD, OL_DF_CONTINUOUS));

      CHECKERROR(olDaSetChannelListSize(hAD, num_channels));
      for (int i = 0; i < num_channels; i++)
      {
         /* Set Channel List and index */
         CHECKERROR(olDaSetChannelListEntry(hAD, i, i));

         /* Set Channel Gain Values */
         CHECKERROR(olDaSetGainListEntry(hAD, i, all_channel_gain));

         /* Set channels coupling type to AC coupling */
         CHECKERROR(olDaSetCouplingType(hAD, i, AC));

         /* Set channels current source to disabled */
         CHECKERROR(olDaSetExcitationCurrentSource(hAD, i, INTERNAL));
      }

#if EN_MULTIPLE_CH_GAIN == 1
      /* Set individual Channel Gain Values */
      CHECKERROR(olDaSetGainListEntry(hAD, 0, channel_0_gain));
      CHECKERROR(olDaSetGainListEntry(hAD, 1, channel_1_gain));
      CHECKERROR(olDaSetGainListEntry(hAD, 2, channel_2_gain));
      CHECKERROR(olDaSetGainListEntry(hAD, 3, channel_3_gain));
#endif
      /* Set the clock and frequency for data acquisition*/
      CHECKERROR(olDaSetTrigger(hAD, OL_TRG_SOFT));
      CHECKERROR(olDaSetClockSource(hAD, OL_CLK_INTERNAL));
      CHECKERROR(olDaSetClockFrequency(hAD, clk_freq));
      CHECKERROR(olDaSetWrapMode(hAD, OL_WRP_NONE));
      // CHECKERROR(olDaSetWrapMode(hAD, OL_WRP_MULTIPLE));

      /* Store the config*/
      CHECKERROR(olDaConfig(hAD));

      /* Allocating memory for data buffers*/
      HBUF hBufs[NUM_OL_BUFFERS];
      for (int i = 0; i < NUM_OL_BUFFERS; i++)
      {
         // if (OLSUCCESS != olDmAllocBuffer(GHND, (int)clk_freq, &hBufs[i]))
         if (OLSUCCESS != olDmCallocBuffer(GHND, 0, (int)clk_freq, 2, &hBufs[i]))
         {
            for (i--; i >= 0; i--)
            {
               olDmFreeBuffer(hBufs[i]);
            }
            exit(1);
         }
         olDaPutBuffer(hAD, hBufs[i]);
      }

      /* Start acquisition*/
      if (OLSUCCESS != (olDaStart(hAD)))
      {
         printf("A/D Operation Start Failed...hit any key to terminate.\n");
      }
      else
      {
         printf("A/D Operation Started...hit any key to terminate.\n\n");
         printf("Buffer Done Count : %ld \r", counter);
      }

      MSG msg;
      SetMessageQueue(50); // Increase the our message queue size so
                           // we don't lose any data acq messages

      // Acquire and dispatch messages until a key is hit...since we are a console app
      // we are using a mix of Windows messages for data acquistion and console approaches
      // for keyboard input.
      //
      while (GetMessage(&msg, // message structure
                        hWnd, // handle of window receiving the message
                        0,    // lowest message to examine
                        0))   // highest message to examine
      {
         TranslateMessage(&msg); // Translates virtual key codes
         DispatchMessage(&msg);  // Dispatches message to window
         if (_kbhit())
         {
            _getch();
            PostQuitMessage(0);
         }
      }

      // abort A/D operation
      olDaAbort(hAD);
      printf("\nA/D Operation Terminated \n");

      for (i = 0; i < NUM_OL_BUFFERS; i++)
      {
         olDmFreeBuffer(hBufs[i]);
      }

      olDaTerminate(hDev);
      exit(0);
   }
}