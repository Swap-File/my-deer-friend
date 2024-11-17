#ifndef _RENDER_H
#define _RENDER_H

bool render_update(int page,int selection_effect, int selection_ota);
void render_init(void);

#define PAGE_OTA 1
#define PAGE_EFFECT 0
#define PAGE_LOGO 2

#endif