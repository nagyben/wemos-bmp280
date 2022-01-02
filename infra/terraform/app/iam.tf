# https://github.com/hashicorp/terraform-provider-google/issues/400#issuecomment-360565229

resource "google_service_account" "esp8266_sa" {
  account_id   = "esp8266-sa${var.env_suffix}"
  display_name = "Service Account for ESP8266 IoT Device"
}

resource "google_service_account_key" "esp8266_key" {
  service_account_id = google_service_account.esp8266_sa.id

  keepers = {
    version = 1
  }
}

resource "google_service_account" "mqtt_tester_sa" {
  count        = var.mqtt_tester_enabled ? 1 : 0
  account_id   = "mqtt-tester-sa${var.env_suffix}"
  display_name = "MQTT Testing Client"
}

resource "google_service_account_key" "mqtt_tester_key" {
  count              = var.mqtt_tester_enabled ? 1 : 0
  service_account_id = google_service_account.mqtt_tester_sa[0].name
  public_key_type    = "TYPE_X509_PEM_FILE"
}