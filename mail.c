/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2014, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

//libcurl4-gnutls-dev
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#define FROM   "<redbanksvr@gmail.com>"
#define TO     "<redbanksvr@gmail.com>"
#define USER   "redbanksvr@gmail.com"
#define PASS   "r3db4nksvr"
struct upload_status upload_ctx;
   
static const char *payload_text[] = {
   "Date: Mon, 30 Jun 2014 21:54:29 +1100\r\n",
   "To: " TO "\r\n",
   "From: " FROM "RedBank\r\n",
   "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@rfcpedant.example.org>\r\n",
   "Subject: Alerta del Sistema de Vigilancia Redundante\r\n",
   "\r\n", /* empty line to divide headers from body, see RFC5322 */
   "",
   NULL
};

struct upload_status {
   int lines_read;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp){
   struct upload_status *upload_ctx = (struct upload_status *)userp;
   const char *data;

   if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
      return 0;
   }

   data = payload_text[upload_ctx->lines_read];

   if(data) {
      size_t len = strlen(data);
      memcpy(ptr, data, len);
      upload_ctx->lines_read++;

      return len;
   }

   return 0;
}

void setMailSettings (CURL *curl){
   struct curl_slist *recipients = NULL;
   
   curl_easy_setopt(curl, CURLOPT_USERNAME, USER);
   curl_easy_setopt(curl, CURLOPT_PASSWORD, PASS);
   curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
   curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
   curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

   /* Add a recipient */
   recipients = curl_slist_append(recipients, TO);
   curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
   
}

void setMailContent(CURL *curl, char* msg){
      
   upload_ctx.lines_read = 0;
   payload_text[6] = msg;

   /* We're using a callback function to specify the payload (the headers and
   * body of the message). You could just use the CURLOPT_READDATA option to
   * specify a FILE pointer to read from. */
   curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
   curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      
}

void sendMail(CURL *curl){
   
   CURLcode res = CURLE_OK;
   /* Send the message */
   res = curl_easy_perform(curl);

   /* Check for errors */
   if(res != CURLE_OK)
   fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
   
}

/*
int main(int argc, char* argv[]) {
   
   CURL *curl;
   curl = curl_easy_init();

   if (curl) {
     
      setMailSettings(curl);
      
      setMailContent(curl,argv[1]);
      
      sendMail(curl);
      
      setMailContent(curl,argv[2]);
      
      sendMail(curl);
      

      //curl_slist_free_all(recipients);

    
      curl_easy_cleanup(curl);
   }

   return 1;
}*/
