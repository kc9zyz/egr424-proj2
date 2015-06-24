//*****************************************************************************
//
// my_uart.c - Driver for the UART.
//
// This code is borrowed from the Stellaris Peripheral Driver Library
//
// The purpose of this driver is to allow the use of the UART peripheral
// on the LM3S6965. Included functions allow for initialization,
// enabling/disabling of the peripheral, and enabling/disabling of
// interrupt capabilities.
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup uart_api
//! @{
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"

//*****************************************************************************
//
// The system clock divider defining the maximum baud rate supported by the
// UART.
//
//*****************************************************************************
#define UART_CLK_DIVIDER        ((CLASS_IS_SANDSTORM ||                       \
                                  (CLASS_IS_FURY && REVISION_IS_A2) ||        \
                                  (CLASS_IS_DUSTDEVIL && REVISION_IS_A0)) ?   \
                                 16 : 8)

//*****************************************************************************
//
//! \internal
//! Checks a UART base address.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function determines if a UART port base address is valid.
//!
//! \return Returns \b true if the base address is valid and \b false
//! otherwise.
//
//*****************************************************************************
#ifdef DEBUG
static tBoolean
UARTBaseValid(unsigned long ulBase)
{
    return((ulBase == UART0_BASE) || (ulBase == UART1_BASE) ||
           (ulBase == UART2_BASE));
}
#endif

//*****************************************************************************
//
//! Sets the configuration of a UART.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulUARTClk is the rate of the clock supplied to the UART module.
//! \param ulBaud is the desired baud rate.
//! \param ulConfig is the data format for the port (number of data bits,
//! number of stop bits, and parity).
//!
//! This function will configure the UART for operation in the specified data
//! format.  The baud rate is provided in the \e ulBaud parameter and the data
//! format in the \e ulConfig parameter.
//!
//! The \e ulConfig parameter is the logical OR of three values: the number of
//! data bits, the number of stop bits, and the parity.  \b UART_CONFIG_WLEN_8,
//! \b UART_CONFIG_WLEN_7, \b UART_CONFIG_WLEN_6, and \b UART_CONFIG_WLEN_5
//! select from eight to five data bits per byte (respectively).
//! \b UART_CONFIG_STOP_ONE and \b UART_CONFIG_STOP_TWO select one or two stop
//! bits (respectively).  \b UART_CONFIG_PAR_NONE, \b UART_CONFIG_PAR_EVEN,
//! \b UART_CONFIG_PAR_ODD, \b UART_CONFIG_PAR_ONE, and \b UART_CONFIG_PAR_ZERO
//! select the parity mode (no parity bit, even parity bit, odd parity bit,
//! parity bit always one, and parity bit always zero, respectively).
//!
//! The peripheral clock will be the same as the processor clock.  This will be
//! the value returned by SysCtlClockGet(), or it can be explicitly hard coded
//! if it is constant and known (to save the code/execution overhead of a call
//! to SysCtlClockGet()).
//!
//! This function replaces the original UARTConfigSet() API and performs the
//! same actions.  A macro is provided in <tt>uart.h</tt> to map the original
//! API to this API.
//!
//! \return None.
//
//*****************************************************************************
void
UARTConfigSetExpClk(unsigned long ulBase, unsigned long ulUARTClk,
                    unsigned long ulBaud, unsigned long ulConfig)
{
    unsigned long ulDiv;

    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    ASSERT(ulBaud != 0);
    ASSERT(ulUARTClk >= (ulBaud * UART_CLK_DIVIDER));

    //
    // Stop the UART.
    //
    UARTDisable(ulBase);

    //
    // Is the required baud rate greater than the maximum rate supported
    // without the use of high speed mode?
    //
    if((ulBaud * 16) > ulUARTClk)
    {
        //
        // Enable high speed mode.
        //
        HWREG(ulBase + UART_O_CTL) |= UART_CTL_HSE;

        //
        // Half the supplied baud rate to compensate for enabling high speed
        // mode.  This allows the following code to be common to both cases.
        //
        ulBaud /= 2;
    }
    else
    {
        //
        // Disable high speed mode.
        //
        HWREG(ulBase + UART_O_CTL) &= ~(UART_CTL_HSE);
    }

    //
    // Compute the fractional baud rate divider.
    //
    ulDiv = (((ulUARTClk * 8) / ulBaud) + 1) / 2;

    //
    // Set the baud rate.
    //
    HWREG(ulBase + UART_O_IBRD) = ulDiv / 64;
    HWREG(ulBase + UART_O_FBRD) = ulDiv % 64;

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_O_LCRH) = ulConfig;

    //
    // Clear the flags register.
    //
    HWREG(ulBase + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    UARTEnable(ulBase);
}

//*****************************************************************************
//
//! Enables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! Sets the UARTEN, TXE, and RXE bits, and enables the transmit and receive
//! FIFOs.
//!
//! \return None.
//
//*****************************************************************************
void
UARTEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Enable the FIFO.
    //
    HWREG(ulBase + UART_O_LCRH) |= UART_LCRH_FEN;

    //
    // Enable RX, TX, and the UART.
    //
    HWREG(ulBase + UART_O_CTL) |= (UART_CTL_UARTEN | UART_CTL_TXE |
                                   UART_CTL_RXE);
}

//*****************************************************************************
//
//! Disables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! Clears the UARTEN, TXE, and RXE bits, then waits for the end of
//! transmission of the current character, and flushes the transmit FIFO.
//!
//! \return None.
//
//*****************************************************************************
void
UARTDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait for end of TX.
    //
    while(HWREG(ulBase + UART_O_FR) & UART_FR_BUSY)
    {
    }

    //
    // Disable the FIFO.
    //
    HWREG(ulBase + UART_O_LCRH) &= ~(UART_LCRH_FEN);

    //
    // Disable the UART.
    //
    HWREG(ulBase + UART_O_CTL) &= ~(UART_CTL_UARTEN | UART_CTL_TXE |
                                    UART_CTL_RXE);
}

//*****************************************************************************
//
//! Determines if there are any characters in the receive FIFO.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function returns a flag indicating whether or not there is data
//! available in the receive FIFO.
//!
//! \return Returns \b true if there is data in the receive FIFO, and \b false
//! if there is no data in the receive FIFO.
//
//*****************************************************************************
tBoolean
UARTCharsAvail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the availability of characters.
    //
    return((HWREG(ulBase + UART_O_FR) & UART_FR_RXFE) ? false : true);
}

//*****************************************************************************
//
//! Receives a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//!
//! Gets a character from the receive FIFO for the specified port.
//!
//! This function replaces the original UARTCharNonBlockingGet() API and
//! performs the same actions.  A macro is provided in <tt>uart.h</tt> to map
//! the original API to this API.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e long.  A \b -1 will be returned if there are no characters present in
//! the receive FIFO.  The UARTCharsAvail() function should be called before
//! attempting to call this function.
//
//*****************************************************************************
long
UARTCharGetNonBlocking(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // See if there are any characters in the receive FIFO.
    //
    if(!(HWREG(ulBase + UART_O_FR) & UART_FR_RXFE))
    {
        //
        // Read and return the next character.
        //
        return(HWREG(ulBase + UART_O_DR));
    }
    else
    {
        //
        // There are no characters, so return a failure.
        //
        return(-1);
    }
}

//*****************************************************************************
//
//! Enables individual UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! Enables the indicated UART interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! The \e ulIntFlags parameter is the logical OR of any of the following:
//!
//! - \b UART_INT_OE - Overrun Error interrupt
//! - \b UART_INT_BE - Break Error interrupt
//! - \b UART_INT_PE - Parity Error interrupt
//! - \b UART_INT_FE - Framing Error interrupt
//! - \b UART_INT_RT - Receive Timeout interrupt
//! - \b UART_INT_TX - Transmit interrupt
//! - \b UART_INT_RX - Receive interrupt
//! - \b UART_INT_DSR - DSR interrupt
//! - \b UART_INT_DCD - DCD interrupt
//! - \b UART_INT_CTS - CTS interrupt
//! - \b UART_INT_RI - RI interrupt
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + UART_O_IM) |= ulIntFlags;
}

//*****************************************************************************
//
//! Disables individual UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is the bit mask of the interrupt sources to be disabled.
//!
//! Disables the indicated UART interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to UARTIntEnable().
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Disable the specified interrupts.
    //
    HWREG(ulBase + UART_O_IM) &= ~(ulIntFlags);
}

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param ulBase is the base address of the UART port.
//! \param bMasked is false if the raw interrupt status is required and true
//! if the masked interrupt status is required.
//!
//! This returns the interrupt status for the specified UART.  Either the raw
//! interrupt status or the status of interrupts that are allowed to reflect to
//! the processor can be returned.
//!
//! \return Returns the current interrupt status, enumerated as a bit field of
//! values described in UARTIntEnable().
//
//*****************************************************************************
unsigned long
UARTIntStatus(unsigned long ulBase, tBoolean bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + UART_O_MIS));
    }
    else
    {
        return(HWREG(ulBase + UART_O_RIS));
    }
}

//*****************************************************************************
//
//! Clears UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is a bit mask of the interrupt sources to be cleared.
//!
//! The specified UART interrupt sources are cleared, so that they no longer
//! assert.  This must be done in the interrupt handler to keep it from being
//! called again immediately upon exit.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to UARTIntEnable().
//!
//! \note Since there is a write buffer in the Cortex-M3 processor, it may take
//! several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (since NVIC still sees the interrupt source
//! asserted).
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntClear(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Clear the requested interrupt sources.
    //
    HWREG(ulBase + UART_O_ICR) = ulIntFlags;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
