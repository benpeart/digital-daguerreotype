#include "gcode.h"
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    tty.c_lflag |= ICANON | ISIG;  /* canonical input */
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN);

    tty.c_iflag &= ~IGNCR;  /* preserve carriage return */
    tty.c_iflag &= ~INPCK;
    tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* no SW flowcontrol */

    tty.c_oflag &= ~OPOST;

    tty.c_cc[VEOL] = 0;
    tty.c_cc[VEOL2] = 0;
    tty.c_cc[VEOF] = 0x04;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
	
    return 0;
}

// Wait for input to become ready for read or timeout reached. If the file-descriptor 
// becomes readable, returns number of milli-seconds left.
// Returns 0 on timeout (i.e. no millis left and nothing to be read).
// Returns -1 on error.
static int AwaitReadReady(int fd, int timeout_millis) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_millis * 1000;

    FD_SET(fd, &read_fds);
    int s = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (s < 0)
        return -1;

    return tv.tv_usec / 1000;
}

int DiscardPendingInput(int fd, int timeout_ms, bool echo_received_data) 
{
    int total_bytes = 0;
    char buf[128];

    if (fd < 0) 
        return 0;

    while (AwaitReadReady(fd, timeout_ms) > 0) 
    {
        int r = read(fd, buf, sizeof(buf));
        if (r < 0) 
        {
            fprintf(stderr, "Error reading serial data", strerror(errno));
            return -1;
        }

        total_bytes += r;
        if (r > 0 && echo_received_data) 
        {
            write(STDERR_FILENO, buf, r);
        }
    }

    return total_bytes;
}

int gcode_open(const char *portname)
{
    int fd;

    // open the serial port to the CNC machine
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    // baudrate 115200, 8 bits, no parity, 1 stop bit
    set_interface_attribs(fd, B115200);

    // If there is some initial chatter (like the grbl connect message), ignore it, until there is some time
    // silence on the wire. That way, we only get OK responses to our requests.
    DiscardPendingInput(fd, 3000, true);

    return fd;
}

int gcode_write(int fd, const char *gcode)
{
	int wlen, len;
	char buf[256];

#ifndef _DEBUG
	printf(gcode);
#endif
	len = strlen(gcode);
	wlen = write(fd, gcode, len);
	if (wlen != len)
	{
		fprintf(stderr, "Error from write: %d, %s\n", wlen, strerror(errno));
		return -1;
	}

	/* delay for output */
	tcdrain(fd);

	/* wait for "ok" response */
	len = read(fd, buf, sizeof(buf) - 1);
	if (len > 0) 
	{
		buf[len] = 0;
		if (!strncasecmp(buf, "ok", 2))
			return 0;

		buf[strlen(buf) - 1] = 0; // remove the trailing \n
		fprintf(stderr, "Read \"%s\" but expected \"ok\"\n", buf);
	} 
	else if (len < 0) 
	{
		fprintf(stderr, "Error from read: %d: %s\n", len, strerror(errno));
	} 
	else /* len == 0 */
	{
		fprintf(stderr, "Nothing read. EOF?\n");
	}               

	return -1;
}

void gcode_close(int fd)
{
    close(fd);
}