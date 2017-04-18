#include "curl/curl.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include "class_data.h"
#include <jsoncpp/json/json.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/MailRecipient.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/AutoPtr.h>


using namespace std;
using namespace Poco::Net;
using namespace Poco;

int sendEmail(char * destination)
{
    string host = "smtp.gmail.com";
    UInt16 port = 465;
    string user = "username@gmail.com";
    string password = "password";
    string to = "username@gmail.com";
    string from = "username@gmail.com";
    string subject = "Your first e-mail message sent using Poco Libraries";
    subject = MailMessage::encodeWord(subject, "UTF-8");
    string content = "Well done! You've successfully sent your first message using Poco SMTPClientSession";

    MailMessage message;
    message.setSender(from);
    message.addRecipient(MailRecipient(MailRecipient::PRIMARY_RECIPIENT, to));
    message.setSubject(subject);
    message.setContentType("text/plain; charset=UTF-8");
    message.setContent(content, MailMessage::ENCODING_8BIT);
            SharedPtr<InvalidCertificateHandler> ptrHandler = new AcceptCertificateHandler(false);


    try {
      //  Poco::Crypto::OpenSSLInitializer::initialize();
        Context::Ptr ptrContext = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
        SSLManager::instance().initializeClient(0, ptrHandler, ptrContext);

        SocketAddress sa(host, port);
        SecureStreamSocket socket(sa);
        SMTPClientSession session(socket);

        try {
            session.login(SMTPClientSession::AUTH_LOGIN, user, password);
            session.sendMessage(message);
            cout << "Message successfully sent" << endl;
            session.close();
           // Poco::Crypto::OpenSSLInitializer::uninitialize();
        } catch (SMTPException &e) {
            cerr << e.displayText() << endl;
            session.close();
            //Poco::Crypto::OpenSSLInitializer::uninitialize();
            return 0;
        }
    } catch (NetException &e) {
        cerr << e.displayText() << endl;
        return 0;
    }
    return 0;
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

int main(int argc, char *argv[]){
    std::string rawData;
    class_data * data; 
   CURL *hnd = curl_easy_init();
   

    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, "http://classes.iastate.edu/app/rest/mystate");


    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, "{\"courseSectionRequests\": [{\"semesterCode\": \"F\",\"semesterYear\": \"2017\",\"departmentCode\": \"COM S\",\"classNumber\": \"311\",\"sectionId\": \" 1\"}]}");

    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA,&rawData);
    CURLcode ret = curl_easy_perform(hnd);

    data = new class_data(rawData);
    char * temp = "asdfs";
    sendEmail(temp);
   
    curl_easy_cleanup(hnd);
}