resource "random_password" "shared_secret" {
  length = 16

  keepers = {
    version = 1
  }
}

resource "google_secret_manager_secret" "api_key" {
  secret_id = "api-key"

  replication {
    automatic = true
  }
}

resource "google_secret_manager_secret_version" "api_key_version" {
  secret = google_secret_manager_secret.api_key.id

  secret_data = random_password.shared_secret.result
}