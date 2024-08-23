/*
 * usb_callbacks.c
 *
 *  Created on: Aug 21, 2024
 *      Author: Sx107
 */

#include <main.h>
#include <tusb.h>

volatile uint8_t usbstate = 0;

void tud_mount_cb(void)
{
	usbstate = 0;
}
void tud_umount_cb(void)
{
}
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}
void tud_resume_cb(void)
{
}


