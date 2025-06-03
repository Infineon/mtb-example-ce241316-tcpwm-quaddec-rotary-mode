/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for TCPWM QuadDec rotary mode Example 
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <inttypes.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define TIME_10MS      10
#define TIME_1MS        1

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Interrupt configuration */
const cy_stc_sysint_t IRQ_CFG =
{
#if defined (CY_IP_M7CPUSS)
    /* For body high */
    .intrSrc = ((NvicMux3_IRQn << CY_SYSINT_INTRSRC_MUXIRQ_SHIFT) | QuadDec_IRQ),
#else
    /* For body Entry */
    .intrSrc = (IRQn_Type)((NvicMux3_IRQn << CY_SYSINT_INTRSRC_MUXIRQ_SHIFT) | QuadDec_IRQ),
#endif
    .intrPriority = 3UL
};

/* Variable */
uint8_t  uart_read_value;
int8_t   cnt = 0;
int8_t   flag_enc = 1;
uint32_t value;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void isr_QuadDec(void);

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. 
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
            CYBSP_DEBUG_UART_CTS,CYBSP_DEBUG_UART_RTS,CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize TCPWM as quadrature decoder */
    Cy_TCPWM_QuadDec_Init(QuadDec_HW, QuadDec_NUM, &QuadDec_config);

    /* Interrupt settings */
    Cy_SysInt_Init(&IRQ_CFG, &isr_QuadDec);
    NVIC_ClearPendingIRQ(NvicMux3_IRQn);
    NVIC_EnableIRQ((IRQn_Type) NvicMux3_IRQn);

    /* Enable TCPWM as quadrature decoder */
    Cy_TCPWM_QuadDec_Enable(QuadDec_HW, QuadDec_NUM);

    /* Start TCPWM as quadrature decoder */
    Cy_TCPWM_TriggerReloadOrIndex_Single(QuadDec_HW, QuadDec_NUM);

    /* Initialize the ENC_A, ENC_B */
    Cy_GPIO_Pin_Init(ENC_A_PORT, ENC_A_PIN, &ENC_A_config);
    Cy_GPIO_Pin_Init(ENC_B_PORT, ENC_B_PIN, &ENC_B_config);
    Cy_GPIO_Clr(ENC_A_PORT, ENC_A_PIN);
    Cy_GPIO_Clr(ENC_B_PORT, ENC_B_PIN);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("********************************************************************************\r\n");
    printf("PDL: Rotary counter mode quadrature decoder example\r\n");
    printf("********************************************************************************\r\n");
    printf("Press 'D' Key to decrease the rotary counter. ");
    printf("Press 'I' key to increase the rotary counter.\r\n\r\n");

    for (;;)
    {
        /* Check if 'I' or 'D' key was pressed */
        if (cyhal_uart_getc(&cy_retarget_io_uart_obj, &uart_read_value, 1)
             == CY_RSLT_SUCCESS)
        {
            /* Change increment and decrement */
            if (uart_read_value == 'i')
            {
                flag_enc = 1;
            }
            if (uart_read_value == 'd')
            {
                flag_enc = 0;
            }
        }

        /* Change waveform for PhiA and PhiB */
        if (flag_enc == 1)
        {
            Cy_GPIO_Inv(ENC_A_PORT, ENC_A_PIN);
            Cy_GPIO_Clr(ENC_B_PORT, ENC_B_PIN);
        }
        else {
            Cy_GPIO_Inv(ENC_B_PORT, ENC_B_PIN);
            Cy_GPIO_Clr(ENC_A_PORT, ENC_A_PIN);
        }

        if(cnt++ > TIME_10MS)
        {
            /* Read and quadrature decoder Counter */
            cnt = 0;
            value = Cy_TCPWM_QuadDec_GetCounter(QuadDec_HW, QuadDec_NUM);
            /* Display counter value */
            printf("Counter value: 0x%" PRIx32 "\r\n", value);
            printf("\x1b[1F");
        }

        /* Delay */
        Cy_SysLib_Delay(TIME_1MS);
    }
}
/*******************************************************************************
* Function Name: isr_QuadDec
********************************************************************************
* Summary:
* This is the interrupt handler function for the QuadDec interrupt.
*
* Parameters:
*  none
*
* Return:
*  void
*******************************************************************************/
void isr_QuadDec(void)
{
    /* Get interrupt source */
    uint32_t intrMask = Cy_TCPWM_GetInterruptStatusMasked(QuadDec_HW, QuadDec_NUM);

    /* Change increment and decrement */
    if (flag_enc == 0)
    {
        flag_enc = 1;
    }
    else if (flag_enc == 1)
    {
        flag_enc = 0;
    }

    /* Clear interrupt source */
    Cy_TCPWM_ClearInterrupt(QuadDec_HW, QuadDec_NUM, intrMask);
}
/* [] END OF FILE */
