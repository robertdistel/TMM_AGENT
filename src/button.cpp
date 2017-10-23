#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "gpio.h"

#include "button.h"

/** Bit mask for the GPIO buttons */
#define BTN_MASK 0x1F

/**
 * @brief Convert a raw value to a button enum value.

 */
static Button raw_to_button(uint8_t btn)
{
	switch (btn) {
	case 1:
		return Select;
	case 2:
		return Right;
	case 4:
		return Down;
	case 8:
		return Up;
	case 16:
		return Left;
	default:
		return Null;
	}
}

uint8_t btn_nblk_raw()
{
	uint8_t r=GPIO_read(PortA);
	r=~r;
	r&=BTN_MASK;
	return r;
}

Button btn_nblk()
{
	return raw_to_button(btn_nblk_raw());
}




uint8_t btn_blk_raw()
{
	timespec t = {0,50000000}; //50 ms



	uint8_t buf;

	uint32_t ctr(0);

	while ((buf = btn_nblk_raw()) == 0 && ctr< 10)
	{
		nanosleep(&t,0);
		ctr++;
	}
	return buf;
}

Button btn_blk()
{
	return raw_to_button(btn_blk_raw());
}

Button btn_return_clk()
{
	Button buf;
	uint32_t ctr = 0;
	timespec t = {0,50000000};



	while ((buf = btn_nblk()) == Null && ctr < 10)
	{
		nanosleep(&t,0);
		ctr++;
	}
	while (btn_nblk_raw() != 0 && ctr < 10)
	{
		nanosleep(&t,0);
		ctr++;
	}
	return buf;
}

int btn_printf(Button button)
{
	switch (button) {
	case Select:
		return printf("Select\n");
		break;
	case Right:
		return printf("Right\n");
		break;
	case Down:
		return printf("Down\n");
		break;
	case Up:
		return printf("Up\n");
		break;
	case Left:
		return printf("Left\n");
		break;
	default:
		return printf("Null\n");
		break;
	}
}
