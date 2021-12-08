/*
 * Http GET and POST routines
 */
HTTPClient http;

void DoHttpGetLoadBalancer()
{
      WiFiClient wifiClient;
      const char* str = "http://nightfoxsecurity.com/get_lb_config?apssid=nightfox_reset";

      http.begin(wifiClient, str);
      http.addHeader("Content-Type", "text/plain");    

      int httpCode = http.GET();

      if (httpCode > 0) {
          String payload = http.getString();
          char response[128];
          payload.toCharArray(response,128);
          LOG_PRINTFLN(1,"Got HTTP RESPONSE: %s", response);
      } else {
          LOG_PRINTFLN(1,"Failed HTTP GET: %s", str);
      }
}
