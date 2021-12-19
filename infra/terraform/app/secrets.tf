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