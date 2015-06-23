//*****************************************************************************
//
// ssi.c - Driver for Synchronous Serial Interface.
//
// Copyright (c) 2005-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 10636 of the Stellaris Peripheral Driver Library.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup ssi_api
//! @{
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/ssi.h"


//Prototype functions not needed in API
void SSIIntClear(unsigned long ulBase, unsigned long ulIntFlags);
void SSIIntDisable(unsigned long ulBase, unsigned long ulIntFlags);
unsigned long SSIIntStatus(unsigned long ulBase, tBoolean bMasked);

volatile unsigned char ssiBusy = 1;

//
//! Interrupt service routine for the SSI Peripheral.
//! The hadnler executes every time the SSI TX bufer is empty after data is sent.
//! This allows an interrup driven busy function that doesn't poll hardware
//! unless an interrupt has occured.
//
void SSIIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Disable the SSI_TXFF interrupt
    //

    SSIIntDisable(SSI0_BASE, SSI_TXFF);
    //
    // Get the interrupt status.
    //
    ulStatus = SSIIntStatus(SSI0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    SSIIntClear(SSI0_BASE, ulStatus);


    ssiBusy = (HWREG(SSI0_BASE + SSI_O_SR) & SSI_SR_BSY) ? true : false;

}
//*****************************************************************************
//
// A mapping of timer base address to interupt number.
//
//*****************************************************************************
static const unsigned long g_ppulSSIIntMap[][2] =
{
    { SSI0_BASE, INT_SSI0 },
    { SSI1_BASE, INT_SSI1 },
    { SSI2_BASE, INT_SSI2 },
    { SSI3_BASE, INT_SSI3 },
};

//*****************************************************************************
//
//! \internal
//! Checks an SSI base address.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! This function determines if a SSI module base address is valid.
//!
//! \return Returns \b true if the base address is valid and \b false
//! otherwise.
//
//*****************************************************************************
#ifdef DEBUG
static tBoolean
SSIBaseValid(unsigned long ulBase)
{
    return((ulBase == SSI0_BASE) || (ulBase == SSI1_BASE) ||
           (ulBase == SSI2_BASE) || (ulBase == SSI3_BASE));
}
#endif

//*****************************************************************************
//
//! \internal
//! Gets the SSI interrupt number.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! Given a SSI base address, returns the corresponding interrupt number.
//!
//! \return Returns an SSI interrupt number, or -1 if \e ulBase is invalid.
//
//*****************************************************************************
static long
SSIIntNumberGet(unsigned long ulBase)
{
    unsigned long ulIdx;

    //
    // Loop through the table that maps SSI base addresses to interrupt
    // numbers.
    //
    for(ulIdx = 0; ulIdx < (sizeof(g_ppulSSIIntMap) /
                            sizeof(g_ppulSSIIntMap[0])); ulIdx++)
    {
        //
        // See if this base address matches.
        //
        if(g_ppulSSIIntMap[ulIdx][0] == ulBase)
        {
            //
            // Return the corresponding interrupt number.
            //
            return(g_ppulSSIIntMap[ulIdx][1]);
        }
    }

    //
    // The base address could not be found, so return an error.
    //
    return(-1);
}

//*****************************************************************************
//
//! Configures the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulSSIClk is the rate of the clock supplied to the SSI module.
//! \param ulProtocol specifies the data transfer protocol.
//! \param ulMode specifies the mode of operation.
//! \param ulBitRate specifies the clock rate.
//! \param ulDataWidth specifies number of bits transferred per frame.
//!
//! This function configures the synchronous serial interface.  It sets
//! the SSI protocol, mode of operation, bit rate, and data width.
//!
//! The \e ulProtocol parameter defines the data frame format.  The
//! \e ulProtocol parameter can be one of the following values:
//! \b SSI_FRF_MOTO_MODE_0, \b SSI_FRF_MOTO_MODE_1, \b SSI_FRF_MOTO_MODE_2,
//! \b SSI_FRF_MOTO_MODE_3, \b SSI_FRF_TI, or \b SSI_FRF_NMW.  The Motorola
//! frame formats encode the following polarity and phase configurations:
//!
//! <pre>
//! Polarity Phase       Mode
//!   0       0   SSI_FRF_MOTO_MODE_0
//!   0       1   SSI_FRF_MOTO_MODE_1
//!   1       0   SSI_FRF_MOTO_MODE_2
//!   1       1   SSI_FRF_MOTO_MODE_3
//! </pre>
//!
//! The \e ulMode parameter defines the operating mode of the SSI module.  The
//! SSI module can operate as a master or slave; if it is a slave, the SSI can
//! be configured to disable output on its serial output line.  The \e ulMode
//! parameter can be one of the following values: \b SSI_MODE_MASTER,
//! \b SSI_MODE_SLAVE, or \b SSI_MODE_SLAVE_OD.
//!
//! The \e ulBitRate parameter defines the bit rate for the SSI.  This bit rate
//! must satisfy the following clock ratio criteria:
//!
//! - FSSI >= 2 * bit rate (master mode); this speed cannot exceed 25 MHz.
//! - FSSI >= 12 * bit rate or 6 * bit rate (slave modes), depending on the
//! capability of the specific microcontroller
//!
//! where FSSI is the frequency of the clock supplied to the SSI module.
//!
//! The \e ulDataWidth parameter defines the width of the data transfers and
//! can be a value between 4 and 16, inclusive.
//!
//! The peripheral clock is the same as the processor clock.  This value is
//! returned by SysCtlClockGet(), or it can be explicitly hard coded if it is
//! constant and known (to save the code/execution overhead of a call to
//! SysCtlClockGet()).
//!
//! This function replaces the original SSIConfig() API and performs the same
//! actions.  A macro is provided in <tt>ssi.h</tt> to map the original API to
//! this API.
//!
//! \return None.
//
//*****************************************************************************
void
SSIConfigSetExpClk(unsigned long ulBase, unsigned long ulSSIClk,
                   unsigned long ulProtocol, unsigned long ulMode,
                   unsigned long ulBitRate, unsigned long ulDataWidth)
{
    unsigned long ulMaxBitRate;
    unsigned long ulRegVal;
    unsigned long ulPreDiv;
    unsigned long ulSCR;
    unsigned long ulSPH_SPO;

    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));
    ASSERT((ulProtocol == SSI_FRF_MOTO_MODE_0) ||
           (ulProtocol == SSI_FRF_MOTO_MODE_1) ||
           (ulProtocol == SSI_FRF_MOTO_MODE_2) ||
           (ulProtocol == SSI_FRF_MOTO_MODE_3) ||
           (ulProtocol == SSI_FRF_TI) ||
           (ulProtocol == SSI_FRF_NMW));
    ASSERT((ulMode == SSI_MODE_MASTER) ||
           (ulMode == SSI_MODE_SLAVE) ||
           (ulMode == SSI_MODE_SLAVE_OD));
    ASSERT(((ulMode == SSI_MODE_MASTER) && (ulBitRate <= (ulSSIClk / 2))) ||
           ((ulMode != SSI_MODE_MASTER) && (ulBitRate <= (ulSSIClk / 12))));
    ASSERT((ulSSIClk / ulBitRate) <= (254 * 256));
    ASSERT((ulDataWidth >= 4) && (ulDataWidth <= 16));

    //
    // Set the mode.
    //
    ulRegVal = (ulMode == SSI_MODE_SLAVE_OD) ? SSI_CR1_SOD : 0;
    ulRegVal |= (ulMode == SSI_MODE_MASTER) ? 0 : SSI_CR1_MS;
    HWREG(ulBase + SSI_O_CR1) = ulRegVal;

    //
    // Set the clock predivider.
    //
    ulMaxBitRate = ulSSIClk / ulBitRate;
    ulPreDiv = 0;
    do
    {
        ulPreDiv += 2;
        ulSCR = (ulMaxBitRate / ulPreDiv) - 1;
    }
    while(ulSCR > 255);
    HWREG(ulBase + SSI_O_CPSR) = ulPreDiv;

    //
    // Set protocol and clock rate.
    //
    ulSPH_SPO = (ulProtocol & 3) << 6;
    ulProtocol &= SSI_CR0_FRF_M;
    ulRegVal = (ulSCR << 8) | ulSPH_SPO | ulProtocol | (ulDataWidth - 1);
    HWREG(ulBase + SSI_O_CR0) = ulRegVal;
}

//*****************************************************************************
//
//! Enables the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! This function enables operation of the synchronous serial interface.  The
//! synchronous serial interface must be configured before it is enabled.
//!
//! \return None.
//
//*****************************************************************************
void
SSIEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Read-modify-write the enable bit.
    //
    HWREG(ulBase + SSI_O_CR1) |= SSI_CR1_SSE;

}

//*****************************************************************************
//
//! Disables the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! This function disables operation of the synchronous serial interface.
//!
//! \return None.
//
//*****************************************************************************
void
SSIDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Read-modify-write the enable bit.
    //
    HWREG(ulBase + SSI_O_CR1) &= ~(SSI_CR1_SSE);
}



//*****************************************************************************
//
//! Unregisters an interrupt handler for the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! This function clears the handler to be called when an SSI interrupt
//! occurs.  This function also masks off the interrupt in the interrupt
//! controller so that the interrupt handler no longer is called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
SSIIntUnregister(unsigned long ulBase)
{
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Determine the interrupt number based on the SSI port.
    //
    ulInt = SSIIntNumberGet(ulBase);

    //
    // Disable the interrupt.
    //
    IntDisable(ulInt);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(ulInt);
}

//*****************************************************************************
//
//! Enables individual SSI interrupt sources.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulIntFlags is a bit mask of the interrupt sources to be enabled.
//!
//! This function enables the indicated SSI interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.  The \e ulIntFlags
//! parameter can be any of the \b SSI_TXFF, \b SSI_RXFF, \b SSI_RXTO, or
//! \b SSI_RXOR values.
//!
//! \return None.
//
//*****************************************************************************
void
SSIIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + SSI_O_IM) |= ulIntFlags;

    ssiBusy = true;
}

//*****************************************************************************
//
//! Disables individual SSI interrupt sources.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulIntFlags is a bit mask of the interrupt sources to be disabled.
//!
//! This function disables the indicated SSI interrupt sources.  The
//! \e ulIntFlags parameter can be any of the \b SSI_TXFF, \b SSI_RXFF,
//!  \b SSI_RXTO, or \b SSI_RXOR values.
//!
//! \return None.
//
//*****************************************************************************
void
SSIIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Disable the specified interrupts.
    //
    HWREG(ulBase + SSI_O_IM) &= ~(ulIntFlags);
}

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param ulBase specifies the SSI module base address.
//! \param bMasked is \b false if the raw interrupt status is required or
//! \b true if the masked interrupt status is required.
//!
//! This function returns the interrupt status for the SSI module.  Either the
//! raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \return The current interrupt status, enumerated as a bit field of
//! \b SSI_TXFF, \b SSI_RXFF, \b SSI_RXTO, and \b SSI_RXOR.
//
//*****************************************************************************
unsigned long
SSIIntStatus(unsigned long ulBase, tBoolean bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + SSI_O_MIS));
    }
    else
    {
        return(HWREG(ulBase + SSI_O_RIS));
    }
}

//*****************************************************************************
//
//! Clears SSI interrupt sources.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulIntFlags is a bit mask of the interrupt sources to be cleared.
//!
//! This function clears the specified SSI interrupt sources so that they no
//! longer assert.  This function must be called in the interrupt handler to
//! keep the interrupts from being triggered again immediately upon exit.  The
//! \e ulIntFlags parameter can consist of either or both the \b SSI_RXTO and
//! \b SSI_RXOR values.
//!
//! \note Because there is a write buffer in the Cortex-M processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \return None.
//
//*****************************************************************************
void
SSIIntClear(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Clear the requested interrupt sources.
    //
    HWREG(ulBase + SSI_O_ICR) = ulIntFlags;
}

//*****************************************************************************
//
//! Puts a data element into the SSI transmit FIFO.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulData is the data to be transmitted over the SSI interface.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SSI module.  If there is no space available in the transmit FIFO,
//! this function waits until there is space available before returning.
//!
//! \note The upper 32 - N bits of \e ulData are discarded by the hardware,
//! where N is the data width as configured by SSIConfigSetExpClk().  For
//! example, if the interface is configured for 8-bit data width, the upper 24
//! bits of \e ulData are discarded.
//!
//! \return None.
//
//*****************************************************************************
void
SSIDataPut(unsigned long ulBase, unsigned long ulData)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));
    ASSERT((ulData & (0xfffffffe << (HWREG(ulBase + SSI_O_CR0) &
                                     SSI_CR0_DSS_M))) == 0);

    //
    // Wait until there is space.
    //
    while(!(HWREG(ulBase + SSI_O_SR) & SSI_SR_TNF))
    {
    }

    //
    // Write the data to the SSI.
    //
    HWREG(ulBase + SSI_O_DR) = ulData;



}




//*****************************************************************************
//
//! Gets a data element from the SSI receive FIFO.
//!
//! \param ulBase specifies the SSI module base address.
//! \param pulData is a pointer to a storage location for data that was
//! received over the SSI interface.
//!
//! This function gets received data from the receive FIFO of the specified SSI
//! module and places that data into the location specified by the \e ulData
//! parameter.  If there is no data in the FIFO, then this function returns a
//! zero.
//!
//! This function replaces the original SSIDataNonBlockingGet() API and
//! performs the same actions.  A macro is provided in <tt>ssi.h</tt> to map
//! the original API to this API.
//!
//! \note Only the lower N bits of the value written to \e pulData contain
//! valid data, where N is the data width as configured by
//! SSIConfigSetExpClk().  For example, if the interface is configured for
//! 8-bit data width, only the lower 8 bits of the value written to \e pulData
//! contain valid data.
//!
//! \return Returns the number of elements read from the SSI receive FIFO.
//
//*****************************************************************************
long
SSIDataGetNonBlocking(unsigned long ulBase, unsigned long *pulData)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //
    // Check for data to read.
    //
    if(HWREG(ulBase + SSI_O_SR) & SSI_SR_RNE)
    {
        *pulData = HWREG(ulBase + SSI_O_DR);
        return(1);
    }
    else
    {
        return(0);
    }
}


//*****************************************************************************
//
//! Determines whether the SSI transmitter is busy or not.
//!
//! the function kicks off the interrup for the TX buffer, and returns the
//! Current result. This ensures that the parameter remains updated when data
//! hasn't been sent in a while. This prevents an error which puts 4 pixels of
//! garbage before each frame due to a busy check in the configuration portion
//
//*****************************************************************************
tBoolean
SSIBusy(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(SSIBaseValid(ulBase));

    //Enable the SSI interrupts
    SSIIntEnable(SSI0_BASE, SSI_TXFF);

    //
    // Determine if the SSI is busy.
    //
    return(ssiBusy);
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
