resource "google_secret_manager_secret" "esp8266_pk" {
  secret_id = "esp8266-pk${var.env_suffix}"

  replication {
    automatic = true
  }
}

resource "google_secret_manager_secret_version" "esp8266_pk_version" {
  secret = google_secret_manager_secret.esp8266_pk.id

  secret_data = google_service_account_key.esp8266_key.private_key
}

resource "google_secret_manager_secret" "mqtt_tester_pk" {
  count     = var.mqtt_tester_enabled ? 1 : 0
  secret_id = "mqtt-tester-pk${var.env_suffix}"

  replication {
    automatic = true
  }
}

resource "google_secret_manager_secret_version" "mqtt_tester_pk_version" {
  count  = var.mqtt_tester_enabled ? 1 : 0
  secret = google_secret_manager_secret.mqtt_tester_pk[0].id

  secret_data = google_service_account_key.mqtt_tester_key[0].private_key
}