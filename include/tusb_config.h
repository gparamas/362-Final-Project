#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_


#ifdef __cplusplus
extern "C" {
#endif


// MCU 
#define CFG_TUSB_MCU OPT_MCU_RP2040

// OS
#define CFG_TUSB_OS OPT_OS_PICO

// Debug (0 = off,
#define CFG_TUSB_DEBUG 0

#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_HOST



// Enable HOST stack
#define CFG_TUH_ENABLED 1


#define CFG_TUD_ENABLED 0


#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED

//--------------------------------------------------------------------+
// HOST CLASS CONFIGURATION


#define CFG_TUH_HID 4   


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
