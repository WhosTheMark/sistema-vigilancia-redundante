/**
 * @file mail.c
 * @author Marcos Campos 10-10108
 * @author Andrea Salcedo 10-10666
 * @date 29 Jun 2014
 * @brief Programa que envia un correo a Red Bank.
 * Este archivo esta basado en un ejemplo publicado en la pagina:
 * http://curl.haxx.se/libcurl/c/smtp-tls.html
 * libcurl4-gnutls-dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

/** @brief Correo de Red Bank. (emisor) */
#define FROM   "<redbanksvr@gmail.com>"

/** @brief Correo de Red Bank. (receptor) */
#define TO     "<redbanksvr@gmail.com>"

/** @brief Usuario del correo de Red Bank. */
#define USER   "redbanksvr@gmail.com"

/** @brief Clave del correo de Red Bank. */
#define PASS   "r3db4nksvr"

struct upload_status upload_ctx;

/** @brief Estructura basica del correo a enviar. */
static  char *payload_text[] = {
   "Date: Mon, 30 Jun 2014 21:54:29 +1100\r\n",
   "To: " TO "\r\n",
   "From: " FROM "RedBank\r\n",
   "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@rfcpedant.example.org>\r\n",
   "Subject: Alerta del Sistema de Vigilancia Redundante\r\n",
   "\r\n",
   "",
   NULL
};

/** @brief Estado de upload. */
struct upload_status {
   int lines_read; /**< Lineas leidas. */
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {

   struct upload_status *upload_ctx = (struct upload_status *)userp;
   const char *data;

   if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
      return 0;
   }

   data = payload_text[upload_ctx->lines_read];

   if (data) {
      size_t len = strlen(data);
      memcpy(ptr, data, len);
      upload_ctx->lines_read++;

      return len;
   }
   return 0;
}

/**
 * @brief Configura los datos del correo.
 * Esto incluye los datos del usuario y del servidor de gmail.
 * @param curl Estructura donde se almacenan los datos del servidor de correos.
 */
void setMailSettings(CURL *curl) {
   struct curl_slist *recipients = NULL;

   curl_easy_setopt(curl, CURLOPT_USERNAME, USER);
   curl_easy_setopt(curl, CURLOPT_PASSWORD, PASS);
   curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
   curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
   curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

   /* Agrega los recipientes. */
   recipients = curl_slist_append(recipients, TO);
   curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
}

/**
 * @brief Configura el contenido del correo a enviar.
 * @param curl Estructura para enviar correos.
 * @param msg Mensaje a enviar por correo.
 */
void setMailContent(CURL *curl, char* msg) {

   upload_ctx.lines_read = 0;

   payload_text[6] = msg;

   curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
   curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
}

/**
 * @brief Procedimiento para enviar correos.
 * @param curl Estructura para enviar correos.
 */
void sendMail(CURL *curl) {

   CURLcode res = CURLE_OK;

   /* Envia el correo. */
   res = curl_easy_perform(curl);

   if(res != CURLE_OK)
      fprintf(stderr, "Error sending email: %s\n",curl_easy_strerror(res));

   printf("Email sent successfully.\n");
}