#include <system/usb/hidparser.h>
#include <common/memoryoperations.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

#define HID_SHOW_INFO 0

char spaces_buff[33];
char* spaces(unsigned cnt) {
    if (cnt > 32)
        return "**";

    MemoryOperations::memset(spaces_buff, ' ', 32);
    spaces_buff[cnt] = 0;
    return spaces_buff;
}
char usage_type_str[][64] = {
    "Undefined", "Generic Desktop", "Simulation", "VR", "Sport", "Game",
    "Generic Device", "Keyboard/Keypad", "LEDs", "Button", "Ordinal", "Telephony", "Consumer", "Digitizer",
    "Reserved", "PID Page", "Unicode", "Reserved", "Reserved", "Reserved", "Alphanumeric Display",

    // offset 0x15
    "Medical Insturments", "Monitor Pages", "Power Pages",

    "Bar Code Scanner page", "Scale page", "Magnetic Stripe Reading (MSR) Devices", "Reserved Point of Sale pages",
    "Camera Control Page" , "Arcade Page"
};

const char* hid_print_usage_type(unsigned type) {
    if (type <= 0x14)
        return usage_type_str[type];
    else if (type <= 0x3F)
        return usage_type_str[0x13];
    else if (type <= 0x40)
        return usage_type_str[0x15];
    else if (type <= 0x7F)
        return usage_type_str[0x13];
    else if (type <= 0x83)
        return usage_type_str[0x16];
    else if (type <= 0x87)
        return usage_type_str[0x17];
    else if (type <= 0x8B)
        return usage_type_str[0x13];
    else if (type <= 0x91)
        return usage_type_str[type - 24];
    else if (type <= 0xFEFF)
        return usage_type_str[0x13];
    else
        return "Error: type > 0xFFFF";
}

char usage_str_page_1[][64] = {
    "Undefined", "Pointer", "Mouse", "Reserved", "Joystick", "Game Pad", "Keyboard", "Keypad", "Multi-axis Controller"
    "Tablet PC System Controls", 

    "X", "Y", "Z", "Rx", "Ry", "Rz", "Slider", "Dial", "Wheel", "Hat switch", "Counted Buffer", "Byte Count", "Motion Wakeup",
    "Start", "Select", "Reserved", "Vx", "Vy", "Vz", "Vbrx", "Vbry", "Vbrz", "Vno", "Feature Notification",
    "Resolution Multiplier",

    "System Control", "System Power Down", "System Sleep", "System Wake Up", "System Context Menu", "System Main Menu",
    "System App Menu", "System Menu Help", "System Menu Exit", "System Menu Select", "System Menu Right", "System Menu Left",
    "System Menu Up", "System Menu Down", "System Cold Restart", "System Warm Restart", "D-pad Up", "D-pad Down",
    "D-pad Right", "D-pad Left", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "System Dock",
    "System Undock", "System Setup", "System Break", "System Debugger Break", "Application Break", "Application Debugger Break",
    "System Speaker Mute", "System Hibernate", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "System Display Invert", "System Display Internal", "System Display External", "System Display Both", "System Display Dual",
    "System Display Toggle Int/Ext", "System Display Swap", "Primary/Secondary", "System Display LCD Autoscale"
};

const char* hid_print_usage(unsigned page, unsigned type) {
    switch (page) {
        case 1:
        if (type <= 0x09)
            return usage_str_page_1[type];
        else if (type <= 0x2F)
            return usage_str_page_1[0x03];
        else if (type <= 0x48)
            return usage_str_page_1[type - 39];
        else if (type <= 0x7F)
            return usage_type_str[0x03];
        else if (type <= 0xB7)
            return usage_type_str[type - 39 - 55]; /// may be off by a few
        else if (type <= 0xFFFF)
            return usage_type_str[0x03];
        else
            return " Error: type > 0xFFFF";
        break;
        
        default:
        return "Unsupported Usage Page";
    }
}

char collection_str[][64] = {
    "Physical", "Application", "Logical", "Report", "Named Array", "Usage Switch", "Usage Modifier"
};

const char* hid_print_collection(unsigned val) {
    if (val <= 0x06)
        return collection_str[val];
    else if (val <= 0x7F)
        return "Reserved";
    else if (val <= 0xFF)
        return "Vendor-defined";
    else
        return "Error: val > 0xFF";
}
uint32_t format_value(uint32_t value, uint8_t size) {
    if (size == 1) 
        value = (uint32_t) (uint8_t) value;
    else if (size == 2) 
        value = (uint32_t) (uint16_t) value;
    return value;
}


bool HIDParser::Parse(struct HID_DATA* data)
{
    bool found = false;
    static unsigned space_cnt = 0;
    static bool did_collection = false;
    static int item_size[4] = { 0, 1, 2, 4 };
    
    while (!found && (this->pos < this->report_desc_size)) {
        // Get new this->item if current this->count is empty 
        if (this->count == 0) {
            if (HID_SHOW_INFO) Log(Info, "\n %02X ", this->report_desc[this->pos]);
            this->item = this->report_desc[this->pos++];
            this->value = 0;
            MemoryOperations::memcpy(&this->value, &this->report_desc[this->pos], item_size[this->item & SIZE_MASK]);
            if (HID_SHOW_INFO) {
                for (int t=0; t<4; t++) {
                    if (t < item_size[this->item & SIZE_MASK])
                        Log(Info, "%02X ", this->report_desc[this->pos + t]);
                    else
                        Log(Info, "   ");
                }
            }
            
            // Pos on next item
            this->pos += item_size[this->item & SIZE_MASK];
        }

        if((this->item & ITEM_MASK) == 0)
            continue;
        
        //Log(Info, "\n (this->item & ITEM_MASK) = 0x%04X", (this->item & ITEM_MASK));
        switch (this->item & ITEM_MASK) {
            case ITEM_UPAGE:
                // Copy upage in usage stack
                this->u_page = (uint16_t) this->value;
                if (HID_SHOW_INFO) Log(Info, "%sUsage Page (%s)", spaces(space_cnt), hid_print_usage_type(this->u_page));
                
                // copy to the usage table, but do not increment the counter incase there is a USAGE entry
                this->usage_table[this->usage_size].u_page = this->u_page;
                this->usage_table[this->usage_size].usage = 0xFF;
                break;
                
            case ITEM_USAGE:
                // Copy global or local u_page if any, in usage stack
                if ((this->item & SIZE_MASK) > 2)
                this->usage_table[this->usage_size].u_page = (uint16_t) (this->value >> 16);
                else
                this->usage_table[this->usage_size].u_page = this->u_page;
                
                // Copy Usage in Usage stack
                this->usage_table[this->usage_size].usage = (uint16_t) (this->value & 0xFFFF);
                if (HID_SHOW_INFO) Log(Info, "%sUsage (%s)", spaces(space_cnt), hid_print_usage(this->u_page, (uint16_t) (this->value & 0xFFFF)));
                
                // Increment Usage stack size
                this->usage_size++;
                
                break;
                
            case ITEM_USAGE_MIN:
                // TODO: is usage_min and max does this way, and are they bit32s or smaller?
                this->usage_min = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sUsage min (%d)", spaces(space_cnt), this->usage_min);
                break;
                
            case ITEM_USAGE_MAX:
                this->usage_max = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sUsage max (%d)", spaces(space_cnt), this->usage_max);
                break;
                
            case ITEM_COLLECTION:
                // Get UPage/Usage from usage_table and store them in this->Data.Path
                this->data.path.node[this->data.path.size].u_page = this->usage_table[0].u_page;
                this->data.path.node[this->data.path.size].usage = this->usage_table[0].usage;
                this->data.path.size++;
                
                // Unstack u_page/Usage from usage_table (never remove the last)
                if (this->usage_size > 0) {
                    uint8_t ii=0;
                    while (ii < this->usage_size) {
                        this->usage_table[ii].usage = this->usage_table[ii+1].usage;
                        this->usage_table[ii].u_page = this->usage_table[ii+1].u_page;
                        ii++;
                    }
                    // Remove Usage
                    this->usage_size--;
                }
                
                // Get Index if any
                if (this->value >= 0x80) {
                    this->data.path.node[this->data.path.size].u_page = 0xFF;
                    this->data.path.node[this->data.path.size].usage = (uint16_t) (this->value & 0x7F);
                    this->data.path.size++;
                }
                
                if (HID_SHOW_INFO) {
                    Log(Info, "%sCollection (%s)", spaces(space_cnt), hid_print_collection(this->value));
                    space_cnt += 2;
                }
                break;
                
            case ITEM_END_COLLECTION:
                this->data.path.size--;
                // Remove Index if any
                if (this->data.path.node[this->data.path.size].u_page == 0xFF)
                    this->data.path.size--;

                if (HID_SHOW_INFO) {
                    if (space_cnt >= 2) space_cnt -= 2;
                    Log(Info, "%sEnd Collection", spaces(space_cnt));
                }
                break;
                
            case ITEM_FEATURE:
            case ITEM_INPUT:
            case ITEM_OUTPUT:
                // An object was found
                found = true;
                
                // Increment object count
                this->cnt_object++;
                
                // Get new this->Count from global value
                if (this->count == 0)
                    this->count = this->report_count;
                
                // Get u_page/Usage from usage_table and store them in this->Data.Path
                this->data.path.node[this->data.path.size].u_page = this->usage_table[0].u_page;
                this->data.path.node[this->data.path.size].usage = this->usage_table[0].usage;
                this->data.path.size++;
                
                // Unstack u_page/Usage from usage_table (never remove the last)
                if (this->usage_size > 0) {
                    uint8_t ii = 0;
                    while (ii < this->usage_size) {
                        this->usage_table[ii].u_page = this->usage_table[ii+1].u_page;
                        this->usage_table[ii].usage = this->usage_table[ii+1].usage;
                        ii++;
                    }
                    // Remove Usage
                    this->usage_size--;
                }
                
                // Copy data type
                this->data.type = (uint8_t) (this->item & ITEM_MASK);
                //Log(Info, "\n this->data.type = %d", this->data.type);
                
                // Copy data attribute
                this->data.attribute = (uint8_t) this->value;
                //Log(Info, "\n this->data.attribute = %d", this->data.attribute);
                
                // Store offset
                this->data.offset = *this->GetReportOffset(this->data.report_id, (uint8_t) (this->item & ITEM_MASK));
                //Log(Info, "\n this->data.offset = %d", this->data.offset);
                
                // Get Object in pData
                MemoryOperations::memcpy(data, &this->data, sizeof(struct HID_DATA));
                
                // Increment Report Offset
                *this->GetReportOffset(this->data.report_id, (uint8_t) (this->item & ITEM_MASK)) += this->data.size;
                
                // Remove path last node
                this->data.path.size--;
                
                // Decrement count
                this->count--;
                
                if (!did_collection) {
                    if (HID_SHOW_INFO) {
                        if ((this->item & ITEM_MASK) == ITEM_FEATURE)
                        Log(Info, "%sFeature ", spaces(space_cnt));
                        if ((this->item & ITEM_MASK) == ITEM_INPUT)
                        Log(Info, "%sInput ", spaces(space_cnt));
                        if ((this->item & ITEM_MASK) == ITEM_OUTPUT)
                        Log(Info, "%sOutput ", spaces(space_cnt));
                        Log(Info, "(%s,%s,%s)", !(this->value & (1<<0)) ? "Data"     : "Constant",
                                            !(this->value & (1<<1)) ? "Array"    : "Variable",
                                            !(this->value & (1<<2)) ? "Absolute" : "Relative");
                                            //!(this->value & (1<<3)) ? "No Wrap"  : "Wrap",
                                            //!(this->value & (1<<4)) ? "Linear"   : "Non Linear",
                                            //!(this->value & (1<<5)) ? "Preferred State" : "No Preferred",
                                            //!(this->value & (1<<6)) ? "No Null"  : "Null State",
                                            //!(this->value & (1<<8)) ? "Bit Fueld" : "Buffered Bytes");
                    }
                    did_collection = true;
                }
                
                break;
                
            case ITEM_REP_ID:
                this->data.report_id = (uint8_t) this->value;
                break;
                
            case ITEM_REP_SIZE:
                this->data.size = (uint8_t) this->value;
                if (HID_SHOW_INFO) Log(Info, "%sreport size (%d)", spaces(space_cnt), this->data.size);
                break;
                
            case ITEM_REP_COUNT:
                this->report_count = (uint8_t) this->value;
                if (HID_SHOW_INFO) Log(Info, "%sreport count (%d)", spaces(space_cnt), this->report_count);
                did_collection = false;
                break;
                
            case ITEM_UNIT_EXP:
                this->data.unit_exp = (uint8_t) this->value;
                // convert 4 bits signed value to 8 bits signed value
                if (this->data.unit_exp > 7)
                this->data.unit_exp |= 0xF0;
                break;
                
            case ITEM_UNIT:
                this->data.unit=this->value;
                break;
                
            case ITEM_LOG_MIN:
                this->data.log_min = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sLogical Min (%d)", spaces(space_cnt), this->data.log_min);
                break;
                
            case ITEM_LOG_MAX:
                this->data.log_max = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sLogical Max (%d)", spaces(space_cnt), this->data.log_max);
                break;
                
            case ITEM_PHY_MIN:
                this->data.phy_min = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sPhysical Min (%d)", spaces(space_cnt), this->data.phy_min);
                break;
                
            case ITEM_PHY_MAX:
                this->data.phy_max = format_value(this->value, item_size[this->item & SIZE_MASK]);
                if (HID_SHOW_INFO) Log(Info, "%sPhysical Max (%d)", spaces(space_cnt), this->data.phy_max);
                break;
                
            default:
                Log(Info, "\n Found unknown item %x", (this->item & ITEM_MASK));
        }
    }
    
    return found;
}

void HIDParser::Reset()
{
    this->pos = 0;
    this->count = 0;
    this->cnt_object = 0;
    this->cnt_report = 0;
    
    this->usage_size = 0;
    this->usage_min = -1;
    this->usage_max = -1;
    MemoryOperations::memset(this->usage_table, 0, sizeof(struct HID_NODE) * USAGE_TAB_SIZE);
    
    MemoryOperations::memset(this->offset_table, 0, MAX_REPORT * 3 * sizeof(int));
    MemoryOperations::memset(&this->data, 0, sizeof(struct HID_DATA));
    
    this->data.report_id = 1; // we must give it a non-zero value or the parser doesn't work
}

bool HIDParser::FindObject(struct HID_DATA* data)
{
    struct HID_DATA found_data;
    this->Reset();
    while (this->Parse(&found_data)) {
        if ((data->path.size > 0) && (found_data.type == data->type) && MemoryOperations::memcmp(found_data.path.node, data->path.node, data->path.size * sizeof(struct HID_NODE)) == 0) 
        {
            MemoryOperations::memcpy(data, &found_data, sizeof(struct HID_DATA));
            data->report_count = this->report_count;
            return true;
        }
        // Found by ReportID/Offset
        else if ((found_data.report_id == data->report_id) && (found_data.type == data->type) && (found_data.offset == data->offset)) {
            MemoryOperations::memcpy(data, &found_data, sizeof(struct HID_DATA));
            data->report_count = this->report_count;
            return true;
        }
    }
    return false;
}
int* HIDParser::GetReportOffset(const common::uint8_t report_id, const common::uint8_t report_type)
{
    int pos = 0;
    while ((pos < MAX_REPORT) && (this->offset_table[pos][0] != 0)) {
        if ((this->offset_table[pos][0] == report_id) && (this->offset_table[pos][1] == report_type))
            return &this->offset_table[pos][2];
        pos++;
    }
    if (pos < MAX_REPORT) {
        // Increment Report count
        this->cnt_report++;
        this->offset_table[pos][0] = report_id;
        this->offset_table[pos][1] = report_type;
        this->offset_table[pos][2] = 0;
        return &this->offset_table[pos][2];
    }
    return 0;
}