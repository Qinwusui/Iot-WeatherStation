#include "ESPAsyncWebServer.h"

class RequestHandler : public AsyncWebHandler
{
public:
   RequestHandler() {}
   virtual ~RequestHandler() {}

   void handleRequest(AsyncWebServerRequest *req){
      
   }
};
