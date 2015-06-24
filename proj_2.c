//*****************************************************************************
//
// proj_2.c - Video Player for the LM3S6965 Development Board
//
// The video player for the LM3S6965 Development Board utilizes the
// UART and SSI peripherals in order to stream video from an attached
// computer to the 128x96x4 on board OLED screen. UART operates on
// interrupts generated from receiving video data from the computer and
// SSI generates interrupts when transission of data to the OLED display
// is complete. Serial communication over USB is driven at 1.5 Mbit/
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include <stdint.h>
#include <stdbool.h>
#include "my_uart.h"
#include "my_ssi.h"



#define QUEUE_LEN 6144
#define BAUD 1500000

//Global memory and flags
uint8_t g_queue[QUEUE_LEN];
volatile uint16_t queueCount = 0;
volatile bool pageRecieved = false;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The UART interrupt handler. Clears the generated interrupt and stores the
// received data in a queue to be sent to the OLED display
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    //
    // Variable to hold the interrupt status
    //
    unsigned long ulStatus;

    //
    // Get the interrrupt status.
    //
    ulStatus = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART0_BASE, ulStatus);


    //
    // Read the next character from the UART and write it into
    // g_queue buffer for sending to OLED display
    //
    g_queue[queueCount++] = UARTCharGetNonBlocking(UART0_BASE);

    //Detect start byte sent from computer
    if(g_queue[queueCount-1] == 0xFF)
    {
        queueCount = 0;
    }

		//Detect an entire frame has completed
    if((queueCount >=QUEUE_LEN))
    {
        pageRecieved = true;
    }


}

//*****************************************************************************
//
// Initializes UART, SSI, and OLED peripherals. Enables interrupts from UART
// and from SSI. Waits for a full page of data to be recieived from UART and
// then sends the page over SSI to the OLED display.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Enable interrupts from SSI0 peripheral
    //
    IntEnable(INT_SSI0);

    //
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);

    //
    // Enable the UART peripheral
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 1,500,000, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(),BAUD ,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    //Loop forever waiting for an image to complete coming over from the UART
    while(1)
    {
		    //Wait for incoming image
    		 if(pageRecieved)
             {
                //Display image received
                RIT128x96x4ImageDraw(g_queue, 0,0, 128,96);


                //Reset pageRecieved to false
                pageRecieved = false;
             }

    }
		//
    // Finished.
    //
	return 0;
}
