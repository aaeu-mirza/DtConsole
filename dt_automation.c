/*-----------------------------------------------------------------------

PROGRAM: dt_automation.cpp

PURPOSE:
    Open Layers data acquisition modified example implementing
    continuous analog input operations for upto 4 channels using
    windows messaging in a console environment.

****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include "oldaapi.h" // requires Open Layers Data Aquisition (olDa) packaged lib files.

/* Config Params*/
#define NUM_CHANNELS 4             // Max 4 for DT9837
#define ALL_CHANNEL_GAIN 1         // Universal gain used for all channels (1 or 10)
#define CLOCK_FREQUENCY 1000.0     // Internal clock frequency
#define OUTPUT_FREQUENCY 1000.0    // Internal clock frequency (Max 46875.0)
#define DEFAULT_WAV_AMPLITUDE 3    // for output
#define DEFAULT_WAV_FREQUENCY 1000 // for output

#define EN_MULTIPLE_CH_GAIN 1 //(1-ON,0-OFF)

#if EN_MULTIPLE_CH_GAIN == 1
#define CHANNEL_GAIN_0 1 // Z
#define CHANNEL_GAIN_1 1 // Y
#define CHANNEL_GAIN_2 1 // X
#define CHANNEL_GAIN_3 1 // N/A
#endif

#define SENSITIVITY_VAL_X 101.3 // mV per g [CALIBRATED: DO NOT CHANGE]
#define SENSITIVITY_VAL_Y 99.7  // mV per g [CALIBRATED: DO NOT CHANGE]
#define SENSITIVITY_VAL_Z 101.9 // mV per g [CALIBRATED: DO NOT CHANGE]

/* olDa error checking */
#define CFG_SUCCESS 0
#define CFG_FAILURE -1

#define ERR_INIT_CONFIG 1
#define ERR_BOARD_CONFIG 2
#define ERR_CHANNEL_CONFIG 3
#define ERR_DATA_CONFIG 4
#define ERR_MEASUREMENT 5
#define ERR_DEINIT_CONFIG 6

#define CHECKERROR(ecode)                           \
   do                                               \
   {                                                \
      ECODE olStatus;                               \
      if (OLSUCCESS != (olStatus = (ecode)))        \
      {                                             \
         printf("OpenLayers Error %d\n", olStatus); \
         return CFG_FAILURE;                        \
      }                                             \
   } while (0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define NUM_OL_BUFFERS 4
#define MAX_BUFFER_SIZE 8000  // 8192 rounded to 8k for even output repeatability
#define AVERAGING_CONSTANT 10 // observed from buffer to output conversion

int counter = 0;
BOOL tfileopen = 0;
DBL textfile_time = 0;
ULNG glist_resume = 0;

BOOL save_data(HDASS hAD_v, HBUF hBuf_v)
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

   status = olDaGetRange(hAD_v, &max, &min);
   if (status == OLNOERROR)
      status = olDaGetEncoding(hAD_v, &encoding);
   if (status == OLNOERROR)
      status = olDaGetResolution(hAD_v, &resolution);
   if (status == OLNOERROR)
      status = olDmGetValidSamples(hBuf_v, &samples);
   if (status == OLNOERROR)
      status = olDmGetDataWidth(hBuf_v, &size);
   if (status == OLNOERROR)
      olDaGetClockFrequency(hAD_v, &freq);
   if (status == OLNOERROR)
      status = olDaGetChannelListSize(hAD_v, &listsize);
   if (status != OLNOERROR)
   {
      olDaGetErrorString(status, lpstr, strlen);
      return FALSE;
   }

   for (j = 0; j < listsize; j++)
   {
      olDaGetGainListEntry(hAD_v, j, &currentglistentry);
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
      fprintf(stream, "Time,accel(x),accel(y),accel(z),dac\n");
   }
   /* get pointer to the buffer */
   CHECKERROR(olDmGetBufferPtr(hBuf_v, (LPVOID *)&pBuffer32));

   while (i < samples)
   {
      if (resolution > 16)
      {
         // Value reserved for channel 4
         value = pBuffer32[i + 3];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &voltage);

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
         value = pBuffer[i + 3];
         olDaCodeToVolts(min, max, gainlist[j] /*gain*/, resolution, encoding, value, &voltage);

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
      fprintf(stream, "%.3f,%f,%f,%f,%f\n", textfile_time, accel_x, accel_y, accel_z, voltage);

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

/* UNUSED: can be used to get a single value from 1 channel*/
void process_data(HDASS hAD_v, HBUF hBuffer)
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
      olDaGetRange(hAD_v, &max, &min);
      olDaGetEncoding(hAD_v, &encoding);
      olDaGetResolution(hAD_v, &resolution);

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
WndProc(HWND hWnd_v, UINT msg, WPARAM hAD_v, LPARAM lParam)
{
   switch (msg)
   {
   case OLDA_WM_BUFFER_DONE:
   {
      printf("Buffer Done Count: %ld \r", counter);
      HBUF hBuf = NULL;
      counter++;
      olDaGetBuffer((HDASS)hAD_v, &hBuf);
      if (hBuf)
      {
         //   process_data( (HDASS)hAD_v, hBuf );
         save_data((HDASS)hAD_v, hBuf);
         olDaPutBuffer((HDASS)hAD_v, hBuf);
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
      return DefWindowProc(hWnd_v, msg, hAD_v, lParam);
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

static HWND hWnd;
static HDEV hDev = NULL;
static HDASS hAD = NULL;
static HBUF hBufs[NUM_OL_BUFFERS];

int initialize_board()
{
   // create a window for messages
   WNDCLASS wc;
   memset(&wc, 0, sizeof(wc));
   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "DtConsoleClass";
   RegisterClass(&wc);

   hWnd = CreateWindow(wc.lpszClassName,
                            NULL,
                            NULL,
                            0, 0, 0, 0,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

   if (!hWnd)
      return CFG_FAILURE;

   CHECKERROR(olDaEnumBoards(EnumBrdProc, (LPARAM)&hDev));
   return CFG_SUCCESS;
}

int config_board_input(HWND *hWnd_p, HDEV *hDev_p, HDASS *hAD_p)
{  
   CHECKERROR(olDaGetDASS(*hDev_p, OLSS_AD, 0, hAD_p));
   CHECKERROR(olDaSetWndHandle(*hAD_p, *hWnd_p, 0));
   CHECKERROR(olDaSetDataFlow(*hAD_p, OL_DF_CONTINUOUS));

   return CFG_SUCCESS;
}

int config_channels_input(HDASS *hAD_p, int num_channels, int all_channel_gain, int channel_0_gain, int channel_1_gain, int channel_2_gain, int channel_3_gain)
{
   CHECKERROR(olDaSetChannelListSize(*hAD_p, num_channels));
   for (int i = 0; i < num_channels; i++)
   {
      /* Set Channel List and index */
      CHECKERROR(olDaSetChannelListEntry(*hAD_p, i, i));

      /* Set Channel Gain Values */
      CHECKERROR(olDaSetGainListEntry(*hAD_p, i, all_channel_gain));

      /* Set channels coupling type to AC coupling */
      CHECKERROR(olDaSetCouplingType(*hAD_p, i, AC));

      /* Set channels current source to disabled */
      CHECKERROR(olDaSetExcitationCurrentSource(*hAD_p, i, INTERNAL));
   }

#if EN_MULTIPLE_CH_GAIN == 1
   /* Set individual Channel Gain Values */
   CHECKERROR(olDaSetGainListEntry(*hAD_p, 0, channel_0_gain));
   CHECKERROR(olDaSetGainListEntry(*hAD_p, 1, channel_1_gain));
   CHECKERROR(olDaSetGainListEntry(*hAD_p, 2, channel_2_gain));
   CHECKERROR(olDaSetGainListEntry(*hAD_p, 3, channel_2_gain));
#endif

   return CFG_SUCCESS;
}

int config_data_input(HDASS *hAD_p, int clk_freq, HBUF hBufs_p[])
{
   /* Set the clock and frequency for data acquisition*/
   CHECKERROR(olDaSetTrigger(*hAD_p, OL_TRG_SOFT));
   CHECKERROR(olDaSetClockSource(*hAD_p, OL_CLK_INTERNAL));
   CHECKERROR(olDaSetClockFrequency(*hAD_p, clk_freq));
   CHECKERROR(olDaSetWrapMode(*hAD_p, OL_WRP_NONE));

   /* Allocating memory for data buffers*/
   for (int i = 0; i < NUM_OL_BUFFERS; i++)
   {
      if (OLSUCCESS != olDmCallocBuffer(GHND, 0, (int)clk_freq, 2, &hBufs_p[i]))
      {
         for (i--; i >= 0; i--)
         {
            olDmFreeBuffer(hBufs_p[i]);
         }
         return CFG_FAILURE;
      }
      olDaPutBuffer(*hAD_p, hBufs_p[i]);
   }

   return CFG_SUCCESS;
}

int measurement_start(HWND *hWnd_p, HDASS *hAD_p, bool timer_en, int timer_duration)
{
   /* Start acquisition*/
   if (OLSUCCESS != (olDaStart(*hAD_p)))
   {
      printf("A/D Operation Start Failed...\n");
   }
   else
   {
      printf("A/D Operation Started...\n");
   }

   if(timer_en)
   {
      printf("Timer Enabled: for %d seconds...\n\n", timer_duration);
   }
   else
   {
      printf("Timer Disabled. Hit any key to temrinate...\n\n", timer_duration);
   }

   MSG msg;
   time_t start = time(0);
   double seconds_since_start = 0;
   SetMessageQueue(50); // Increase the our message queue size so
                        // we don't lose any data acq messages

   // Acquire and dispatch messages until a key is hit...since we are a console app
   // we are using a mix of Windows messages for data acquistion and console approaches
   // for keyboard input.
   //
   while (GetMessage(&msg, // message structure
                     *hWnd_p, // handle of window receiving the message
                     0,    // lowest message to examine
                     0))   // highest message to examine
   {
      TranslateMessage(&msg); // Translates virtual key codes
      DispatchMessage(&msg);  // Dispatches message to window

      if(timer_en)
      {
         seconds_since_start = difftime(time(0), start);

         if (seconds_since_start > timer_duration)
         {
            PostQuitMessage(0);
         }
      }
      else
      {
         if (_kbhit())
         {
            _getch();
            PostQuitMessage(0);
         }
      }
   }

   return CFG_SUCCESS;
}

int deinitialize_inputs(HDASS *hAD_p, HBUF hBufs_p[])
{
   // abort A/D operation
   olDaAbort(*hAD_p);
   printf("\nA/D Operation Terminated \n");

   for (int i = 0; i < NUM_OL_BUFFERS; i++)
   {
      olDmFreeBuffer(hBufs_p[i]);
   }
   return CFG_SUCCESS;
}

int deinit_board()
{
   CHECKERROR(olDaReleaseDASS(hAD));
   CHECKERROR(olDaTerminate(hDev));
   return CFG_SUCCESS;
}

int measure(bool use_default_values, int num_channels, float clk_freq, int all_channel_gain, int channel_0_gain, int channel_1_gain, int channel_2_gain, int channel_3_gain, bool timer_en, int timer_duration)
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

   if(config_board_input(&hWnd, &hDev, &hAD) == CFG_FAILURE) 
      return ERR_BOARD_CONFIG;
   if(config_channels_input(&hAD,num_channels,all_channel_gain,channel_0_gain,channel_1_gain,channel_2_gain,channel_3_gain) == CFG_FAILURE) 
      return ERR_CHANNEL_CONFIG;
   if(config_data_input(&hAD, clk_freq,hBufs) == CFG_FAILURE) 
      return ERR_DATA_CONFIG;

   /* Store the config*/
   CHECKERROR(olDaConfig(hAD));

   if(measurement_start(&hWnd, &hAD, timer_en, timer_duration) == CFG_FAILURE) 
      return ERR_MEASUREMENT;
   if(deinitialize_inputs(&hAD,hBufs) == CFG_FAILURE) 
      return ERR_DEINIT_CONFIG;

   return CFG_SUCCESS;
}

/* This function generates a simple squarewave at the specified amplitude, frequency and duration (in s) */
int generate(bool use_default_values, bool read_input, float clk_freq, int all_channel_gain, int amplitude, int wave_freq, int duration)
{
   if (use_default_values)
   {
      all_channel_gain = ALL_CHANNEL_GAIN;
      clk_freq = OUTPUT_FREQUENCY;
      wave_freq = DEFAULT_WAV_FREQUENCY;
      amplitude = DEFAULT_WAV_AMPLITUDE;
   }

   DBL min, max, freq;
   float volts;
   long minvalue, maxvalue;
   UINT encoding, resolution;
   UINT size, peak, dma, i, j;
   UINT wavefreq;
   UINT channel = 0;

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
   HDASS hDA = NULL;
   HDASS hAD = NULL;
   HBUF hBuf = NULL;
   HBUF hBufs[NUM_OL_BUFFERS];
   PWORD lpbuf = NULL;

   CHECKERROR(olDaEnumBoards(EnumBrdProc, (LPARAM)&hDev));

   CHECKERROR(olDaGetDASS(hDev, OLSS_DA, 0, &hDA));
   CHECKERROR(olDaGetSSCapsEx(hDA, OLSSCE_MAXTHROUGHPUT, &freq));
   CHECKERROR(olDaGetSSCaps(hDA, OLSSC_NUMDMACHANS, &dma));

   dma = MIN(1, dma); /* try for one dma channel   */
   freq = clk_freq;

   CHECKERROR(olDaSetWndHandle(hDA, hWnd, (UINT)NULL));
   CHECKERROR(olDaSetDataFlow(hDA, OL_DF_CONTINUOUS));

   CHECKERROR(olDaSetChannelListSize(hDA, 1));
   CHECKERROR(olDaSetChannelListEntry(hDA, 0, 0));
   CHECKERROR(olDaSetGainListEntry(hDA, 0, all_channel_gain));

   /* Set the clock and frequency for data acquisition*/
   CHECKERROR(olDaSetClockFrequency(hDA, freq));
   CHECKERROR(olDaSetDmaUsage(hDA, dma));
   CHECKERROR(olDaSetWrapMode(hDA, OL_WRP_SINGLE));

   /* Store the config*/
   CHECKERROR(olDaConfig(hDA));

   if (read_input)
   {
      CHECKERROR(olDaGetDASS(hDev, OLSS_AD, 0, &hAD));
      CHECKERROR(olDaSetWndHandle(hAD, hWnd, 0));
      CHECKERROR(olDaSetDataFlow(hAD, OL_DF_CONTINUOUS));
      CHECKERROR(olDaSetChannelListSize(hAD, 4));
      CHECKERROR(olDaSetChannelListEntry(hAD, 0, 0));
      CHECKERROR(olDaSetChannelListEntry(hAD, 1, 1));
      CHECKERROR(olDaSetChannelListEntry(hAD, 2, 2));
      CHECKERROR(olDaSetChannelListEntry(hAD, 3, 3));
      CHECKERROR(olDaSetCouplingType(hAD, 3, AC));
      CHECKERROR(olDaSetExcitationCurrentSource(hAD, 3, INTERNAL));
      CHECKERROR(olDaSetClockFrequency(hAD, 1000));
      CHECKERROR(olDaSetWrapMode(hAD, OL_WRP_NONE));
      CHECKERROR(olDaSetTrigger(hAD, OL_TRG_SOFT));
      CHECKERROR(olDaSetClockSource(hAD, OL_CLK_INTERNAL));
      CHECKERROR(olDaConfig(hAD));
   }

   /* get sub system information for code/volts conversion */
   CHECKERROR(olDaGetRange(hDA, &max, &min));
   CHECKERROR(olDaGetEncoding(hDA, &encoding));
   CHECKERROR(olDaGetResolution(hDA, &resolution));

   /* convert max full scale to DAC units */
   volts = amplitude;
   olDaVoltsToCode(min, max, 1 /*gain*/, resolution, encoding, volts, &maxvalue);

   /* convert min full scale to DAC units */
   volts = -(amplitude);
   olDaVoltsToCode(min, max, 1 /*gain*/, resolution, encoding, volts, &minvalue);

   /* 100 hz square wave or the best the board can do */
   wavefreq = wave_freq; // 10Hz - 400Hz
   size = MAX_BUFFER_SIZE;
   peak = (size * AVERAGING_CONSTANT) / wavefreq;

   /* allocate the output buffer */
   CHECKERROR(olDmCallocBuffer(GMEM_FIXED, 0, (ULNG)size, 2, &hBuf));
   CHECKERROR(olDmGetBufferPtr(hBuf, (LPVOID *)&lpbuf));

   double angle = 0;
   /* fill the output buffer with a simple square wave */
   for (i = 0, j = 0; i < size; i += peak)
   {
      while (j < i + (peak / 2))
         lpbuf[j++] = (UINT)minvalue;
      while (j < i + peak)
         lpbuf[j++] = (UINT)maxvalue;
   }

   /* Test Sin Wave Implementation (DOESNT WORK)*/
   // for (i=0,j=0;i<size;i+=peak)
   // {
   //    angle = 2.0 * 3.14159265 * i / size;
   //    volts = (amplitude * sin(angle));
   //    olDaVoltsToCode(min,max, 1 /*gain*/, resolution, encoding, volts, &minvalue);
   //    printf("val:%f %d\n",volts,minvalue);
   //    lpbuf[i++] = (UINT)minvalue;
   // }

   /* for DAC's must set the number of valid samples in buffer */
   CHECKERROR(olDmSetValidSamples(hBuf, size));

   /* Put the buffer to the DAC */
   CHECKERROR(olDaPutBuffer(hDA, hBuf));

   if (read_input)
   {
      /* Allocating memory for data buffers*/
      for (int i = 0; i < NUM_OL_BUFFERS; i++)
      {
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
   }

   /* Start acquisition*/
   if (OLSUCCESS != (olDaStart(hDA)))
   {
      printf("D/A Operation Start Failed...hit any key to terminate.\n");
   }
   else
   {
      printf("D/A Operation Started...hit any key to terminate.\n\n");
   }

   printf("\nAquiring Data for %d seconds \n", duration);
   if (read_input)
   {
      /* Start acquisition*/
      if (OLSUCCESS != (olDaStart(hAD)))
      {
         printf("A/D Operation Start Failed...hit any key to terminate.\n");
      }
      else
      {
         printf("A/D Operation Started...hit any key to terminate.\n\n");
      }

      MSG msg;
      time_t start = time(0);
      double seconds_since_start = 0;
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
         seconds_since_start = difftime(time(0), start);

         if (seconds_since_start > duration)
         {
            PostQuitMessage(0);
         }

         /* Keyboard stop implementation*/
         // if (_kbhit())
         // {
         //    _getch();
         //    PostQuitMessage(0);
         // }
      }
   }
   else
   {
      Sleep(duration * 1000);
   }

   /*
      get the output buffer from the DAC subsystem and
      free the output buffer
   */
   olDaAbort(hDA);
   if (read_input)
   {
      // abort A/D operation
      olDaAbort(hAD);
      printf("\nA/D Operation Terminated \n");

      for (i = 0; i < NUM_OL_BUFFERS; i++)
      {
         olDmFreeBuffer(hBufs[i]);
      }
      CHECKERROR(olDaReleaseDASS(hAD));
   }
   CHECKERROR(olDaGetBuffer(hDA, &hBuf));
   CHECKERROR(olDmFreeBuffer(hBuf));

   /* release the subsystem and the board */

   CHECKERROR(olDaReleaseDASS(hDA));
   CHECKERROR(olDaTerminate(hDev));

   return CFG_SUCCESS;
   // exit(0);
}