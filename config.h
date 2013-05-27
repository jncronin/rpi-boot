/* rpi-boot configuration options
 *
 * Define/undefine any of the following to enable/disable them
 */

/* Enable the framebuffer as an output device */
#define ENABLE_FRAMEBUFFER

/* Choose the default font (NB only zero or one font can be selected at a time) */
#define ENABLE_FONT0

/* Enable support for the on-board SD card reader */
#define ENABLE_SD

/* Enable support for Master Boot Record parsing (highly recommended if ENABLE_SD is set) */
#define ENABLE_MBR

/* Enable support for the FAT filesystem */
#define ENABLE_FAT

/* Enable support for the ext2 filesystem */
#define ENABLE_EXT2

/* Enable support for the Raspbootin serial protocol */
#undef ENABLE_RASPBOOTIN

/* Enable support for the serial port for debugging and Raspbootin (required for ENABLE_RASPBOOTIN) */
#define ENABLE_SERIAL

/* Enable experimental USB host support */
#undef ENABLE_USB
