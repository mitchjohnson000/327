#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <unistd.h>

#include "class_data.h"
#include "curl/curl.h"

#define FROM    "<mitchjohnson000@gmail.com>"
#define CC      "<theoraclejohnson98@gmail.com>"
 
 const char **payload_text = (const char **)malloc(sizeof(char *) * 3);

 
struct upload_status {
  int lines_read;
};
 
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
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
 
int sendEmail(CURL * cu,char * destination)
{
 // CURL *cu;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;
 
  upload_ctx.lines_read = 0;
 
  //cu = curl_easy_init();
  if(cu) {
    /* Set username and password */ 
    curl_easy_setopt(cu, CURLOPT_USERNAME, "mitchjohnson000");
    curl_easy_setopt(cu, CURLOPT_PASSWORD, "Johnsmit000!");
 
    /* This is the URL for your mailserver. Note the use of smtps:// rather
     * than smtp:// to request a SSL based connection. */ 
    curl_easy_setopt(cu, CURLOPT_URL, "smtps://smtp.gmail.com");
 
    /* If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you. */ 
#ifdef SKIP_PEER_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
 
    /* If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure. */ 
#ifdef SKIP_HOSTNAME_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
 
    /* Note that this option isn't strictly required, omitting it will result
     * in libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */ 
    curl_easy_setopt(cu, CURLOPT_MAIL_FROM, FROM);
 
    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */ 
    recipients = curl_slist_append(recipients, destination);
    recipients = curl_slist_append(recipients, CC);
    curl_easy_setopt(cu, CURLOPT_MAIL_RCPT, recipients);
 
    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */ 
    curl_easy_setopt(cu, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(cu, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(cu, CURLOPT_UPLOAD, 1L);
 
    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the
     * transfer */ 
    curl_easy_setopt(cu, CURLOPT_VERBOSE, 1L);
 
    /* Send the message */ 
    res = curl_easy_perform(cu);
 
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* Free the list of recipients */ 
    curl_slist_free_all(recipients);
 
    /* Always cleanup */ 
   // curl_easy_cleanup(cu);
  }
 
  return (int)res;
}

struct BufferStruct
{
char * buffer;
size_t size;
};

// This is the function we pass to LC, which writes the output to a BufferStruct
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    std::string *mem = reinterpret_cast<std::string*>(data);
    mem->append(static_cast<char*>(ptr),realsize);

return realsize;
}

class_data * requestClassData(CURL * hnd,std::string year,std::string semcode,std::string deptcode,std::string class_number,std::string section_id){
    std::string rawData;
    class_data * data;
    
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, "http://classes.iastate.edu/app/rest/mystate");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    Json::Value root;
    Json::Value array;
    Json::Value object;
    object["semesterCode"] = semcode;
    object["semesterYear"] = year;
    object["departmentCode"] =deptcode;
    object["classNumber"] = class_number;
    object["sectionId"] = section_id;
    array[0] = object;
    root["courseSectionRequests"] = array;
    
    Json::FastWriter fastWriter;
    std::string jsonMessage = fastWriter.write(root);
    const char * t = jsonMessage.c_str();
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS,t);

    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA,&rawData);
    CURLcode ret = curl_easy_perform(hnd);

    data = new class_data(rawData);
    printf("Number of Seats Currently Open %d",data->getNumOpenSeats());

    return data;
}

int main(int argc, char *argv[]){
    class_data * data;
    CURL * curl = curl_easy_init();
      
           
    while(true){
        data = requestClassData(curl,argv[1],argv[2],argv[3],argv[4],argv[5]);
        if(data->getNumOpenSeats() != 0){
                std::string subject = "Subject: " + data->getClassName() + ": Your class has seats avaliable \r";
                std::ostringstream s;
                int i = data->getNumOpenSeats();
                s << i;
                std::string body = "Your class only has " + s.str() +  " seats avaliable. Log on to AccessPlus ASAP\r";
                char * blank = "\r\n";
                payload_text[0] = (char *)malloc((sizeof(char) * subject.length()));
                payload_text[1] = (char *)malloc((sizeof(char) * strlen(blank)));
                payload_text[2] = (char *)malloc((sizeof(char) * body.length()));
                payload_text[3] = NULL;

                payload_text[0]= subject.c_str(); 
                payload_text[1] = blank;
                payload_text[2] = body.c_str();
                free(data);
                sendEmail(curl,argv[6]);
                break;
        }
        free(data);
        usleep(5000000);
    }
    curl_easy_cleanup(curl);
}