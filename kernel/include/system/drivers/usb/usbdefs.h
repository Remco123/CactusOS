#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__USBDEFS_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__USBDEFS_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        // Reset wait times.  USB 2.0 specs, page 153, section 7.1.7.5, paragraph 3
        #define USB_TDRSTR   50   // reset on a root hub
        #define USB_TDRST    10   // minimum delay for a reset
        #define USB_TRHRSI    3   // No more than this between resets for root hubs
        #define USB_TRSTRCY  10   // reset recovery
        #define PINDC_OFF 0b00
        #define PINDC_AMBER 0b01
        #define PINDC_GREEN 0b10
        #define ENDP_CONTROL 0

        struct DEVICE_DESC {
            common::uint8_t  len;
            common::uint8_t  type;
            common::uint16_t usb_ver;
            common::uint8_t  _class;
            common::uint8_t  subclass;
            common::uint8_t  protocol;
            common::uint8_t  max_packet_size;
            common::uint16_t vendorid;
            common::uint16_t productid;
            common::uint16_t device_rel;
            common::uint8_t  manuf_indx;   // index value
            common::uint8_t  prod_indx;    // index value
            common::uint8_t  serial_indx;  // index value
            common::uint8_t  configs;      // Number of configurations
        } __attribute__((packed));

        struct REQUEST_PACKET {
            common::uint8_t  request_type;
            common::uint8_t  request;
            common::uint16_t value;
            common::uint16_t index;
            common::uint16_t length;
        } __attribute__((packed));

        // config descriptor
        struct CONFIG_DESC {
            common::uint8_t  len;
            common::uint8_t  type;
            common::uint16_t tot_len;
            common::uint8_t  num_interfaces;
            common::uint8_t  config_val;
            common::uint8_t  config_indx;
            common::uint8_t  bm_attrbs;
            common::uint8_t  max_power;
        } __attribute__((packed));

        struct STRING_DESC {
            common::uint8_t  len;         // length of whole desc in bytes
            common::uint8_t  type;
            common::uint16_t string[127];
        } __attribute__((packed));

        struct INTERFACE_ASSOSIATION_DESC {
            common::uint8_t  len;             // len of this desc (8)
            common::uint8_t  type;            // type = 11
            common::uint8_t  interface_num;   // first interface number to start association
            common::uint8_t  count;           // count of continuous interfaces for association
            common::uint8_t  _class;          //
            common::uint8_t  subclass;        //
            common::uint8_t  protocol;        //
            common::uint8_t  function_indx;   // string id of this association
        } __attribute__((packed));

        // interface descriptor
        struct INTERFACE_DESC {
            common::uint8_t  len;
            common::uint8_t  type;
            common::uint8_t  interface_num;
            common::uint8_t  alt_setting;
            common::uint8_t  num_endpoints;
            common::uint8_t  interface_class;
            common::uint8_t  interface_sub_class;
            common::uint8_t  interface_protocol;
            common::uint8_t  interface_indx;
        } __attribute__((packed));

        // endpoint descriptor
        struct ENDPOINT_DESC {
            common::uint8_t  len;
            common::uint8_t  type;
            common::uint8_t  end_point;        // 6:0 end_point number, 7 = IN (set) or OUT (clear)
            common::uint8_t  bm_attrbs;        // 
            common::uint16_t max_packet_size;  // 10:0 = max_size, 12:11 = max transactions, 15:13 = reserved
            common::uint8_t  interval;
        } __attribute__((packed));

        struct IF_HID_DESC {
            common::uint8_t  len;
            common::uint8_t  type;
            common::uint16_t release;
            common::uint8_t  countryCode;
            common::uint8_t  numDescriptors;
            // Types and lenght folowing below 
        } __attribute__((packed));

        // setup packets
        #define DEV_TO_HOST     0x80
        #define HOST_TO_DEV     0x00
        #define REQ_TYPE_STNDRD 0x00
        #define REQ_TYPE_CLASS  0x20
        #define REQ_TYPE_VENDOR 0x40
        #define REQ_TYPE_RESV   0x60
        #define RECPT_DEVICE    0x00
        #define RECPT_INTERFACE 0x01
        #define RECPT_ENDPOINT  0x02
        #define RECPT_OTHER     0x03
        #define STDRD_GET_REQUEST   (DEV_TO_HOST | REQ_TYPE_STNDRD | RECPT_DEVICE)
        #define STDRD_SET_REQUEST   (HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_DEVICE)
        #define STDRD_SET_INTERFACE (HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_INTERFACE)

        // device requests
        enum DeviceRequest { GET_STATUS=0, CLEAR_FEATURE, SET_FEATURE=3, SET_ADDRESS=5, GET_DESCRIPTOR=6, SET_DESCRIPTOR,
                GET_CONFIGURATION, SET_CONFIGURATION,
        // interface requests
                GET_INTERFACE, SET_INTERFACE,
        // standard endpoint requests
                SYNCH_FRAME,
        // Device specific
                GET_MAX_LUNS = 0xFE, BULK_ONLY_RESET
        };

        // Descriptor types
        enum DescriptorTypes { 
            DEVICE=1, 
            CONFIG, 
            STRING, 
            INTERFACE, 
            ENDPOINT, 
            DEVICE_QUALIFIER,
            OTHER_SPEED_CONFIG, 
            INTERFACE_POWER, 
            OTG, 
            DEBUG, 
            INTERFACE_ASSOSIATION,
            
            HID=0x21,
            HID_REPORT, 
            HID_PHYSICAL, 
            
            INTERFACE_FUNCTION = 0x24,
            ENDPOINT_FUNCTION,
            
            HUB = 0x29
        };
    }
}

#endif