/**
 * @file twitter.cpp
 * @author Marcos Campos 10-10108
 * @author Andrea Salcedo 10-10666
 * @date 29 Jun 2014
 * @brief Programa que publica un nuevo tweet en la cuenta de twitter de Red Bank.
 * Este archivo esta basado en un ejemplo publicado en la pagina:
 * http://twitcurl.googlecode.com/svn/trunk/twitterClient
 */


#include <cstdio>
#include <iostream>
#include <fstream>
#include "libtwitcurl/twitcurl.h"

/** @brief El usuario de la cuenta de twitter de Red Bank. */
#define USER "redbanksvr"

/** @brief La clave de la cuenta de twitter de Red Bank. */
#define PASS "r3db4nksvr"

/**
 * @brief Publica un nuevo tweet en la cuenta de Red Bank.
 * @param argc Numero de argumentos.
 * @param argv Mensaje a publicar.
 */
int main(int argc, char *argv[]) {

   std::string userName(USER);
   std::string passWord(PASS);

   twitCurl twitterObj;
   std::string tmpStr;
   std::string replyMsg;
   char tmpBuf[1024];

   twitterObj.setTwitterUsername(userName);
   twitterObj.setTwitterPassword(passWord);

   /* Proceso de autenticacion. */
   twitterObj.getOAuth().setConsumerKey(std::string("vlC5S1NCMHHg8mD1ghPRkA"));
   twitterObj.getOAuth().setConsumerSecret(std::string("3w4cIrHyI3IYUZW5O2ppcFXmsACDaENzFdLIKmEU84"));

   /* Revisa si estan los archivos de autenticacion. */
   std::string myOAuthAccessTokenKey("");
   std::string myOAuthAccessTokenSecret("");
   std::ifstream oAuthTokenKeyIn;
   std::ifstream oAuthTokenSecretIn;

   oAuthTokenKeyIn.open("twitterClient_token_key.txt");
   oAuthTokenSecretIn.open("twitterClient_token_secret.txt");

   memset( tmpBuf, 0, 1024 );
   oAuthTokenKeyIn >> tmpBuf;
   myOAuthAccessTokenKey = tmpBuf;

   memset( tmpBuf, 0, 1024 );
   oAuthTokenSecretIn >> tmpBuf;
   myOAuthAccessTokenSecret = tmpBuf;

   oAuthTokenKeyIn.close();
   oAuthTokenSecretIn.close();

   if (myOAuthAccessTokenKey.size() && myOAuthAccessTokenSecret.size()) {

      twitterObj.getOAuth().setOAuthTokenKey( myOAuthAccessTokenKey );
      twitterObj.getOAuth().setOAuthTokenSecret( myOAuthAccessTokenSecret );

   } else {

      printf("Error: files twitterClient_token_key.txt & ");
      printf("twitterClient_token_secret.txt were not found.\n");
      exit(EXIT_FAILURE);
   }

   /* Verificacion de credenciales. */
   if (twitterObj.accountVerifyCredGet())
      twitterObj.getLastWebResponse(replyMsg);

   else {

      twitterObj.getLastCurlError(replyMsg);
      printf("Error verifying credentials: %s\n", replyMsg.c_str());
   }

    /* Publica un nuevo tweet con el mensaje ingresado por terminal. */
    memset( tmpBuf, 0, 1024 );
    tmpStr = argv[1];
    replyMsg = "";

    if (twitterObj.statusUpdate(tmpStr))
        twitterObj.getLastWebResponse(replyMsg);

    else {

        twitterObj.getLastCurlError( replyMsg );
        printf("Error posting tweet: %s\n", replyMsg.c_str());
    }

   printf("Tweet sent successfully.\n");
   return 1;
}