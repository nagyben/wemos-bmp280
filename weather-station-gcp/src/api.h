#ifndef API_H
#define API_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <base64.hpp>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

// DO NOT COMMIT THIS
const char PEMKEY[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAyY/Wf9evR3QCDt4c6t91zFV1NBBql0nzHf5QvAqcctJ7KfA2
UQRY/Ld9pv6jT50yUCrIVFHDUyrKgJ+MDkSGdj2jP8mq7ZBLFnLITDpLKSBtNBOp
M0SZi5vz3Ed2yloerOG4F3aa2lBxW4PhQ6O2v2dCx4hwpZSNWEJimLx5wdlv7VWn
0YfYoCoLmfIHqTV6OWkxH8PSQZOqUoT+Rxg/+gYaMS1xl8foMA411wiZXJetA6hI
+Ph2zS46yhN2Y5DmybT+C/o8u+Mixe98B21ahLVUdeXy+vXo5DdgVcR5tKloqM1L
6n25xYuCSBsfRroayX49a76bF4slSF66Ykl3owIDAQABAoIBAAqPuOOV/5802u7A
gK9l6hw81hlWSt+GhnCp5L0VqUDy+nUmGxJ/sdH8I19etFv1lq9QrpKMfjmbYmT+
9y3ANXN4o1cyT/yGtQiGFyy3WTzccBxsc082Zv1DvLFsJTNRKY5RfX3tPwv1bxsm
e+U5bNf2py8hXRWwBP/86oGqTo7zp+29phKWf/WiqJc7xmsReiSxkWHY31bMt2PY
14sJoRsq6CbSAOtFM1Wog/6r9BK+UfDatNpBeUFtQXySIWKbEy96FTWVMqXe+or0
dyXKP1WNIk7TYfpy4hAHFUnj64lMq2e+QToOyXIGcG2ZJvfoGUpkbVG24pB7S0ad
x9pAgqECgYEA7veqzh1FUaFgaH741TWk4x+1MeIkaejgEglr6Qu0gyrnFWrvIxT6
xNpAkYj7ns94hbLc+vZeEvzHrW6NQpbxCmMizkSlM6SpWuKton5iatxA2/Yr/lpf
HKD/LrvYE+yNfNPaXoh0W0tpTw5Uomko9+f8TMIA6HLIA+Y00sw78ukCgYEA1+2l
y4Wy5+X9zeeB8fqBcOs0z7joOkzFBm0ph3MQh9Hz+vRNiLaHduoqjiZlCieOoJfd
hDW4GMBCjTip/SzCBRAmMGZjexII/sKtydxu5YjPuo1ROrcwCCG8fnZ/TRb3VwCC
u9v/RHk/Y630EbwW77Qz+9W1oDPn+bryZ86uxqsCgYEAhcaACAK624fLwLPT+Qee
1sbZWKOQXvnO9knv7vZuhz9tPcAvPyRvfePwYYid07mxbqdCrftdjqOT5LMOwUhe
h8IXqgEjulVjuU1MhrrZvZivdnPJDQrqU8yNDkx6Gi1Cx66RgHpDKRh+S5NqLcFQ
/fcQdqfkejxHXGdzCs7qgIECgYBLlOff4aDKA3tfw5V8ug7tE6ecvkCrh4u/wB41
R1WV9SvNwA4TuLCaM0VKFK5xa39FP2NOj+8s2r9w51C5Sh1v4EM91dkkjx+O7V4f
toUq20S7LBQu1Uh+0DBGo/vTZCKX1ZntYAhuU3KY1Z8FRi1RIma/1AKLXR/qDkIj
i/kpmQKBgQDd1uPrvAB/+X5T6piwXiqEauxyDiPal5b4zAy5Ho8X9csJ3OdfKQVE
z9RLh+Jnut+Ahk9C5/UpcXkBVyR2oyAW8FhL9grK8JgYhAoBVhRuVy60F4c6ooku
6OiO6mxXR36OcPvHIHq7sG6Nav3fNOq2kQa+HcrP5NL5iaRILyHrKg==
-----END RSA PRIVATE KEY-----
)EOF";

String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data)
{
  String payload = "";

  http.begin(client, url);
  http.addHeader("content-type", "application/json");

  char buffer[512];
  sprintf(buffer, "Making request to %s with data %s", url, data);
  DEBUG_PRINTLN(buffer);
  int httpStatusCode = http.POST(data);

  if (httpStatusCode > 0)
  {
    DEBUG_PRINT("HTTP Response code: ");
    DEBUG_PRINTLN(httpStatusCode);
    payload = http.getString();
  }
  else
  {
    DEBUG_PRINT("Error code: ");
    DEBUG_PRINTLN(httpStatusCode);
  }
  // Free resources
  http.end();

  return payload;
}

void getJWT(const unsigned char* payload, char* base64Output) {
  BearSSL::PrivateKey *private_key;
  private_key = new BearSSL::PrivateKey(PEMKEY);
  unsigned char output[500];
  br_rsa_pkcs1_sign signature = br_rsa_pkcs1_sign_get_default();
  signature(BR_HASH_OID_SHA256, payload, sizeof(payload), private_key->getRSA(), output);
  encode_base64(output, sizeof(output), (unsigned char*) base64Output);
}

#endif