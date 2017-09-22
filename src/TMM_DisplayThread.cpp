/*
 * DisplayThread.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */


#include "TMM_DisplayThread.h"
#include "Configuration.h"
#include "gpio.h"
#include "button.h"
#include "lcd.h"
#include <unistd.h>

#include <memory>
#include <thread>




Colour GetColor(const char* s)
{
	switch (*s)
	{
	case 'R': return Red;
	case 'B': return Blue;
	case 'G': return Green;
	case 'K': return Black;
	case 'W': return White;
	case 'Y': return Yellow;
	case 'C': return Cyan;
	case 'M': return Magenta;
	}
	return Black;
}


class TMM_DisplayThread::Context
{
public:

	Context (Configuration* config_) :
		halt_thread(false), ctx (),config(config_)
{
}

	bool halt_thread;

	std::thread ctx;
	Configuration* config;
	static void do_thread (std::shared_ptr<Context> pctx);
};




void TMM_DisplayThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	if(GPIO_open() !=0 || LCD_init(0)!=0)
	{
		fprintf(stderr,"GPIO Extender or LCD not detected - is the LCD Plate Fitted?\n");
		return; //looks like there is no GPIO expander
	}
	bool dirty(true);
	Button b(Null);
	do {
		switch (b)
		{
		case Button::Up:
			if (pctx->config->selected_domain<(Configuration::noDomains()-1))
			{
				pctx->config->selected_domain+=1;
				printf("UP\n");
				dirty=true;
			}
			break;
		case Button::Down:
			if (pctx->config->selected_domain>0)
			{
				printf("DOWN\n");
				pctx->config->selected_domain-=1;
				dirty=true;
			}
			break;
		case Button::Select:
			pctx->config->shutdown=true;
		break;
		default:
			break;
		}
		if (dirty)
		{
			LCD_clear();
			LCD_colour(GetColor(pctx->config->getDomainName()));
			LCD_wrap_printf(&pctx->config->getDomainName()[2]);
			usleep(100000); //debouncing
		}
		b=btn_return_clk();
	} while(!pctx->halt_thread && !pctx->config->shutdown);
}





TMM_DisplayThread::TMM_DisplayThread (Configuration* config_) :
									  pcontext (new Context (config_))
{
}


void TMM_DisplayThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}

void TMM_DisplayThread::stop_thread ()
{
	pcontext->halt_thread=true;
	pcontext->ctx.join ();
}







