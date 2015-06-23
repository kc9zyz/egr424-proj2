//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 10636 of the EK-LM3S6965 Firmware Package.
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
//! \addtogroup example_list
//! <h1>Hello World (hello)</h1>
//!
//! A very simple ``hello world'' example.  It simply displays ``hello world''
//! on the OLED and is a starting point for more complicated applications.
//
//*****************************************************************************

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
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
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
    // Read the next character from the UART and write it back to the UART.
    //
    g_queue[queueCount++] = UARTCharGetNonBlocking(UART0_BASE);

    //Detect start byte
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
// Print "Hello world!" to the OLED on the Stellaris evaluation board.
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
