#ifndef STRINGS_H
#define STRINGS_H

#define DOCTYPE_HTML4 "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
#define FOOTER  "<p><i>IncubatorEmulator " __DATE__ " " __TIME__ "</p></i>"

const char msg404[] =
  DOCTYPE_HTML4 "\r\n"
  "<html>\r\n"
  "<head><title>Error</title></head>\r\n"
  "<body>\r\n"
  "<h1 align=\"center\">Error 404: page not found</h1>\r\n"
  "<hr>\r\n" FOOTER "\r\n"
  "</body>\r\n"
  "</html>\r\n";

const char msg413[] =
  DOCTYPE_HTML4 "\r\n"
  "<html>\r\n"
  "<head><title>Error</title></head>\r\n"
  "<body>\r\n"
  "<h1 align=\"center\">Error 413: payload too large</h1>\r\n"
  "<p align=\"center\">Message size is over 8 KiB</p>\r\n"
  "<hr>\r\n" FOOTER "\r\n"
  "</body>\r\n"
  "</html>\r\n";

const char msgWelcome[] = 
  DOCTYPE_HTML4 "\r\n"
  "<html>\r\n"
  "<head><title>Incubator</title></head>\r\n"
  "<body>\r\n"
  "<h1 align=\"center\">The IoT-incubator</h1>\r\n"
  "<p align=\"center\">Created by Kondratenko Daniel in 2021</p>\r\n"
  "<hr>\r\n" FOOTER "\r\n"
  "</body>\r\n"
  "</html>\r\n";

const char state_0[] = "state 0";

const char request_state[] = "request_state";
const char request_config[] = "request_config";

const char current_temp[] = "current_temp";
const char current_humid[] = "current_humid";

const char needed_temp[] = "needed_temp";
const char needed_humid[] = "needed_humid";

const char rotations_per_day[] = "rotations_per_day";

#endif
