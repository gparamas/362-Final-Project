#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_


#ifdef __cplusplus
extern "C" {
#endif


// MCU 
#define CFG_TUSB_MCU OPT_MCU_RP2040

// OS
#define CFG_TUSB_OS OPT_OS_PICO

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_HOST
#endif

#ifndef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED 1
#endif

#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED 0
#endif

#ifndef CFG_TUH_MAX_SPEED
#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED
#endif

#ifndef CFG_TUH_HID
#define CFG_TUH_HID 4
#endif


#ifndef CFG_TUSB_MEM_SECTION
  #define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
  #define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
