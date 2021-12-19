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