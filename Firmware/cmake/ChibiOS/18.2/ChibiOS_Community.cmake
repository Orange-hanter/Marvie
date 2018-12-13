set( CHIBIOS_SOURCES_community_hal_COMP         community/os/hal/src/hal_comp.c )
set( CHIBIOS_SOURCES_community_hal_CRC          community/os/hal/src/hal_crc.c )
set( CHIBIOS_SOURCES_community_hal_EEPROM       community/os/hal/src/hal_eeprom.c 
                                                community/os/hal/src/hal_ee24xx.c
                                                community/os/hal/src/hal_ee25xx.c )
set( CHIBIOS_SOURCES_community_hal_EICU         community/os/hal/src/hal_eicu.c )
set( CHIBIOS_SOURCES_community_hal_NAND         community/os/hal/src/hal_nand.c )
set( CHIBIOS_SOURCES_community_hal_ONEWIRE      community/os/hal/src/hal_onewire.c )
set( CHIBIOS_SOURCES_community_hal_QEI          community/os/hal/src/hal_qei.c )
set( CHIBIOS_SOURCES_community_hal_RNG          community/os/hal/src/hal_rng.c )
set( CHIBIOS_SOURCES_community_hal_TIMCAP       community/os/hal/src/hal_timcap.c )
set( CHIBIOS_SOURCES_community_hal_USB_HID      community/os/hal/src/hal_usb_hid.c )
set( CHIBIOS_SOURCES_community_hal_USB_MSD      community/os/hal/src/hal_usb_msd.c )
set( CHIBIOS_SOURCES_community_hal_USBH         community/os/hal/src/hal_usbh.c 
                                                community/os/hal/src/usbh/hal_usbh_debug.c
                                                community/os/hal/src/usbh/hal_usbh_desciter.c
                                                community/os/hal/src/usbh/hal_usbh_hub.c
                                                community/os/hal/src/usbh/hal_usbh_msd.c
                                                community/os/hal/src/usbh/hal_usbh_ftdi.c
                                                community/os/hal/src/usbh/hal_usbh_aoa.c
                                                community/os/hal/src/usbh/hal_usbh_hid.c
                                                community/os/hal/src/usbh/hal_usbh_uvc.c )



set( CHIBIOS_SOURCES_community_hal_F0 )
set( CHIBIOS_SOURCES_community_hal_CRC_F0       community/os/hal/ports/STM32/LLD/CRCv1/hal_crc_lld.c )
set( CHIBIOS_SOURCES_community_hal_QEI_F0       community/os/hal/ports/STM32/LLD/TIMv1/hal_qei_lld.c )
set( CHIBIOS_SOURCES_community_hal_TIMCAP_F0    community/os/hal/ports/STM32/LLD/TIMv1/hal_timcap_lld.c )

set( CHIBIOS_INCLUDES_community_hal_F0 )
set( CHIBIOS_INCLUDES_community_hal_CRC_F0      community/os/hal/ports/STM32/LLD/CRCv1 )
set( CHIBIOS_INCLUDES_community_hal_QEI_F0      community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_TIMCAP_F0   community/os/hal/ports/STM32/LLD/TIMv1 )



set( CHIBIOS_SOURCES_community_hal_F1 )
set( CHIBIOS_SOURCES_community_hal_CRC_F1       community/os/hal/ports/STM32/LLD/CRCv1/hal_crc_lld.c )
set( CHIBIOS_SOURCES_community_hal_EICU_F1      community/os/hal/ports/STM32/LLD/TIMv1/hal_eicu_lld.c )
set( CHIBIOS_SOURCES_community_hal_NAND_F1      community/os/hal/ports/STM32/LLD/FSMCv1/hal_nand_lld.c )
set( CHIBIOS_SOURCES_community_hal_QEI_F1       community/os/hal/ports/STM32/LLD/TIMv1/hal_qei_lld.c )
set( CHIBIOS_SOURCES_community_hal_TIMCAP_F1    community/os/hal/ports/STM32/LLD/TIMv1/hal_timcap_lld.c )

set( CHIBIOS_INCLUDES_community_hal_F1 )
set( CHIBIOS_INCLUDES_community_hal_CRC_F1      community/os/hal/ports/STM32/LLD/CRCv1 )
set( CHIBIOS_INCLUDES_community_hal_EICU_F1     community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_NAND_F1     community/os/hal/ports/STM32/LLD/FSMCv1 )
set( CHIBIOS_INCLUDES_community_hal_QEI_F1      community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_TIMCAP_F1   community/os/hal/ports/STM32/LLD/TIMv1 )



set( CHIBIOS_SOURCES_community_hal_F3 )
set( CHIBIOS_SOURCES_community_hal_COMP_F3      community/os/hal/ports/STM32/LLD/COMPv1/hal_comp_lld.c )
set( CHIBIOS_SOURCES_community_hal_CRC_F3       community/os/hal/ports/STM32/LLD/CRCv1/hal_crc_lld.c )
set( CHIBIOS_SOURCES_community_hal_EICU_F3      community/os/hal/ports/STM32/LLD/TIMv1/hal_eicu_lld.c )
set( CHIBIOS_SOURCES_community_hal_QEI_F3       community/os/hal/ports/STM32/LLD/TIMv1/hal_qei_lld.c )
set( CHIBIOS_SOURCES_community_hal_TIMCAP_F3    community/os/hal/ports/STM32/LLD/TIMv1/hal_timcap_lld.c )

set( CHIBIOS_INCLUDES_community_hal_F3 )
set( CHIBIOS_INCLUDES_community_hal_COMP_F3     community/os/hal/ports/STM32/LLD/COMPv1 )
set( CHIBIOS_INCLUDES_community_hal_CRC_F3      community/os/hal/ports/STM32/LLD/CRCv1 )
set( CHIBIOS_INCLUDES_community_hal_EICU_F3     community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_QEI_F3      community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_TIMCAP_F3   community/os/hal/ports/STM32/LLD/TIMv1 )



set( CHIBIOS_SOURCES_community_hal_F4           community/os/hal/ports/STM32/LLD/DMA2Dv1/hal_stm32_dma2d.c )
set( CHIBIOS_SOURCES_community_hal_CRC_F4       community/os/hal/ports/STM32/LLD/CRCv1/hal_crc_lld.c )
set( CHIBIOS_SOURCES_community_hal_EICU_F4      community/os/hal/ports/STM32/LLD/TIMv1/hal_eicu_lld.c )
set( CHIBIOS_SOURCES_community_hal_NAND_F4      community/os/hal/ports/STM32/LLD/FSMCv1/hal_nand_lld.c )
set( CHIBIOS_SOURCES_community_hal_QEI_F4       community/os/hal/ports/STM32/LLD/TIMv1/hal_qei_lld.c )
set( CHIBIOS_SOURCES_community_hal_TIMCAP_F4    community/os/hal/ports/STM32/LLD/TIMv1/hal_timcap_lld.c )
set( CHIBIOS_SOURCES_community_hal_USBH_F4      community/os/hal/ports/STM32/LLD/USBHv1/hal_usbh_lld.c )

set( CHIBIOS_INCLUDES_community_hal_F4          community/os/hal/ports/STM32/LLD/DMA2Dv1 )
set( CHIBIOS_INCLUDES_community_hal_CRC_F4      community/os/hal/ports/STM32/LLD/CRCv1 )
set( CHIBIOS_INCLUDES_community_hal_EICU_F4     community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_NAND_F4     community/os/hal/ports/STM32/LLD/FSMCv1 )
set( CHIBIOS_INCLUDES_community_hal_QEI_F4      community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_TIMCAP_F4   community/os/hal/ports/STM32/LLD/TIMv1 )
set( CHIBIOS_INCLUDES_community_hal_USBH_F4     community/os/hal/ports/STM32/LLD/USBHv1 )



set( CHIBIOS_SOURCES_community_hal_F7 )
set( CHIBIOS_SOURCES_community_hal_NAND_F7      community/os/hal/ports/STM32/LLD/FSMCv1/hal_nand_lld.c )

set( CHIBIOS_INCLUDES_community_hal_F7 )
set( CHIBIOS_INCLUDES_community_hal_NAND_F7     community/os/hal/ports/STM32/LLD/FSMCv1 )

list( FIND CHIBIOS_COMMUNITY_HAL_COMPONENTS COMMUNITY community_index )
if( community_index LESS 0 )
    return()
endif()

list( APPEND CHIBIOS_SOURCES_hal community/os/hal/src/hal_community.c )
list( APPEND CHIBIOS_INCLUDES_hal community/os/hal/include )

if( CHIBIOS_SOURCES_community_hal_${STM32_FAMILY} )
    list( APPEND CHIBIOS_SOURCES_hal ${CHIBIOS_SOURCES_community_hal_${STM32_FAMILY}} )
endif()
    
if( CHIBIOS_INCLUDES_community_hal_${STM32_FAMILY} )
    list( APPEND CHIBIOS_INCLUDES_hal ${CHIBIOS_INCLUDES_community_hal_${STM32_FAMILY}} )
endif()

foreach( comp ${CHIBIOS_COMMUNITY_HAL_COMPONENTS} )
    if( CHIBIOS_SOURCES_community_hal_${comp}_${STM32_FAMILY} )
        list( APPEND CHIBIOS_SOURCES_hal ${CHIBIOS_SOURCES_community_hal_${comp}_${STM32_FAMILY}} )
    endif()
    if( CHIBIOS_INCLUDES_community_hal_${comp}_${STM32_FAMILY} )
        list( APPEND CHIBIOS_INCLUDES_hal ${CHIBIOS_INCLUDES_community_hal_${comp}_${STM32_FAMILY}} )
    endif()
endforeach()