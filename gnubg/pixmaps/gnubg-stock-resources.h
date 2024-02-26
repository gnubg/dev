#ifndef __RESOURCE_gnubg_stock_H__
#define __RESOURCE_gnubg_stock_H__

#include <gio/gio.h>

extern GResource *gnubg_stock_get_resource (void);

extern void gnubg_stock_register_resource (void);
extern void gnubg_stock_unregister_resource (void);

#endif
