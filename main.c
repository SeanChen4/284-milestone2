

// Standard includes
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"


#define APPLICATION_VERSION     "1.4.0"
//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     10000

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static unsigned char g_ucTxBuff[TR_BUFF_SIZE];
static unsigned char g_ucRxBuff[TR_BUFF_SIZE];
static unsigned char ucTxBuffNdx;
static unsigned char ucRxBuffNdx;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//
//! SPI Slave Interrupt handler
//!
//! This function is invoked when SPI slave has its receive register full or
//! transmit register empty.
//!
//! \return None.
//
//*****************************************************************************
static void SlaveIntHandler()
{
    unsigned long ulRecvData;
    unsigned long ulStatus;

    ulStatus = MAP_SPIIntStatus(GSPI_BASE,true);

    MAP_SPIIntClear(GSPI_BASE,SPI_INT_RX_FULL|SPI_INT_TX_EMPTY);

    if(ulStatus & SPI_INT_TX_EMPTY)
    {
        MAP_SPIDataPut(GSPI_BASE,g_ucTxBuff[ucTxBuffNdx%TR_BUFF_SIZE]);
        ucTxBuffNdx++;
    }

    if(ulStatus & SPI_INT_RX_FULL)
    {
        MAP_SPIDataGetNonBlocking(GSPI_BASE,&ulRecvData);
        g_ucTxBuff[ucRxBuffNdx%TR_BUFF_SIZE] = ulRecvData;
        Report("%c",ulRecvData);
        ucRxBuffNdx++;
    }
}

//*****************************************************************************
//
//! SPI Master mode main loop
//!
//! This function configures SPI modelue as master and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************
void MasterMain()
{

    unsigned long ulUserData;
    unsigned long ulDummy;

    //
    // Initialize the message
    //
    memcpy(g_ucTxBuff,MASTER_MSG,sizeof(MASTER_MSG));

    //
    // Set Tx buffer index
    //
    ucTxBuffNdx = 0;
    ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVELOW |
                     SPI_WL_32));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    //
    // Print mode on uart
    //
    //Message("Enabled SPI Interface in Master Mode\n\r");

    //
    // User input
    //
    //Report("Press any key to transmit data....");

    //
    // Read a character from UART terminal
    //
    ulUserData = MAP_UARTCharGet(UARTA0_BASE);


    //
    // Send the string to slave. Chip Select(CS) needs to be
    // asserted at start of transfer and deasserted at the end.
    //
    MAP_SPITransfer(GSPI_BASE,g_ucTxBuff,g_ucRxBuff,50,
            SPI_CS_ENABLE|SPI_CS_DISABLE);

    //
    // Report to the user
    //
    //Report("\n\rSend      %s",g_ucTxBuff);
    //Report("Received  %s",g_ucRxBuff);

    //
    // Print a message
    //
    //Report("\n\rType here (Press enter to exit) :");

    //
    // Initialize variable
    //
    ulUserData = 0;

    //
    // Enable Chip select
    //
    MAP_SPICSEnable(GSPI_BASE);



    MAP_SPIDataPut(GSPI_BASE,ulUserData);

    ulUserData = ulUserData << 30



    MAP_SPIDataGet(GSPI_BASE,&ulDummy);

    if(ulDummy>=1024){
        ulDummy = 1023;
    }

    for(i=0;i<10;i++){
        sum = sum + ulDummy/10;
        if(i=9){
            i = 1;
            Report("Data:%f",sum);

        }
    }
    //
    // Loop until user "Enter Key" is
    // pressed
    //
    /*while(ulUserData != '\r')
    {
        //
        // Read a character from UART terminal
        //
        ulUserData = MAP_UARTCharGet(UARTA0_BASE);

        //
        // Echo it back
        //
        MAP_UARTCharPut(UARTA0_BASE,ulUserData);

        //
        // Push the character over SPI
        //
        MAP_SPIDataPut(GSPI_BASE,ulUserData);

        //
        // Clean up the receive register into a dummy
        // variable
        //
        MAP_SPIDataGet(GSPI_BASE,&ulDummy);
    }*/

    //
    // Disable chip select
    //
    MAP_SPICSDisable(GSPI_BASE);
}

//*****************************************************************************
//
//! SPI Slave mode main loop
//!
//! This function configures SPI modelue as slave and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************

static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Main function for spi demo application
//!
//! \param none
//!
//! \return None.
//
//*****************************************************************************
void main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Muxing UART and SPI lines.
    //
    PinMuxConfig();

    //
    // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    //
    // Initialising the Terminal.
    //
    InitTerm();

    //
    // Clearing the Terminal.
    //
    ClearTerm();

    //
    // Display the Banner
    //
    Message("\n\n\n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\t\t        CC3200 SPI Demo Application  \n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\n\n\n\r");

    //
    // Reset the peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);

#if MASTER_MODE

    MasterMain();

#else

    SlaveMain();

#endif

    while(1)
    {

    }

}

