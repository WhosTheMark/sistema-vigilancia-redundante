#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#define MSGSIZE_CODE 200
#define DATESIZE 26


void rmWhiteSpaces(char *str) {

   int i;

   for(i = strlen(str)-1; str[i] == ' '; --i);

   str[i+1] = '\0';
}

int cmpRegex(char *regexStr, char *msg) {

   regex_t regex;

   if (regcomp(&regex,regexStr,0)) {
      printf("Error compiling regular expression.\n");
      return 0;
   }

   if (regexec(&regex,msg,0,NULL,0) == 0)
      return 1;

   regfree(&regex);
   return 0;
}


int getCode(char *originalMsg) {

   char **validMsgs = (char *[]) {  "the peripheral device is busy",
                                    "response of device not completely received",
                                    "device asked for termination",
                                    "application started",
                                    "ports verification",
                                    "communication offline",
                                    "device status change",
                                    "low cash alert",
                                    "service mode entered",
                                    "safe door opened",
                                    "top cassette removed",
                                    "top cassette inserted",
                                    "second cassette removed",
                                    "second cassette inserted",
                                    "third cassette removed",
                                    "third cassette inserted",
                                    "bottom cassette removed",
                                    "bottom cassette inserted",
                                    "safe door closed"
                                 };

   int i, NUMCODES = 19;

   char *event = originalMsg + DATESIZE;
   char msg[MSGSIZE_CODE];

   for(i = 0; i < strlen(event); ++i)
      msg[i] = tolower(event[i]);

   msg[i] = '\0';

   rmWhiteSpaces(msg);


   for(i = 0; i < NUMCODES; ++i) {

      if (strcmp(msg,validMsgs[i]) == 0)
         return i;
   }

   if (cmpRegex("^running out of notes in cassette [[:digit:]][[:digit:]]*$",msg))
      return NUMCODES;

   if (cmpRegex("^cassette [[:digit:]][[:digit:]]* empty$",msg))
      return ++NUMCODES;

   return -1;
}
/*
int main () {


   printf("code: %d", getCode("Sun Jun 22 02:07:40 2014, low cash alert"));
}
*/