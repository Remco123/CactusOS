#ifndef __CACTUSOS__SYSTEM__COMPONENTS__EDID_H
#define __CACTUSOS__SYSTEM__COMPONENTS__EDID_H

#include <system/components/systemcomponent.h>

namespace CactusOS
{
    namespace system
    {
        struct EDIDInfoBlock
        {
            common::uint8_t header[8];
            common::uint16_t manufacturer_id;
            common::uint16_t product_id;
            common::uint32_t serial_number;
            common::uint8_t week_of_manufacture;
            common::uint8_t year_of_manufacture;
            common::uint8_t version;
            common::uint8_t revision;

            common::uint8_t video_input_definition;
            common::uint8_t max_horizontal_image_size;
            common::uint8_t max_vertical_image_size;
            common::uint8_t display_gamma;
            common::uint8_t feature_support;

            common::uint8_t red_green_lo;
            common::uint8_t blue_white_lo;
            common::uint8_t red_x_hi;
            common::uint8_t red_y_hi;
            common::uint8_t green_x_hi;
            common::uint8_t green_y_hi;
            common::uint8_t blue_x_hi;
            common::uint8_t blue_y_hi;
            common::uint8_t white_x_hi;
            common::uint8_t white_y_hi;

            common::uint8_t established_timings_1;
            common::uint8_t established_timings_2;
            common::uint8_t manufacturer_reserved_timings;

            common::uint16_t standard_timings[8];

            struct {
                common::uint16_t flag; //0 when used as discriptor, otherwise timing
                common::uint8_t flag2;
                common::uint8_t dataType;
                common::uint8_t flag3;
                common::uint8_t descriptorData[13];
            } detailed_timings[4];

            common::uint8_t extension_flag;
            common::uint8_t checksum;
        } __attribute__((packed));

        struct TimingsInfoBlock
        {
            common::uint16_t pixel_clock;
            /* Only valid if the pixel clock is non-null.  */
            common::uint8_t horizontal_active_lo;
            common::uint8_t horizontal_blanking_lo;
            common::uint8_t horizontal_hi;
            common::uint8_t vertical_active_lo;
            common::uint8_t vertical_blanking_lo;
            common::uint8_t vertical_hi;
            common::uint8_t horizontal_sync_offset_lo;
            common::uint8_t horizontal_sync_pulse_width_lo;
            common::uint8_t vertical_sync_lo;
            common::uint8_t sync_hi;
            common::uint8_t horizontal_image_size_lo;
            common::uint8_t vertical_image_size_lo;
            common::uint8_t image_size_hi;
            common::uint8_t horizontal_border;
            common::uint8_t vertical_border;
            common::uint8_t flags;
        } __attribute__((packed));


        #define EDID_SUCCES 0x004F
        
        class EDID : public SystemComponent
        {
        public:
            EDID();

            void AcquireEDID();
            void PreferedMode(int* widthPtr, int* heightPtr);
        };
    }
}

#endif