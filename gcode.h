//
// functions to transmit gcode to a serial device
//

#pragma once

int gcode_open(const char *portname);
int gcode_write(int fd, const char *gcode);
void gcode_close(int fd);
