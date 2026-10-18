#include <zorix/zorix.h>

void zorix_app_main(void) {
    zk_u64 screen=zk_screen_info();
    zk_u32 width=(zk_u32)(screen>>32);
    zk_u32 height=(zk_u32)screen;

    if (zk_abi_version()!=ZORIX_ABI_VERSION || width<640 || height<480)
        return;

    zk_glass(180,140,width-180,height-140,0x17354f,190,26,12,110);
    zk_symbol(7,212,178,42,0x8fd4ff,240);
    zk_text("HELLO ZORIX",272,188,0xf7fbff,2);
    zk_text("BUILT WITH THE ZORIX NATIVE SDK",212,252,0xb8cfdf,1);
    zk_present(0,0,width,height);
    zk_sleep_ms(1000);
}
