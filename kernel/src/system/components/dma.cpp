#include <system/components/dma.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

DMAController::DMAController()
: SystemComponent("Direct Memory Access Controller (DMA)", "Controller for the legacy Intel 8237 DMA chip")
{

}

void DMAController::SetChannelAddress(uint8_t channel, uint8_t low, uint8_t high)
{
    if (channel > 8)
		return;

	uint16_t port = 0;
	switch ( channel ) {

		case 0: {port = DMA0_CHAN0_ADDR_REG; break;}
		case 1: {port = DMA0_CHAN1_ADDR_REG; break;}
		case 2: {port = DMA0_CHAN2_ADDR_REG; break;}
		case 3: {port = DMA0_CHAN3_ADDR_REG; break;}
		case 4: {port = DMA1_CHAN4_ADDR_REG; break;}
		case 5: {port = DMA1_CHAN5_ADDR_REG; break;}
		case 6: {port = DMA1_CHAN6_ADDR_REG; break;}
		case 7: {port = DMA1_CHAN7_ADDR_REG; break;}
	}

	outportb(port, low);
	outportb(port, high);
}
void DMAController::SetChannelCounter(uint8_t channel, uint8_t low, uint8_t high)
{
	if (channel > 8)
		return;

	uint16_t port = 0;
	switch ( channel ) {

		case 0: {port = DMA0_CHAN0_COUNT_REG; break;}
		case 1: {port = DMA0_CHAN1_COUNT_REG; break;}
		case 2: {port = DMA0_CHAN2_COUNT_REG; break;}
		case 3: {port = DMA0_CHAN3_COUNT_REG; break;}
		case 4: {port = DMA1_CHAN4_COUNT_REG; break;}
		case 5: {port = DMA1_CHAN5_COUNT_REG; break;}
		case 6: {port = DMA1_CHAN6_COUNT_REG; break;}
		case 7: {port = DMA1_CHAN7_COUNT_REG; break;}
	}

	outportb(port, low);
	outportb(port, high);
}
void DMAController::SetExternalPageRegister(uint8_t reg, uint8_t val)
{
	if (reg > 14)
		return;

	uint16_t port = 0;
	switch ( reg ) {

		case 1: {port = DMA_PAGE_CHAN1_ADDRBYTE2; break;}
		case 2: {port = DMA_PAGE_CHAN2_ADDRBYTE2; break;}
		case 3: {port = DMA_PAGE_CHAN3_ADDRBYTE2; break;}
		case 4: {return;}//! nothing should ever write to register 4
		case 5: {port = DMA_PAGE_CHAN5_ADDRBYTE2; break;}
		case 6: {port = DMA_PAGE_CHAN6_ADDRBYTE2; break;}
		case 7: {port = DMA_PAGE_CHAN7_ADDRBYTE2; break;}
	}

	outportb(port, val);
}
void DMAController::SetChannelMode(uint8_t channel, uint8_t mode)
{

	int dma = (channel < 4) ? 0 : 1;
	int chan = (dma==0) ? channel : channel-4;

	MaskChannel(channel);
	outportb ((channel < 4) ? (uint16_t)(DMA0_MODE_REG) : (uint16_t)(DMA1_MODE_REG), chan | (mode));
	UnmaskAll(dma);
}
void DMAController::ChannelPrepareRead(uint8_t channel)
{
	SetChannelMode(channel, DMA_MODE_READ_TRANSFER | DMA_MODE_TRANSFER_SINGLE | DMA_MODE_MASK_AUTO);
}
void DMAController::ChannelPrepareWrite(uint8_t channel)
{
	SetChannelMode(channel, DMA_MODE_WRITE_TRANSFER | DMA_MODE_TRANSFER_SINGLE | DMA_MODE_MASK_AUTO);
}
void DMAController::MaskChannel(uint8_t channel)
{
	if (channel <= 4)
		outportb(DMA0_CHANMASK_REG, (1 << (channel-1)));
	else
		outportb(DMA1_CHANMASK_REG, (1 << (channel-5)));
}
void DMAController::UnmaskChannel(uint8_t channel)
{
	if (channel <= 4)
		outportb(DMA0_CHANMASK_REG, channel);
	else
		outportb(DMA1_CHANMASK_REG, channel);
}
void DMAController::ResetFlipFlop(int dma)
{
	if (dma < 2)
		return;

	//! it doesnt matter what is written to this register
	outportb((dma == 0) ? (uint16_t)(DMA0_CLEARBYTE_FLIPFLOP_REG) : (uint16_t)(DMA1_CLEARBYTE_FLIPFLOP_REG), 0xff);
}
void DMAController::Reset(int dma)
{
	//! it doesnt matter what is written to this register
	outportb(DMA0_TEMP_REG, 0xff);
}
void DMAController::UnmaskAll(int dma)
{
	//! it doesnt matter what is written to this register
	outportb(DMA1_UNMASK_ALL_REG, 0xff);
}