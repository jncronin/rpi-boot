/* rpi-boot configuration options
 *
 * Define/undefine any of the following to enable/disable them
 * 
 * Don't use C++ style comments (//) in this file!
 */

/* Enable the framebuffer as an output device */
#define ENABLE_FRAMEBUFFER

/* Choose the default font (NB only zero or one font can be selected at a time) */
#define ENABLE_DEFAULT_FONT

/* Choose a different font, the following example looks for font0.bin */
#undef ENABLE_ALTERNATIVE_FONT

/* Provide the name (example looks for font0.bin) height and width of the alternative font
 * See http://www.cl.cam.ac.uk/projects/raspberrypi/tutorials/os/downloads.html#fonts
 * for some example fonts
*/
#define ALTERNATIVE_FONT		font0
#define ALTERNATIVE_FONT_H		16
#define ALTERNATIVE_FONT_W		8

/* Define this if the least significant bit of each character is the leftmost one */
#define ALTERNATIVE_FONT_LSB_LEFT

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
