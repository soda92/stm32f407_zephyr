*** Settings ***
Suite Setup                   Setup
Suite Teardown                Teardown
Test Setup                    App Test Setup
Test Teardown                 Test Teardown
Resource                      ${RENODEKEYWORDS}

*** Variables ***
${UART}                       sysbus.usart1
${BTN_SAVE}                   sysbus.gpioPortA.pin1
${URI}                        @${CURDIR}/../build/zephyr/zephyr.elf

*** Keywords ***
App Test Setup
    Reset Emulation
    Execute Command           include @scripts/single-node/stm32f4_discovery.resc

Press Button
    [Arguments]               ${button}
    Execute Command           ${button} Press
    Sleep                     100ms
    Execute Command           ${button} Release

*** Test Cases ***
Should Boot and Respond to Buttons
    Execute Command           sysbus LoadELF ${URI}
    Create Terminal Tester    ${UART}
    Start Emulation

    # 1. Wait for boot
    Wait For Line On Uart     *** Booting Zephyr OS ***
    
    # 2. Match the records found (Renode returns 0x00 for empty SPI, so it finds 1024 records)
    Wait For Line On Uart     Found 1024 saved records

    # 3. Test the Save Button (PA1)
    # This will trigger the "Flash full, erasing history" warning in your code
    Press Button              ${BTN_SAVE}
    Wait For Line On Uart     Flash full, erasing history...
    Wait For Line On Uart     Saved Temp 0.00 to flash at index 0
